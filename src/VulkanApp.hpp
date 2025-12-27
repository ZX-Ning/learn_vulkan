#ifndef VULKANAPP_HPP
#define VULKANAPP_HPP

// c++ std libs
#include <array>
#include <cstdint>
#include <vector>

// vulkan-hpp headers
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "WindowApp.hpp"
#include "utils.hpp"

// glfw
#include <GLFW/glfw3.h>

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr bool enableValidationLayers = true;

inline const std::vector<char const*> validationLayers = {};
inline const std::vector<const char*> requiredDeviceExtension = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
    vk::KHRDynamicRenderingExtensionName
};

class WindowApp;

class VulkanApp {
public:
    struct SurfaceImages {
        vk::Image image;
        vk::raii::ImageView imageView = nullptr;
        // vk::raii::ImageView imageViewNorm;
        vk::raii::Semaphore renderComplete = nullptr;
    };
    struct SwapChain {
        vk::raii::SwapchainKHR swapChain = nullptr;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::Extent2D extent;
        std::vector<SurfaceImages> images;
        void reset() {
            swapChain = nullptr;
            extent = {0, 0},
            images.clear();
        }
    };
    struct SimpleBuffer {
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
    };
    struct Frame {
        vk::raii::CommandBuffer cmdBuffer = nullptr;
        vk::raii::Semaphore presentComplete = nullptr;
        vk::raii::Fence fences = nullptr;
    };
    struct AppState {
        RGBAColor clearColor{0.f, 0.f, 0.f, 1.f};
    };

private:
    WindowApp windowApp;
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    uint32_t minImageCount;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::Device device = nullptr;
    uint32_t queueFamilyIndex = ~0;
    vk::raii::Queue queue = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;
    vk::raii::CommandPool commandPool = nullptr;
    std::array<Frame, MAX_FRAMES_IN_FLIGHT> frames;
    SwapChain swapChain;
    uint32_t frameIndex = 0;

    SimpleBuffer vertexBuffer;
    AppState state;

    bool framebufferResized = false;
    void init();
    void initImgui();
    void recreateSwapChain();
    void onResize();
    void drawFrame();

public:
    explicit VulkanApp(WindowApp&&);
    void run();

    DISABLE_COPY(VulkanApp)
};

#endif  // VULKANAPP_HPP
