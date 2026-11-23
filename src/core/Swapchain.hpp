#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <vector>
#include <vulkan/vulkan_raii.hpp>

class VulkanContext;

struct SurfaceImage {
    vk::Image image;
    vk::raii::ImageView imageView{nullptr};
};

struct SwapChain {
    vk::raii::SwapchainKHR vkSwapChain{nullptr};
    vk::SurfaceFormatKHR surfaceFormat;
    vk::Extent2D extent;
    std::vector<SurfaceImage> images;

    SwapChain(
        const VulkanContext& context,
        vk::Extent2D drawableExtent,
        vk::SwapchainKHR oldSwapchain = {}
    );
};

#endif  // SWAPCHAIN_HPP
