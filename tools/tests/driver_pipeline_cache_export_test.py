"""Exercise the renderer's cache writer with a fake driver and real cache files.

No game assets or GPU are required. Production writer and async polling methods
are extracted to cover allocation decisions and exporter error propagation.
"""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]

PRELUDE = r'''
#include <gpu/driver_pipeline_cache.h>
#include <cstdio>
#include <future>
#include <new>
#include <stdexcept>

static void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
// Only the nonempty error matters here; no dependency on the runtime's fmt.
namespace fmt {
template<class... Args> std::string format(const char* text, Args&&...) { return text; }
}
static unsigned warnings = 0;
#define LOG_WARNING(...) (++warnings)
struct Driver {
    size_t reportedSize = 0;
    unsigned exports = 0;
    bool exportAllowed = false, exportSucceeds = true, failAllocation = false;
    std::vector<uint8_t> payload{1, 2, 3, 4};
    size_t getPipelineCacheSize() const { return reportedSize; }
    bool getPipelineCacheData(std::vector<uint8_t>& data) {
        ++exports;
        Check(exportAllowed, "cache larger than the cap was exported before rejection");
        if (failAllocation) throw std::bad_alloc();
        if (!exportSucceeds) return false;
        data = payload;
        return true;
    }
};
struct Renderer {
    Driver* device;
    std::atomic<size_t> driverCacheBytes{0};
    std::filesystem::path driverCachePath;
    std::future<std::string> driverCacheWrite;
'''

TAIL = r'''
};
int main(int argc, char** argv) try {
    Check(argc == 2, "expected scratch directory");
    namespace dpc = gpu::driver_pipeline_cache;
    const auto path = std::filesystem::path(argv[1]) / "driver-cache.bin";
    const std::vector<uint8_t> saved{9, 8, 7};
    std::string error;
    Check(dpc::Write(path, saved, error), "seed cache file");
    auto contents = [&] {
        std::vector<uint8_t> data;
        Check(dpc::Read(path, data, error) == dpc::ReadStatus::Loaded, "read cache file");
        return data;
    };
    Driver driver;
    Renderer renderer{&driver, saved.size(), path};

    // A driver-reported oversized cache must never request the export allocation.
    driver.reportedSize = dpc::kMaxBytes + 1;
    Check(!renderer.WriteDriverPipelineCache().empty(), "oversized cache not reported");
    Check(driver.exports == 0 && contents() == saved, "oversized cache touched export or file");
    Check(renderer.driverCacheBytes == driver.reportedSize, "oversized size not remembered");
    Check(renderer.WriteDriverPipelineCache().empty(), "unchanged oversized cache warned again");
    Check(driver.exports == 0, "unchanged oversized cache exported");
    ++driver.reportedSize;
    Check(!renderer.WriteDriverPipelineCache().empty(), "further oversized growth not reported");
    Check(driver.exports == 0 && contents() == saved, "further oversized growth exported");

    // At the cap, export is permitted. The fake backend returns a small blob:
    // pipeline creation may change the actual size between query and export.
    renderer.driverCacheBytes = 0;
    driver.reportedSize = dpc::kMaxBytes;
    driver.exportAllowed = true;
    Check(renderer.WriteDriverPipelineCache().empty(), "at-cap cache rejected");
    Check(driver.exports == 1 && contents() == driver.payload, "at-cap export not written");
    Check(renderer.driverCacheBytes == driver.payload.size(), "actual exported size not recorded");

    driver.reportedSize = driver.payload.size();
    Check(renderer.WriteDriverPipelineCache().empty() && driver.exports == 1, "unchanged cache exported");
    --driver.reportedSize;
    Check(renderer.WriteDriverPipelineCache().empty() && driver.exports == 1, "smaller cache exported");

    // Backends that cannot query their size must retain the export fallback.
    driver.reportedSize = 0;
    driver.payload.push_back(5);
    Check(renderer.WriteDriverPipelineCache().empty(), "unknown-size fallback failed");
    Check(driver.exports == 2 && contents() == driver.payload, "unknown-size cache not written");

    driver.reportedSize = driver.payload.size() + 1;
    driver.exportSucceeds = false;
    const auto previous = renderer.driverCacheBytes.load();
    Check(renderer.WriteDriverPipelineCache().empty(), "failed export returned an error");
    Check(driver.exports == 3 && contents() == driver.payload, "failed export changed the file");
    Check(renderer.driverCacheBytes == previous, "failed export changed the remembered size");

    // Export allocates even for a cache below the cap. Its allocation failure
    // must become an error result, including when the render thread polls a
    // completed background save (that future.get() is outside its try block).
    driver.exportSucceeds = true;
    driver.failAllocation = true;
    renderer.SaveDriverPipelineCache(false);
    Check(renderer.driverCacheWrite.valid(), "background writer did not start");
    renderer.driverCacheWrite.wait();
    bool escaped = false;
    try { renderer.SaveDriverPipelineCache(false); }
    catch (const std::bad_alloc&) { escaped = true; }
    Check(!escaped, "async exporter allocation error escaped into the render thread");
    Check(warnings == 1, "background export error not reported");
    Check(!renderer.driverCacheWrite.get().empty(), "export allocation failure not returned as an error");
    Check(contents() == driver.payload && renderer.driverCacheBytes == previous,
        "allocation failure changed the file or remembered size");
    Check(!renderer.WriteDriverPipelineCache().empty(), "direct export allocation failure not contained");
    driver.failAllocation = false;
    driver.payload.push_back(6);
    Check(renderer.WriteDriverPipelineCache().empty(), "retry after allocation failure failed");
    Check(contents() == driver.payload && renderer.driverCacheBytes == driver.payload.size(),
        "retry did not persist the cache");
    std::filesystem::remove(path);
    std::puts("PASS: production cache export: size cap, file preservation, async/direct allocation failure reporting, retry");
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'LostOdysseyRecomp/gpu/renderer.cpp').read_text()
    begin = '            void SaveDriverPipelineCache(bool wait)\n'
    end = '            // Caller holds pipelineRecipeMutex or is the render thread.\n'
    if source.count(begin) != 1 or source.count(end) != 1:
        raise RuntimeError('Production extraction boundary changed')
    method = source[source.index(begin):source.index(end)]
    cpp = out / 'driver_cache_export.cpp'
    cpp.write_text(PRELUDE + method + TAIL)
    flags = ['-std=c++20', '-pthread', '-I' + str(ROOT / 'LostOdysseyRecomp')]
    if args.sanitize:
        flags += ['-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined']
    else:
        flags += ['-O2']
    binary = out / 'driver_cache_export'
    build = subprocess.run([args.cxx, *flags, str(cpp), '-o', str(binary)],
                           capture_output=True, text=True, timeout=120)
    (out / 'build.log').write_text(build.stdout + build.stderr)
    if build.returncode:
        print(build.stdout + build.stderr, end='')
        return build.returncode
    run = subprocess.run([str(binary), str(out)], capture_output=True, text=True, timeout=30)
    (out / 'run.log').write_text(run.stdout + run.stderr)
    print(run.stdout + run.stderr, end='')
    return run.returncode


if __name__ == '__main__':
    raise SystemExit(main())
