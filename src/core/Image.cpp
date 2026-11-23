#include "Image.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

GpuImage::GpuImage(VmaAllocator allocator, const ImageDesc& desc) : allocator_(allocator), desc_(desc) {
    if (!allocator || !desc.extent.width || !desc.extent.height || !desc.extent.depth ||
        !desc.mipLevels || !desc.arrayLayers || !desc.usage || desc.format == vk::Format::eUndefined) {
        throw std::invalid_argument("Image requires nonzero dimensions, levels, layers, usage and format");
    }
    if (desc.mipLevels > std::bit_width(std::max({desc.extent.width, desc.extent.height, desc.extent.depth}))) {
        throw std::invalid_argument("Image has too many mip levels");
    }
    if ((desc.type == vk::ImageType::e1D && (desc.extent.height != 1 || desc.extent.depth != 1)) ||
        (desc.type == vk::ImageType::e2D && desc.extent.depth != 1) ||
        (desc.type == vk::ImageType::e3D && desc.arrayLayers != 1)) {
        throw std::invalid_argument("Image dimensions do not match image type");
    }
    vk::ImageCreateInfo info{
        .flags = desc.flags, .imageType = desc.type, .format = desc.format, .extent = desc.extent, .mipLevels = desc.mipLevels, .arrayLayers = desc.arrayLayers, .samples = desc.samples, .tiling = desc.tiling, .usage = desc.usage, .sharingMode = vk::SharingMode::eExclusive, .initialLayout = vk::ImageLayout::eUndefined
    };
    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    VkImage image;
    if (vmaCreateImage(allocator_, &*info, &allocationInfo, &image, &allocation_, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image");
    }
    image_ = image;
}
GpuImage::~GpuImage() {
    release();
}
GpuImage::GpuImage(GpuImage&& other) noexcept : allocator_(other.allocator_),
                                                image_(std::exchange(other.image_, {})),
                                                allocation_(std::exchange(other.allocation_, nullptr)),
                                                desc_(other.desc_) {}
GpuImage& GpuImage::operator=(GpuImage&& other) noexcept {
    if (this != &other) {
        release();
        allocator_ = other.allocator_;
        desc_ = other.desc_;
        image_ = std::exchange(other.image_, {});
        allocation_ = std::exchange(other.allocation_, nullptr);
    }
    return *this;
}
void GpuImage::release() noexcept {
    if (image_) {
        vmaDestroyImage(allocator_, static_cast<VkImage>(image_), allocation_);
    }
    image_ = nullptr;
    allocation_ = nullptr;
}
vk::raii::ImageView createImageView(const vk::raii::Device& device, const GpuImage& image, vk::ImageViewCreateInfo info) {
    if (!image.handle()) {
        throw std::invalid_argument("Cannot view a moved-from image");
    }
    const auto& range = info.subresourceRange;
    const auto& desc = image.desc();
    if (range.baseMipLevel >= desc.mipLevels || range.baseArrayLayer >= desc.arrayLayers ||
        !range.levelCount || !range.layerCount || !range.aspectMask ||
        (range.levelCount != VK_REMAINING_MIP_LEVELS && range.levelCount > desc.mipLevels - range.baseMipLevel) ||
        (range.layerCount != VK_REMAINING_ARRAY_LAYERS && range.layerCount > desc.arrayLayers - range.baseArrayLayer)) {
        throw std::out_of_range("Image view subresource range is invalid");
    }
    info.image = image.handle();
    if (info.format == vk::Format::eUndefined) {
        info.format = desc.format;
    }
    return device.createImageView(info);
}
