"""Inject image/image-view allocation failures into Plume's texture factories.

Compiles the production constructor, view creation, destructor and factories
with the pinned Plume/Vulkan/VMA types and fake native allocation calls. No GPU,
Vulkan loader or game data is used.
"""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PLUME = ROOT / 'thirdparty/plume'

PRELUDE = r'''
#include <plume_render_interface_types.h>
#include <plume_log.h>
#include <vk_mem_alloc.h>
#include <cstdio>
#include <stdexcept>
#include <type_traits>

static void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class T> T Handle(uintptr_t value) {
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}
enum class Failure { None, Image, View };
static Failure failure = Failure::None;
static unsigned images = 0, views = 0, freedImages = 0, freedViews = 0;
static VmaPool expectedPool = VK_NULL_HANDLE;

VkResult vmaCreateImage(VmaAllocator, const VkImageCreateInfo* image,
    const VmaAllocationCreateInfo* allocation, VkImage* result,
    VmaAllocation* memory, VmaAllocationInfo*) {
    ++images;
    Check(allocation->pool == expectedPool, "wrong allocation pool");
    Check(image->extent.width == 64 && image->extent.height == 64, "wrong image extent");
    if (failure == Failure::Image) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    *result = Handle<VkImage>(2);
    *memory = Handle<VmaAllocation>(4);
    return VK_SUCCESS;
}
void vmaDestroyImage(VmaAllocator, VkImage image, VmaAllocation memory) {
    Check(image == Handle<VkImage>(2) && memory == Handle<VmaAllocation>(4), "wrong image cleanup");
    Check(++freedImages == 1, "image freed twice");
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(VkDevice,
    const VkImageViewCreateInfo* info, const VkAllocationCallbacks*, VkImageView* result) {
    ++views;
    Check(info->image == Handle<VkImage>(2), "wrong image for view");
    Check(info->subresourceRange.levelCount == 1 && info->subresourceRange.layerCount == 1,
        "wrong default view range");
    if (failure == Failure::View) return VK_ERROR_OUT_OF_HOST_MEMORY;
    *result = Handle<VkImageView>(3);
    return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(VkDevice, VkImageView view, const VkAllocationCallbacks*) {
    Check(view == Handle<VkImageView>(3), "wrong view cleanup");
    Check(++freedViews == 1, "view freed twice");
}

namespace plume {
// Host object shells: native creation, ownership and factory bodies below are
// production code. Descriptors, enums and allocation structures are real types.
struct RenderTexture { virtual ~RenderTexture() = default; };
struct VulkanPool;
struct VulkanDevice {
    VkDevice vk = Handle<VkDevice>(1);
    VmaAllocator allocator = Handle<VmaAllocator>(5);
    std::unique_ptr<RenderTexture> createTexture(const RenderTextureDesc&);
};
struct VulkanPool {
    VulkanDevice* device;
    VmaPool vk = Handle<VmaPool>(6);
    std::unique_ptr<RenderTexture> createTexture(const RenderTextureDesc&);
};
struct VulkanTexture : RenderTexture {
    VkImage vk = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkFormat imageFormat = VK_FORMAT_UNDEFINED;
    VkImageSubresourceRange imageSubresourceRange{};
    VulkanDevice* device = nullptr;
    VulkanPool* pool = nullptr;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo{};
    bool ownership = false;
    RenderTextureDesc desc;
    struct { std::unique_ptr<int> view; } redBlueSwapView;
    VulkanTexture(VulkanDevice*, VulkanPool*, const RenderTextureDesc&);
    ~VulkanTexture() override;
    void createImageView(VkFormat);
    void fillSubresourceRange();
};
'''

TAIL = r'''
}
int main() try {
    using namespace plume;
    VulkanDevice device;
    VulkanPool pool{&device};
    const auto desc = RenderTextureDesc::Texture2D(64, 64, 1, RenderFormat::R8G8B8A8_UNORM);
    unsigned cases = 0;
    for (const bool pooled : {false, true}) {
        for (const auto fail : {Failure::None, Failure::Image, Failure::View}) {
            ++cases;
            failure = fail;
            images = views = freedImages = freedViews = 0;
            expectedPool = pooled ? pool.vk : VK_NULL_HANDLE;
            auto texture = pooled ? pool.createTexture(desc) : device.createTexture(desc);
            Check(bool(texture) == (fail == Failure::None),
                pooled ? "pool returned an unusable texture" : "device returned an unusable texture");
            Check(images == 1, "image creation not attempted once");
            if (fail == Failure::Image) {
                Check(views == 0 && freedImages == 0 && freedViews == 0,
                    "image allocation failure attempted view creation or cleanup of unowned handles");
            } else if (fail == Failure::View) {
                Check(views == 1 && freedImages == 1 && freedViews == 0,
                    "default view failure did not reclaim the allocated image exactly once");
            } else {
                auto* native = static_cast<VulkanTexture*>(texture.get());
                Check(native->vk != VK_NULL_HANDLE && native->imageView != VK_NULL_HANDLE,
                    "successful texture lost native handles");
                Check(views == 1 && freedImages == 0 && freedViews == 0, "live texture freed early");
                texture.reset();
                Check(freedImages == 1 && freedViews == 1, "successful texture cleanup changed");
            }
        }
    }
    std::printf("PASS: %u production Vulkan texture allocation/view failure and ownership cases (device and pool)\n", cases);
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    return 1;
}
'''


def between(source, begin, end):
    if source.count(begin) != 1 or source.count(end) != 1:
        raise RuntimeError('Production extraction boundary changed')
    return source[source.index(begin):source.index(end)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--source', type=Path, default=PLUME / 'plume_vulkan.cpp')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = args.source.read_text()
    bodies = [
        between(source, '    VkFormat toVk(RenderFormat format)', '    static VkVertexInputRate toVk('),
        between(source, '    static VkImageAspectFlags toViewAspectFlags(', '    static VkImageAspectFlags toAspectFlags('),
        between(source, '    VulkanTexture::VulkanTexture(VulkanDevice *device, VulkanPool *pool,',
                '    VulkanTexture::VulkanTexture(VulkanDevice *device, VkImage image)'),
        between(source, '    VulkanTexture::~VulkanTexture()', '    std::unique_ptr<RenderTextureView> VulkanTexture::createTextureView('),
        between(source, '    void VulkanTexture::fillSubresourceRange()', '    // VulkanTextureView\n'),
        between(source, '    std::unique_ptr<RenderTexture> VulkanPool::createTexture(', '    // VulkanQueueFamily\n'),
        between(source, '    std::unique_ptr<RenderTexture> VulkanDevice::createTexture(',
                '    std::unique_ptr<RenderAccelerationStructure> VulkanDevice::createAccelerationStructure('),
    ]
    cpp = out / 'vulkan_texture_creation.cpp'
    cpp.write_text(PRELUDE + ''.join(bodies) + TAIL)
    flags = ['-std=c++20', '-DPLUME_SDL_VULKAN_ENABLED', '-I' + str(PLUME),
             '-I' + str(PLUME / 'contrib/Vulkan-Headers/include'),
             '-I' + str(PLUME / 'contrib/VulkanMemoryAllocator/include'),
             '-I' + str(ROOT / 'thirdparty/SDL/include')]
    if args.sanitize:
        flags += ['-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined']
    else:
        flags += ['-O2']
    binary = out / 'vulkan_texture_creation'
    build = subprocess.run([args.cxx, *flags, str(cpp), '-o', str(binary)],
                           capture_output=True, text=True, timeout=120)
    (out / 'build.log').write_text(build.stdout + build.stderr)
    if build.returncode:
        print(build.stdout + build.stderr, end='')
        return build.returncode
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
    (out / 'run.log').write_text(run.stdout + run.stderr)
    print(run.stdout + run.stderr, end='')
    return run.returncode


if __name__ == '__main__':
    raise SystemExit(main())
