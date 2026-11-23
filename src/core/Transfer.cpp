#include "Transfer.hpp"

#include <algorithm>
#include <stdexcept>

#include "VulkanContext.hpp"

namespace {
void checkTransfer(
    const Buffer& buffer,
    vk::BufferUsageFlagBits usage,
    vk::DeviceSize offset,
    size_t count
) {
    if (!(buffer.usage() & usage)) {
        throw std::invalid_argument("Missing buffer transfer usage");
    }
    if (!buffer.handle() || offset > buffer.size() ||
        count > buffer.size() - offset) {
        throw std::out_of_range("Transfer exceeds buffer size");
    }
}

void barrier(
    vk::raii::CommandBuffer& cmd,
    vk::PipelineStageFlags2 srcStage,
    vk::AccessFlags2 srcAccess,
    vk::PipelineStageFlags2 dstStage,
    vk::AccessFlags2 dstAccess
) {
    vk::MemoryBarrier2 memory{
        .srcStageMask = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{}.setMemoryBarriers(memory));
}

void beforeCopy(vk::raii::CommandBuffer& cmd) {
    barrier(
        cmd,
        vk::PipelineStageFlagBits2::eAllCommands,
        vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite | vk::AccessFlagBits2::eTransferRead
    );
}
}  // namespace

void UploadBatch::upload(
    vk::raii::CommandBuffer& cmd,
    const Buffer& dst,
    std::span<const uint8_t> data,
    vk::DeviceSize offset
) {
    checkTransfer(
        dst, vk::BufferUsageFlagBits::eTransferDst, offset, data.size_bytes()
    );
    if (data.empty()) {
        return;
    }
    staging_.emplace_back(
        allocator_,
        BufferDesc{
            data.size_bytes(),
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryAccess::Upload
        }
    );
    staging_.back().write(data);
    cmd.copyBuffer(
        staging_.back().handle(),
        dst.handle(),
        vk::BufferCopy{0, offset, data.size_bytes()}
    );
}

void uploadBufferSync(
    const VulkanContext& ctx,
    const Buffer& dst,
    std::span<const uint8_t> data,
    vk::DeviceSize offset,
    size_t chunkSize
) {
    checkTransfer(
        dst, vk::BufferUsageFlagBits::eTransferDst, offset, data.size_bytes()
    );
    if (!chunkSize) {
        throw std::invalid_argument("Transfer chunk size must be nonzero");
    }
    for (size_t done = 0; done < data.size_bytes();) {
        auto count = std::min(chunkSize, data.size_bytes() - done);
        UploadBatch batch(ctx.allocator());
        ctx.submitAndWait([&](vk::raii::CommandBuffer& cmd) {
            beforeCopy(cmd);
            batch.upload(cmd, dst, data.subspan(done, count), offset + done);
            barrier(
                cmd,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::AccessFlagBits2::eMemoryRead |
                    vk::AccessFlagBits2::eMemoryWrite
            );
        });
        done += count;
    }
}

void readBufferSync(
    const VulkanContext& ctx,
    const Buffer& src,
    std::span<uint8_t> data,
    vk::DeviceSize offset,
    size_t chunkSize
) {
    checkTransfer(
        src, vk::BufferUsageFlagBits::eTransferSrc, offset, data.size_bytes()
    );
    if (!chunkSize) {
        throw std::invalid_argument("Transfer chunk size must be nonzero");
    }
    if (data.empty()) {
        return;
    }
    Buffer staging(
        ctx.allocator(),
        {std::min(chunkSize, data.size_bytes()),
         vk::BufferUsageFlagBits::eTransferDst,
         MemoryAccess::Readback}
    );
    for (size_t done = 0; done < data.size_bytes();) {
        auto count = std::min(chunkSize, data.size_bytes() - done);
        ctx.submitAndWait([&](vk::raii::CommandBuffer& cmd) {
            beforeCopy(cmd);
            cmd.copyBuffer(
                src.handle(),
                staging.handle(),
                vk::BufferCopy{offset + done, 0, count}
            );
            barrier(
                cmd,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eHost,
                vk::AccessFlagBits2::eHostRead
            );
        });
        staging.read(data.subspan(done, count));
        done += count;
    }
}

namespace {
uint32_t pixelBytes(vk::Format format) {
    switch (format) {
        case vk::Format::eR8Unorm:
        case vk::Format::eR8Uint:
        case vk::Format::eR8Sint:
            return 1;
        case vk::Format::eR8G8Unorm:
        case vk::Format::eR16Sfloat:
            return 2;
        case vk::Format::eR8G8B8A8Unorm:
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eB8G8R8A8Unorm:
        case vk::Format::eB8G8R8A8Srgb:
        case vk::Format::eR32Uint:
        case vk::Format::eR32Sint:
        case vk::Format::eR32Sfloat:
        case vk::Format::eR16G16Sfloat:
            return 4;
        case vk::Format::eR16G16B16A16Sfloat:
        case vk::Format::eR32G32Sfloat:
            return 8;
        case vk::Format::eR32G32B32A32Sfloat:
            return 16;
        default:
            throw std::invalid_argument(
                "Upload format is not a supported uncompressed color format"
            );
    }
}
size_t checkedMultiply(size_t a, size_t b) {
    if (b && a > SIZE_MAX / b) {
        throw std::out_of_range("Image upload size overflow");
    }
    return a * b;
}
size_t checkedAdd(size_t a, size_t b) {
    if (a > SIZE_MAX - b) {
        throw std::out_of_range("Image upload size overflow");
    }
    return a + b;
}
void validateImageUpload(
    const GpuImage& image, size_t dataSize, const ImageUploadDesc& upload
) {
    const auto& desc = image.desc();
    const auto& r = upload.region;
    const auto& sub = r.imageSubresource;
    if (!image.handle() ||
        !(desc.usage & vk::ImageUsageFlagBits::eTransferDst) ||
        desc.samples != vk::SampleCountFlagBits::e1) {
        throw std::invalid_argument(
            "Image upload needs a single-sample transfer destination"
        );
    }
    if (sub.aspectMask != vk::ImageAspectFlagBits::eColor) {
        throw std::invalid_argument("Image upload supports color aspect only");
    }
    if (sub.mipLevel >= desc.mipLevels ||
        sub.baseArrayLayer >= desc.arrayLayers || !sub.layerCount ||
        sub.layerCount > desc.arrayLayers - sub.baseArrayLayer) {
        throw std::out_of_range("Image upload subresource is invalid");
    }
    auto fits = [](int32_t offset, uint32_t count, uint32_t limit) {
        return offset >= 0 && count && static_cast<uint32_t>(offset) < limit &&
               count <= limit - offset;
    };
    if (!fits(
            r.imageOffset.x,
            r.imageExtent.width,
            std::max(1u, desc.extent.width >> sub.mipLevel)
        ) ||
        !fits(
            r.imageOffset.y,
            r.imageExtent.height,
            std::max(1u, desc.extent.height >> sub.mipLevel)
        ) ||
        !fits(
            r.imageOffset.z,
            r.imageExtent.depth,
            std::max(1u, desc.extent.depth >> sub.mipLevel)
        )) {
        throw std::out_of_range("Image upload extent exceeds mip bounds");
    }
    const auto bytes = pixelBytes(desc.format);
    if (r.bufferOffset % 4 || r.bufferOffset % bytes) {
        throw std::invalid_argument("Image upload buffer offset is misaligned");
    }
    const size_t row =
        r.bufferRowLength ? r.bufferRowLength : r.imageExtent.width;
    const size_t height =
        r.bufferImageHeight ? r.bufferImageHeight : r.imageExtent.height;
    if (row < r.imageExtent.width || height < r.imageExtent.height) {
        throw std::invalid_argument(
            "Image upload row/image pitch is too small"
        );
    }
    const size_t slices = checkedMultiply(sub.layerCount, r.imageExtent.depth);
    auto pixels = checkedAdd(
        checkedMultiply(slices - 1, height), r.imageExtent.height - 1
    );
    pixels = checkedAdd(checkedMultiply(pixels, row), r.imageExtent.width);
    const auto needed =
        checkedAdd(r.bufferOffset, checkedMultiply(pixels, bytes));
    if (dataSize < needed) {
        throw std::out_of_range(
            "Image upload data is too short for its region"
        );
    }
    if (upload.after.layout == vk::ImageLayout::eUndefined ||
        upload.after.layout == vk::ImageLayout::ePreinitialized) {
        throw std::invalid_argument(
            "Image upload requires a valid final layout"
        );
    }
}
}  // namespace

void imageBarrier(
    vk::raii::CommandBuffer& cmd,
    const GpuImage& image,
    vk::ImageSubresourceRange range,
    ImageAccess before,
    ImageAccess after
) {
    const auto& desc = image.desc();
    if (!image.handle() || !range.aspectMask ||
        range.baseMipLevel >= desc.mipLevels ||
        range.baseArrayLayer >= desc.arrayLayers || !range.levelCount ||
        !range.layerCount ||
        (range.levelCount != VK_REMAINING_MIP_LEVELS &&
         range.levelCount > desc.mipLevels - range.baseMipLevel) ||
        (range.layerCount != VK_REMAINING_ARRAY_LAYERS &&
         range.layerCount > desc.arrayLayers - range.baseArrayLayer)) {
        throw std::out_of_range("Image barrier subresource range is invalid");
    }
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = before.stages,
        .srcAccessMask = before.access,
        .dstStageMask = after.stages,
        .dstAccessMask = after.access,
        .oldLayout = before.layout,
        .newLayout = after.layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image.handle(),
        .subresourceRange = range
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
}

void UploadBatch::upload(
    vk::raii::CommandBuffer& cmd,
    const GpuImage& dst,
    std::span<const uint8_t> data,
    const ImageUploadDesc& desc
) {
    validateImageUpload(dst, data.size_bytes(), desc);
    staging_.emplace_back(
        allocator_,
        BufferDesc{
            data.size_bytes(),
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryAccess::Upload
        }
    );
    staging_.back().write(data);
    auto sub = desc.region.imageSubresource;
    vk::ImageSubresourceRange range{
        sub.aspectMask, sub.mipLevel, 1, sub.baseArrayLayer, sub.layerCount
    };
    ImageAccess transfer{
        vk::ImageLayout::eTransferDstOptimal,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite
    };
    imageBarrier(cmd, dst, range, desc.before, transfer);
    cmd.copyBufferToImage(
        staging_.back().handle(), dst.handle(), transfer.layout, desc.region
    );
    imageBarrier(cmd, dst, range, transfer, desc.after);
}
void uploadImageSync(
    const VulkanContext& ctx,
    const GpuImage& dst,
    std::span<const uint8_t> data,
    const ImageUploadDesc& desc
) {
    UploadBatch batch(ctx.allocator());
    ctx.submitAndWait([&](auto& cmd) { batch.upload(cmd, dst, data, desc); });
}
