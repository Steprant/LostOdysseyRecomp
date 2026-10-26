"""Compile the production draw-mode/side-effect gate with the real query tracker.

No game execution or hardware timing is claimed. --renderer accepts upstream's
renderer.cpp to demonstrate the failing-before no-op draw regression.
"""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PRELUDE = r'''
#include "gpu/occlusion_queries.h"
#include <cstdio>
#include <stdexcept>
using namespace gpu;
constexpr uint32_t REG_RB_DEPTHCONTROL = 0x2200;
static unsigned checks=0;
static void Check(bool ok, const char* why) {
    ++checks;
    if(!ok) throw std::runtime_error(why);
}
struct Fixture {
    occlusion::Tracker occlusion{occlusion::Mode::Strict};
    struct { uint32_t mode=0, modeMask=0; } drops;
    uint32_t modeControl=5, depthControl=0, recorded=0, readOnlyDepthDrawsSkipped=0;
    bool skipReadOnlyDepthDraws=true;
    uint32_t Reg(uint32_t reg) {
        Check(reg==REG_RB_DEPTHCONTROL,"only depth/stencil state is read by this gate");
        return depthControl;
    }
    void Draw() {
        if(modeControl==4 || modeControl==5) occlusion.DrawSeen();
        /* PRODUCTION_GATE */
        ++recorded;
        if(occlusion.Active()) occlusion.DrawMeasured({1,0,1.0});
    }
};
'''
TESTS = r'''
int main() {
    Fixture first;
    first.Draw();
    Check(first.recorded==0 && first.readOnlyDepthDrawsSkipped==1,"depth-only draw with no writes must be omitted");
    first.depthControl=2; // Depth testing alone reads the attachment.
    first.Draw();
    Check(first.recorded==0 && first.readOnlyDepthDrawsSkipped==2,"read-only depth test is not an attachment write");
    for(bool active : {false,true}) {
        // Do not accumulate measured parts during the exhaustive state sweep.
        for(uint32_t mode=0;mode<8;++mode) {
            Fixture f;
            f.modeControl=mode;
            for(uint32_t depth=0;depth<65536;++depth) {
                if(active) f.occlusion.Begin(0x1020);
                f.depthControl=depth;
                const uint32_t before=f.recorded;
                f.Draw();
                const bool valid=mode==4 || mode==5;
                const bool effect=mode==4 || (depth&1) || (depth&4) || active;
                Check(f.recorded-before==unsigned(valid&&effect),"all color/depth/stencil/query effects must be retained");
            }
        }
    }
    for(const auto mode : {occlusion::Mode::Fast,occlusion::Mode::Strict,occlusion::Mode::History}) {
        Fixture f;
        f.occlusion.SetMode(mode);
        const auto begin=f.occlusion.Begin(0x1020,occlusion::HistoryOwner(0x823cd744,1));
        f.Draw();
        Check(f.recorded==1 && f.readOnlyDepthDrawsSkipped==0,"read-only query geometry is recorded in every policy");
        const auto early=f.occlusion.End(0x1000);
        const std::vector<uint64_t> samples{7};
        const auto completed=f.occlusion.Complete(1,samples);
        if(mode!=occlusion::Mode::Fast) {
            Check(!early && completed.size()==1,"strict/history query waits for the measured draw");
            const auto count=completed[0].record.zpassA+completed[0].record.zpassB-begin.record.zpassA-begin.record.zpassB;
            Check(count==7,"query sample count is unchanged");
        }
    }
    Fixture disabled;
    disabled.skipReadOnlyDepthDraws=false;
    disabled.Draw();
    Check(disabled.recorded==1 && disabled.readOnlyDepthDrawsSkipped==0,"opt-out retains the original draw");
    Fixture color;
    color.modeControl=4;
    color.Draw();
    Check(color.recorded==1,"color draws are never inferred to be invisible");
    Fixture stencil;
    stencil.depthControl=1; // Even all KEEP stencil state takes the conservative path.
    stencil.Draw();
    Check(stencil.recorded==1,"all stencil-enabled draws remain untouched");
    std::printf("PASS: %u checks; exhaustive modes/depth state, real occlusion policies, opt-out\n",checks);
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--cxx", default="g++")
    p.add_argument("--sanitize", action="store_true")
    p.add_argument("--compile-only", action="store_true", help="Link a cross-compiled fixture without executing it")
    p.add_argument("--renderer", type=Path, default=ROOT / "LostOdysseyRecomp/gpu/renderer.cpp")
    a = p.parse_args()
    source = a.renderer.read_text(encoding="utf-8")
    begin = "                if (modeControl != 4 && modeControl != 5)"
    end = "                RefreshAnisotropicFiltering();"
    if source.count(begin) != 1 or source.count(end) != 1:
        raise RuntimeError("DrawImpl extraction boundaries changed")
    gate = source[source.index(begin):source.index(end)]
    a.out.mkdir(parents=True, exist_ok=True)
    cpp = a.out / "read_only_depth_draw.cpp"
    exe = a.out / "read_only_depth_draw"
    cpp.write_text(PRELUDE.replace("/* PRODUCTION_GATE */", gate) + TESTS, encoding="utf-8")
    cmd = [a.cxx, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror", "-Wno-missing-field-initializers", "-I", str(ROOT / "LostOdysseyRecomp"), str(cpp), "-o", str(exe)]
    if a.sanitize:
        cmd += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    subprocess.run(cmd, check=True)
    if not a.compile_only:
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
