#pragma once
#include <cstdint>

namespace gpu::frame_plan {

// SGSR2 upscales only a 3D scene. Menus, pause screens and transitions have
// none (NoEligibleScene): they render at SGSR2's reduced input size and
// SGSR1 or bilinear enlarges them, so their text is soft. After a run of
// such frames the CPU plan asks for SGSR2's native-AA size (the output size)
// instead; the first frame SGSR2 submits again (a 3D scene is back) returns
// to the chosen quality. A short cooldown after returning keeps a scene that
// flickers between eligible and not from reallocating every few frames.
struct Sgsr2IdlePolicy {
    static constexpr uint32_t kEnterFrames = 20;     // ~0.7 s at 30 FPS
    static constexpr uint64_t kCooldownFrames = 90;  // ~3 s at 30 FPS

    bool native = false;
    uint32_t idleFrames = 0;
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
            native = false;
            returnedFrame = renderFrame;
            returnedOnce = true;
            return Change::ReturnToQuality;
        }
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
