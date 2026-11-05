# Steprant Android 1.5

**[Download APK 1.5](https://github.com/Steprant/LostOdysseyRecomp-Android/releases/download/v1.5-steprant.20261104/LostOdyssey-Steprant-v1.5-20261104.apk) · [Release](https://github.com/Steprant/LostOdysseyRecomp-Android/releases/tag/v1.5-steprant.20261104) · [Checksum and validation](STEPRANT_RELEASE_NOTES.md#apk-and-verification)**

This Android-focused edition builds on [Lost Odyssey Recomp](https://github.com/freefrank/LostOdysseyRecomp) by **Frank / freefrank and the original community**. The integration branch retains the official **v1.1.0 base** and Android work; **1.5** is Steprant's app version. Its goals are smoother sustained play, lower redundant CPU/GPU load, less heat and longer battery life. No measured device-wide FPS, thermal or battery improvement is claimed for this release.

## What's included

- **Optional Qualcomm SGSR1:** spatial reconstruction in the existing Vulkan presentation path. In **Settings → Graphics**, turn **SGSR1 On**, choose the **High** scaling filter, set input resolution below output (for example, 720p rendering to 1080p), save and restart. It applies to eligible SDR upscaling. Native-size output, downscaling, HDR, Standard filtering and temporal-upscaler output keep their normal paths. It is not frame generation. [SGSR1 details](notes/sgsr1-presentation.md).
- **TV and HDMI mode:** independent external display output, 16:9 fitting, 720p/1080p mode requests, controller forwarding and optional phone blackout. Android and the display decide the actual mode. Touch wakes the phone; **10 seconds after the last touch, blackout returns automatically**, without covering TV output. The panel is dimmed and remains powered. Open **Settings → System → TV mode (Android)** or **CTRL → TV mode**. [TV guide](ANDROID_TV_HDMI.md).
- **Renderer and CPU work:** changes target redundant resolve barriers, read-only depth draws, covered clears, texture descriptors, pipeline lookups and shadow bookkeeping. These implemented paths do not by themselves prove whole-game speed or energy gains. [GPU investigation](notes/rapporto-gpu-android-20261026.md).
- **Extreme Tester:** the **Intensive** diagnostic profile is no longer limited to two minutes. Manual recording continues until stopped or an I/O/storage error, with selectable 250–2000 ms process sampling, bounded trace files, CPU, audio queues, frame intervals, process-memory samples and available thermal/battery data. ZIP export includes an estimated 1% low accepted-present rate, not physical display FPS. Open **Settings → System → Android diagnostics**. Battery temperature is not a GPU temperature; diagnostics can affect performance. [Tester guide](ANDROID_TESTER.md).
- **EXP ×2 and SP ×2:** open **LB+RB → Cheats → Quick tools**. The independent switches are saved. EXP applies after level-based calculation and keeps the game's per-battle cap. SP doubles victory skill points before skill learning consumes them; existing progress and gold are unchanged.
- **Audio:** bounded queue recovery after stalls, protection against unnecessary AAudio reopening and XMA scheduling fixes. **Some crackling/disturbances remain** and A/V synchronization needs further device testing. The new guest-wait duration clamp corrects diagnostics; the supplied logs do not establish a new audio root cause. [Earlier technical report](reports/steprant-20261104-audio-sgsr1.md).
- **Other work:** MediaCodec for supported replacement videos, HDR work and expanded debug-menu skill data. MediaCodec does not fix original WMV playback. Each feature keeps its platform and game-data conditions. [Movie notes](notes/android-movie-decoder.md).

## Installation

Install **`LostOdyssey-Steprant-v1.5-20261104.apk`** from the download above. It requires Android 8.0+, ARM64 and Vulkan. The launcher name is **Lost Odyssey Steprant**, with the Steprant icon and package `io.github.freefrank.lostodyssey.steprant`. The manifest reports **versionName 1.5 / versionCode 1050000**.

The app can coexist with the official package. Private settings and saves are separate: export saves from the original app and import them into the fork if needed. Future Steprant updates must retain the same package and signing key. Game data uses `Android/data/io.github.freefrank.lostodyssey.steprant/files/game/` or the folder you select; exported logs are under the same package's `files/logs/` directory.

Supply your own supported game data and import it as described in the [installation guide](INSTALLING.md#android). Do not include game files or private saves in bug reports.

The published APK is **47,694,826 bytes**. SHA-256:

```text
4b809aea520746a3b2cba56eb0ab49461914a645ab9967d8f37fe4e8f6f0871f
```

On 5 November 2026 the public APK was downloaded again, its checksum matched GitHub's digest, and its signature, manifest, ZIP/ARM64 structure and 16 KB alignment passed. The embedded native revision is `faf4842d87a6-dirty`; local build changes are indicated, so a bit-for-bit reproduction from the release tag is not established. [Full release verification](STEPRANT_RELEASE_NOTES.md#apk-and-verification).

## Testing and contributions

Prior build validation reports 82 JVM tests, 15 host tests, eight SGSR1 shader variants, TV probe checks, lint and a full signed ARM64 build. Synthetic/build checks and the public APK checks above are separate from gameplay validation. No physical Xiaomi 14 run was performed for this publication check. Stable 60 FPS, image quality, A/V synchronization, HDMI compatibility, diagnostics overhead and power/temperature improvements still need actual-device testing.

For performance comparisons, use identical scenes, settings, driver, diagnostic mode and starting temperature over sustained runs. The November 4 1080-internal-to-720-output traces do not exercise SGSR1 upscaling. [Source/build report](reports/steprant-20261104-audio-sgsr1.md) · [Authorized build inputs and Actions](notes/android-fork-build-inputs.md).

General changes are proposed upstream, including [SGSR1 #717](https://github.com/freefrank/LostOdysseyRecomp/pull/717), [AAudio #767](https://github.com/freefrank/LostOdysseyRecomp/pull/767) and [audio queue/XMA #769](https://github.com/freefrank/LostOdysseyRecomp/pull/769). A proposal is not an official upstream release; consult each pull request for its current review status.

For a fork-specific problem, record the Steprant version, Android version, device/driver, settings, scene and reproduction steps. Review exported diagnostics for private information before sharing. The fork's issue tracker is currently disabled; do not report a fork-only regression upstream without reproducing it in an official build.

The runtime retains its [GPL-3.0 license](../LICENSE), dependency notices and original community credits. The official v1.1.0 base, the required integration branch name and historical reports retain their original version references; they are not obsolete APK download instructions.
