#include "Buffer.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace {
void checkVma(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(operation);
    }
}
}  // namespace

Buffer::Buffer(VmaAllocator allocator, const BufferDesc& desc)
    : allocator_(allocator), desc_(desc) {
    if (!allocator || !desc.size || !desc.usage) {
        throw std::invalid_argument(
            "Buffer requires an allocator, nonzero size and usage"
        );
    }
    vk::BufferCreateInfo info{
        .size = desc.size,
        .usage = desc.usage,
        .sharingMode = vk::SharingMode::eExclusive
    };
    VmaAllocationCreateInfo allocationInfo{};
    allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    switch (desc.memory) {
        case MemoryAccess::Device:
            allocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;
        case MemoryAccess::Upload:
            allocationInfo.flags =
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        case MemoryAccess::Readback:
            allocationInfo.flags =
                VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;
            allocationInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            break;
    }
    VkBuffer buffer;
    VmaAllocationInfo result{};
    checkVma(
        vmaCreateBuffer(
            allocator_, &*info, &allocationInfo, &buffer, &allocation_, &result
        ),
        "Failed to allocate buffer"
    );
    buffer_ = buffer;
    mapped_ = result.pMappedData;
}

Buffer::~Buffer() {
    release();
}

Buffer::Buffer(Buffer&& other) noexcept
    : allocator_(other.allocator_),
      buffer_(std::exchange(other.buffer_, {})),
      allocation_(std::exchange(other.allocation_, nullptr)),
      mapped_(std::exchange(other.mapped_, nullptr)),
      desc_(other.desc_) {
    other.desc_.size = 0;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        release();
        allocator_ = other.allocator_;
        buffer_ = std::exchange(other.buffer_, {});
        allocation_ = std::exchange(other.allocation_, nullptr);
        mapped_ = std::exchange(other.mapped_, nullptr);
        desc_ = other.desc_;
        other.desc_.size = 0;
    }
    return *this;
}

void Buffer::release() noexcept {
    if (buffer_) {
        vmaDestroyBuffer(
            allocator_, static_cast<VkBuffer>(buffer_), allocation_
        );
    }
    buffer_ = nullptr;
    allocation_ = nullptr;
    mapped_ = nullptr;
}

void Buffer::checkRange(vk::DeviceSize offset, size_t size) const {
    if (!buffer_ || offset > desc_.size || size > desc_.size - offset) {
        throw std::out_of_range("Buffer access exceeds its logical size");
    }
}

void Buffer::write(std::span<const uint8_t> data, vk::DeviceSize offset) {
    checkRange(offset, data.size_bytes());
    if (desc_.memory != MemoryAccess::Upload || !mapped_) {
        throw std::logic_error("Buffer is not CPU writable");
    }
    if (data.empty()) {
        return;
    }
    std::memcpy(
        static_cast<uint8_t*>(mapped_) + offset, data.data(), data.size_bytes()
    );
    checkVma(
        vmaFlushAllocation(allocator_, allocation_, offset, data.size_bytes()),
        "Failed to flush buffer memory"
    );
}

void Buffer::read(std::span<uint8_t> data, vk::DeviceSize offset) const {
    checkRange(offset, data.size_bytes());
    if (desc_.memory != MemoryAccess::Readback || !mapped_) {
        throw std::logic_error("Buffer is not CPU readable");
    }
    if (data.empty()) {
        return;
    }
    checkVma(
        vmaInvalidateAllocation(
            allocator_, allocation_, offset, data.size_bytes()
        ),
        "Failed to invalidate buffer memory"
    );
    std::memcpy(
        data.data(),
        static_cast<const uint8_t*>(mapped_) + offset,
        data.size_bytes()
    );
}
