#include "Swapchain.hpp"

#include <algorithm>
#include <stdexcept>

#include "VulkanContext.hpp"

namespace {
vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
    for (auto desired : {
             vk::Format::eB8G8R8A8Srgb,
             vk::Format::eR8G8B8A8Srgb,
             vk::Format::eB8G8R8A8Unorm,
             vk::Format::eR8G8B8A8Unorm
         }) {
        for (const auto& format : formats) {
            if (format.format == desired &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format;
            }
        }
    }
    throw std::runtime_error("No supported swapchain surface format");
}

vk::Extent2D chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities,
    vk::Extent2D fbSize
) {
    if (capabilities.currentExtent.width != 0xFFFFFFFF) {
        return capabilities.currentExtent;
    }

    return {
        .width = std::clamp<uint32_t>(
            fbSize.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        ),
        .height = std::clamp<uint32_t>(
            fbSize.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        )
    };
}

uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) &&
        (surfaceCapabilities.maxImageCount < minImageCount)) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

}  // namespace

SwapChain::SwapChain(
    const VulkanContext& context,
    vk::Extent2D drawableExtent,
    vk::SwapchainKHR oldSwapchain
) {
    auto surfaceCapabilities =
        context.physicalDevice().getSurfaceCapabilitiesKHR(context.surface());
    extent = chooseSwapExtent(surfaceCapabilities, drawableExtent);
    surfaceFormat = chooseSwapSurfaceFormat(
        context.physicalDevice().getSurfaceFormatsKHR(context.surface())
    );
    if (extent.width == 0 || extent.height == 0) {
        return;
    }
    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface = context.surface(),
        .minImageCount = chooseSwapMinImageCount(surfaceCapabilities),
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = true,
        .oldSwapchain = oldSwapchain
    };

    vkSwapChain = context.device().createSwapchainKHR(swapChainCreateInfo);
    auto swapChainImages = vkSwapChain.getImages();
    images.reserve(swapChainImages.size());

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = surfaceFormat.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    for (auto image : swapChainImages) {
        imageViewCreateInfo.image = image;
        images.push_back({image, context.device().createImageView(imageViewCreateInfo)});
    }
}
