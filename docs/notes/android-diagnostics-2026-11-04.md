# Android diagnostic review — 2026-11-04

The two private archives `LostOdyssey-diagnostics-20261104-1826.zip` and
`LostOdyssey-diagnostics-20261104-1909.zip` were inspected locally. They are
not committed or uploaded. Both report Xiaomi 23127PN0CG, Android 16, SM8650,
Turnip Adreno 750, advanced profile, 1280×720 output, 1080 internal render
height and `sgsr1=1`. The recorded frame-rate setting differs from the target
at recording start (60→30 in the first archive, 30→60 in the second); the
settings snapshot is from export time. No exact time of the switch or matching
scene can be reconstructed.

| Archive | Duration | Retained accepted-present intervals | p95 / p99 interval | Queue-empty delta / clock resets | Process CPU median (one core) | Battery temperature |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 18:26 | 652 s | 24,225 | 33.88 / 49.53 ms | 93 / 2 | 181% | 43.0–45.0 °C |
| 19:09 | 2,431 s | 63,196 (last 1,831 s) | 34.29 / 43.21 ms | 439 / 12 | 202% | 40.2–46.9 °C |

The interval statistics describe accepted present calls, not displayed FPS.
The rolling trace does not retain every frame of the longer session. The first
trace contains an approximately 57.9-second gap. Both show zero XMA decode
failures and zero synchronous pipeline-creation failures. Android thermal
status remained 0; battery temperature is neither CPU nor GPU junction
temperature. Hardware GPU utilization, power, VRAM, physical audio underruns,
output scanout timestamps and precise A/V latency are unavailable here.

The `runtime.log` entries have a shared process prefix, so repeated warnings
in the two ZIPs are not independent occurrences. They include a single audio
device reopen after no drain for 57,790 ms following resume, several
`WAIT_REG_MEM stalled 5s` and GPU progress warnings. A guest wait-duration
diagnostic printed unsigned-wrap values near 2^64 ms; this value cannot be
used as an elapsed duration. The watchdog captured its timestamp before a
concurrent wait began; the duration is now clamped to zero in this case. This
corrects the log only, with no change to guest wait behavior. The audio
queue-empty counter is a software queue
observation, not proof of Android hardware underrun. The log does not identify
the cause of occasional crackle. Existing audio fixes were retained; no
untested buffer or priority change was introduced.

The SGSR1 shader and presentation branches exist, and the log reports its
SDR/high-quality selection. The actual filter runs only for upscaling with
input smaller than output. Both recordings instead report 1080 internal
height and 720 output height. Consequently these traces do not demonstrate
SGSR1 execution or any quality, efficiency or Adreno speedup. SGSR2 requires
per-pixel motion vectors, temporal history and reconstruction validation;
there is no technically verified integration to enable here.

Earlier retained November 3 and November 4 sessions reported queue-empty
deltas of 497, 232, 317, 119 and 838, with corresponding clock resets 27,
7, 26, 9 and 72, and zero XMA failures. Different durations, scenes and
settings prevent a causal before/after comparison. A useful device experiment
is the same fixed scene at 720→1080 SDR, High filter, SGSR1 off/on in alternating
order with warm caches, fixed 30 FPS baseline and separate 60 FPS attempt;
record displayed FrameTimeline, frame intervals, battery/thermal trajectory,
power if available and audio after resume. Compare the same input resolution
and inspect UI, thin edges, HDR and bloom visually. A 60 FPS or audio fix is
not claimed from these archives.
