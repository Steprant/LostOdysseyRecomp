# Android audio: resume reopen, XMA mutex contention and buffer A/B — 2026-11-06

Branch `fix/android-audio-xma-contention-20261106`, based on
`integration/v1.1.0-steprant-all` at `077ac142`. Intended for the 1.6 build.

## Evidence from the 2026-11-04 diagnostics

Three ZIPs, two processes: 18:26 and 19:09 are snapshots of one run of
`1.1.0-Steprant-SGSR1-AudioFix` (60 FPS); 23:40 is 1.5 at 30 FPS.

| | old build, 60 FPS, ~57 min | 1.5, 30 FPS, ~33 min active |
|---|---:|---:|
| `starved` (SDL queue empty at submit) | 700 | 151 |
| `clock_resets` | 14 | 1 |

The builds differ in frame rate too, so this is not a build A/B.

- 81 of 94 queue-empty events in the 1.5 trace occur on frames with a normal
  present interval (< 40 ms). The renderer is not stalling the audio driver.
- Android thermal status stays 0 throughout both runs (battery 36–47 °C).
- Frames with a queue-empty event show about twice the XMA decode attempts
  (1.5: median 13 vs 9; old build: 19 vs 10) and more decode wall time
  (old build p90: 3.9 ms vs 0.7 ms). Events arrive in bursts.
- Both AAudio reopens ("not draining 158405 ms / 57790 ms after resume")
  match the background gaps in `frame_timing.csv` (158.6 s / 57.9 s): the
  reopen fired at the moment of resume.

## Changes

1. **Resume watchdog.** `ReopenIfStalledAfterResume` measured the stall from
   the start of the full-queue wait, which began before the background pause.
   It now counts from `max(waitStarted, resumedAt)`, so a healthy stream is no
   longer closed (dropping the queue, ~70 ms reopen) on every resume after a
   long pause. A device still not draining 300 ms after the resume is reopened
   as before.
2. **XMA decoder contention.** Guest kick/lock/clear commands and context
   allocation take the global XMA mutex that the worker holds for a whole
   `Work(id)`, FFmpeg decodes included; the worker ran at normal priority
   with no core placement while the audio driver runs at audio priority.
   - The worker now uses the game's performance cores and the audio thread
     priority (`LO_XMA_DECODER_BOOST=0` restores the old placement).
   - `Work` decodes at most 16 iterations per mutex hold (`LO_XMA_WORK_BUDGET`,
     `0` = previous single hold of up to 4096). A context with remaining work
     and progress is re-enabled for the next worker pass; a lock/clear taken
     in between overrides it like a kick. A chunk without progress stops as
     the old guard did.
   - Between contexts the worker yields (bounded) to commands waiting on the
     mutex instead of retaking it at once.
3. **Instrumentation.** The 10 s audio log line appends, as peaks since the
   previous line: `reserve`, `wake_late_max_us`, `callback_max_us`,
   `xma_wait_max_us`, `xma_wait_audio_max_us`, `xma_hold_max_us`,
   `xma_contended_audio`. Existing fields keep their order and meaning.
4. **Buffer A/B.** Android diagnostics page → *Audio buffer*: 43 ms (default,
   8 frames), 64 ms (12) or 85 ms (16); applied live, saved as
   `audio_reserve_frames`, reported as `reserve=` in the log. Diagnostics-only
   environment overrides: `LO_AUDIO_RESERVE_FRAMES`, `LO_AUDIO_DEVICE_FRAMES`
   (SDL device buffer, 256–2048, default 512).

Defaults unchanged apart from (1) and (2).

## Verification performed

| Check | Result |
|---|---|
| `xma_streaming` with production `xma.cpp`, ASan/UBSan | 1,523 checks passed, including budgets 1/2/3/16 producing frames, ring and context identical to a single hold |
| `audio_timing_test` (resume watchdog with the logged 158,405 ms case, reserve/device validation, chunk continuation, concurrent peaks), ASan/UBSan | passed |
| `audio_queue_refill_test`, `audio_callback_test`, ASan/UBSan | passed |
| `settings_menu` suite, including the Android Tester page with the new row | 15/15 passed |
| `audio.cpp` syntax check, desktop and Android branches, against pinned SDL 3 headers and runtime stubs | passed |

Not performed: full ARM64 build/APK (needs the private build input), Android
installation, listening tests, A/V sync. Whether the XMA hypothesis explains
the audible crackling is to be decided by the new log fields on the Xiaomi 14.

## Suggested device session

Same save, 30 FPS fixed, ~10 minutes in a sound-heavy scene, Tester
*Advanced*, one background/resume at a noted time. Note the times of audible
glitches. Repeat with *Audio buffer* 64/85 ms, and once with
`LO_XMA_WORK_BUDGET=0` + `LO_XMA_DECODER_BOOST=0` (debuggable builds via ADB
extras) as the old-behaviour baseline.
