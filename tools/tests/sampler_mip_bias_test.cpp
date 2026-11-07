// The temporal-SR mip bias reaches only eligible scene-texture samplers and
// survives palette reconfiguration (anisotropy changes keep it, and back).
#include <gpu/sampler_palette.h>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace {
int checks = 0;
void Require(bool value, const char* what) { ++checks; if (!value) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); } }
struct Sampler { uint64_t key; };
struct Set {};
}

int main()
{
    using namespace gpu::sampling;
    constexpr uint64_t trilinear = 0x1 | (0x1 << 2) | (0x1 << 4);
    const uint64_t scene = Recipe(trilinear, true), ui = Recipe(trilinear, false);
    Require(EffectiveKey(scene, 0) == trilinear, "no bias, no anisotropy: guest key");
    Require(MipBiasForKey(EffectiveKey(scene, 0, 2)) == -0.5f, "1.5x code 2 is -0.5 LOD");
    Require(MipBiasForKey(EffectiveKey(scene, 8, 4)) == -1.0f, "bias combines with anisotropy");
    Require(((EffectiveKey(scene, 8, 4) >> 16) & 7) == 3, "anisotropy code unchanged by the bias");
    Require(EffectiveKey(ui, 8, 4) == trilinear, "ineligible (UI, render targets) never biased");
    Require(MipBiasForKey(EffectiveKey(scene, 0, 99)) == -1.75f, "bias code clamps at -1.75");

    Palette<Sampler, Set> palette;
    const auto make = [](uint64_t key) { return std::make_shared<Sampler>(Sampler{key}); };
    const auto set = [](const auto&) { return std::make_shared<Set>(); };
    Require(palette.Initialize(make, set), "initialize");
    const auto index = palette.Select(scene, make, set);
    Require(index.has_value(), "select scene recipe");
    Require(palette.Reconfigure(0, 3, make, set), "apply bias");
    Require(palette.MipBias() == 3 && MipBiasForKey(palette.Current()->samplers[*index]->key) == -0.75f,
        "existing slot rebuilt with the bias");
    Require(palette.Reconfigure(16, make, set), "anisotropy change");
    Require(palette.MipBias() == 3 && MipBiasForKey(palette.Current()->samplers[*index]->key) == -0.75f,
        "anisotropy change keeps the bias");
    const auto later = palette.Select(Recipe(trilinear | (1 << 6), true), make, set);
    Require(later && MipBiasForKey(palette.Current()->samplers[*later]->key) == -0.75f, "new slots take the bias");
    Require(palette.Reconfigure(16, 0, make, set) &&
        MipBiasForKey(palette.Current()->samplers[*index]->key) == 0.0f, "bias removed when SR stops");
    std::printf("sampler mip bias: %d checks passed\n", checks);
    return 0;
}
