#include "FrameContext.hpp"

#include <array>
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
    vk::raii::DescriptorSet&& postSet,
    vk::Extent2D extent
)
    : sceneDescriptorSet(std::move(sceneSet)),
      postSourceSet(std::move(postSet)) {
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
    const vk::Extent3D imageExtent{extent.width, extent.height, 1};
    auto depth = createDepthTexture(ctx, imageExtent);
    auto color = createPingPongTexture(ctx, imageExtent);

    const vk::DescriptorImageInfo source{
        .sampler = *color->sampler(),
        .imageView = *color->view(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    const vk::WriteDescriptorSet write{
        .dstSet = *postSourceSet,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &source,
    };
    ctx.device().updateDescriptorSets(write, {});
    depthTexture = std::move(depth);
    pingpongTexture = std::move(color);
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
    vk::DescriptorSetLayout sceneSetLayout,
    vk::DescriptorSetLayout postSetLayout
) {
    std::vector<vk::DescriptorSetLayout> descriptorLayouts;
    descriptorLayouts.reserve(static_cast<size_t>(frameCount) * 2);
    for (uint32_t i = 0; i < frameCount; ++i) {
        descriptorLayouts.push_back(sceneSetLayout);
        descriptorLayouts.push_back(postSetLayout);
    }
    const auto setAllocInfo = vk::DescriptorSetAllocateInfo{
        .descriptorPool = ctx.descriptorPool(),
    }.setSetLayouts(descriptorLayouts);
    auto sets = ctx.device().allocateDescriptorSets(setAllocInfo);

    std::vector<FrameContext> frames;
    frames.reserve(frameCount);
    for (size_t i = 0; i < sets.size(); i += 2) {
        frames.emplace_back(ctx, std::move(sets[i]), std::move(sets[i + 1]), extent);
    }

    return frames;
}
