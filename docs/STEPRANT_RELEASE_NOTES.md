# Steprant Android 1.5

**[Download APK 1.5](https://github.com/Steprant/LostOdysseyRecomp-Android/releases/download/v1.5-steprant.20261104/LostOdyssey-Steprant-v1.5-20261104.apk) · [GitHub release](https://github.com/Steprant/LostOdysseyRecomp-Android/releases/tag/v1.5-steprant.20261104) · [Android guide](STEPRANT_ANDROID.md)**

This Android edition builds on the official Lost Odyssey Recomp v1.1.0 by **Frank / freefrank and the original community**. Steprant's 1.5 version is independent of upstream's numbering.

## Highlights

- **Qualcomm SGSR1:** optional spatial reconstruction for eligible Vulkan SDR upscaling. Select **Settings → Graphics → SGSR1 → On**, use the **High** filter and a lower render resolution, such as 720p → 1080p, then save and restart. It is not frame generation; native-size output, downscaling, HDR and temporal-upscaler output use their normal paths.
- **TV/HDMI:** independent 16:9 external output, resolution requests and controller forwarding. With phone blackout enabled, the black phone screen returns **10 seconds after the last touch**; TV output continues. The phone panel is dimmed, not physically powered off.
- **Extreme Tester / Intensive profile:** recording has **no two-minute limit** and continues until manually stopped or an I/O/storage error. It includes configurable 250/500/1000/2000 ms process sampling, bounded trace storage, memory samples, ZIP export and an estimated 1% low accepted-present rate. Available thermal/battery data depends on Android. Diagnostics can add overhead.
- **EXP ×2 and SP ×2:** independent saved switches in **LB+RB → Cheats → Quick tools**. EXP keeps the game's battle cap; SP doubles the victory award before skill learning. Existing skill progress and gold are unchanged.
- **Audio:** existing queue recovery, AAudio reopen protection and XMA scheduling fixes are retained. 1.5 also corrects an unsigned-wrap value in guest-wait diagnostic logs. **Some crackling/audio disturbances remain**; this logging correction is not evidence of a new audio root-cause fix.
- **Android efficiency:** renderer/CPU changes reduce redundant operations in selected paths. Smoother sustained play, lower heat and longer autonomy are goals; no controlled before/after FPS, temperature or battery-life improvement has been established for 1.5.

## APK and verification

| Field | Published APK 1.5 |
|---|---|
| File | `LostOdyssey-Steprant-v1.5-20261104.apk` |
| Size | 47,694,826 bytes |
| Version name / code | `1.5` / `1050000` |
| Package / launcher | `io.github.freefrank.lostodyssey.steprant` / **Lost Odyssey Steprant** |
| Requirements | Android 8.0+ (API 26), ARM64, Vulkan |
| Release tag | `v1.5-steprant.20261104` |
| Release source commit | `a701bfb64eea1beb7b7120b547f80f6adc886789` |
| Embedded native revision | `faf4842d87a6-dirty` |
| Signing certificate SHA-256 | `ecaadcdc715ff7e4aeaaf265ec11f1854d4039175dbc7e53fd4f86fd03af6f3f` |

APK SHA-256:

```text
4b809aea520746a3b2cba56eb0ab49461914a645ab9967d8f37fe4e8f6f0871f
```

The public download was independently rechecked on **5 November 2026**: the computed SHA-256 matches GitHub's asset digest, ZIP CRC and ARM64 runtime structure pass, APK Signature Scheme v2 verifies, and `zipalign -c -P 16 4` passes. The manifest confirms the package and version above. The certificate is an Android debug certificate used for this fork's signed release APK; future updates must preserve that signing identity. The embedded `-dirty` revision records local changes at build time and does not establish a bit-for-bit source reproduction from the release tag.

The prior build record reports a complete ARM64 native build, **82 JVM tests**, **15 host tests**, **8 SGSR1 shader variants**, Android TV probe checks and lint. Those counts are prior build validation, not newly repeated gameplay tests. No physical Xiaomi 14 gameplay run was performed in this publication check. Stable 60 FPS, HDMI/audio routing, A/V synchronization and thermal/battery improvements still require actual-device validation. The November 4 diagnostic sessions rendered 1080 internally to 720 output and therefore do not demonstrate the SGSR1 upscaling pass.

## Installation, data and credits

The Steprant icon and package let this edition coexist with the official app. Private saves/settings are separate; export/import saves when moving between them. Supply your own supported game data. No game input or private diagnostic archive is attached to the release.

The runtime remains [GPL-3.0](../LICENSE). Dependency notices remain in the source tree and packaged license directory, including [SGSR1's BSD-3-Clause license](../thirdparty/licenses/SnapdragonGSR.txt). All original project and dependency credits are retained. This is an unofficial Android fork, maintained separately from [Frank's official project](https://github.com/freefrank/LostOdysseyRecomp).

See [Android build inputs and workflow limitations](notes/android-fork-build-inputs.md) for the unresolved private-input requirement. The previous 1.1.0 APK and its historical records remain available in the release history; they are not the current 1.5 download.
