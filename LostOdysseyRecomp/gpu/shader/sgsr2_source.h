#pragma once
// Qualcomm Snapdragon Game Super Resolution 2 (SGSR2), 2-pass compute
// variant, adapted from SnapdragonStudios/snapdragon-gsr sgsr/v2
// (see thirdparty/licenses/SnapdragonGSR.txt). HLSL for DXC (cs_6_0, SPIR-V).
//
// Adaptations to the renderer's temporal inputs (temporal_frame_inputs.h):
// - Depth is R32_FLOAT, reversed Z when depthReversed != 0 (0 = far plane):
//   the nearest sample is the maximum, the far-plane test is depth > 1e-5 and
//   the depth-separation tolerance scales with the value instead of 1 - value.
// - Motion arrives as backward, unjittered render-pixel vectors (+X right,
//   +Y down: previous = current + motion) from motion replay and the camera
//   fill, so the convert pass stores SGSR2's NDC difference
//   (current - previous) * 2 instead of decoding a velocity texture or
//   reprojecting with a clip-to-previous-clip matrix.
// - A motion-invalidity value above 0.5 marks the pixel as disoccluded
//   (depth clip 1), so its history is replaced rather than blended.
// - Texture gathers use a point sampler; every read stays inside the image.
// - A reset frame never reads the previous history (it may be uninitialised).
// - SDR input (display-referred, [0, 1]) skips the HDR tone map around the
//   YCoCg packing and the output is clipped to [0, 1]; HDR input keeps the
//   reference tone map with an exact inverse. Non-finite colour reads as 0.
namespace gpu::shader {
inline constexpr const char* kSgsr2Source = R"SGSR2(
//============================================================================================================
//
//
//                  Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
//                              SPDX-License-Identifier: BSD-3-Clause
//
//============================================================================================================
struct Sgsr2Parameters {
    uint2  renderSize;
    uint2  displaySize;
    float2 renderSizeRcp;
    float2 displaySizeRcp;
    float2 jitterOffset;      // render pixels, raster displacement of this frame
    float  preExposure;       // 0: SDR input, no tone map; > 0: HDR (reference tone map)
    float  cameraFovAngleHor; // tan(horizontal fov / 2)
    float  minLerpContribution;
    uint   sameCamera;
    uint   reset;
    uint   depthReversed;
};
[[vk::push_constant]] ConstantBuffer<Sgsr2Parameters> params;

#ifdef SGSR2_CONVERT
[[vk::binding(0,0)]] Texture2D<float>  inputDepth : register(t0);
[[vk::binding(1,0)]] Texture2D<float2> inputMotion : register(t1);
[[vk::binding(2,0)]] Texture2D<float4> inputColor : register(t2);
[[vk::binding(3,0)]] Texture2D<float>  inputInvalidity : register(t3);
[[vk::binding(4,0)]] SamplerState pointClamp : register(s0);
[[vk::binding(5,0)]] [[vk::image_format("rgba16f")]] RWTexture2D<float4> motionDepthClipAlpha : register(u0);
[[vk::binding(6,0)]] [[vk::image_format("r32ui")]]   RWTexture2D<uint>   ycocgColor : register(u1);

float Nearest(float a, float b) { return params.depthReversed != 0 ? max(a, b) : min(a, b); }
float Nearest4(float4 v) { return Nearest(Nearest(v.x, v.y), Nearest(v.z, v.w)); }

[numthreads(8, 8, 1)]
void sgsr2Convert(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= params.renderSize.x || id.y >= params.renderSize.y) return;
    const float2 texel = params.renderSizeRcp;
    // Gathers around the corners of this texel: x = (i0,j1), y = (i1,j1),
    // z = (i1,j0), w = (i0,j0) of the 2x2 footprint.
    const float2 gatherCoord = float2(id.xy) * texel;
    const float4 topLeft = inputDepth.GatherRed(pointClamp, gatherCoord);
    const float4 topRight = inputDepth.GatherRed(pointClamp, gatherCoord + float2(texel.x * 2.0, 0.0));
    const float4 bottomLeft = inputDepth.GatherRed(pointClamp, gatherCoord + float2(0.0, texel.y * 2.0));
    const float4 bottomRight = inputDepth.GatherRed(pointClamp, gatherCoord + texel * 2.0);
    const float nearest = Nearest(Nearest(topLeft.y, topRight.x), Nearest(bottomLeft.z, bottomRight.w));
    const float topLeft4 = Nearest4(topLeft);

    float depthClip = 0.0;
    const bool geometry = params.depthReversed != 0 ? nearest > 1.0e-05 : nearest < 1.0 - 1.0e-05;
    if (geometry) {
        const float topRight4 = Nearest4(topRight);
        const float bottomLeft4 = Nearest4(bottomLeft);
        const float bottomRight4 = Nearest4(bottomRight);
        const float Ksep = 1.37e-05;
        const float separation = Ksep * params.cameraFovAngleHor * length(float2(params.renderSize)) *
            (params.depthReversed != 0 ? nearest : 1.0 - nearest);
        const float EPSILON = 1.19e-07;
        float weight = 0.0;
        weight += saturate(separation / (abs(nearest - topLeft4) + EPSILON));
        weight += saturate(separation / (abs(nearest - topRight4) + EPSILON));
        weight += saturate(separation / (abs(nearest - bottomLeft4) + EPSILON));
        weight += saturate(separation / (abs(nearest - bottomRight4) + EPSILON));
        depthClip = saturate(1.0 - weight * 0.25);
    }
    if (inputInvalidity.Load(int3(id.xy, 0)) > 0.5) depthClip = 1.0;

    // Backward render-pixel motion -> SGSR2's NDC (current - previous), +Y down.
    // A non-finite vector (never expected) is treated as a disocclusion.
    float2 motion = -2.0 * inputMotion.Load(int3(id.xy, 0)) * texel;
    if (any(!isfinite(motion))) { motion = float2(0.0, 0.0); depthClip = 1.0; }

    // YCoCg 11:11:10 packing, as the reference. Display-referred SDR input
    // ([0, 1]) already fits YCoCg's [0, 1] range, so the reference's
    // rgb / (max + preExposure) tone map runs only for HDR input: blending in
    // that per-pixel space biased SDR colours next to fine detail (the
    // lavapipe fixture measured about half the error without it).
    float3 rgb = inputColor.Load(int3(id.xy, 0)).rgb;
    if (any(!isfinite(rgb))) rgb = float3(0.0, 0.0, 0.0);
    float colorMax = 1.0;
    if (params.preExposure > 0.0) {
        rgb = max(rgb, 0.0);
        colorMax = max(max(rgb.x, rgb.y), rgb.z) + params.preExposure;
        rgb /= colorMax;
    } else {
        rgb = saturate(rgb);
    }
    float3 ycocg;
    ycocg.x = 0.25 * (rgb.x + 2.0 * rgb.y + rgb.z);
    ycocg.y = saturate(0.5 * rgb.x + 0.5 - 0.5 * rgb.z);
    ycocg.z = saturate(ycocg.x + ycocg.y - rgb.x);
    const uint x11 = uint(ycocg.x * 2047.5);
    const uint y11 = uint(ycocg.y * 2047.5);
    const uint z10 = uint(ycocg.z * 1023.5);
    ycocgColor[id.xy] = (x11 << 21u) | (y11 << 10u) | z10;
    motionDepthClipAlpha[id.xy] = float4(motion, depthClip, colorMax);
}
#endif

#ifdef SGSR2_UPSCALE
[[vk::binding(0,0)]] Texture2D<float4> previousHistory : register(t0);
[[vk::binding(1,0)]] Texture2D<float4> motionDepthClipAlpha : register(t1);
[[vk::binding(2,0)]] Texture2D<uint>   ycocgColor : register(t2);
[[vk::binding(3,0)]] SamplerState linearClamp : register(s0);
[[vk::binding(4,0)]] SamplerState pointClamp : register(s1);
[[vk::binding(5,0)]] [[vk::image_format("rgba16f")]] RWTexture2D<float4> sceneColorOutput : register(u0);
[[vk::binding(6,0)]] [[vk::image_format("rgba16f")]] RWTexture2D<float4> historyOutput : register(u1);

float FastLanczos(float base)
{
    const float y = base - 1.0;
    const float y2 = y * y;
    return (0.75 * y + y2) * y2;
}
float3 DecodeColor(uint sample32)
{
    const uint x11 = sample32 >> 21u;
    const uint y11 = sample32 & (2047u << 10u);
    const uint z10 = sample32 & 1023u;
    return float3(float(x11) * (1.0 / 2047.5), float(y11) * 4.76953602e-7 - 0.5, float(z10) * (1.0 / 1023.5) - 0.5);
}

struct Box {
    float4 upsampled;
    float3 center, variance, minimum, maximum;
    float weight;
};
void Accumulate(inout Box box, float3 color, float2 offset, float kernelBias2, float curveBias, bool first)
{
    const float offsetDot = dot(offset, offset);
    const float lanczos = FastLanczos(saturate(offsetDot * kernelBias2));
    box.upsampled += float4(color * lanczos, lanczos);
    const float boxWeight = exp(offsetDot * curveBias);
    box.minimum = first ? color : min(box.minimum, color);
    box.maximum = first ? color : max(box.maximum, color);
    const float3 weighted = color * boxWeight;
    box.center += weighted;
    box.variance += color * weighted;
    box.weight += boxWeight;
}

[numthreads(8, 8, 1)]
void sgsr2Upscale(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= params.displaySize.x || id.y >= params.displaySize.y) return;
    const float2 renderSize = float2(params.renderSize);
    const float2 displaySize = float2(params.displaySize);
    const float biasMaxViewportScale = min(displaySize.x / renderSize.x, 1.99);
    const float scaleFactor = min(20.0, pow((displaySize.x / renderSize.x) * (displaySize.y / renderSize.y), 3.0));
    const float2 jitter = params.jitterOffset;

    const float2 outputUv = (float2(id.xy) + 0.5) * params.displaySizeRcp;
    const float2 jitterUv = saturate(outputUv + jitter * params.renderSizeRcp);
    const int2 inputPos = int2(min(jitterUv * renderSize, renderSize - 1.0));

    const float4 mda = motionDepthClipAlpha.SampleLevel(linearClamp, jitterUv, 0);
    const float2 motion = mda.xy;
    const float2 previousUv = saturate(outputUv - 0.5 * motion);
    const float depthFactor = mda.z;
    const float colorMax = mda.w;

    // After a reset the previous history is never read: it may be a fresh,
    // uninitialised image (or a discarded frame's), and NaN * 0 is NaN.
    float4 history = previousHistory.SampleLevel(linearClamp, previousUv, 0);
    if (params.reset != 0 || any(!isfinite(history))) history = float4(0.0, 0.0, 0.0, 0.0);
    float3 historyColor = history.xyz;
    const float wFactor = saturate(abs(history.w));

    const float kernelFactor = saturate(wFactor + float(params.reset));
    const float biasMax = biasMaxViewportScale - biasMaxViewportScale * kernelFactor;
    const float biasMin = max(1.0, 0.3 + 0.3 * biasMax);
    const float biasFactor = max(0.25 * depthFactor, kernelFactor);
    const float kernelBias = lerp(biasMax, biasMin, biasFactor) * 0.5;
    const float kernelBias2 = kernelBias * kernelBias;
    const float motionViewportLength = length(motion * displaySize);
    const float curveBias = lerp(-2.0, -3.0, saturate(motionViewportLength * 0.02));

    const float2 sourceOffset = float2(inputPos) + 0.5 - jitter - outputUv * renderSize;
    const int2 last = int2(params.renderSize) - 1;
    const uint centre = ycocgColor.Load(int3(inputPos, 0));
    const uint left = ycocgColor.Load(int3(max(inputPos - int2(1, 0), 0), 0));
    const uint right = ycocgColor.Load(int3(min(inputPos + int2(1, 0), last), 0));
    const uint above = ycocgColor.Load(int3(max(inputPos - int2(0, 1), 0), 0));
    const uint below = ycocgColor.Load(int3(min(inputPos + int2(0, 1), last), 0));

    Box box = (Box)0;
    Accumulate(box, DecodeColor(below), sourceOffset + float2(0.0, 1.0), kernelBias2, curveBias, true);
    Accumulate(box, DecodeColor(right), sourceOffset + float2(1.0, 0.0), kernelBias2, curveBias, false);
    Accumulate(box, DecodeColor(left), sourceOffset + float2(-1.0, 0.0), kernelBias2, curveBias, false);
    Accumulate(box, DecodeColor(centre), sourceOffset, kernelBias2, curveBias, false);
    Accumulate(box, DecodeColor(above), sourceOffset + float2(0.0, -1.0), kernelBias2, curveBias, false);
    if (params.sameCamera != 0) {
        Accumulate(box, DecodeColor(ycocgColor.Load(int3(min(inputPos + 1, last), 0))),
            sourceOffset + float2(1.0, 1.0), kernelBias2, curveBias, false);
        Accumulate(box, DecodeColor(ycocgColor.Load(int3(clamp(inputPos + int2(-1, 1), 0, last), 0))),
            sourceOffset + float2(-1.0, 1.0), kernelBias2, curveBias, false);
        Accumulate(box, DecodeColor(ycocgColor.Load(int3(clamp(inputPos + int2(1, -1), 0, last), 0))),
            sourceOffset + float2(1.0, -1.0), kernelBias2, curveBias, false);
        Accumulate(box, DecodeColor(ycocgColor.Load(int3(max(inputPos - 1, 0), 0))),
            sourceOffset + float2(-1.0, -1.0), kernelBias2, curveBias, false);
    }

    const float boxWeightRcp = 1.0 / box.weight;
    box.center *= boxWeightRcp;
    box.variance *= boxWeightRcp;
    box.variance = sqrt(abs(box.variance - box.center * box.center));

    float4 upsampled = box.upsampled;
    upsampled.xyz = clamp(upsampled.xyz / upsampled.w, box.minimum - 0.05, box.maximum + 0.05);
    upsampled.w *= 1.0 / 3.0;

    const float oneMinusW = 1.0 - wFactor;
    float baseUpdate = oneMinusW - oneMinusW * depthFactor;
    baseUpdate = min(baseUpdate, lerp(baseUpdate, upsampled.w * 10.0, saturate(10.0 * motionViewportLength)));
    baseUpdate = min(baseUpdate, lerp(baseUpdate, upsampled.w, saturate(motionViewportLength * 0.05)));
    float baseAlpha = baseUpdate;

    const float EPSILON = 1.192e-07;
    const float boxScale = max(depthFactor, saturate(motionViewportLength * 0.05));
    const float boxSize = lerp(scaleFactor, 1.0, boxScale);
    const float3 scaledVariance = box.variance * boxSize;
    const float3 boxMaximum = min(box.maximum, box.center + scaledVariance);
    const float3 boxMinimum = max(box.minimum, box.center - scaledVariance);

    const float3 clampedColor = clamp(historyColor, boxMinimum, boxMaximum);
    const float startLerp = abs(motion.x) + abs(motion.y) > 0.000001 ? 0.0 : params.minLerpContribution;
    const bool outside = any(boxMinimum > historyColor) || any(historyColor > boxMaximum);
    const float lerpContribution = saturate(outside ? startLerp : 1.0);
    historyColor = lerp(clampedColor, historyColor, lerpContribution);
    baseAlpha = lerp(min(baseAlpha, 0.1), baseAlpha, lerpContribution);

    const float alphaSum = max(EPSILON, baseAlpha + upsampled.w);
    const float alpha = saturate(upsampled.w / alphaSum + float(params.reset));
    upsampled.xyz = lerp(historyColor, upsampled.xyz, alpha);
    historyOutput[id.xy] = float4(upsampled.xyz, wFactor);

    // YCoCg -> RGB. SDR: Lanczos overshoot past white is clipped here, as it
    // would be later. HDR: the inverse tone map, exact (the reference
    // subtracts from 1 + 600/65504, about 2% darker); the floor keeps the
    // same fp16 ceiling.
    const float xz = upsampled.x - upsampled.z;
    float3 rgb = saturate(float3(xz + upsampled.y, upsampled.x + upsampled.z, xz - upsampled.y));
    if (params.preExposure > 0.0) {
        const float componentMax = max(max(rgb.x, rgb.y), rgb.z);
        float scale = params.preExposure / max(1.0 - componentMax, 600.0 / 65504.0);
        if (colorMax > 4000.0) scale = colorMax;
        rgb *= scale;
    }
    sceneColorOutput[id.xy] = float4(rgb, 1.0);
}
#endif
)SGSR2";
}
