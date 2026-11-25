#ifndef POST_PROCESS_PASS_HPP
#define POST_PROCESS_PASS_HPP

#include <vulkan/vulkan_raii.hpp>

struct ImDrawData;

class PostProcessPass {
public:
    explicit PostProcessPass(const vk::raii::Device& device);

    vk::DescriptorSetLayout sourceSetLayout() const noexcept;
    void setColorFormat(const vk::raii::Device& device, vk::Format format);

    // Drawing ImGui requires a backend configured for the target color format.
    void record(
        vk::raii::CommandBuffer& cmd,
        vk::ImageView target,
        vk::Extent2D extent,
        vk::DescriptorSet source,
        float gamma,
        ImDrawData* drawData = nullptr
    ) const;

private:
    vk::raii::DescriptorSetLayout sourceSetLayout_{nullptr};
    vk::raii::PipelineLayout layout_{nullptr};
    vk::raii::Pipeline pipeline_{nullptr};
    vk::Format colorFormat_ = vk::Format::eUndefined;
};

#endif  // POST_PROCESS_PASS_HPP
