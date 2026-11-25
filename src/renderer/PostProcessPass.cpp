#include "PostProcessPass.hpp"

#include <backends/imgui_impl_vulkan.h>

#include <array>

#include "../Consts.hpp"
#include "../core/RenderPipeline.hpp"
#include "../utils.hpp"

PostProcessPass::PostProcessPass(const vk::raii::Device& device) {
    const vk::DescriptorSetLayoutBinding binding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };
    sourceSetLayout_ = device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{}.setBindings(binding)
    );
    const vk::PushConstantRange gammaRange{
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(float),
    };
    layout_ = device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo{}
            .setSetLayouts(*sourceSetLayout_)
            .setPushConstantRanges(gammaRange)
    );
}

vk::DescriptorSetLayout PostProcessPass::sourceSetLayout() const noexcept {
    return *sourceSetLayout_;
}

void PostProcessPass::setColorFormat(
    const vk::raii::Device& device, vk::Format format
) {
    if (format == colorFormat_) {
        return;
    }
    const auto spirv = decodeSpirv(readFile(Consts::SHADER_POST_SPV_PATH));
    const std::array targets{ColorTargetDesc{.format = format}};
    pipeline_ = createGraphicsPipeline(
        device,
        {
            .layout = *layout_,
            .vertex =
                {
                    .spirv = spirv,
                    .entryPoint = "vertMain",
                    .specialization = {},
                },
            .fragment =
                {
                    .spirv = spirv,
                    .entryPoint = "fragMain",
                    .specialization = {},
                },
            .vertexBindings = {},
            .vertexAttributes = {},
            .colorTargets = targets,
        }
    );
    colorFormat_ = format;
}

void PostProcessPass::record(
    vk::raii::CommandBuffer& cmd,
    vk::ImageView target,
    vk::Extent2D extent,
    vk::DescriptorSet source,
    float gamma,
    ImDrawData* drawData
) const {
    const vk::RenderingAttachmentInfo color{
        .imageView = target,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eDontCare,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };
    cmd.beginRendering({
        .renderArea = {.extent = extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color,
    });

    const float width = static_cast<float>(extent.width);
    const float height = static_cast<float>(extent.height);
    cmd.setViewport(0, vk::Viewport{0.0f, height, width, -height, 0.0f, 1.0f});
    cmd.setScissor(0, vk::Rect2D{.extent = extent});
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline_);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, *layout_, 0, source, {}
    );
    cmd.pushConstants<float>(
        *layout_, vk::ShaderStageFlagBits::eFragment, 0, gamma
    );
    cmd.draw(3, 1, 0, 0);
    if (drawData) {
        ImGui_ImplVulkan_RenderDrawData(drawData, *cmd);
    }
    cmd.endRendering();
}
