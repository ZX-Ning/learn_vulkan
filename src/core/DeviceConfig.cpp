#include "DeviceConfig.hpp"

#include <algorithm>
#include <stdexcept>

namespace {
// One mapping serves support queries and feature-chain creation. Unsupported
// bundles never reach creation, so no subset of an optional bundle is enabled.
bool applyCapability(DeviceCapability capability, const DeviceCapabilities& caps, DeviceFeatureChain& features) {
    auto& f11 = features.get<vk::PhysicalDeviceVulkan11Features>();
    auto& f12 = features.get<vk::PhysicalDeviceVulkan12Features>();
    auto& f13 = features.get<vk::PhysicalDeviceVulkan13Features>();
    bool supported = true;
    auto request = [&](vk::Bool32& requested, vk::Bool32 available) {
        requested = vk::True;
        supported = supported && static_cast<bool>(available);
    };
    switch (capability) {
        case DeviceCapability::Synchronization:
            request(f13.synchronization2, caps.features13.synchronization2);
            break;
        case DeviceCapability::Rendering:
            request(f13.synchronization2, caps.features13.synchronization2);
            request(f13.dynamicRendering, caps.features13.dynamicRendering);
            request(f11.shaderDrawParameters, caps.features11.shaderDrawParameters);
            break;
        case DeviceCapability::Int8Storage:
            request(f12.shaderInt8, caps.features12.shaderInt8);
            request(f12.storageBuffer8BitAccess, caps.features12.storageBuffer8BitAccess);
            request(f12.uniformAndStorageBuffer8BitAccess, caps.features12.uniformAndStorageBuffer8BitAccess);
            break;
        case DeviceCapability::Float16Storage:
            request(f12.shaderFloat16, caps.features12.shaderFloat16);
            request(f11.storageBuffer16BitAccess, caps.features11.storageBuffer16BitAccess);
            request(f11.uniformAndStorageBuffer16BitAccess, caps.features11.uniformAndStorageBuffer16BitAccess);
            break;
        case DeviceCapability::DeviceScopeMemoryModel:
            request(f12.vulkanMemoryModel, caps.features12.vulkanMemoryModel);
            request(f12.vulkanMemoryModelDeviceScope, caps.features12.vulkanMemoryModelDeviceScope);
            break;
        default:
            throw std::invalid_argument("Unknown device capability");
    }
    return supported;
}
}  // namespace
const char* capabilityName(DeviceCapability capability) {
    switch (capability) {
        case DeviceCapability::Synchronization:
            return "Synchronization";
        case DeviceCapability::Rendering:
            return "Rendering";
        case DeviceCapability::Int8Storage:
            return "Int8Storage";
        case DeviceCapability::Float16Storage:
            return "Float16Storage";
        case DeviceCapability::DeviceScopeMemoryModel:
            return "DeviceScopeMemoryModel";
    }
    throw std::invalid_argument("Unknown device capability");
}
bool DeviceCapabilities::supports(DeviceCapability capability) const {
    DeviceFeatureChain requested;
    return applyCapability(capability, *this, requested);
}
bool DeviceCapabilities::supportsExtension(const std::string& name) const {
    return std::find(extensions.begin(), extensions.end(), name) != extensions.end();
}
DeviceCapabilities queryDeviceCapabilities(const vk::raii::PhysicalDevice& device) {
    DeviceCapabilities result;
    result.properties = device.getProperties();
    if (result.properties.apiVersion < vk::ApiVersion13) {
        return result;
    }
    for (const auto& ext : device.enumerateDeviceExtensionProperties()) {
        result.extensions.emplace_back(ext.extensionName.data());
    }
    result.queues = device.getQueueFamilyProperties();
    auto features = device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features>();
    result.features11 = features.get<vk::PhysicalDeviceVulkan11Features>();
    result.features12 = features.get<vk::PhysicalDeviceVulkan12Features>();
    result.features13 = features.get<vk::PhysicalDeviceVulkan13Features>();
    result.features11.pNext = result.features12.pNext = result.features13.pNext = nullptr;
    return result;
}
bool DevicePlan::enabled(DeviceCapability capability) const {
    return std::find(enabledCapabilities.begin(), enabledCapabilities.end(), capability) != enabledCapabilities.end();
}
bool DevicePlan::enabledExtension(const std::string& name) const {
    return std::find(enabledExtensions.begin(), enabledExtensions.end(), name) != enabledExtensions.end();
}
DeviceResolution resolveDevice(const vk::raii::PhysicalDevice& device, const DeviceCapabilities& caps, const DeviceRequirements& requirements, vk::SurfaceKHR surface) {
    DeviceErrors errors;
    DevicePlan plan{};
    if (caps.properties.apiVersion < vk::ApiVersion13) {
        errors.emplace_back("Vulkan 1.3 is required");
    }
    if (!requirements.queueFlags) {
        errors.emplace_back("Queue flags must not be empty");
    }
    if (requirements.present && !surface) {
        errors.emplace_back("Presentation requires a surface");
    }
    bool found = false;
    for (uint32_t i = 0; i < caps.queues.size(); ++i) {
        if (caps.queues[i].queueCount && (caps.queues[i].queueFlags & requirements.queueFlags) == requirements.queueFlags &&
            (!requirements.present || (surface && device.getSurfaceSupportKHR(i, surface)))) {
            plan.queueFamilyIndex = i;
            found = true;
            break;
        }
    }
    if (!found) {
        errors.emplace_back("No queue family satisfies all requested operations");
    }
    auto capability = [&](DeviceCapability value, bool required) {
        if (caps.supports(value)) {
            if (!plan.enabled(value)) {
                plan.enabledCapabilities.push_back(value);
                if (value == DeviceCapability::Rendering && !plan.enabled(DeviceCapability::Synchronization)) {
                    plan.enabledCapabilities.push_back(DeviceCapability::Synchronization);
                }
            }
        }
        else {
            (required ? errors : plan.unavailableOptional).push_back(capabilityName(value));
        }
    };
    for (auto f : requirements.requiredCapabilities) {
        capability(f, true);
    }
    for (auto f : requirements.optionalCapabilities) {
        capability(f, false);
    }
    for (const auto& ext : requirements.extensions) {
        std::string reason;
        if (!caps.supportsExtension(ext.name)) {
            reason = "extension unavailable";
        }
        else if (ext.check) {
            reason = ext.check(device, caps);
        }
        if (!reason.empty()) {
            (ext.required ? errors : plan.unavailableOptional).push_back(ext.name + ": " + reason);
        }
        else {
            if (plan.enabledExtension(ext.name)) {
                errors.push_back("Duplicate extension request: " + ext.name);
                continue;
            }
            plan.enabledExtensions.push_back(ext.name);
            if (ext.enable) {
                ext.enable(plan.featureChain);
            }
        }
    }
    if (requirements.present && !plan.enabledExtension(vk::KHRSwapchainExtensionName)) {
        errors.emplace_back("Presentation requires VK_KHR_swapchain");
    }
    if (!errors.empty()) {
        return errors;
    }
    for (auto value : plan.enabledCapabilities) {
        applyCapability(value, caps, plan.featureChain);
    }
    return plan;
}
