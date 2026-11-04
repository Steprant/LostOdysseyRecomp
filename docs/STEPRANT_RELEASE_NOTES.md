# Steprant Android v1.1.0 — test build

> Prepared release notes. The APK has not yet been published as a GitHub release asset.

An experimental Android-focused build based on Lost Odyssey Recomp v1.1.0. The fork aims to improve sustained performance and investigate heat and battery use. Device benchmarks are still needed.

## Highlights

- Optional Qualcomm SGSR1 for eligible Vulkan SDR upscaling. Turn **Settings → Graphics → SGSR1** on, select the **High** filter and a lower render resolution such as 720p for 1080p output; save and restart.
- Experimental TV/HDMI output with 16:9 fitting, resolution requests, controller forwarding and optional phone dimming.
- Renderer and CPU work to reduce redundant graphics operations, with an in-game Android diagnostic mode for frame timing, audio and available thermal/battery readings.
- Experimental audio queue recovery, AAudio stall detection and XMA scheduling changes. MediaCodec replacement-video support, HDR work and debug-menu skill additions are also included.
- Separate **Lost Odyssey Steprant** app identity and icon. Its package can coexist with the official app; private saves and settings are separate.

## APK

- File: `LostOdyssey-Steprant-v1.1.0-SGSR1-AudioFix.apk`
- Android ARM64, Android 8.0 or later, Vulkan required.
- SHA-256: `5072c0f05457c5fa4247971a1685cd6bd41eaedeccdd92ea490b83b8fe88a99e`
- Embedded runtime revision: `ffadae541193-dirty`. The exact relationship between local build changes and the committed source tree has not been established.
- Supply your own supported game data. The APK is not a substitute for the game discs.

This is a test build. Audio quality, A/V sync, stable 60 FPS, image quality, power consumption and thermals need physical-device checks; improvements are not guaranteed.

Full details: [Android fork update](STEPRANT_ANDROID.md). General changes are proposed upstream in [SGSR1 #717](https://github.com/freefrank/LostOdysseyRecomp/pull/717), [AAudio #767](https://github.com/freefrank/LostOdysseyRecomp/pull/767), and [audio #769](https://github.com/freefrank/LostOdysseyRecomp/pull/769).
