#ifndef GPU_IMAGE_HPP
#define GPU_IMAGE_HPP
#include <vk_mem_alloc.h>

#include <vulkan/vulkan_raii.hpp>

struct ImageDesc {
    vk::ImageCreateFlags flags{};
    vk::ImageType type = vk::ImageType::e2D;
    vk::Format format = vk::Format::eUndefined;
    vk::Extent3D extent{1, 1, 1};
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
    vk::ImageUsageFlags usage;
};
// Move-only image allocation; views/samplers are independent objects. Callers
// destroy all views and finish GPU usage before destroying/reassigning the image.
class GpuImage {
public:
    GpuImage(VmaAllocator allocator, const ImageDesc& desc);
    ~GpuImage();
    GpuImage(const GpuImage&) = delete;
    GpuImage& operator=(const GpuImage&) = delete;
    GpuImage(GpuImage&& other) noexcept;
    GpuImage& operator=(GpuImage&& other) noexcept;
    vk::Image handle() const noexcept {
        return image_;
    }
    const ImageDesc& desc() const noexcept {
        return desc_;
    }

private:
    void release() noexcept;
    VmaAllocator allocator_;
    vk::Image image_;
    VmaAllocation allocation_ = nullptr;
    ImageDesc desc_;
};
vk::raii::ImageView createImageView(const vk::raii::Device& device, const GpuImage& image, vk::ImageViewCreateInfo info);
// Samplers use vk::raii::Sampler directly and may be shared across images.
#endif
