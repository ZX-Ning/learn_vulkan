#ifndef FORWARDPASS_HPP
#define FORWARDPASS_HPP

#include <vulkan/vulkan_raii.hpp>

class Buffer;
struct RGBAColor;
struct Scene;

struct ForwardPassContext {
    vk::raii::CommandBuffer& cmd;
    const vk::raii::PipelineLayout& pipelineLayout;
    vk::ImageView colorView;
    vk::ImageView depthView;
    vk::Extent2D extent;
    vk::DescriptorSet sceneSet;
    Buffer& sceneUniform;
    const Scene& scene;
    const RGBAColor& clearColor;
};

struct ForwardPass {
    void record(const ForwardPassContext& ctx);
};

#endif  // FORWARDPASS_HPP
