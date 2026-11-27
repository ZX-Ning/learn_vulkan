#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP
#include <functional>

#include "Buffer.hpp"
#include "DeviceConfig.hpp"
#include "VulkanUtils.hpp"

enum class GpuPreference { None, Discrete, Integrated };

struct ContextDesc {
    std::string applicationName = "Vulkan application";
    bool validation = false;
    // A preference, not a requirement. Falls back to another eligible device;
    // None selects the first eligible device in Vulkan enumeration order.
    GpuPreference gpuPreference = GpuPreference::Discrete;
    std::vector<std::string> instanceExtensions;
    // Optional adapter callback. Core has no SDL/window-system dependency.
    std::function<vk::raii::SurfaceKHR(const vk::raii::Instance&)>
        createSurface;
    DeviceRequirements device;
};

class VulkanContext {
public:
    explicit VulkanContext(const ContextDesc& desc);
    ~VulkanContext() = default;
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    const vk::raii::Instance& instance() const noexcept {
        return instance_;
    }
    const vk::raii::PhysicalDevice& physicalDevice() const noexcept {
        return physicalDevice_;
    }
    const vk::raii::Device& device() const noexcept {
        return device_;
    }
    const vk::raii::SurfaceKHR& surface() const noexcept {
        return surface_;
    }
    const vk::raii::Queue& queue() const noexcept {
        return queue_;
    }
    const vk::raii::CommandPool& commandPool() const noexcept {
        return commandPool_;
    }
    const vk::raii::DescriptorPool& descriptorPool() const noexcept {
        return descriptorPool_;
    }
    uint32_t queueFamilyIndex() const noexcept {
        return plan_->queueFamilyIndex;
    }
    VmaAllocator allocator() const noexcept {
        return *allocator_;
    }
    const DeviceCapabilities& capabilities() const noexcept {
        return capabilities_;
    }
    const DevicePlan& enabledDevice() const noexcept {
        return *plan_;
    }

    // The context must outlive the returned buffer.
    [[nodiscard]] Buffer createBuffer(const BufferDesc& desc) const;

    // Referenced GPU resources survive until return. Queue/pool access must be
    // externally synchronized. A recording exception occurs before submission.
    void submitAndWait(
        const std::function<void(vk::raii::CommandBuffer&)>& record
    ) const;

private:
    vk::raii::Context context_;
    vk::raii::Instance instance_{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger_{nullptr};
    vk::raii::SurfaceKHR surface_{nullptr};
    vk::raii::PhysicalDevice physicalDevice_{nullptr};
    DeviceCapabilities capabilities_;
    std::unique_ptr<DevicePlan> plan_;
    vk::raii::Device device_{nullptr};
    vk::raii::Queue queue_{nullptr};
    vk::raii::CommandPool commandPool_{nullptr};
    vk::raii::DescriptorPool descriptorPool_{nullptr};
    VmaAllocatorWrapper allocator_;
};
#endif
