#include <gpu/pass_profile.h>
#include <cassert>
#include <cstdio>

int main()
{
    using gpu::pass_profile::Reporter;
    using gpu::pass_profile::Summary;
    plume::RenderPassProfile scene;
    scene.timed = true;
    scene.listNs = 20'000'000;
    scene.barrierCalls = 6; scene.copyCalls = 2; scene.dispatches = 1;
    plume::RenderPassProfile::Pass big{2403, 1080, 1, true, false, 0, 100, 20'000'000, 20'000'000, 12'000'000};
    plume::RenderPassProfile::Pass shadow{1024, 1024, 0, true, false, 1u << 30, 50, 0, 4'000'000, 3'000'000};
    plume::RenderPassProfile::Pass empty{2403, 1080, 1, true, false, 0, 0, 20'000'000, 20'000'000, 1'000'000};
    scene.passes = {big, shadow, empty};
    plume::RenderPassProfile present;
    present.timed = true; present.listNs = 2'000'000;
    present.passes = {{2670, 1200, 1, false, false, 0, 1, 0, 2670ull * 1200 * 4, 1'500'000}};

    Summary summary;
    for (int frame = 0; frame < 2; ++frame) { summary.Add(scene, false); summary.Add(present, true); }
    assert(summary.frames == 2 && summary.lists == 4 && summary.untimedLists == 0);
    const auto& stats = summary.targets.at({2403, 1080, 1, true});
    assert(stats.passes == 4 && stats.emptyPasses == 2 && stats.draws == 200 && stats.gpuNs == 26'000'000);
    assert(summary.targets.at({1024, 1024, 0, true}).clearStarts == 2);
    // Outside time: list minus its passes (20 - 16 ms) plus present (2 - 1.5 ms), per frame.
    assert(summary.listNs - summary.passNs == 2 * 4'000'000ull + 2 * 500'000ull);
    const std::string line = summary.Format();
    std::printf("%s\n", line.c_str());
    assert(line.find("passes=3.0") != std::string::npos);        // per frame
    assert(line.find("outside_ms=4.50") != std::string::npos);
    assert(line.find("2403x1080 cd passes=2.0 empty=1.0") != std::string::npos);
    assert(line.find("present: passes=1.0 ms=1.50") != std::string::npos);

    Reporter reporter;
    const auto t0 = std::chrono::steady_clock::time_point(std::chrono::seconds(100));
    assert(reporter.Add(scene, false, t0).empty());
    assert(reporter.Add(present, true, t0 + std::chrono::seconds(5)).empty());
    assert(!reporter.Add(present, true, t0 + std::chrono::seconds(11)).empty());
    assert(reporter.Add(present, true, t0 + std::chrono::seconds(12)).empty()); // new window
    std::printf("PASS pass profile summary and reporting window\n");
}
