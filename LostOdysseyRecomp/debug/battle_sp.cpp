#include <stdafx.h>
#include <os/logger.h>
#include <settings/config.h>
#include "battle_sp.h"
#include <atomic>
#include <cstdint>

extern "C" PPC_FUNC(__imp__sub_82AC1CE0);

namespace
{
    std::atomic<bool>& DoubleSp()
    {
        static std::atomic<bool> enabled{settings::GetConfig().battleSpDouble};
        return enabled;
    }
}

bool debug_menu::BattleSpDoubleEnabled()
{
    return DoubleSp().load(std::memory_order_relaxed);
}

void debug_menu::SetBattleSpDoubleEnabled(bool enabled)
{
    DoubleSp().store(enabled, std::memory_order_relaxed);
    if (!settings::SaveBattleSpDouble(enabled))
        LOG_WARNING("debug menu: failed to persist battle SP x2 setting");
    LOG_INFO("debug menu: battle SP x2 {}", enabled);
}

// Victory initializer sub_82AC6D88 invokes the battle SP calculator once at
// 82AC6DE8, then stores its return in result+0x5c. The later skill-learning
// routine sub_82AC6C60 reads that award for each eligible character. Change
// only this return value, before any character progress or save data is touched.
// SP is converted as signed by that routine, so saturate at INT32_MAX.
PPC_FUNC(sub_82AC1CE0)
{
    const uint32_t caller = static_cast<uint32_t>(ctx.lr);
    __imp__sub_82AC1CE0(ctx, base);
    if (caller != 0x82AC6DEC || !debug_menu::BattleSpDoubleEnabled())
        return;
    const uint32_t award = ctx.r3.u32;
    constexpr uint32_t maxAward = 0x7FFFFFFFu;
    if (award > 0 && award <= maxAward)
        ctx.r3.u64 = award > maxAward / 2 ? maxAward : award * 2u;
}
