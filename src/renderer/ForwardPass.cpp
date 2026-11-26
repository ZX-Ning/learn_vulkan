#include "ForwardPass.hpp"

#include <array>

#include "../Consts.hpp"
#include "../core/Buffer.hpp"
#include "../core/RenderPipeline.hpp"
#include "../core/vertex.hpp"
#include "../utils.hpp"
#include "ForwardShaderData.hpp"
#include "Model.hpp"
#include "Scene.hpp"
#include "Textures.hpp"

namespace {

vk::Viewport makeViewport(vk::Extent2D extent) {
    auto width = static_cast<float>(extent.width);
    auto height = static_cast<float>(extent.height);
    return {0.0f, height, width, -height, 0.0f, 1.0f};
}

}  // namespace

ForwardPass::ForwardPass(
    const vk::raii::Device& device, const vk::raii::PipelineLayout& layout
) : layout_(layout) {
    const auto shader = decodeSpirv(readFile(Consts::SHADER_SPV_PATH));
    const std::array vertexBindings{ModelVertex::bindingDescription()};
    const auto vertexAttributes = ModelVertex::attributeDescriptions();
    const std::array colorTargets{ColorTargetDesc{.format = PING_PONG_COLOR_FORMAT}};
    pipeline_ = createGraphicsPipeline(
        device,
        {
            .layout = *layout_,
            .vertex = {.spirv = shader, .entryPoint = "vertMain", .specialization = {}},
            .fragment = {.spirv = shader, .entryPoint = "fragMain", .specialization = {}},
            .vertexBindings = vertexBindings,
            .vertexAttributes = vertexAttributes,
            .rasterization = {
                .polygonMode = vk::PolygonMode::eFill,
                .cullMode = vk::CullModeFlagBits::eBack,
                .frontFace = vk::FrontFace::eCounterClockwise,
                .lineWidth = 1.0f,
            },
            .depthStencil = {
                .depthTestEnable = vk::True,
                .depthWriteEnable = vk::True,
                .depthCompareOp = vk::CompareOp::eLess,
            },
            .colorTargets = colorTargets,
            .depthFormat = SCENE_DEPTH_FORMAT,
        }
    );
}

void ForwardPass::record(const ForwardPassContext& ctx) const {
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

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        layout_,
        0,
        ctx.sceneSet,
        {}
    );
    for (const auto& object : ctx.scene.objects) {
        const DefaultPushConstant pushConstant{glm::transpose(object.calcModelMatrix())};
        cmd.pushConstants<DefaultPushConstant>(
            layout_, vk::ShaderStageFlagBits::eAllGraphics, 0, pushConstant
        );
        for (const auto& primitive : object.model->primitives) {
            primitive.bind(layout_, cmd);
            primitive.render(cmd);
        }
    }
    cmd.endRendering();
}
