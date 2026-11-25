#include "ForwardPass.hpp"

#include "../core/Buffer.hpp"
#include "../utils.hpp"
#include "ForwardShaderData.hpp"
#include "Model.hpp"
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
    const float aspectRatio = static_cast<float>(ctx.extent.width) / ctx.extent.height;
    const DefaultSceneUBO ubo{
        glm::transpose(ctx.scene.view.calcMat()),
        glm::transpose(ctx.scene.camera.calcMat(aspectRatio)),
    };
    ctx.sceneUniform.write(objectAsRawBytes(ubo));

    vk::RenderingAttachmentInfo colorAttachmentInfo{
        .imageView = ctx.colorView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {.color = {ctx.clearColor.srgbToLinear()}}
    };
    vk::RenderingAttachmentInfo depthAttachmentInfo{
        .imageView = ctx.depthView,
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

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        ctx.pipelineLayout,
        0,
        ctx.sceneSet,
        {}
    );
    for (const auto& object : ctx.scene.objects) {
        object.model->bind(ctx.pipelineLayout, cmd);
        const DefaultPushConstant pushConstant{glm::transpose(object.calcModelMatrix())};
        cmd.pushConstants<DefaultPushConstant>(
            ctx.pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, pushConstant
        );
        object.model->render(cmd);
    }
    cmd.endRendering();
}
