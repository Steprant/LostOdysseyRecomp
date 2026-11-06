#include <gpu/adreno_generation.h>
#include <gpu/shader/cache.h>
#include <gpu/shader/portable_shader_pack_location.h>
#include <cstdio>
#include <cstdlib>

static int failures = 0;
static void Check(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}

int main()
{
    using namespace gpu::adreno;
    Check(GenerationFromModel("Adreno750v2") == 7, "kgsl model Adreno750v2");
    Check(GenerationFromModel("Adreno (TM) 750") == 7, "Vulkan device name");
    Check(GenerationFromModel("Adreno650v2") == 6, "Adreno 650");
    Check(GenerationFromModel("Adreno830") == 8, "Adreno 830");
    Check(GenerationFromModel("Mali-G715") == 0 && GenerationFromModel("Adreno") == 0 && GenerationFromModel("Adreno 7") == 0,
          "no model number");
    Check(GenerationFromSoc("SM8650") == 7 && GenerationFromSoc("SM8750") == 8 && GenerationFromSoc("SM8250") == 0 &&
          GenerationFromSoc("") == 0, "Snapdragon parts");

    // A runtime choice of uniform-buffer constants changes the renderer's
    // identity but never the distribution pack it looks for.
    namespace cache = xenos::cache;
    namespace pack = xenos::portable_pack;
    const auto pointer = cache::MakeIdentity(cache::Backend::Vulkan, "dxc");
    const auto published = pack::IdentityOf(pack::Flavor::Vulkan);
    Check(!cache::UsesUboConstants(pointer) && !cache::UsesUboConstants(published), "pointer form by default");
    cache::SetRuntimeUboConstants(true);
    Check(cache::UboConstants(), "the runtime choice selects uniform buffers");
    const auto ubo = cache::MakeIdentity(cache::Backend::Vulkan, "dxc");
    Check(cache::UsesUboConstants(ubo) && cache::IdentityKey(ubo) != cache::IdentityKey(pointer), "a separate identity");
    Check(cache::IdentityKey(pack::IdentityOf(pack::Flavor::Vulkan)) == cache::IdentityKey(published),
          "the distribution pack identity is unchanged");
    cache::SetRuntimeUboConstants(false);
    Check(!cache::UboConstants(), "the choice can be withdrawn before the renderer starts");
    if (failures) return 1;
    std::printf("PASS adreno generation and runtime shader-constant form\n");
}
