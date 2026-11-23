#include "ForwardPass.hpp"

#include "../AppState.hpp"
#include "../ImguiApp.hpp"
#include "../core/Texture.hpp"
#include "ForwardRenderLayout.hpp"
#include "FrameContext.hpp"
#include "Scene.hpp"

namespace {

vk::Viewport makeViewport(vk::Extent2D extent) {
    auto width = static_cast<float>(extent.width);
    auto height = static_cast<float>(extent.height);
    return {0.0f, height, width, -height, 0.0f, 1.0f};
}

}  // namespace

void ForwardPass::record(const ForwardPassContext& ctx) {
    auto& cmd = ctx.cmd;
    ctx.frame.transitionDepthToAttachment(cmd);
    vk::RenderingAttachmentInfo colorAttachmentInfo{
        .imageView = ctx.colorView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {.color = {ctx.state.clearColor.srgbToLinear()}}
    };
    vk::RenderingAttachmentInfo depthAttachmentInfo{
        .imageView = ctx.frame.depthTexture->view(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = {.depthStencil = vk::ClearDepthStencilValue(1.0f, 0)}
    };

    cmd.beginRendering({
        .renderArea = {.offset = {0, 0}, .extent = ctx.extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo,
    });

    cmd.setViewport(0, makeViewport(ctx.extent));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), ctx.extent));

    if (ctx.state.currentScene.has_value()) {
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            ctx.layout.pipelineLayout,
            0,
            *ctx.frame.sceneDescriptorSet,
            nullptr
        );
        Scene& scene = *ctx.state.currentScene;
        scene.render(
            ctx.layout.pipelineLayout,
            cmd,
            *ctx.frame.sceneUniformBuf,
            static_cast<float>(ctx.extent.width) / ctx.extent.height
        );
    }
    ctx.imgui.renderVk(ctx.imgui.drawImgui(ctx.state), cmd);
    cmd.endRendering();
}
