"""Check XMA output ring cursor wrapping through the production Work/Consume.

Synthetic context words and PCM replace guest memory and the packet decoder.
The actual context, ring, Work and Consume bodies are compiled; no FFmpeg,
game assets or audio device are required.
"""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]

PRELUDE = r'''
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
static void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
static uint32_t ByteSwap(uint32_t value) { return __builtin_bswap32(value); }
static unsigned warnings = 0;
#define LOG_WARNING(...) (++warnings)
#define LOG_INFO(...) ((void)0)
'''

SHELLS = r'''
struct Context {
    std::atomic<bool> allocated{false}, enabled{false};
    uint32_t remainingSubframes = 0, pcmOffset = 0;
    std::array<uint8_t, 2048> pcm{};
    int32_t freeBlocks = 0;
    uint32_t packetsSkip = 0, copiedBits = 0, workHistoryCount = 0, publishedInputOffset = 0;
    bool hasPublishedInputOffset = false;
    uint64_t explicitSeeks = 0, writtenSamples = 0;
    std::chrono::steady_clock::time_point diagnosticStart{}, diagnosticReport{};
    struct WorkSnapshot { ContextData data; uint32_t skip, copied; };
    std::array<WorkSnapshot, 8> workHistory{};
    void ReportSamples(const char*, uint32_t) {}
};
static Context g_contexts[1];
static std::array<uint32_t, 16> guest{};
static std::vector<uint8_t> memory;
static unsigned packets = 0;
static bool g_trace = false, g_traceContextWrites = false;
static uint8_t* ContextHost(uint32_t id) {
    Check(id == 0, "unexpected context id");
    return reinterpret_cast<uint8_t*>(guest.data());
}
static uint8_t* Physical(uint32_t address) {
    Check(address == 0x1000, "unexpected output pointer");
    return memory.data();
}
static void ProcessPacket(Context& ctx, ContextData& data) {
    Check(!ctx.remainingSubframes && data.anyInputValid(), "unexpected packet request");
    ++packets;
    ctx.pcm.fill(0x6D);
    ctx.pcmOffset = 0;
    ctx.remainingSubframes = 1;
    data.setInput0Valid(false);
    data.setInput1Valid(false);
}
'''

TAIL = r'''
static unsigned cases = 0;
static void Run(uint32_t blocks, uint32_t read, uint32_t write, bool stereo) {
    ++cases;
    // Leave mapped guest memory after the logical buffer. A misplaced output
    // write must not corrupt this guard region, just as it must not corrupt an
    // adjacent guest allocation. It remains addressable for a bounded failure.
    memory.assign(34 * kOutputBytesPerBlock, 0xA5);
    auto expected = memory;
    ContextData data{};
    data.SetBits(data.d[0], 22, 5, blocks);
    data.setOutputWriteOffset(write);
    data.setOutputReadOffset(read);
    data.setOutputValid(true);
    data.setInput0Valid(true);
    data.SetBits(data.d[1], 29, 1, stereo);
    data.d[7] = 0x1000;
    data.Store(guest.data());
    const auto originalGuest = guest;
    auto& ctx = g_contexts[0];
    ctx.allocated = ctx.enabled = true;
    ctx.remainingSubframes = ctx.pcmOffset = 0;
    ctx.hasPublishedInputOffset = false;
    ctx.freeBlocks = 0;
    packets = warnings = 0;
    const uint32_t channels = stereo ? 2 : 1;
    uint32_t free = 0;
    if (blocks) {
        const uint32_t r = read % blocks, w = write % blocks;
        free = r == w ? blocks : w < r ? r - w : blocks - w + r;
        if (free >= channels) {
            for (uint32_t block = 0; block < channels; ++block)
                std::fill_n(expected.begin() + ((w + block) % blocks) * kOutputBytesPerBlock,
                            kOutputBytesPerBlock, 0x6D);
        }
    }
    Work(0);
    Check(std::all_of(memory.begin() + blocks * kOutputBytesPerBlock, memory.end(),
                     [](uint8_t v) { return v == 0xA5; }),
          "XMA output crossed the logical ring boundary");
    Check(memory == expected, "XMA PCM differs from normalized ring output");
    if (!blocks) {
        Check(packets == 0 && warnings == 1, "zero-capacity ring did not stop before decoding");
        Check(guest == originalGuest, "zero-capacity ring published a changed context");
        return;
    }
    Check(warnings == 0, "valid ring produced a warning");
    Check(ctx.freeBlocks == int32_t(free - (free >= channels ? channels : 0)),
          "free space used unwrapped read/write cursors");
    Check(packets == unsigned(free >= channels), "wrong packet count for available ring space");
    ContextData published{};
    published.Load(guest.data());
    const uint32_t expectedWrite = free >= channels ? (write % blocks + channels) % blocks : write;
    Check(published.outputWriteOffset() == expectedWrite, "wrong published write cursor");
    Check(published.outputReadOffset() == read, "decoder overwrote the guest read cursor");
    Check(published.outputValid() == !(free == channels), "wrong full-ring output validity");
    if (free < channels)
        Check(guest == originalGuest, "insufficient space changed the guest context");
}
int main() try {
    unsetenv("LO_XMA_ERROR_CAPTURE_DIR");
    // Reproduce the old out-of-ring write before sweeping every 5-bit cursor
    // combination on every nonzero capacity representable by the context.
    Run(4, 0, 7, false);
    for (bool stereo : {false, true}) {
        for (uint32_t blocks = 1; blocks <= 31; ++blocks)
            for (uint32_t read = 0; read <= 31; ++read)
                for (uint32_t write = 0; write <= 31; ++write)
                    Run(blocks, read, write, stereo);
        // Modulo must stay after the capacity validation, in both channel modes.
        Run(0, 0, 0, stereo);
        Run(0, 31, 31, stereo);
    }
    std::printf("PASS: %u production XMA ring cases: wrapped 5-bit cursors, guards, free space, PCM, stereo, publication, zero capacity\n", cases);
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
}
'''


def between(source, begin, end):
    if source.count(begin) != 1 or source.count(end) != 1:
        raise RuntimeError('Production extraction boundary changed')
    return source[source.index(begin):source.index(end)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'LostOdysseyRecomp/apu/xma.cpp').read_text()
    constants = between(source, '        constexpr uint32_t kOutputBytesPerBlock',
                        '        // Host-side view of XMA_CONTEXT_DATA:')
    context = between(source, '        struct ContextData\n', '        bool g_audioDiagnostics = false;')
    ring = between(source, '        struct Ring\n', '        void ClearContext(')
    consume_work = between(source, '        void Consume(', '        void WorkerMain(')
    cpp = out / 'xma_output_ring.cpp'
    cpp.write_text(PRELUDE + constants + context + 'static bool g_audioDiagnostics = false;\n' +
                   ring + SHELLS + consume_work + TAIL)
    flags = ['-std=c++20', '-pthread']
    if args.sanitize:
        flags += ['-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined']
    else:
        flags += ['-O2']
    binary = out / 'xma_output_ring'
    build = subprocess.run([args.cxx, *flags, str(cpp), '-o', str(binary)],
                           capture_output=True, text=True, timeout=120)
    (out / 'build.log').write_text(build.stdout + build.stderr)
    if build.returncode:
        print(build.stdout + build.stderr, end='')
        return build.returncode
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
    (out / 'run.log').write_text(run.stdout + run.stderr)
    print(run.stdout + run.stderr, end='')
    return run.returncode


if __name__ == '__main__':
    raise SystemExit(main())
