# Android GPU load: measurements, compact targets, ADPF, display rate — 2026-11-06

Branch `perf/android-gpu-load-20261106` (base `integration/v1.1.0-steprant-all`
at `077ac142`), merged with the audio and SGSR1 test branches into
`integration/v1.6-candidate-20261106`.

## What the 2026-11-04 traces say

| Xiaomi 14, 1.5 build | value |
|---|---|
| Renderer GPU time per frame, 30 FPS, internal 1080 | 21–26 ms (62–79% busy) |
| Same, 60 FPS attempts | 15–18 ms per frame, 92–98% busy |
| Frames slower than 43 ms (33 min session) | 624, median 68.8 ms (one missed vsync) |
| GPU time of those frames | median 50 ms, p90 74 ms; median 1084 draws |
| Slow frames with inline pipeline creation / blocked present | 2 / 0 |
| Android thermal status | 0 throughout |

30 FPS drops are GPU-bound heavy scenes, not CPU, shader compilation or
thermal throttling. Menus with ~120 draws still cost ~25 ms: the load is
per-pixel and fixed per-frame work, not geometry.

Source inspection: every colour render target was RGBA16F (8 bytes/pixel),
including the console's 8-bit and 10-bit fixed-point formats; depth is
D32_S8. Shadow resolution 2x allocates 2560x1440 shadow maps (console:
1280x720). Internal "1080" on the 2670x1200 panel renders 2403x1080
(+25% over 1920x1080). BC textures are native; render targets keep UBWC
(no storage/mutable flags).

## Changes

1. **Pass profile (diagnostics).** plume brackets every Vulkan render pass
   with timestamps outside the pass and counts draws, barriers, copies,
   dispatches and the bytes its load/store ops move. While the Tester records
   with Advanced/Intensive (or `LO_PASS_PROFILE=1`) the log gets a
   `gpu passes:` line every 10 s: per-frame passes, empty passes, pass and
   outside-pass GPU ms, load/store MB, the eight costliest targets, and
   `format_draws_per_frame` (guest draws per Xenos colour format). Lists are
   read when next begun, after their fence: no GPU wait.
2. **Compact render targets** (Tester → *Compact render targets*, saved as
   `compact_render_targets`, `LO_COMPACT_RT=0/1`; restart). With the HDR
   scene off, Xenos 8_8_8_8(_GAMMA) targets become RGBA8 and fixed
   2_10_10_10 become A2B10G10R10: the bits the console stores (the epilogue
   already clamps both classes to [0, 1]), half the bytes per pixel. 7e3,
   16-bit and 32-bit classes and depth are unchanged. Resolves from a compact
   8888 target to an 8888 texture become plain copies. First run with the
   option compiles the new pipeline variants on demand.
3. **ADPF power hints** (Tester → *Power hints (ADPF)*, `performance_hints`,
   `LO_ADPF=0/1`, `LO_ADPF_TARGET_PERCENT`; live). One APerformanceHint
   session for the guest main and render threads reports the busier thread's
   CPU time per frame against 50% of the frame period, with prefer power
   efficiency on API 35+. A `performance hints:` log line every 10 s.
4. **Match display refresh** (Tester, `display_rate_hint`, default on; live).
   `Surface.setFrameRate(fps, FIXED_SOURCE)` on the phone surface for the
   native target, so a 120 Hz panel can run 60/30 Hz. TV output keeps its
   60 Hz request.
5. **900p render resolution**: 1600x900 at 16:9, 2003x900 on the phone panel.
6. **SGSR1 kept on swapchain format change** (it fell back to the ordinary
   filter until restart).

All options except (4) and (6) are off by default until measured.

## Verification performed

- Lavapipe: profiled presentation pass (timestamps, byte counts, draw count);
  7/7 presentation readbacks; SGSR1 reference comparison (other branch).
- Host: pass-profile summary, ADPF policy, render resolution (14,055 checks),
  settings menu suite including every new Tester row, config round-trip.
- Production `renderer.cpp` and `video.cpp` compile in the asset-free boundary,
  also with the Android branches (`-D__ANDROID__`); Java/JNI contract test.
- CI "Steprant Android integration" (android, host, vulkan jobs) on the branch.

Not performed: ARM64 APK build (private input), device run, measured FPS,
GPU time, temperature or battery.

## Device protocol (Xiaomi 14)

Same save and scene, preferably a heavy one (the fire boss or a crowded
battle), 30 FPS, warm shader cache, Tester *Advanced*, 5 minutes each:

| Run | Render | Shadow | Compact targets | ADPF |
|---|---|---|---|---|
| A (baseline) | 1080 | 2x | off | off |
| B | 1080 | 2x | **on** | off |
| C | 1080 | **1x** | on | off |
| D | **900** | 1x | on | off |
| E | best of B–D | | on | **on** |

Compare per run: `gpu passes:` pass_ms/outside_ms/load+store MB, slow frames
and 1% low from the Tester, `performance hints:` work, battery temperature.
Restart after changing compact targets. Display refresh matching is on in
all runs; turn it off once to compare panel power if wanted.

## Device sessions of 2026-11-06 evening (1.6 test build `a81a116a`)

Three Tester recordings (22:16, 22:25, 22:35; the last two are one run),
Turnip (Mesa 26.0.0-devel) on the Xiaomi 14, 30 FPS, FXAA, SGSR1, shadows 2x.

- Steady scenes hold 30.0 FPS (p95 34 ms). GPU busy 21–25 ms per frame
  (65–75% at 30 FPS). Slow intervals sit in scene changes with 4,000+ draw
  frames; inline pipeline compiles are near zero and the thermal status
  stays 0 (battery up to 44.8 °C).
- Main scene per frame: about 110 render passes and 1,800–1,900 draws;
  ~25 colour+depth passes and ~25 depth-only passes on the same target, in
  alternation; 73–80 barriers, 10–13 copies. Colour formats by draws:
  7e3 ≈ 450 + 7e3_as_16 ≈ 285 against 8888 ≈ 95, so compact targets change
  little here.
- Resolution sensitivity is low. A depth-only draw costs ≈ 3.7 µs at
  2003×920 and at 2403×1104, a scene draw ≈ 8.0 µs and ≈ 8.9 µs: GPU time
  follows draws (vertex and per-draw shader work), not pixels.
- Presentation: SGSR1 runs as a compute dispatch at 1080 → 2670×1200 and as
  a graphics pass at 900p, about 2–2.5 ms either way; FXAA at 2403×1080
  about 1.3 ms.
- Audio: no queue starvation during play in either XMA lock mode, no clock
  resets, the audio thread never waits for the XMA decoder.

### Consequences in the build

1. **Uniform-buffer shader constants** (Tester *Shader constants in UBO*,
   restart). Pointer-form SPIR-V reads every `XeConst(n)`, the sampler
   indices and the vertex-fetch offsets with `vk::RawBufferLoad` from device
   addresses — on Adreno, a global memory load each, which fits the
   per-draw, resolution-independent cost above. The UBO form (the Switch
   pack's) reads the same `cbuffer`s from set 5; Turnip can upload
   statically indexed ranges into the constant registers and serves the
   rest from the constant cache. No published pack has this form: the first
   start runs the full shader preparation on the phone (translation + DXC,
   progress screen, several minutes) and later starts load the local
   startup bundle. Turnip keeps its dynamic-offset descriptor set on Adreno
   7xx+ (`gpu::adreno::Generation`); otherwise the renderer keeps the
   pointer form.
2. **One pass for depth-only draws** (live): keeps the scene colour target
   bound for depth-only draws, removing ~45 pass changes per frame. An
   older note measured Turnip sysmem ~9% slower; to be measured again with
   the pass profile (`depth_only_keeps_color=` in the format line).
3. **Turnip render mode** (restart): Automatic / Sysmem / GMEM.
4. The title screen's full-screen mesh layer is no longer drawn
   (`LO_SCREEN_PATTERN=1` restores it).

### Next device protocol

Same save and spot, 30 FPS, Tester *Intensive*, 3 minutes standing still +
2 minutes moving, for each of:

| Run | Shader constants | Depth-only pass | Turnip mode |
|---|---|---|---|
| A | pointer | off | Automatic |
| B | pointer | **on** | Automatic |
| C | **UBO** (after its first preparation) | off | Automatic |
| D | UBO | best of A/B | Automatic |
| E | best | best | **Sysmem**, then **GMEM** |

Compare `gpu passes:` pass_ms + outside_ms and per-target ms, the Tester
GPU busy and 1% low.

## A/B results of 2026-11-07 and what the build keeps

Six recordings (08:43–10:13), Xiaomi 14, Turnip, 1080 internal, 30 FPS
unless noted. Medians over gameplay windows (> 1,200 draws per frame):

| Run | Constants | Depth-only pass | Turnip | Passes | Pass ms | GPU ms/frame |
|---|---|---|---|---:|---:|---:|
| 08:43 | pointer | separate | auto | 104 | 19.4 | 24.2 |
| 09:12 | UBO | separate | auto | 102 | 19.3 | 24.2 (13% more draws) |
| 09:33 | UBO | kept | GMEM | 67 | 26.6 | 31.4 |
| 10:03 | UBO | kept | sysmem | 64 | 17.9 | 23.1 |
| 10:13 | UBO | kept | auto | 74 | 17.8 | 22.1 |

Per scene draw: pointer 8.6 µs, UBO 7.9 µs. In the 10:13 run 720p
(1602×736) costs ~12.7 ms against ~22 ms at 1080p in comparable scenes.

Kept: depth-only draws in the scene pass (Android default), compact 8-bit
targets (always on Android), display refresh matching, 64 ms audio buffer.
Removed: UBO constants (~7% for a long first-start compile and a ~900 MB
bundle), Turnip render mode (GMEM much slower), ADPF hints (no effect on a
GPU-bound frame), per-voice XMA locks (audio clean without them).

The remaining large lever is rendering fewer pixels: SGSR1 already
reconstructs 720p/900p to the 2670×1200 panel, but it is spatial only. A
temporal upscaler (Qualcomm SGSR2, BSD-3) fed with the renderer's jitter,
depth and motion vectors would let 720p look close to native.
