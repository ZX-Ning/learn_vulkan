#include "RenderPipeline.hpp"

#include <array>
#include <cstring>
#include <stdexcept>

namespace {
void validateSpirv(std::span<const uint32_t> words) {
    if (words.size() < 5 || words[0] != 0x07230203) {
        throw std::invalid_argument("Invalid SPIR-V header");
    }
}

vk::raii::ShaderModule createModule(
    const vk::raii::Device& device,
    const ShaderStageDesc& desc
) {
    validateSpirv(desc.spirv);
    if (desc.entryPoint.empty() || desc.entryPoint.find('\0') != std::string::npos) {
        throw std::invalid_argument("Shader entry point must be a nonempty name");
    }
    return device.createShaderModule({
        .codeSize = desc.spirv.size_bytes(),
        .pCode = desc.spirv.data(),
    });
}

vk::SpecializationInfo specialization(const SpecializationDesc& desc) {
    for (size_t i = 0; i < desc.entries.size(); ++i) {
        const auto& entry = desc.entries[i];
        if (!entry.size || entry.offset > desc.data.size_bytes() ||
            entry.size > desc.data.size_bytes() - entry.offset) {
            throw std::invalid_argument("Specialization entry exceeds data size");
        }
        for (size_t j = 0; j < i; ++j) {
            if (desc.entries[j].constantID == entry.constantID) {
                throw std::invalid_argument("Duplicate specialization constant ID");
            }
        }
    }
    return {
        .mapEntryCount = static_cast<uint32_t>(desc.entries.size()),
        .pMapEntries = desc.entries.data(),
        .dataSize = desc.data.size_bytes(),
        .pData = desc.data.data()
    };
}
}  // namespace

std::vector<uint32_t> decodeSpirv(std::span<const uint8_t> bytes) {
    if (bytes.size_bytes() % sizeof(uint32_t)) {
        throw std::invalid_argument("SPIR-V byte count must be a multiple of four");
    }
    std::vector<uint32_t> words(bytes.size_bytes() / sizeof(uint32_t));
    if (!bytes.empty()) {
        std::memcpy(words.data(), bytes.data(), bytes.size_bytes());
    }
    validateSpirv(words);
    return words;
}

vk::raii::Pipeline createGraphicsPipeline(
    const vk::raii::Device& device,
    const GraphicsPipelineDesc& desc,
    vk::Optional<const vk::raii::PipelineCache> cache
) {
    auto vertex = createModule(device, desc.vertex);
    auto fragment = createModule(device, desc.fragment);
    auto vertexSpec = specialization(desc.vertex.specialization);
    auto fragmentSpec = specialization(desc.fragment.specialization);
    std::array stages{
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *vertex,
            .pName = desc.vertex.entryPoint.c_str(),
            .pSpecializationInfo = &vertexSpec
        },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *fragment,
            .pName = desc.fragment.entryPoint.c_str(),
            .pSpecializationInfo = &fragmentSpec
        }
    };
    vk::PipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput
        .setVertexBindingDescriptions(desc.vertexBindings)
        .setVertexAttributeDescriptions(desc.vertexAttributes);
    vk::PipelineViewportStateCreateInfo viewport{.viewportCount = 1, .scissorCount = 1};
    std::vector<vk::Format> formats;
    std::vector<vk::PipelineColorBlendAttachmentState> blends;
    for (const auto& target : desc.colorTargets) {
        formats.push_back(target.format);
        blends.push_back(target.blend);
    }
    vk::PipelineColorBlendStateCreateInfo blend{};
    blend.setAttachments(blends);
    std::array dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamic{};
    dynamic.setDynamicStates(dynamicStates);
    vk::PipelineRenderingCreateInfo rendering{
        .colorAttachmentCount = static_cast<uint32_t>(formats.size()),
        .pColorAttachmentFormats = formats.data(),
        .depthAttachmentFormat = desc.depthFormat,
        .stencilAttachmentFormat = desc.stencilFormat
    };
    vk::GraphicsPipelineCreateInfo info{
        .pNext = &rendering,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vertexInput,
        .pInputAssemblyState = &desc.inputAssembly,
        .pViewportState = &viewport,
        .pRasterizationState = &desc.rasterization,
        .pMultisampleState = &desc.multisample,
        .pDepthStencilState = &desc.depthStencil,
        .pColorBlendState = &blend,
        .pDynamicState = &dynamic,
        .layout = desc.layout
    };
    return device.createGraphicsPipeline(cache, info);
}

vk::raii::Pipeline createComputePipeline(
    const vk::raii::Device& device,
    const ComputePipelineDesc& desc,
    vk::Optional<const vk::raii::PipelineCache> cache
) {
    auto module = createModule(device, desc.shader);
    auto spec = specialization(desc.shader.specialization);
    return device.createComputePipeline(
        cache,
        {
            .stage = {
                .stage = vk::ShaderStageFlagBits::eCompute,
                .module = *module,
                .pName = desc.shader.entryPoint.c_str(),
                .pSpecializationInfo = &spec,
            },
            .layout = desc.layout,
        }
    );
}
