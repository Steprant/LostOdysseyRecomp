#pragma once
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace gpu::draw_attachment {
// Keep a color-only draw (including a masked/discarding one) on its original
// raster grid. With depth bound and no writable color components, the depth
// attachment alone defines that grid; an unrelated color mapping must not
// constrain its framebuffer. The caller keeps the original pixel shader and
// depth/stencil state, and performs explicit EDRAM alias clears separately.
constexpr bool DepthOnly(uint32_t colorWriteMask, bool hasDepth) {
    return (colorWriteMask & 0xFu) == 0 && hasDepth;
}
// A depth-only draw whose color target has the depth target's size may keep
// that color attachment bound (zero write mask) instead of switching to a
// depth-only framebuffer, so the scene's alternating colour and depth-only
// draws stay in one render pass. Apple GPUs store and reload tiles at every
// encoder break, so this is ~15% faster on Metal. On the Xiaomi 14 with Turnip
// (2026-11-07) it cut ~104 to ~65-75 render passes per frame and the pass
// time by ~8%, so Android keeps colour bound too. SetKeepColorForDepthOnly
// decides elsewhere; LO_DEPTH_ONLY_KEEP_COLOR=0/1 overrides both.
inline std::atomic<bool> g_keepColorForDepthOnly{false};
inline void SetKeepColorForDepthOnly(bool keep) { g_keepColorForDepthOnly.store(keep, std::memory_order_relaxed); }
inline bool KeepColorForDepthOnly() {
    static const int forced = [] {
        const char* value = std::getenv("LO_DEPTH_ONLY_KEEP_COLOR");
        return value ? (std::strcmp(value, "0") != 0 ? 1 : 0) : -1;
    }();
    if (forced >= 0) return forced == 1;
#if defined(__APPLE__) || defined(__ANDROID__)
    return true;
#else
    return g_keepColorForDepthOnly.load(std::memory_order_relaxed);
#endif
}
}
