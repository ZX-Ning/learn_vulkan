#ifndef TRANSFER_HPP
#define TRANSFER_HPP

#include <vector>

#include "Buffer.hpp"
#include "Image.hpp"

class VulkanContext;

struct ImageAccess {
    vk::ImageLayout layout = vk::ImageLayout::eUndefined;
    vk::PipelineStageFlags2 stages{};
    vk::AccessFlags2 access{};
};
struct ImageUploadDesc {
    vk::BufferImageCopy region;
    ImageAccess before;
    ImageAccess after;
};
// Transitions exactly the supplied range; no implicit layout tracking or queue
// ownership transfer.
void imageBarrier(
    vk::raii::CommandBuffer& cmd,
    const GpuImage& image,
    vk::ImageSubresourceRange range,
    ImageAccess before,
    ImageAccess after
);

// Owns staging only. Keep the batch and destination resources alive until GPU
// completion. Recording helpers emit copies; callers record usage barriers.
class UploadBatch {
public:
    explicit UploadBatch(VmaAllocator allocator) : allocator_(allocator) {}
    UploadBatch(const UploadBatch&) = delete;
    UploadBatch& operator=(const UploadBatch&) = delete;
    UploadBatch(UploadBatch&&) noexcept = default;
    UploadBatch& operator=(UploadBatch&&) noexcept = default;

    void upload(
        vk::raii::CommandBuffer& cmd,
        const Buffer& dst,
        std::span<const uint8_t> data,
        vk::DeviceSize offset = 0
    );

    // Initially supports uncompressed color formats; validates region/data
    // bounds. Emits before -> TransferDst -> after transitions for the selected
    // mip/layers.
    void upload(
        vk::raii::CommandBuffer& cmd,
        const GpuImage& dst,
        std::span<const uint8_t> data,
        const ImageUploadDesc& desc
    );

private:
    VmaAllocator allocator_;
    std::vector<Buffer> staging_;
};

// Transfer barriers require the device synchronization2 feature.
// Blocking convenience helpers for initialization/experiments. Previous GPU
// work on the buffer must be on ctx.queue() or explicitly synchronized with it.
// Broad usage barriers are provided here; recorded batches use explicit
// barriers.
void uploadBufferSync(
    const VulkanContext& ctx,
    const Buffer& dst,
    std::span<const uint8_t> data,
    vk::DeviceSize offset = 0,
    size_t chunkSize = 256u << 20
);
void readBufferSync(
    const VulkanContext& ctx,
    const Buffer& src,
    std::span<uint8_t> data,
    vk::DeviceSize offset = 0,
    size_t chunkSize = 256u << 20
);

void uploadImageSync(
    const VulkanContext& ctx,
    const GpuImage& dst,
    std::span<const uint8_t> data,
    const ImageUploadDesc& desc
);
#endif
