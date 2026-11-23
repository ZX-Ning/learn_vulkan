#ifndef DEVICE_CONFIG_HPP
#define DEVICE_CONFIG_HPP
#include <functional>
#include <string>
#include <variant>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

// Application-level capability bundles. Every feature in a bundle must be
// supported; optional bundles are enabled completely or left disabled.
enum class DeviceCapability {
    Synchronization,  // synchronization2
    Rendering,    // synchronization2 + dynamicRendering + shaderDrawParameters
    Int8Storage,  // int8 arithmetic + 8-bit storage/uniform-buffer access
    Float16Storage,  // float16 arithmetic + 16-bit storage/uniform-buffer
                     // access
    DeviceScopeMemoryModel  // Vulkan memory model + device scope
};
const char* capabilityName(DeviceCapability capability);

struct DeviceCapabilities {
    vk::PhysicalDeviceProperties properties;
    std::vector<std::string> extensions;
    std::vector<vk::QueueFamilyProperties> queues;
    vk::PhysicalDeviceVulkan11Features features11;
    vk::PhysicalDeviceVulkan12Features features12;
    vk::PhysicalDeviceVulkan13Features features13;
    bool supports(DeviceCapability capability) const;
    bool supportsExtension(const std::string& name) const;
};

DeviceCapabilities queryDeviceCapabilities(
    const vk::raii::PhysicalDevice& device
);

// Vulkan-Hpp owns and links the fixed set of feature structures. Add newly
// supported extension types here when needed; optional nodes start unlinked in
// DevicePlan.
using DeviceFeatureChain = vk::StructureChain<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan12Features,
    vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>;

struct ExtensionRequest {
    std::string name;
    bool required = true;
    // Return an empty string when supported, otherwise a diagnostic reason.
    std::function<
        std::string(const vk::raii::PhysicalDevice&, const DeviceCapabilities&)>
        check;
    // Modify fields through get<T>() and relink<T>() for supported optional
    // nodes. Do not assign whole structures, which would overwrite their pNext
    // links.
    std::function<void(DeviceFeatureChain&)> enable;
};

struct DeviceRequirements {
    vk::QueueFlags queueFlags = vk::QueueFlagBits::eCompute;
    bool present = false;
    std::vector<DeviceCapability> requiredCapabilities;
    std::vector<DeviceCapability> optionalCapabilities;
    std::vector<ExtensionRequest> extensions;
};

struct DevicePlan {
    DevicePlan() {
        featureChain
            .unlink<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    }
    uint32_t queueFamilyIndex = ~0u;
    std::vector<DeviceCapability> enabledCapabilities;
    std::vector<std::string> enabledExtensions;
    std::vector<std::string> unavailableOptional;
    DeviceFeatureChain featureChain;
    bool enabled(DeviceCapability capability) const;
    bool enabledExtension(const std::string& name) const;
};
using DeviceErrors = std::vector<std::string>;
using DeviceResolution = std::variant<DevicePlan, DeviceErrors>;
DeviceResolution resolveDevice(
    const vk::raii::PhysicalDevice& device,
    const DeviceCapabilities& capabilities,
    const DeviceRequirements& requirements,
    vk::SurfaceKHR surface = {}
);
#endif
