#ifndef FRAMECONTEXT_HPP
#define FRAMECONTEXT_HPP

#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "../core/Texture.hpp"

class Buffer;
class VulkanContext;

struct FrameContext {
    vk::raii::DescriptorSet sceneDescriptorSet{nullptr};
    std::array<std::unique_ptr<Texture>, 2> pingpongTexture;
    std::shared_ptr<Buffer> sceneUniformBuf;
    std::unique_ptr<Texture> depthTexture;

    FrameContext(
        const VulkanContext& ctx,
        vk::raii::DescriptorSet&& sceneSet,
        vk::Extent2D extent
    );

    void recreateTextures(const VulkanContext& ctx, vk::Extent2D extent);
    void transitionDepthToAttachment(vk::raii::CommandBuffer& cmd);

    static std::vector<FrameContext> createFrameInFlights(
        const VulkanContext& ctx,
        uint32_t frameCount,
        vk::Extent2D extent,
        vk::DescriptorSetLayout sceneSetLayout
    );
};

#endif  // FRAMECONTEXT_HPP
