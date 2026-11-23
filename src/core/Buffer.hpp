#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vk_mem_alloc.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vulkan/vulkan_raii.hpp>

// This selects a memory access policy, independently of Vulkan buffer usage.
enum class MemoryAccess { Device, Upload, Readback };

struct BufferDesc {
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    MemoryAccess memory = MemoryAccess::Device;
};

// Move-only owner. The allocator and device must outlive this object, and the
// application must finish GPU accesses before destroying or overwriting it.
class Buffer {
public:
    Buffer(VmaAllocator allocator, const BufferDesc& desc);
    ~Buffer();
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    vk::Buffer handle() const noexcept {
        return buffer_;
    }
    vk::DeviceSize size() const noexcept {
        return desc_.size;
    }
    vk::BufferUsageFlags usage() const noexcept {
        return desc_.usage;
    }
    MemoryAccess memoryAccess() const noexcept {
        return desc_.memory;
    }

    // CPU access only; these perform flush/invalidate for noncoherent memory.
    // GPU/CPU execution synchronization remains the caller's responsibility.
    void write(std::span<const uint8_t> data, vk::DeviceSize offset = 0);
    void read(std::span<uint8_t> data, vk::DeviceSize offset = 0) const;

private:
    void release() noexcept;
    void checkRange(vk::DeviceSize offset, size_t size) const;
    VmaAllocator allocator_ = nullptr;
    vk::Buffer buffer_;
    VmaAllocation allocation_ = nullptr;
    void* mapped_ = nullptr;
    BufferDesc desc_{};
};

#endif
