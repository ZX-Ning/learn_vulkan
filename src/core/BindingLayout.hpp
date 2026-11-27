#ifndef BINDING_LAYOUT_HPP
#define BINDING_LAYOUT_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

class Buffer;

struct DescriptorBinding {
    uint32_t binding;
    vk::DescriptorType type;
    vk::ShaderStageFlags stages;
};

struct BufferBinding {
    uint32_t binding;
    const Buffer& buffer;
    vk::DeviceSize offset = 0;
    vk::DeviceSize range = VK_WHOLE_SIZE;
};

// Describes one descriptor per binding. The device must outlive the layout.
class BindingLayout {
public:
    BindingLayout(
        const vk::raii::Device& device,
        std::span<const DescriptorBinding> declarations
    );

    vk::DescriptorSetLayout handle() const noexcept {
        return *layout_;
    }

    // The pool must support freeing individual sets and outlive the set.
    [[nodiscard]] vk::raii::DescriptorSet allocate(
        vk::DescriptorPool pool
    ) const;

    // The set must have been allocated with this layout. The caller manages
    // GPU synchronization and keeps the bound buffers alive while in use.
    void updateBufferBindings(
        vk::DescriptorSet set, std::span<const BufferBinding> bindings
    ) const;

private:
    const vk::raii::Device& device_;
    std::vector<DescriptorBinding> declarations_;
    vk::raii::DescriptorSetLayout layout_{nullptr};
};

#endif
