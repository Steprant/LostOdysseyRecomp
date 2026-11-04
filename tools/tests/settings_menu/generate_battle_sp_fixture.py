"""Exercise the production battle SP hook at the verified victory call boundary."""
import argparse
from pathlib import Path
import re

PREAMBLE = r'''
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <settings/config.h>
#include <debug/battle_sp.h>
#define LOG_INFO(...) ((void)0)
#define LOG_WARNING(...) ((void)0)
struct Reg { union { uint64_t u64 = 0; uint32_t u32; }; };
struct PPCContext { Reg r3; uint64_t lr = 0, cookie = 0x12345678; };
#define PPC_FUNC(name) void name(PPCContext& ctx, uint8_t* base)
settings::Config config;
unsigned calls = 0, saves = 0;
namespace settings {
Config GetConfig() { return config; }
bool SaveBattleSpDouble(bool enabled) { ++saves; config.battleSpDouble = enabled; return true; }
}
extern "C" PPC_FUNC(__imp__sub_82AC1CE0) {
    ++calls;
    ctx.r3.u64 = base[0];
    if (base[0] == 255) ctx.r3.u64 = 0x7fffffff;
    ctx.lr = 0xfeed;
}
void Check(bool ok, const char* why) { if (!ok) { std::fprintf(stderr, "%s\n", why); std::exit(1); } }
'''

TESTS = r'''
void Run(uint8_t reward, uint32_t caller, uint64_t expected) {
    uint8_t guest[8] = {reward, 33, 44, 55};
    PPCContext ctx;
    ctx.lr = caller;
    auto before = calls;
    sub_82AC1CE0(ctx, guest);
    Check(calls == before + 1, "guest SP calculator must run once");
    Check(ctx.r3.u64 == expected, "SP reward at victory boundary");
    Check(ctx.lr == 0xfeed && ctx.cookie == 0x12345678 && guest[1] == 33,
        "unrelated register and memory state changed");
}
int main(int argc, char**) {
    config.battleSpDouble = argc > 1;
    Check(debug_menu::BattleSpDoubleEnabled() == config.battleSpDouble, "persisted startup value");
    if (argc > 1) { Run(5, 0x82AC6DEC, 10); return 0; }
    Run(5, 0x82AC6DEC, 5);
    debug_menu::SetBattleSpDoubleEnabled(true);
    Check(config.battleSpDouble && saves == 1, "SP preference persists");
    Run(0, 0x82AC6DEC, 0);
    Run(7, 0x82AC6DEC, 14);
    Run(7, 0x82AC6DE8, 7);
    Run(7, 0x12345678, 7);
    Run(255, 0x82AC6DEC, 0x7fffffff);
    debug_menu::SetBattleSpDoubleEnabled(false);
    Run(7, 0x82AC6DEC, 7);
    Check(!config.battleSpDouble && saves == 2, "SP preference disables independently");
    std::puts("battle SP hook: call boundary, default, persistence, zero and saturation passed");
}
'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = (args.root / "LostOdysseyRecomp/debug/battle_sp.cpp").read_text(encoding="utf-8")
    source = re.sub(r"^#include[^\n]*\n", "", source, flags=re.MULTILINE)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(PREAMBLE + source + TESTS, encoding="utf-8")

if __name__ == "__main__":
    main()
