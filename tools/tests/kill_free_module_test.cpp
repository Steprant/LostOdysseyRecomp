// Plume's kill-free fragment modules on a headless Vulkan device (lavapipe):
// a shader whose only discard is behind a run-time test gets a second module
// with that OpKill turned into OpUnreachable, pipelines that ask for it render
// the same image when the test is off, and shaders with two kills keep one module.
// Vertex modules with varyings also get a position-only copy (their output
// stores dropped) that pipelines without a fragment shader use: same depth.
#include <plume_render_interface.h>
#include <plume_vulkan.h>
#include <stdafx.h>
#include <gpu/shader/dxc_compiler.h>
#include <cstdio>
#include <cstring>

namespace plume { std::unique_ptr<RenderInterface> CreateVulkanInterface(); }

namespace {
using namespace plume;
constexpr uint32_t kSize = 16;
int checks = 0, failures = 0;
void Check(bool ok, const char* what) { ++checks; if (!ok) ++failures; std::printf("  %s: %s\n", what, ok ? "PASS" : "FAIL"); }

constexpr char kVs[] = R"(
float4 main(uint id : SV_VertexID) : SV_Position
{
    const float2 p = float2((id << 1) & 2, id & 2);
    return float4(p * 2.0 - 1.0, 0.5, 1.0);
}
)";
// One kill, behind a push-constant test (the Xenos alpha-test shape).
constexpr char kOneKill[] = R"(
struct Push { uint test; float reference; };
[[vk::push_constant]] ConstantBuffer<Push> push;
float4 main(float4 position : SV_Position) : SV_Target0
{
    float4 colour = float4(0.25, 0.5, 0.75, (position.x + position.y) / 32.0);
    if (push.test & 1u) { if (!(colour.a > push.reference)) discard; }
    return colour;
}
)";
// A shader kill (Xenos KILL) plus the alpha test: two kills.
constexpr char kTwoKills[] = R"(
struct Push { uint test; float reference; };
[[vk::push_constant]] ConstantBuffer<Push> push;
float4 main(float4 position : SV_Position) : SV_Target0
{
    float4 colour = float4(0.25, 0.5, 0.75, (position.x + position.y) / 32.0);
    clip(position.x - 4.0);
    if (push.test & 1u) { if (!(colour.a > push.reference)) discard; }
    return colour;
}
)";

// Position plus varyings that share its inputs (the depth-only draw shape).
constexpr char kVaryingVs[] = R"(
struct Out { float4 position : SV_Position; float4 uv : TEXCOORD0; float3 normal : NORMAL; };
Out main(uint id : SV_VertexID)
{
    const float2 p = float2((id << 1) & 2, id & 2);
    Out o;
    o.position = float4(p * 2.0 - 1.0, 0.2 + 0.15 * p.x + 0.1 * p.y, 1.0);
    o.uv = float4(p, sin(p.x * 3.0), cos(p.y * 5.0));
    o.normal = normalize(float3(p, 1.0));
    return o;
}
)";
constexpr char kVaryingPs[] = R"(
float4 main(float4 position : SV_Position, float4 uv : TEXCOORD0, float3 normal : NORMAL) : SV_Target0
{
    return float4(uv.xy, normal.z, uv.w);
}
)";

std::unique_ptr<RenderShader> Compile(RenderDevice* device, const char* source, const char* profile)
{
    auto compiled = xenos::CompileCachedHlsl(source, "main", profile, xenos::ShaderBinaryFormat::Spirv);
    if (!compiled.ok) { std::printf("compile %s: %s\n", profile, compiled.errors.c_str()); return nullptr; }
    return device->createShader(compiled.bytecode.data(), compiled.bytecode.size(), "main", RenderShaderFormat::SPIRV);
}

int Run(RenderDevice* device)
{
    auto queue = device->createCommandQueue(RenderCommandListType::DIRECT);
    auto list = queue->createCommandList();
    auto fence = device->createCommandFence();
    auto target = device->createTexture(RenderTextureDesc::Texture2D(kSize, kSize, 1, RenderFormat::R8G8B8A8_UNORM, RenderTextureFlag::RENDER_TARGET));
    auto readback = device->createBuffer(RenderBufferDesc::ReadbackBuffer(kSize * kSize * 4));
    const RenderTexture* attachments[] = {target.get()};
    auto framebuffer = device->createFramebuffer(RenderFramebufferDesc(attachments, 1));
    RenderPipelineLayoutBuilder layoutBuilder;
    layoutBuilder.begin(false, false);
    layoutBuilder.addPushConstant(0, 0, 8, RenderShaderStageFlag::PIXEL);
    layoutBuilder.end();
    auto layout = layoutBuilder.create(device);
    auto vs = Compile(device, kVs, "vs_6_0");
    auto oneKill = Compile(device, kOneKill, "ps_6_0");
    auto twoKills = Compile(device, kTwoKills, "ps_6_0");
    if (!queue || !list || !fence || !target || !readback || !framebuffer || !layout || !vs || !oneKill || !twoKills) return 1;

    Check(static_cast<VulkanShader*>(oneKill.get())->vkWithoutKill != VK_NULL_HANDLE, "single-kill shader gets a kill-free module");
    Check(static_cast<VulkanShader*>(twoKills.get())->vkWithoutKill == VK_NULL_HANDLE, "two-kill shader keeps one module");
    Check(static_cast<VulkanShader*>(vs.get())->vkWithoutKill == VK_NULL_HANDLE, "vertex shader has none");

    const auto render = [&](RenderShader* ps, bool withoutKill, uint32_t test, std::vector<uint8_t>& out) {
        RenderGraphicsPipelineDesc desc;
        desc.pipelineLayout = layout.get();
        desc.vertexShader = vs.get();
        desc.pixelShader = ps;
        desc.pixelShaderWithoutKill = withoutKill;
        desc.renderTargetFormat[0] = RenderFormat::R8G8B8A8_UNORM;
        desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
        desc.renderTargetCount = 1;
        desc.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
        auto pipeline = device->createGraphicsPipeline(desc);
        if (!pipeline) return false;
        const struct { uint32_t test; float reference; } push{test, 0.5f};
        list->begin();
        list->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(target.get(), RenderTextureLayout::COLOR_WRITE));
        list->setFramebuffer(framebuffer.get());
        list->clearColor(0, RenderColor(0, 0, 0, 0));
        const RenderViewport viewport(0, 0, kSize, kSize);
        const RenderRect scissor(0, 0, kSize, kSize);
        list->setViewports(&viewport, 1);
        list->setScissors(&scissor, 1);
        list->setGraphicsPipelineLayout(layout.get());
        list->setPipeline(pipeline.get());
        list->setGraphicsPushConstants(0, &push);
        list->drawInstanced(3, 1, 0, 0);
        list->setFramebuffer(nullptr);
        list->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(target.get(), RenderTextureLayout::COPY_SOURCE));
        list->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(readback.get(), RenderFormat::R8G8B8A8_UNORM, kSize, kSize, 1, 64),
            RenderTextureCopyLocation::Subresource(target.get()));
        list->end();
        const RenderCommandList* lists[] = {list.get()};
        queue->executeCommandLists(lists, 1, nullptr, 0, nullptr, 0, fence.get());
        queue->waitForCommandFence(fence.get());
        const auto* bytes = static_cast<const uint8_t*>(readback->map());
        out.assign(bytes, bytes + kSize * kSize * 4);
        readback->unmap();
        return true;
    };
    std::vector<uint8_t> generic, killFree, tested;
    Check(render(oneKill.get(), false, 0, generic), "generic pipeline renders");
    Check(render(oneKill.get(), true, 0, killFree), "kill-free pipeline renders");
    Check(!generic.empty() && generic == killFree, "same image with the test off");
    Check(render(oneKill.get(), false, 1, tested), "alpha test on, generic module");
    size_t discarded = 0;
    for (size_t i = 3; i < tested.size(); i += 4) discarded += tested[i] == 0;
    Check(discarded > 0 && discarded < kSize * kSize, "the generic module still discards when the test is on");
    std::vector<uint8_t> twoGeneric, twoKillFree;
    Check(render(twoKills.get(), false, 0, twoGeneric) && render(twoKills.get(), true, 0, twoKillFree) && twoGeneric == twoKillFree,
        "two-kill shader ignores the request and keeps its clip()");

    // Position-only vertex modules.
    auto varyingVs = Compile(device, kVaryingVs, "vs_6_0");
    auto varyingPs = Compile(device, kVaryingPs, "ps_6_0");
    auto depth = device->createTexture(RenderTextureDesc::DepthTarget(kSize, kSize, RenderFormat::D32_FLOAT));
    auto depthReadback = device->createBuffer(RenderBufferDesc::ReadbackBuffer(64 * kSize * 4));
    const RenderTexture* colourAndDepth[] = {target.get()};
    auto depthFramebuffer = device->createFramebuffer(RenderFramebufferDesc(colourAndDepth, 1, depth.get()));
    if (!varyingVs || !varyingPs || !depth || !depthReadback || !depthFramebuffer) return 1;
    Check(static_cast<VulkanShader*>(varyingVs.get())->vkPositionOnly != VK_NULL_HANDLE, "vertex shader with varyings gets a position-only module");
    Check(static_cast<VulkanShader*>(vs.get())->vkPositionOnly == VK_NULL_HANDLE, "position-only vertex shader keeps one module");
    Check(static_cast<VulkanShader*>(oneKill.get())->vkPositionOnly == VK_NULL_HANDLE, "pixel shader has no position-only module");
    const auto renderDepth = [&](RenderShader* ps, std::vector<uint8_t>& out) {
        RenderGraphicsPipelineDesc desc;
        desc.pipelineLayout = layout.get();
        desc.vertexShader = varyingVs.get();
        desc.pixelShader = ps;
        desc.renderTargetFormat[0] = RenderFormat::R8G8B8A8_UNORM;
        desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
        desc.renderTargetBlend[0].renderTargetWriteMask = 0;
        desc.renderTargetCount = 1;
        desc.depthTargetFormat = RenderFormat::D32_FLOAT;
        desc.depthEnabled = true;
        desc.depthWriteEnabled = true;
        desc.depthFunction = RenderComparisonFunction::ALWAYS;
        desc.primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
        auto pipeline = device->createGraphicsPipeline(desc);
        if (!pipeline) return false;
        list->begin();
        list->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(target.get(), RenderTextureLayout::COLOR_WRITE));
        list->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(depth.get(), RenderTextureLayout::DEPTH_WRITE));
        list->setFramebuffer(depthFramebuffer.get());
        list->clearDepth(true, 1.0f);
        const RenderViewport viewport(0, 0, kSize, kSize);
        const RenderRect scissor(0, 0, kSize, kSize);
        list->setViewports(&viewport, 1);
        list->setScissors(&scissor, 1);
        list->setGraphicsPipelineLayout(layout.get());
        list->setPipeline(pipeline.get());
        list->drawInstanced(3, 1, 0, 0);
        list->setFramebuffer(nullptr);
        list->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(depth.get(), RenderTextureLayout::COPY_SOURCE));
        list->copyTextureRegion(RenderTextureCopyLocation::PlacedFootprint(depthReadback.get(), RenderFormat::D32_FLOAT, kSize, kSize, 1, 64),
            RenderTextureCopyLocation::Subresource(depth.get()));
        list->end();
        const RenderCommandList* lists[] = {list.get()};
        queue->executeCommandLists(lists, 1, nullptr, 0, nullptr, 0, fence.get());
        queue->waitForCommandFence(fence.get());
        const auto* bytes = static_cast<const uint8_t*>(depthReadback->map());
        out.assign(bytes, bytes + 64 * kSize * 4);
        depthReadback->unmap();
        return true;
    };
    std::vector<uint8_t> depthFull, depthPositionOnly;
    Check(renderDepth(varyingPs.get(), depthFull), "depth with the full vertex module renders");
    Check(renderDepth(nullptr, depthPositionOnly), "depth-only pipeline with the position-only module renders");
    bool written = false;
    for (size_t row = 0; row < kSize && !depthFull.empty(); ++row)
        for (size_t x = 0; x < kSize; ++x) {
            float value = 1.0f;
            std::memcpy(&value, depthFull.data() + (row * 64 + x) * 4, 4);
            written |= value < 0.99f;
        }
    bool same = depthFull.size() == depthPositionOnly.size();
    for (size_t row = 0; row < kSize && same; ++row)
        same = std::memcmp(depthFull.data() + row * 64 * 4, depthPositionOnly.data() + row * 64 * 4, kSize * 4) == 0;
    Check(written && same, "position-only module writes the same depth");
    std::printf("kill-free modules: %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
}

int main()
{
    auto api = plume::CreateVulkanInterface();
    if (!api) return 2;
    auto device = api->createDevice();
    if (!device) return 2;
    return Run(device.get());
}
