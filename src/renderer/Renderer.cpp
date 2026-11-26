#include "Renderer.hpp"

#include "../core/Transfer.hpp"
#include "../core/VulkanContext.hpp"
#include "ForwardRenderLayout.hpp"
#include "Presenter.hpp"
#include "RenderSettings.hpp"

Renderer::Renderer(
    const VulkanContext& context,
    const ForwardRenderLayout& layout,
    uint32_t frameCount
)
    : context_(context),
      layout_(layout),
      frameCount_(frameCount),
      forwardPass_(context.device(), layout.pipelineLayout),
      postProcessPass_(context.device()) {}

void Renderer::recordFrame(
    const PresentFrame& frame,
    const Scene& scene,
    const RenderSettings& settings,
    ImDrawData* drawData
) {
    postProcessPass_.setColorFormat(context_.device(), frame.colorFormat);
    if (frames_.empty()) {
        frames_ = FrameContext::createFrameInFlights(
            context_,
            frameCount_,
            frame.extent,
            *layout_.sceneSetLayout,
            postProcessPass_.sourceSetLayout()
        );
    }
    else if (resourceExtent_ != frame.extent) {
        // Presenter drains the old frames before recreating the swapchain.
        for (auto& resources : frames_) {
            resources.recreateTextures(context_, frame.extent);
        }
    }
    resourceExtent_ = frame.extent;

    auto& resources = frames_[frame.frameIndex];
    const auto& sceneColor = *resources.pingpongTexture;
    constexpr vk::ImageSubresourceRange colorRange{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .levelCount = 1,
        .layerCount = 1,
    };
    constexpr ImageAccess colorAttachment{
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
    };
    constexpr ImageAccess fragmentSampled{
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderSampledRead,
    };
    // The frame slot is idle. The forward pass clears the previous contents.
    imageBarrier(
        frame.cmd, sceneColor.image(), colorRange, {}, colorAttachment
    );
    resources.transitionDepthToAttachment(frame.cmd);
    forwardPass_.record({
        .cmd = frame.cmd,
        .colorView = *sceneColor.view(),
        .depthView = *resources.depthTexture->view(),
        .extent = frame.extent,
        .sceneSet = *resources.sceneDescriptorSet,
        .sceneUniform = *resources.sceneUniformBuf,
        .scene = scene,
        .clearColor = settings.clearColor,
    });
    imageBarrier(
        frame.cmd,
        sceneColor.image(),
        colorRange,
        colorAttachment,
        fragmentSampled
    );
    postProcessPass_.record(
        frame.cmd,
        frame.colorView,
        frame.extent,
        *resources.postSourceSet,
        settings.gamma,
        drawData
    );
}
