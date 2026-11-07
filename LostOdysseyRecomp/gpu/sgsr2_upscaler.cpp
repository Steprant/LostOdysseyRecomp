#include "sgsr2_upscaler.h"

#if defined(LO_GPU_PLUME)
#include "shader/dxc_compiler.h"
#include "shader/sgsr2_source.h"

#include <plume_render_interface.h>
#include <os/logger.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace gpu::sgsr2 {
using namespace plume;

namespace {
constexpr uint32_t kGroup = 8;
// Isolated SR lists in flight at once: each needs its own descriptor sets
// until its batch completes. A frame finding none free skips SGSR2 once.
constexpr size_t kSlots = 4;

struct PushConstants {
    uint32_t renderSize[2];
    uint32_t displaySize[2];
    float renderSizeRcp[2];
    float displaySizeRcp[2];
    float jitterOffset[2];
    float preExposure;
    float cameraFovAngleHor;
    float minLerpContribution;
    uint32_t sameCamera;
    uint32_t reset;
    uint32_t depthReversed;
};
static_assert(sizeof(PushConstants) == 64);

// LO_SGSR2_JITTER_SIGN=-1 flips the jitter handed to SGSR2, to check the
// convention on screen (a wrong sign blurs static detail).
float JitterSign()
{
    static const float sign = [] {
        const char* value = std::getenv("LO_SGSR2_JITTER_SIGN");
        return value && std::atof(value) < 0 ? -1.0f : 1.0f;
    }();
    return sign;
}
}

resolution::Size RenderSizeFor(resolution::Size output, upscaling::FsrQuality quality)
{
    static constexpr double kRatio[] = {1.5, 1.7, 2.0, 1.0};
    const double ratio = kRatio[std::min<uint32_t>(uint32_t(quality), 3)];
    return {std::max(1u, uint32_t(output.width / ratio + 0.5)), std::max(1u, uint32_t(output.height / ratio + 0.5))};
}

struct Controller::Impl {
    RenderDevice* device = nullptr;
    Config config{};
    bool ready = false, poisoned = false;
    std::unique_ptr<RenderShader> convertShader, upscaleShader;
    std::unique_ptr<RenderPipelineLayout> convertLayout, upscaleLayout;
    std::unique_ptr<RenderPipeline> convertPipeline, upscalePipeline;
    std::unique_ptr<RenderSampler> linearClamp, pointClamp;
    std::unique_ptr<RenderTexture> motionDepthClip, ycocg;
    std::array<std::unique_ptr<RenderTexture>, 2> history;
    struct Slot { std::unique_ptr<RenderDescriptorSet> convert, upscale; };
    std::array<Slot, kSlots> slots;
    struct Use { uint64_t useId = 0, serial = 0; size_t slot = 0; };
    std::deque<Use> uses;
    uint64_t nextUse = 1;
    uint32_t historyWritten = 0; // Index of the newest history.
    bool historyValid = false;
    std::optional<uint64_t> lastFrame;

    void DropResources()
    {
        motionDepthClip.reset(); ycocg.reset();
        for (auto& texture : history) texture.reset();
        historyValid = false;
        lastFrame.reset();
    }
    void DropAll()
    {
        DropResources();
        for (auto& slot : slots) { slot.convert.reset(); slot.upscale.reset(); }
        convertPipeline.reset(); upscalePipeline.reset();
        convertLayout.reset(); upscaleLayout.reset();
        convertShader.reset(); upscaleShader.reset();
        linearClamp.reset(); pointClamp.reset();
        uses.clear();
        ready = false;
        device = nullptr;
    }

    bool CreatePipelines()
    {
        const auto compile = [&](const char* define, const char* entry) -> std::unique_ptr<RenderShader> {
            const std::string source = std::string("#define ") + define + "\n" + shader::kSgsr2Source;
            auto compiled = xenos::CompileCachedHlsl(source, entry, "cs_6_0", xenos::ShaderBinaryFormat::Spirv);
            if (!compiled.ok) {
                LOG_WARNING("SGSR2: {} shader: {}", entry, compiled.errors);
                return nullptr;
            }
            return device->createShader(compiled.bytecode.data(), compiled.bytecode.size(), entry, RenderShaderFormat::SPIRV);
        };
        convertShader = compile("SGSR2_CONVERT", "sgsr2Convert");
        upscaleShader = compile("SGSR2_UPSCALE", "sgsr2Upscale");
        if (!convertShader || !upscaleShader) return false;

        RenderSamplerDesc samplerDesc;
        samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = RenderTextureAddressMode::CLAMP;
        samplerDesc.mipmapMode = RenderMipmapMode::NEAREST;
        linearClamp = device->createSampler(samplerDesc);
        samplerDesc.minFilter = samplerDesc.magFilter = RenderFilter::NEAREST;
        pointClamp = device->createSampler(samplerDesc);
        if (!linearClamp || !pointClamp) return false;

        RenderDescriptorSetBuilder convertSet;
        convertSet.begin();
        for (uint32_t binding = 0; binding < 4; ++binding) convertSet.addTexture(binding);
        convertSet.addSampler(4);
        convertSet.addReadWriteTexture(5);
        convertSet.addReadWriteTexture(6);
        convertSet.end();
        RenderDescriptorSetBuilder upscaleSet;
        upscaleSet.begin();
        for (uint32_t binding = 0; binding < 3; ++binding) upscaleSet.addTexture(binding);
        upscaleSet.addSampler(3);
        upscaleSet.addSampler(4);
        upscaleSet.addReadWriteTexture(5);
        upscaleSet.addReadWriteTexture(6);
        upscaleSet.end();

        const auto makeLayout = [&](RenderDescriptorSetBuilder& set) {
            RenderPipelineLayoutBuilder layout;
            layout.begin(false, false);
            layout.addPushConstant(0, 0, sizeof(PushConstants), RenderShaderStageFlag::COMPUTE);
            layout.addDescriptorSet(set);
            layout.end();
            return layout.create(device);
        };
        convertLayout = makeLayout(convertSet);
        upscaleLayout = makeLayout(upscaleSet);
        if (!convertLayout || !upscaleLayout) return false;
        convertPipeline = device->createComputePipeline(
            RenderComputePipelineDesc(convertLayout.get(), convertShader.get(), kGroup, kGroup, 1));
        upscalePipeline = device->createComputePipeline(
            RenderComputePipelineDesc(upscaleLayout.get(), upscaleShader.get(), kGroup, kGroup, 1));
        if (!convertPipeline || !upscalePipeline) return false;
        for (auto& slot : slots) {
            slot.convert = convertSet.create(device);
            slot.upscale = upscaleSet.create(device);
            if (!slot.convert || !slot.upscale) return false;
            slot.convert->setSampler(4, pointClamp.get());
            slot.upscale->setSampler(3, linearClamp.get());
            slot.upscale->setSampler(4, pointClamp.get());
        }
        return true;
    }

    bool CreateResources()
    {
        const auto storage = RenderTextureFlag::STORAGE | RenderTextureFlag::UNORDERED_ACCESS;
        motionDepthClip = device->createTexture(RenderTextureDesc::Texture2D(config.renderWidth, config.renderHeight, 1,
            RenderFormat::R16G16B16A16_FLOAT, storage));
        ycocg = device->createTexture(RenderTextureDesc::Texture2D(config.renderWidth, config.renderHeight, 1,
            RenderFormat::R32_UINT, storage));
        for (auto& texture : history)
            texture = device->createTexture(RenderTextureDesc::Texture2D(config.outputWidth, config.outputHeight, 1,
                RenderFormat::R16G16B16A16_FLOAT, storage));
        historyValid = false;
        lastFrame.reset();
        return motionDepthClip && ycocg && history[0] && history[1];
    }

    std::optional<size_t> FreeSlot() const
    {
        for (size_t slot = 0; slot < kSlots; ++slot)
            if (std::none_of(uses.begin(), uses.end(), [&](const Use& use) { return use.slot == slot; })) return slot;
        return std::nullopt;
    }
};

Controller::Controller() : impl_(std::make_unique<Impl>()) {}
Controller::~Controller() = default;

Status Controller::EnsureSession(RenderDevice& device, const Config& config)
{
    auto& p = *impl_;
    if (p.poisoned) return Status::Failed;
    if (!config.renderWidth || !config.renderHeight || !config.outputWidth || !config.outputHeight ||
        config.renderWidth > config.outputWidth || config.renderHeight > config.outputHeight ||
        config.outputWidth > 16384 || config.outputHeight > 16384)
        return Status::InputUnavailable;
    if (p.ready && p.device == &device && p.config == config) return Status::Ready;
    if (p.ready && p.device == &device) {
        // A new size or device epoch: the GPU may still read the old images,
        // so the renderer drains and retries (NeedsReconfigure), as for FSR.
        if (!p.uses.empty()) return Status::NeedsReconfigure;
        p.config = config;
        p.DropResources();
        if (!p.CreateResources()) { p.poisoned = true; return Status::Failed; }
        LOG_INFO("SGSR2: resized {}x{} -> {}x{}", config.renderWidth, config.renderHeight,
            config.outputWidth, config.outputHeight);
        return Status::Ready;
    }
    if (!p.uses.empty()) return Status::NeedsReconfigure;
    p.DropAll();
    p.device = &device;
    p.config = config;
    if (!p.CreatePipelines() || !p.CreateResources()) {
        LOG_WARNING("SGSR2: unavailable (shader, pipeline or image creation failed)");
        p.DropAll();
        p.poisoned = true;
        return Status::Failed;
    }
    p.ready = true;
    LOG_INFO("SGSR2: ready {}x{} -> {}x{}", config.renderWidth, config.renderHeight, config.outputWidth, config.outputHeight);
    return Status::Ready;
}

Attempt Controller::RecordIsolated(RenderCommandList& commands, const Config& config,
    const temporal::TemporalFrameInputs& inputs, const FrameValues& frame, RenderTexture& output)
{
    Attempt attempt{};
    auto& p = *impl_;
    if (p.poisoned) { attempt.status = Status::Failed; return attempt; }
    if (!p.ready || !(p.config == config)) { attempt.status = Status::NeedsReconfigure; return attempt; }
    const auto& color = inputs.color;
    // Every input is read from (0, 0) with the colour's extent.
    if (!color.Complete() || !inputs.depth.Complete() || !inputs.motion.Complete() ||
        !inputs.motionInvalidity.Complete() || color.x || color.y || inputs.depth.x || inputs.depth.y ||
        inputs.motion.x || inputs.motion.y || inputs.motionInvalidity.x || inputs.motionInvalidity.y ||
        color.width != config.renderWidth || color.height != config.renderHeight ||
        inputs.depth.width < color.width || inputs.depth.height < color.height ||
        inputs.motion.width < color.width || inputs.motion.height < color.height ||
        inputs.motionInvalidity.width < color.width || inputs.motionInvalidity.height < color.height ||
        !temporal::KnownDepthConvention(inputs.depthConvention)) {
        attempt.status = Status::InputUnavailable;
        return attempt;
    }
    const auto slotIndex = p.FreeSlot();
    if (!slotIndex) { attempt.status = Status::InputUnavailable; return attempt; }
    auto& slot = p.slots[*slotIndex];

    const bool frameGap = !p.lastFrame || inputs.renderFrameId != *p.lastFrame + 1;
    const bool reset = inputs.resetHistory || !p.historyValid || frameGap;
    const uint32_t read = p.historyWritten, write = p.historyWritten ^ 1u;

    PushConstants constants{};
    constants.renderSize[0] = config.renderWidth; constants.renderSize[1] = config.renderHeight;
    constants.displaySize[0] = config.outputWidth; constants.displaySize[1] = config.outputHeight;
    constants.renderSizeRcp[0] = 1.0f / config.renderWidth; constants.renderSizeRcp[1] = 1.0f / config.renderHeight;
    constants.displaySizeRcp[0] = 1.0f / config.outputWidth; constants.displaySizeRcp[1] = 1.0f / config.outputHeight;
    constants.jitterOffset[0] = JitterSign() * float(inputs.jitter.pixelX);
    constants.jitterOffset[1] = JitterSign() * float(inputs.jitter.pixelY);
    // 0 selects the SDR path (no tone map); HDR keeps the reference's.
    constants.preExposure = inputs.colorEncoding == temporal::ColorEncoding::HdrLinear ? 1.0f : 0.0f;
    constants.cameraFovAngleHor = std::isfinite(frame.cameraFovAngleHor) && frame.cameraFovAngleHor > 0.0f ?
        frame.cameraFovAngleHor : 1.0f;
    // A still camera keeps a share of out-of-box history (less shimmer on
    // static detail); any motion lets the clamp decide alone.
    constants.minLerpContribution = frame.sameCamera ? 0.3f : 0.0f;
    constants.sameCamera = frame.sameCamera ? 1u : 0u;
    constants.reset = reset ? 1u : 0u;
    constants.depthReversed = inputs.depthConvention == temporal::DepthConvention::Reversed ? 1u : 0u;

    slot.convert->setTexture(0, inputs.depth.texture, RenderTextureLayout::SHADER_READ);
    slot.convert->setTexture(1, inputs.motion.texture, RenderTextureLayout::SHADER_READ);
    slot.convert->setTexture(2, color.texture, RenderTextureLayout::SHADER_READ);
    slot.convert->setTexture(3, inputs.motionInvalidity.texture, RenderTextureLayout::SHADER_READ);
    slot.convert->setTexture(5, p.motionDepthClip.get(), RenderTextureLayout::GENERAL);
    slot.convert->setTexture(6, p.ycocg.get(), RenderTextureLayout::GENERAL);
    slot.upscale->setTexture(0, p.history[read].get(), RenderTextureLayout::GENERAL);
    slot.upscale->setTexture(1, p.motionDepthClip.get(), RenderTextureLayout::GENERAL);
    slot.upscale->setTexture(2, p.ycocg.get(), RenderTextureLayout::GENERAL);
    slot.upscale->setTexture(5, &output, RenderTextureLayout::GENERAL);
    slot.upscale->setTexture(6, p.history[write].get(), RenderTextureLayout::GENERAL);

    const uint64_t useId = p.nextUse++;
    p.uses.push_back({useId, 0, *slotIndex});
    attempt.useId = useId;

    commands.begin();
    // GENERAL -> GENERAL still orders the previous frame's compute work.
    commands.barriers(RenderBarrierStage::COMPUTE, std::vector<RenderTextureBarrier>{
        RenderTextureBarrier(p.motionDepthClip.get(), RenderTextureLayout::GENERAL),
        RenderTextureBarrier(p.ycocg.get(), RenderTextureLayout::GENERAL),
        RenderTextureBarrier(p.history[read].get(), RenderTextureLayout::GENERAL),
        RenderTextureBarrier(p.history[write].get(), RenderTextureLayout::GENERAL),
        RenderTextureBarrier(&output, RenderTextureLayout::GENERAL)});
    commands.setComputePipelineLayout(p.convertLayout.get());
    commands.setPipeline(p.convertPipeline.get());
    commands.setComputePushConstants(0, &constants);
    commands.setComputeDescriptorSet(slot.convert.get(), 0);
    commands.dispatch((config.renderWidth + kGroup - 1) / kGroup, (config.renderHeight + kGroup - 1) / kGroup, 1);
    commands.barriers(RenderBarrierStage::COMPUTE, std::vector<RenderTextureBarrier>{
        RenderTextureBarrier(p.motionDepthClip.get(), RenderTextureLayout::GENERAL),
        RenderTextureBarrier(p.ycocg.get(), RenderTextureLayout::GENERAL)});
    commands.setComputePipelineLayout(p.upscaleLayout.get());
    commands.setPipeline(p.upscalePipeline.get());
    commands.setComputePushConstants(0, &constants);
    commands.setComputeDescriptorSet(slot.upscale.get(), 0);
    commands.dispatch((config.outputWidth + kGroup - 1) / kGroup, (config.outputHeight + kGroup - 1) / kGroup, 1);
    commands.barriers(RenderBarrierStage::ALL, std::vector<RenderTextureBarrier>{
        RenderTextureBarrier(&output, RenderTextureLayout::GENERAL),
        RenderTextureBarrier(p.history[write].get(), RenderTextureLayout::GENERAL)});
    commands.end();

    p.historyWritten = write;
    p.historyValid = true;
    p.lastFrame = inputs.renderFrameId;
    if (reset && (inputs.renderFrameId % 120 == 0 || !inputs.resetHistory))
        LOG_INFO("SGSR2: history reset frame={} input_reset={} gap={}", inputs.renderFrameId,
            inputs.resetHistory, frameGap);
    attempt.status = Status::Ready;
    return attempt;
}

void Controller::OnBatchSubmitted(uint64_t useId, uint64_t serial)
{
    for (auto& use : impl_->uses) if (use.useId == useId) use.serial = serial;
}
void Controller::OnBatchDiscarded(uint64_t useId)
{
    auto& uses = impl_->uses;
    uses.erase(std::remove_if(uses.begin(), uses.end(), [&](const Impl::Use& use) { return use.useId == useId; }), uses.end());
    // Its history write never ran: start the next frame fresh.
    impl_->historyValid = false;
}
void Controller::ReleaseCompletedThrough(uint64_t serial)
{
    auto& uses = impl_->uses;
    uses.erase(std::remove_if(uses.begin(), uses.end(),
        [&](const Impl::Use& use) { return use.serial && use.serial <= serial; }), uses.end());
}
bool Controller::HasFeatureState() const { return impl_->ready; }
void Controller::ReleaseFeatureAfterGpuDrain() { impl_->DropAll(); impl_->poisoned = false; }
void Controller::ShutdownAfterGpuDrain() { impl_->DropAll(); impl_->poisoned = false; }
void Controller::AbandonUsesAfterDeviceLoss() { impl_->uses.clear(); impl_->DropAll(); }

} // namespace gpu::sgsr2
#endif
