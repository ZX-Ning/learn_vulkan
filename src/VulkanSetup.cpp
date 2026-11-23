#include "VulkanSetup.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include <stdexcept>

#include "WindowApp.hpp"
ContextDesc rendererContextDesc(WindowApp& window, bool validation) {
    ContextDesc desc;
    desc.applicationName = "Learn Vulkan";
    desc.validation = validation;
    Uint32 count = 0;
    auto extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    if (!extensions) {
        throw std::runtime_error(SDL_GetError());
    }
    for (Uint32 i = 0; i < count; ++i) {
        desc.instanceExtensions.emplace_back(extensions[i]);
    }
    desc.createSurface = [&window](const auto& instance) {
        return window.createSurface(instance);
    };
    desc.device.queueFlags =
        vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
    desc.device.present = true;
    desc.device.requiredCapabilities = {DeviceCapability::Rendering};
    desc.device.extensions = {
        {vk::KHRSwapchainExtensionName},
        {vk::EXTMemoryBudgetExtensionName, false}
    };
    return desc;
}
