#ifndef FORWARDPASS_HPP
#define FORWARDPASS_HPP

#include <vulkan/vulkan_raii.hpp>

class Buffer;
struct RGBAColor;
struct Scene;

struct ForwardPassContext {
    vk::raii::CommandBuffer& cmd;
    vk::ImageView colorView;
    vk::ImageView depthView;
    vk::Extent2D extent;
    vk::DescriptorSet sceneSet;
    Buffer& sceneUniform;
    const Scene& scene;
    const RGBAColor& clearColor;
};

class ForwardPass {
public:
    ForwardPass(
        const vk::raii::Device& device,
        const vk::raii::PipelineLayout& layout
    );
    void record(const ForwardPassContext& ctx) const;

private:
    const vk::raii::PipelineLayout& layout_;
    vk::raii::Pipeline pipeline_{nullptr};
};

#endif  // FORWARDPASS_HPP
