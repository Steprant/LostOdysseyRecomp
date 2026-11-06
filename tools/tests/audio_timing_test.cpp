#include <apu/audio_timing.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main()
{
    using namespace apu::detail;
    using namespace std::chrono;
    using clock = steady_clock;

    // Resume watchdog: logs of 2026-11-04 reopened AAudio "158405 ms after
    // resume" because the full-queue wait began before a 158 s background
    // pause. Only the time since the foreground return may count.
    const auto t0 = clock::time_point(seconds(1000));
    const auto waitStarted = t0;
    const auto resumed = t0 + milliseconds(158405);
    assert(StalledSinceResume(resumed, waitStarted, resumed) == clock::duration::zero());
    assert(StalledSinceResume(resumed + milliseconds(100), waitStarted, resumed) == milliseconds(100));
    assert(StalledSinceResume(resumed + milliseconds(100), waitStarted, resumed) < milliseconds(300));
    // A genuinely stuck device after the resume still exceeds the timeout.
    assert(StalledSinceResume(resumed + milliseconds(400), waitStarted, resumed) >= milliseconds(300));
    // A wait that began after the resume is measured from its own start.
    assert(StalledSinceResume(resumed + seconds(5), resumed + seconds(4), resumed) == seconds(1));

    // Reserve choices: platform default, 12 or 16; anything else falls back.
    assert(ReserveFrames(0, 8) == 8 && ReserveFrames(12, 8) == 12 && ReserveFrames(16, 8) == 16);
    for (uint32_t bad : {1u, 4u, 7u, 9u, 13u, 17u, 64u, 0xFFFFFFFFu}) assert(ReserveFrames(bad, 8) == 8);
    assert(ReserveFrames(12, 4) == 12 && ReserveFrames(0, 4) == 4);

    assert(DeviceSampleFrames(0, 512) == 512 && DeviceSampleFrames(1024, 512) == 1024);
    assert(DeviceSampleFrames(256, 512) == 256 && DeviceSampleFrames(2048, 512) == 2048);
    for (uint32_t bad : {128u, 300u, 768u, 4096u}) assert(DeviceSampleFrames(bad, 512) == 512);

    // XMA chunk continuation needs an exhausted budget and some progress.
    assert(!ContinueXmaChunk(false, 10, 2, 0, 9, false, true));
    assert(!ContinueXmaChunk(true, 10, 10, 64, 64, false, false));
    assert(ContinueXmaChunk(true, 10, 8, 64, 64, false, false));
    assert(ContinueXmaChunk(true, 10, 10, 64, 96, false, false));
    assert(ContinueXmaChunk(true, 10, 10, 64, 64, false, true));

    // Peaks: largest value wins across threads, Take() resets.
    PeakMicros peak;
    assert(peak.Take() == 0);
    peak.Record(-milliseconds(5));
    assert(peak.Take() == 0);
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t)
        threads.emplace_back([&, t] { for (int i = 0; i < 1000; ++i) peak.Record(microseconds(t * 1000 + i)); });
    for (auto& thread : threads) thread.join();
    assert(peak.Take() == 7999);
    assert(peak.Take() == 0);
    std::cout << "PASS audio timing helpers (resume watchdog, reserve, device frames, XMA chunks, peaks)\n";
}
