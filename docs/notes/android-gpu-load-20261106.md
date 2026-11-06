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
