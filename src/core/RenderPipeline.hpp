#ifndef RENDERPIPELINE_HPP
#define RENDERPIPELINE_HPP

#include <span>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

struct SpecializationDesc {
    std::span<const vk::SpecializationMapEntry> entries;
    std::span<const uint8_t> data;
};

// All spans are borrowed only for the duration of pipeline creation.
struct ShaderStageDesc {
    std::span<const uint32_t> spirv;
    std::string entryPoint = "main";
    SpecializationDesc specialization;
};

struct ColorTargetDesc {
    vk::Format format;
    vk::PipelineColorBlendAttachmentState blend{
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
    };
};

struct GraphicsPipelineDesc {
    vk::PipelineLayout layout;
    ShaderStageDesc vertex;
    ShaderStageDesc fragment;
    std::span<const vk::VertexInputBindingDescription> vertexBindings;
    std::span<const vk::VertexInputAttributeDescription> vertexAttributes;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };
    vk::PipelineRasterizationStateCreateInfo rasterization{
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f
    };
    vk::PipelineMultisampleStateCreateInfo multisample{
        .rasterizationSamples = vk::SampleCountFlagBits::e1
    };
    vk::PipelineDepthStencilStateCreateInfo depthStencil{};
    std::span<const ColorTargetDesc> colorTargets;
    vk::Format depthFormat = vk::Format::eUndefined;
    vk::Format stencilFormat = vk::Format::eUndefined;
};

struct ComputePipelineDesc {
    vk::PipelineLayout layout;
    ShaderStageDesc shader;
};

// Decode byte-oriented file input into aligned SPIR-V words and validate its header.
std::vector<uint32_t> decodeSpirv(std::span<const uint8_t> bytes);

// Dynamic rendering, with dynamic viewport and scissor. The device/layout and
// pipeline must remain alive while commands referencing them execute.
vk::raii::Pipeline createGraphicsPipeline(
    const vk::raii::Device& device,
    const GraphicsPipelineDesc& desc,
    vk::Optional<const vk::raii::PipelineCache> cache = nullptr
);
vk::raii::Pipeline createComputePipeline(
    const vk::raii::Device& device,
    const ComputePipelineDesc& desc,
    vk::Optional<const vk::raii::PipelineCache> cache = nullptr
);

#endif
