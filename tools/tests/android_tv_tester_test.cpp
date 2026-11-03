#include "diagnostics/android_tester.h"
#include "gpu/android_tv.h"
#include <cstdlib>
#include <iostream>

static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}

int main() {
    using namespace diagnostics::android_tester;
    for (unsigned fps : {30u, 60u, 90u, 120u}) {
        Recorder recorder;
        recorder.enabled = true;
        require(recorder.Start(1000000000, fps), "start recording");
        uint64_t now = 1000000000;
        for (int output : {720, 1080, 0, 1080, 0}) {
            gpu::android_tv::SetOutput(output != 0, output);
            const auto tv = gpu::android_tv::Snapshot();
            require(tv.height == unsigned(output), "TV output survives independent recorder changes");
            require(recorder.recording && recorder.TargetFps() == fps, "TV migration preserves recording target");
            // A destroyed/recreated surface can produce no-present callbacks.
            recorder.Add(now + 1, 100, 50, false, 0, 0);
            now += 1000000000 / fps;
            recorder.Add(now, 100, 50, true, 0, 0);
        }
        auto frames = recorder.Drain();
        require(frames.size() == 10, "surface gaps do not lose diagnostic samples");
        for (size_t i = 0; i < frames.size(); ++i) {
            const auto& frame = frames[i];
            require(frame.presented == bool(i % 2), "presentation state preserved");
            require(frame.intervalNs == ((i > 1 && i % 2) ? 1000000000 / fps : 0),
                    "surface callbacks do not corrupt 30/60/90/120 FPS intervals");
        }
        recorder.Stop(now);
        recorder.ReleaseBuffer();
        recorder.collectorIdle = true;
        require(recorder.Start(now + 1, fps), "recording restarts after TV detachment and collector completion");
        recorder.Stop(now + 2);
    }
    std::cout << "PASS: TV detach/reattach and Tester coexist at 30/60/90/120 FPS\n";
}
