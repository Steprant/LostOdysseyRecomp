# Steprant Android fork — November 2026 update

This experimental fork builds on [Lost Odyssey Recomp](https://github.com/freefrank/LostOdysseyRecomp) by freefrank and its contributors. The integration branch combines the official v1.1.0 base with Android-focused changes and retains shared fixes proposed upstream. Its aims are smoother play, efficient rendering, and sustained performance with lower heat and battery use where measurements support it. These are goals, not measured guarantees.

## What's included

- **Optional Qualcomm SGSR1:** spatial reconstruction in the existing Vulkan presentation path. In **Settings → Graphics**, turn **SGSR1 On**, choose the **High** scaling filter, set an input resolution below the output (for example, 720p rendering to 1080p), save and restart. It applies to eligible SDR upscaling; native-size output, HDR, Standard filtering and temporal-upscaler output keep their normal paths. See [SGSR1 details](notes/sgsr1-presentation.md).
- **TV and HDMI mode:** experimental independent external display output, 16:9 fitting, 720p/1080p mode requests, controller forwarding and optional screen dimming. Android and the display decide the actual mode. The phone screen is dimmed, not physically powered off. Open **Settings → System → TV mode (Android)** or **CTRL → TV mode**. See the [TV guide](ANDROID_TV_HDMI.md).
- **Renderer and CPU work:** changes target redundant resolve barriers, read-only depth draws, covered clears, texture descriptor work, pipeline lookups and shadow bookkeeping. Their effect depends on scene and device. [GPU investigation](notes/rapporto-gpu-android-20261026.md).
- **Android diagnostics:** opt-in in-game recordings for frame intervals, CPU, audio queues and available thermal/battery data, with additional profiles and ZIP export. Open **Settings → System → Android diagnostics**. Availability depends on the device and Android. Battery temperature is not a GPU temperature. [Tester guide](ANDROID_TESTER.md).
- **Audio experiments:** bounded recovery of the output queue after stalls, a guard against unnecessary AAudio reopenings, and a no-progress XMA scheduling fix. The code does not establish that crackling or A/V synchronization is solved on a device. [Technical report](reports/steprant-20261104-audio-sgsr1.md).
- **Other work:** experimental MediaCodec support for supported replacement videos (not a fix for original WMV playback), HDR transfer work and expanded debug-menu skill data. Each remains subject to its own platform and game-data conditions. [Movie notes](notes/android-movie-decoder.md).

## Installation

Steprant's package ID is `io.github.freefrank.lostodyssey.steprant`, with the launcher name **Lost Odyssey Steprant** and its own icon. It can coexist with the official app. Private settings and saves are separate: export saves from the original app and import them into the fork if needed. Later updates of the Steprant package need the same signing key. Android 8.0 or later, an ARM64 device and Vulkan are required.

Supply your own supported game data and import it as described in the [installation guide](INSTALLING.md#android). Do not send game files or private saves in bug reports.

The November 4 Android test APK is named `LostOdyssey-Steprant-v1.1.0-SGSR1-AudioFix.apk`. Its SHA-256 is:

```text
5072c0f05457c5fa4247971a1685cd6bd41eaedeccdd92ea490b83b8fe88a99e
```

The native runtime reports `ffadae541193-dirty`. That identifies the revision used in the build but also indicates local changes at build time; an exact match to the committed source tree has not been established. The ZIP integrity and checksum were independently rechecked. The APK is available as a [GitHub test release](https://github.com/Steprant/LostOdysseyRecomp/releases/tag/v1.1.0-steprant.20261104). [Download the APK](https://github.com/Steprant/LostOdysseyRecomp/releases/download/v1.1.0-steprant.20261104/LostOdyssey-Steprant-v1.1.0-SGSR1-AudioFix.apk) or read the [release notes](STEPRANT_RELEASE_NOTES.md).

## Testing and contributions

The codebase includes synthetic audio and Vulkan readback tests and prior Android build and JVM checks, described in the [November 4 report](reports/steprant-20261104-audio-sgsr1.md). A build or synthetic test does not establish gameplay, sound quality, stable 60 FPS, lower heat or battery savings. Compare identical scenes and settings over sustained runs on actual hardware.

General changes are shared for upstream review, including [SGSR1 #717](https://github.com/freefrank/LostOdysseyRecomp/pull/717), [AAudio #767](https://github.com/freefrank/LostOdysseyRecomp/pull/767), and [audio queue/XMA #769](https://github.com/freefrank/LostOdysseyRecomp/pull/769). These proposals are drafts and are not equivalent to upstream releases. Check their current status before relying on them.

For a fork-specific issue, include the Steprant build, Android version, device and driver, settings, scene and reproduction steps. Export diagnostic logs only after reviewing them for private information; do not report a fork-only regression upstream without confirming it in an upstream build.
