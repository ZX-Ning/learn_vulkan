#include "VulkanContext.hpp"

#include <algorithm>
#include <print>
#include <stdexcept>

namespace {
std::vector<const char*> pointers(const std::vector<std::string>& names) {
    std::vector<const char*> result;
    for (const auto& name : names) {
        result.push_back(name.c_str());
    }
    return result;
}
vk::raii::Instance createInstance(
    const vk::raii::Context& context, const ContextDesc& desc
) {
    std::vector<std::string> extensions = desc.instanceExtensions;
    if (desc.validation &&
        std::ranges::find(extensions, vk::EXTDebugUtilsExtensionName) ==
            extensions.end()) {
        extensions.emplace_back(vk::EXTDebugUtilsExtensionName);
    }
    auto available = context.enumerateInstanceExtensionProperties();
    for (const auto& name : extensions) {
        if (std::none_of(
                available.begin(), available.end(), [&](const auto& e) {
                    return name == e.extensionName.data();
                }
            )) {
            throw std::runtime_error("Missing instance extension: " + name);
        }
    }
    std::vector<const char*> layers;
    if (desc.validation) {
        auto availableLayers = context.enumerateInstanceLayerProperties();
        if (std::none_of(
                availableLayers.begin(),
                availableLayers.end(),
                [](const auto& layer) {
                    return std::string_view(layer.layerName.data()) ==
                           "VK_LAYER_KHRONOS_validation";
                }
            )) {
            throw std::runtime_error("Missing VK_LAYER_KHRONOS_validation");
        }
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }
    auto names = pointers(extensions);
    vk::ApplicationInfo app{
        .pApplicationName = desc.applicationName.c_str(),
        .apiVersion = vk::ApiVersion13
    };
    return context.createInstance(
        vk::InstanceCreateInfo{.pApplicationInfo = &app}
            .setPEnabledExtensionNames(names)
            .setPEnabledLayerNames(layers)
    );
}
vk::raii::DebugUtilsMessengerEXT setupDebugMessenger(
    const vk::raii::Instance& instance, bool enableValidation
) {
    if (!enableValidation) {
        return nullptr;
    }
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback =
            [](vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
               vk::DebugUtilsMessageTypeFlagsEXT type,
               const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
               void*) -> vk::Bool32 {
            if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError ||
                severity ==
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
                std::println(
                    "[{}] {}, {}",
                    vk::to_string(severity),
                    vk::to_string(type),
                    pCallbackData->pMessage
                );
            }
            return vk::False;
        }
    };
    return instance.createDebugUtilsMessengerEXT(
        debugUtilsMessengerCreateInfoEXT
    );
}

vk::raii::DescriptorPool createDescriptorPool(const vk::raii::Device& device) {
    std::vector<vk::DescriptorPoolSize> poolSizes{};
    for (int i = 0; i <= 10; i++) {
        poolSizes.push_back({(vk::DescriptorType)i, 1 << 10});
    }
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = static_cast<uint32_t>(1 << 12),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    return vk::raii::DescriptorPool(device, poolInfo);
}

}  // namespace

VulkanContext::VulkanContext(const ContextDesc& desc) {
    instance_ = createInstance(context_, desc);
    debugMessenger_ = setupDebugMessenger(instance_, desc.validation);
    if (desc.createSurface) {
        surface_ = desc.createSurface(instance_);
        if (!*surface_) {
            throw std::runtime_error("Surface adapter returned a null surface");
        }
    }
    auto devices = instance_.enumeratePhysicalDevices();
    std::string failures;
    int selectedScore = -1;
    for (auto& candidate : devices) {
        auto caps = queryDeviceCapabilities(candidate);
        auto resolution =
            resolveDevice(candidate, caps, desc.device, *surface_);
        if (const auto* errors = std::get_if<DeviceErrors>(&resolution)) {
            failures += std::string(caps.properties.deviceName.data()) + ":";
            for (const auto& error : *errors) {
                failures += "\n  " + error;
            }
            failures += "\n";
            continue;
        }
        const auto type = caps.properties.deviceType;
        int score = 0;
        if (desc.gpuPreference != GpuPreference::None) {
            const auto preferredType =
                desc.gpuPreference == GpuPreference::Integrated
                    ? vk::PhysicalDeviceType::eIntegratedGpu
                    : vk::PhysicalDeviceType::eDiscreteGpu;
            if (type == preferredType) {
                score = 2;
            }
            else if (
                type == vk::PhysicalDeviceType::eIntegratedGpu ||
                type == vk::PhysicalDeviceType::eDiscreteGpu
            ) {
                score = 1;
            }
        }
        if (score > selectedScore) {
            selectedScore = score;
            physicalDevice_ = vk::raii::PhysicalDevice(instance_, *candidate);
            capabilities_ = std::move(caps);
            plan_ = std::make_unique<DevicePlan>(
                std::get<DevicePlan>(std::move(resolution))
            );
        }
    }
    if (!plan_) {
        throw std::runtime_error("No device meets requirements:\n" + failures);
    }
    float priority = 1.f;
    vk::DeviceQueueCreateInfo queueInfo{
        .queueFamilyIndex = plan_->queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &priority
    };
    auto extensionNames = pointers(plan_->enabledExtensions);
    vk::DeviceCreateInfo info{
        .pNext = &plan_->featureChain.get<vk::PhysicalDeviceFeatures2>()
    };
    info.setQueueCreateInfos(queueInfo).setPEnabledExtensionNames(
        extensionNames
    );
    device_ = physicalDevice_.createDevice(info);
    queue_ = device_.getQueue(plan_->queueFamilyIndex, 0);
    commandPool_ = device_.createCommandPool(
        {.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
         .queueFamilyIndex = plan_->queueFamilyIndex}
    );
    descriptorPool_ = createDescriptorPool(device_);
    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr =
        instance_.getDispatcher()->vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr =
        device_.getDispatcher()->vkGetDeviceProcAddr;
    VmaAllocatorCreateInfo allocatorInfo{};
    if (plan_->enabledExtension(vk::EXTMemoryBudgetExtensionName)) {
        allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    }
    allocatorInfo.physicalDevice = *physicalDevice_;
    allocatorInfo.device = *device_;
    allocatorInfo.instance = *instance_;
    allocatorInfo.vulkanApiVersion = vk::ApiVersion13;
    allocatorInfo.pVulkanFunctions = &functions;
    allocator_ = VmaAllocatorWrapper(new VmaAllocator{});
    if (vmaCreateAllocator(&allocatorInfo, allocator_.get()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator");
    }
    std::println(
        "[Vulkan] device={} | validation={}",
        capabilities_.properties.deviceName.data(),
        desc.validation ? "enabled" : "disabled"
    );
}

void VulkanContext::submitAndWait(
    const std::function<void(vk::raii::CommandBuffer&)>& record
) const {
    auto buffers = device_.allocateCommandBuffers({
        .commandPool = *commandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    });
    auto& cmd = buffers.front();
    cmd.begin({
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    });
    record(cmd);
    cmd.end();
    vk::raii::Fence completion{device_, vk::FenceCreateInfo{}};
    queue_.submit(vk::SubmitInfo{}.setCommandBuffers(*cmd), *completion);
    if (device_.waitForFences(*completion, vk::True, UINT64_MAX) !=
        vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for command submission");
    }
}
