# Steprant Android: audio reserve recovery, SGSR1 and APK — 2026-11-04

Work remains on `integration/v1.1.0-steprant-all`, with official v1.1.0 in
its ancestry. The fork's `main` was not changed. This report contains aggregate
technical results; original logs, Xbox input, game assets and signing material
are excluded from Git.

## Recovered work and original evidence

Recovered all five November 3/4 diagnostic ZIPs, the November 3 log ZIP, both
previous full APKs, the dedicated development keystore, the supplied supported
`default.xex`, earlier October logs and the existing technical reports.

The November 4 `frame_timing.csv` contains 8,648 retained samples:

| Counter | First | Last | Difference |
| --- | ---: | ---: | ---: |
| Audio queue-empty observations | 37 | 875 | 838 |
| Audio deadline resets | 4 | 76 | 72 |
| XMA decode attempts | 20 | 130,021 | 130,001 |
| XMA decode failures | 0 | 0 | 0 |

The retained recording lasts approximately 292 seconds and starts with a
30 FPS target. These are cumulative counter differences, not a count of
measured hardware underruns. SDL's queued-input count can reach zero while
the backend still has output buffered. Zero decode failures does not prove
that every audible discontinuity has been identified.

The four November 3 recordings have queue-empty differences of 497, 232, 317
and 119, with deadline-reset differences of 27, 7, 26 and 9. All have zero
reported XMA decode failures. One log reopens AAudio after 39,493 ms since
resume while only six frames remain queued, below the eight-frame full limit.
The already committed false-reopen correction `4bb494d6` is retained.

The November 3 audit records 23 contributions: 19 fork integrations and four
already merged upstream. Their recorded heads or official merge commits all
remain ancestors of this branch. Shared source files also received targeted
changes and regression coverage; ancestry alone is not a gameplay test.

The original full APK matches its published verification record:
`b84e9d33042acb878f9b8bd2c24ff5f5197430ca2db26fada5746d3f81a0d598`.
Both old APKs embed revision `1d102312609e-dirty`. Their native runtimes have
the same 92,440,864-byte size and differ by seven bytes. The AudioBaseline APK
was a binary-adjusted comparison build, not a fresh compilation of the later
source head. Neither old file establishes inclusion of `4bb494d6`.

Unfinished items were therefore: a full rebuild including the later audio
changes, a controller-accessible SGSR1 setting, distinct launcher artwork and
name, and tests reconciled with the XMA rollback. All are included in the new
source/build. The new APK is compiled and packaged from source rather than
renamed from either earlier APK.

## Audio changes

The driver discarded overdue deadlines after a host stall but resumed only
one callback per audio period. Production then merely matched consumption,
leaving the intended output reserve depleted. A bounded recovery helper now
refills toward the existing eight-frame target on the guest audio driver
thread. It stops when the device is unavailable/paused, no queue progress is
made, or its finite callback budget is exhausted. Recovery also rebases the
driver deadline so the same frames do not trigger another catch-up burst.

The target remains eight 256-sample frames, approximately 42.7 ms at 48 kHz;
the device-buffer hint and queue cap are unchanged. This is a reserve-recovery
change, not a larger buffering policy or a claim about end-to-end latency.
Fast-forward keeps its existing path. `refill_frames` is added to the audio
log for the next physical-device comparison.

The XMA rollback left a no-progress case: when the current input is invalid
but the other input is valid, the worker could retry `ProcessPacket` up to
4,096 times under its mutex. The active-input guard now yields after pending
PCM is consumed. Crucially, upstream v1.1.0 empty-output retirement is retained;
the other output-valid change from #727 is not reinstated. The streaming test
now checks this deliberate behavior and rearms the output as the guest does.

The former `xma_streaming` regression failure after the rollback is resolved.
Production assembly/Work/Consume tests pass 1,509 checks with ASan/UBSan. Queue
fixtures exercise finite recovery, no progress, pauses and synthetic 30/60 FPS
scheduling with 256/512/1024-sample consumption bursts. Unavoidable stalls can
still cause discontinuities; physical listening and A/V sync remain untested.

## SGSR1 and renderer

The official Qualcomm algorithm was already present in the integrated branch,
but only an environment variable selected it. It is verified against the
pinned official source, matching after whitespace normalization; the local
adapter is separate and the BSD-3-Clause notice/license is retained.

Use **Settings → Graphics → SGSR1 → On**, **High** scaling filter, and a lower
render resolution such as **720p** for a **1080p** output surface, then Save and
restart. Off restores ordinary filtering. The saved key is `sgsr1=0/1`; Tester
exports now retain that key for subsequent comparisons. No Xiaomi-only check
or Adreno-only extension is introduced.

The existing presentation fragment and storage/compute paths use SGSR1. It
does not add a render image, history, descriptor set or GPU pass. Shader
compilation failure retains the ordinary filter. Native-size images,
reductions, Standard/bilinear, HDR, calibration and temporal-upscaler output
retain their previous behavior. Host overlays retain composition order.
Guest UI already drawn at a lower input resolution scales with the source;
native-resolution reconstruction of that UI is not implemented or claimed.

All eight production SPIR-V variants compile, without native FP16 types.
The integrated production presentation passes seven Vulkan readback modes on
Mesa lavapipe 25.2.8, including explicit setting selection, compute output,
padding, AA order, bounds, flat regions, native identity, HDR and calibration.
The extended HDR transfer mode passes 151 cases / 185,922 RGB comparisons.
The clean SGSR1 review branch passes six modes: its older base lacks the
extended HDR-transfer CLI mode, which is registered only where present.

The reproducible game-free fixture is `tools/tests/presentation`; the matching
workflow uploads its test log. See `docs/notes/sgsr1-presentation.md` for setup.
No Adreno FPS, total driver VRAM, frame-time, battery or thermal improvement
has been measured. The absence of additional image/history allocations is a
source-level property, not a total-GPU-memory measurement.

## Android identity and packaging

Launcher name: **Lost Odyssey Steprant**. Package:
`io.github.freefrank.lostodyssey.steprant`. The original S/medallion/sword mark
has adaptive and monochrome Android resources; its vector source is tracked.
The Java/JNI namespace remains `io.github.freefrank.lostodyssey`.

The distinct package, app-private data and manifest authorities allow the fork
and official app to coexist. The existing private Steprant development key is
reused, preserving update compatibility with the earlier fork APK. Its
certificate SHA-256 is
`ecaadcdc715ff7e4aeaaf265ec11f1854d4039175dbc7e53fd4f86fd03af6f3f`.
No signing key or password is committed. Existing storage/import permissions
remain intact. Shared extracted-game folders can be selected independently;
private saves can be exported/imported using the app's existing flow.

The full runtime is regenerated from the privately supplied supported input,
then compiled in Release for ARM64/API 26 with NDK 28.2.13676358. Only the
unchanged open-source Android DXC dependency is reused from the earlier
verified package, with SHA-256
`f27d9850a3b43c26eac55a33119fdd519daac7b67c3f80515dd79cb1d08a51d7`.
The new `libmain.so`, SDL and Adreno hook libraries are built from source;
the C++ runtime is supplied by the pinned NDK.
Gradle 8.11.1 / AGP 8.9.3, JDK 17 and SDK/build-tools 35 perform actual
assemble, unit-test and lint tasks. In this host, exact official Maven files
and Gradle variant metadata are mirrored for offline resolution because Java
egress is unavailable; no dependency version is substituted.

The dedicated APK filename is
`LostOdyssey-Steprant-v1.1.0-SGSR1-AudioFix.apk`. Verification includes ZIP
integrity, manifest/DEX, the separate package/label/authorities, ARM64 native
libraries and JNI exports, v2 signature, 16 KB alignment and absence of
original Xbox input/game asset files. The final delivery provides the exact
APK hash, avoiding a self-referential hash inside its source commit.

## Validation and contributions

| Actual check | Result |
| --- | --- |
| Native review regressions, ASan/UBSan | 66/66 passed |
| Settings/menu/raster/tester fixtures | 13/13 passed |
| Production XMA streaming | 1,509 checks passed |
| Android release JVM tests | 82/82 passed |
| Android release lint | No errors; 64 warnings retained |
| Movie/demux/session/host JNI protocol | 3/3 passed |
| Python discovery | 44/44 passed |
| Thermal lifecycle fixtures | 9/9 passed |
| TV/touch/aspect/resolution/pacing and Java/native command checks | Passed |
| SGSR1 SPIR-V compilation | 8/8 variants passed |
| Integrated Vulkan readbacks on software ICD | 7/7 passed |
| Clean SGSR1 review branch Vulkan/menu tests | 6/6 and 5/5 passed |
| Clean upstream audio branch queue/XMA fixtures | Passed |
| Full ARM64 runtime, signed APK and packaging checks | Passed |

The Linux home-namespace fixture is excluded on this restricted host; it is
not counted as a passing test. Host JNI protocol tests do not run Android
MediaCodec hardware. No emulator/physical Android installation, listening,
gameplay, sustained 60 FPS or Adreno performance acceptance is claimed.

Published implementation commits: `8aa9925f` (audio), `b562791c` (SGSR1
settings), `a18536e9` (fork identity). This report's commit also adds the
standalone Vulkan tests/workflow and Tester SGSR1 metadata.

General changes are proposed upstream in draft PR
[#769](https://github.com/freefrank/LostOdysseyRecomp/pull/769), audio source
commit `c87e5d5b`, and updated draft PR
[#717](https://github.com/freefrank/LostOdysseyRecomp/pull/717), source commit
`4dc2895a`. The independently proposed AAudio recovery PR #767 remains
separate. Branding/package defaults remain in the fork. No upstream merge is
performed.

The Android workflow names and verifies the new artifact. No Actions run is
available for the integration branch in the observed GitHub results, and the
connected interface cannot upload a local APK as an Actions artifact or set
its private build-input secret. The APK is built and delivered locally; no
Actions URL or successful CI run is invented.
