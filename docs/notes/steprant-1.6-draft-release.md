# Lost Odyssey Steprant Android 1.6 — test build (draft, not published)

Built by the Android APK workflow from `integration/v1.6-candidate-20261106`. Package `io.github.freefrank.lostodyssey.steprant`, versionName 1.6 / versionCode 1060000, signed with the same certificate as 1.5 (SHA-256 `ecaadcdc715ff7e4aeaaf265ec11f1854d4039175dbc7e53fd4f86fd03af6f3f`), so it installs over 1.5.

## Changes since 1.5

- Audio: no AAudio reopen on every return from a long background pause; the XMA decoder no longer holds its lock for a whole voice and runs on performance cores at audio priority; selectable audio buffer (43/64/85 ms) and timing peaks in the log; optional per-voice XMA locks (diagnostics page, restart).
- Graphics: "Mobile 30 FPS" preset (1080 internal, 30 FPS, SGSR1, Low shadows, compact targets, power and refresh hints); optional compact 8/10-bit render targets with their pipelines prefetched; optional ADPF power hints; display refresh matched to the frame rate; 900p render resolution.
- SGSR1: stays enabled after a swapchain format change; the log and the performance overlay say whether it is active and, if not, why.
- Title screen: its full-screen mesh pattern is no longer drawn.
- Android diagnostics, for A/B tests (off by default): shader constants in uniform buffers (experimental; the first start compiles the shaders on the phone, several minutes), one render pass for depth-only draws, Turnip render mode (Automatic / Sysmem / GMEM).
- Diagnostics: per-render-pass GPU profile in Tester recordings.

## Validation

CI: native ARM64 build, JVM tests, lint, APK Signature Scheme v2, `zipalign -c -P 16 4`, runtime structure check; host regression, presentation readback and integration workflows. **Not yet tested on a device**: gameplay, FPS, temperature, audio listening and A/V sync on the Xiaomi 14 are still to be checked. New options are off by default except the display refresh hint.

Game data is supplied by the player; this APK contains no game executable or assets. GPL-3.0; dependency notices are included in the app.
