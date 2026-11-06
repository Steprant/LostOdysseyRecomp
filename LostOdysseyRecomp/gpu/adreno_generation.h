#pragma once
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

// The Adreno generation (6 for Adreno 6xx, 7 for 7xx, 8 for 8xx) before a
// Vulkan driver is loaded, or 0 when unknown. Used only to decide how Turnip
// may be configured at load time: on 7xx and later it has eight hardware
// descriptor sets, so reserving one for dynamic buffer offsets still leaves
// the renderer's six (gpu::video LoadCustomVulkanDriver).
namespace gpu::adreno
{
// "Adreno750v2", "Adreno (TM) 750", "Adreno740": the first digit of the
// three-digit model number is the generation.
inline uint32_t GenerationFromModel(std::string_view model)
{
    const auto at = model.find("Adreno");
    if (at == std::string_view::npos) return 0;
    for (size_t i = at + 6; i + 2 < model.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(model[i]))) continue;
        if (std::isdigit(static_cast<unsigned char>(model[i + 1])) && std::isdigit(static_cast<unsigned char>(model[i + 2])))
            return uint32_t(model[i] - '0');
        return 0;
    }
    return 0;
}

// Snapdragon parts with an Adreno 7xx or 8xx GPU, by ro.soc.model.
inline uint32_t GenerationFromSoc(std::string_view soc)
{
    struct Part { std::string_view soc; uint32_t generation; };
    static constexpr Part parts[] = {
        {"SM8550", 7}, {"SM8650", 7}, {"SM8635", 7}, {"SM7675", 7}, {"SM7635", 7}, {"SM8475", 7}, {"SM8450", 7},
        {"SM7550", 7}, {"SM8750", 8}, {"SM8735", 8}, {"SM8850", 8},
    };
    for (const auto& part : parts)
        if (soc.starts_with(part.soc)) return part.generation;
    return 0;
}

inline uint32_t Generation()
{
#if defined(__ANDROID__)
    for (const char* path : {"/sys/class/kgsl/kgsl-3d0/gpu_model", "/sys/kernel/gpu/gpu_model"}) {
        std::ifstream file(path);
        std::string model;
        if (file && std::getline(file, model))
            if (const uint32_t generation = GenerationFromModel(model)) return generation;
    }
    char soc[PROP_VALUE_MAX]{};
    if (__system_property_get("ro.soc.model", soc) > 0)
        return GenerationFromSoc(soc);
#endif
    return 0;
}
}
