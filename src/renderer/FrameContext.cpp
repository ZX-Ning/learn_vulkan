#include "FrameContext.hpp"

#include <utility>

#include "../core/Buffer.hpp"
#include "../core/Texture.hpp"
#include "Textures.hpp"
#include "../core/VulkanContext.hpp"
#include "../core/VulkanUtils.hpp"
#include "ForwardShaderData.hpp"

FrameContext::FrameContext(
    const VulkanContext& ctx,
    vk::raii::DescriptorSet&& sceneSet,
    vk::Extent2D extent
)
    : sceneDescriptorSet(std::move(sceneSet)) {
    sceneUniformBuf = std::make_shared<Buffer>(ctx.allocator(), BufferDesc{
        .size = sizeof(DefaultSceneUBO),
        .usage = vk::BufferUsageFlagBits::eUniformBuffer,
        .memory = MemoryAccess::Upload
    });
    recreateTextures(ctx, extent);

    vk::DescriptorBufferInfo bufferInfo{
        .buffer = sceneUniformBuf->handle(),
        .offset = 0,
        .range = sizeof(DefaultSceneUBO)
    };
    std::array descriptorWrites{
        vk::WriteDescriptorSet{
            .dstSet = *sceneDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo
        }
    };
    ctx.device().updateDescriptorSets(descriptorWrites, {});
}

void FrameContext::recreateTextures(const VulkanContext& ctx, vk::Extent2D extent) {
    vk::Extent3D imageExtent{extent.width, extent.height, 1};
    depthTexture = createDepthTexture(ctx, imageExtent);
    pingpongTexture[0] = createPingPongTexture(ctx, imageExtent);
    pingpongTexture[1] = createPingPongTexture(ctx, imageExtent);
}

void FrameContext::transitionDepthToAttachment(vk::raii::CommandBuffer& cmd) {
    transitionImageLayout(
        cmd,
        depthTexture->image().handle(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        {},
        vk::PipelineStageFlagBits2::eEarlyFragmentTests |
            vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentRead |
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::ImageAspectFlagBits::eDepth
    );
}

std::vector<FrameContext> FrameContext::createFrameInFlights(
    const VulkanContext& ctx,
    uint32_t frameCount,
    vk::Extent2D extent,
    vk::DescriptorSetLayout sceneSetLayout
) {
    std::vector<vk::DescriptorSetLayout> descriptorLayouts(frameCount, sceneSetLayout);
    vk::DescriptorSetAllocateInfo setAllocInfo{
        .descriptorPool = ctx.descriptorPool(),
        .descriptorSetCount = frameCount,
        .pSetLayouts = descriptorLayouts.data()
    };
    auto sets = ctx.device().allocateDescriptorSets(setAllocInfo);

    std::vector<FrameContext> frames;
    frames.reserve(frameCount);
    for (auto& set : sets) {
        frames.emplace_back(ctx, std::move(set), extent);
    }

    return frames;
}
