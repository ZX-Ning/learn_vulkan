#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstdint>
#include <vector>

#include "ForwardPass.hpp"
#include "FrameContext.hpp"
#include "PostProcessPass.hpp"

struct ForwardRenderLayout;
struct ImDrawData;
struct PresentFrame;
struct RenderSettings;
struct Scene;
class VulkanContext;

class Renderer {
public:
    Renderer(
        const VulkanContext& context,
        const ForwardRenderLayout& layout,
        uint32_t frameCount
    );
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void recordFrame(
        const PresentFrame& frame,
        const Scene& scene,
        const RenderSettings& settings,
        ImDrawData* drawData
    );

private:
    const VulkanContext& context_;
    const ForwardRenderLayout& layout_;
    uint32_t frameCount_;
    ForwardPass forwardPass_;
    PostProcessPass postProcessPass_;
    std::vector<FrameContext> frames_;
    vk::Extent2D resourceExtent_{};
};

#endif  // RENDERER_HPP
