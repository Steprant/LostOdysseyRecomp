#pragma once
#include <cstdint>

namespace gpu::frame_plan {

// SGSR2 upscales only a 3D scene. Menus, pause screens and transitions have
// none (NoEligibleScene): they render at SGSR2's reduced input size and
// SGSR1 or bilinear enlarges them, so their text is soft. After a run of
// such frames the CPU plan asks for SGSR2's native-AA size (the output size)
// instead. It returns to the chosen quality once a 3D scene has been
// upscaled for kReturnFrames frames in a row: a fade-in after a menu or a
// loading screen finishes at one resolution, without a resize in the middle
// (the first device session saw ghosting in transitions when it returned on
// the first 3D frame). A cooldown after returning keeps a scene that flickers
// between eligible and not from reallocating every few frames.
struct Sgsr2IdlePolicy {
    // 8 frames: a menu is sharp after ~0.27 s at 30 FPS (30 frames took a
    // full second, the soft text the player saw); single black frames of a
    // cut stay below it. The renderer keeps the other size's render targets
    // and SGSR2's pipelines, so the switch no longer rebuilds them.
    static constexpr uint32_t kEnterFrames = 8;
    static constexpr uint32_t kReturnFrames = 12;    // ~0.4 s at 30 FPS, rendered at the output size
    static constexpr uint64_t kCooldownFrames = 90;  // ~3 s at 30 FPS

    bool native = false;
    uint32_t idleFrames = 0, sceneFrames = 0;
    uint64_t returnedFrame = 0;
    bool returnedOnce = false;

    enum class Change : uint8_t { None, EnterNative, ReturnToQuality };

    // One observation per rendered frame: submitted, or fallen back with or
    // without an eligible scene.
    Change Observe(bool submitted, bool noScene, uint64_t renderFrame)
    {
        if (submitted) {
            idleFrames = 0;
            if (!native) return Change::None;
            if (++sceneFrames < kReturnFrames) return Change::None;
            sceneFrames = 0;
            native = false;
            returnedFrame = renderFrame;
            returnedOnce = true;
            return Change::ReturnToQuality;
        }
        sceneFrames = 0;
        if (!noScene) return Change::None; // a scene SGSR2 skipped for another reason
        if (idleFrames < kEnterFrames) ++idleFrames;
        if (native || idleFrames < kEnterFrames) return Change::None;
        if (returnedOnce && renderFrame < returnedFrame + kCooldownFrames) return Change::None;
        native = true;
        return Change::EnterNative;
    }
    void Reset() { *this = {}; }
};

} // namespace gpu::frame_plan
