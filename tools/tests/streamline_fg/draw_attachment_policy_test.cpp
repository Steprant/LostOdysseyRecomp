#include <gpu/draw_attachment_policy.h>
#include <cstdio>

int main() {
    unsigned checks = 0;
    const auto check = [&](bool condition, const char* message) {
        ++checks;
        if (!condition) std::fprintf(stderr, "FAIL: %s\n", message);
        return condition;
    };
    // Mode 5 already produces mask 0 in renderer. Mode 4 may also mask every
    // component while retaining a pixel shader with depth exports/discard.
    if (!check(gpu::draw_attachment::DepthOnly(0, true), "depth-only PSO and framebuffer omit color")) return 1;
    for (uint32_t mask = 1; mask <= 15; ++mask)
        if (!check(!gpu::draw_attachment::DepthOnly(mask, true), "any written RGBA component keeps color")) return 1;
    for (uint32_t mask = 0; mask <= 15; ++mask)
        if (!check(!gpu::draw_attachment::DepthOnly(mask, false), "no depth preserves original color raster target")) return 1;
    if (!check(gpu::draw_attachment::DepthOnly(0xF0, true), "only RT0 RGBA bits determine writes")) return 1;
    // The live setting decides when no environment override is present.
    if (!check(!gpu::draw_attachment::KeepColorForDepthOnly(), "depth-only draws switch framebuffers by default")) return 1;
    gpu::draw_attachment::SetKeepColorForDepthOnly(true);
    if (!check(gpu::draw_attachment::KeepColorForDepthOnly(), "the setting keeps colour bound")) return 1;
    gpu::draw_attachment::SetKeepColorForDepthOnly(false);
    if (!check(!gpu::draw_attachment::KeepColorForDepthOnly(), "the setting applies live")) return 1;
    std::printf("PASS: %u draw attachment selection checks (CPU policy only)\n", checks);
}
