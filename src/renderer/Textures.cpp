#include "Textures.hpp"

#include "../core/VulkanContext.hpp"
namespace {
Texture texture(
    const VulkanContext& ctx,
    const vk::Extent3D& extent,
    vk::Format format,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspect,
    bool sampled,
    vk::SamplerAddressMode address = vk::SamplerAddressMode::eClampToEdge
) {
    GpuImage image(
        ctx.allocator(), {.format = format, .extent = extent, .usage = usage}
    );
    auto view = createImageView(
        ctx.device(),
        image,
        {
            .viewType = vk::ImageViewType::e2D,
            .subresourceRange = {
                .aspectMask = aspect,
                .levelCount = 1,
                .layerCount = 1,
            },
        }
    );
    vk::raii::Sampler sampler{nullptr};
    if (sampled) {
        sampler = ctx.device().createSampler({
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = address,
            .addressModeV = address,
            .addressModeW = address,
        });
    }
    return Texture(std::move(image), std::move(view), std::move(sampler));
}
}  // namespace

std::shared_ptr<Texture> createDefaultTexture(
    const VulkanContext& ctx, const vk::Extent3D& extent
) {
    return std::make_shared<Texture>(texture(
        ctx,
        extent,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor,
        true,
        vk::SamplerAddressMode::eRepeat
    ));
}
std::unique_ptr<Texture> createDepthTexture(
    const VulkanContext& ctx, const vk::Extent3D& extent
) {
    return std::make_unique<Texture>(texture(
        ctx,
        extent,
        SCENE_DEPTH_FORMAT,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth,
        false
    ));
}
std::unique_ptr<Texture> createPingPongTexture(
    const VulkanContext& ctx, const vk::Extent3D& extent
) {
    return std::make_unique<Texture>(texture(
        ctx,
        extent,
        PING_PONG_COLOR_FORMAT,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor,
        true
    ));
}
