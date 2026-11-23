#ifndef RENDER_HPP
#define RENDER_HPP

#include <cstdint>
#include <vector>

#include "ForwardPass.hpp"
#include "FrameContext.hpp"

struct AppState;
struct ForwardRenderLayout;
struct ImguiApp;
struct PresentFrame;
class VulkanContext;

class RenderApp {
public:
    RenderApp(
        AppState& state,
        VulkanContext& context,
        ForwardRenderLayout& layout,
        ImguiApp& imgui,
        uint32_t frameCount
    );
    RenderApp(const RenderApp&) = delete;
    RenderApp& operator=(const RenderApp&) = delete;

    void recordFrame(const PresentFrame& frame);

private:
    AppState& state_;
    VulkanContext& context_;
    ForwardRenderLayout& layout_;
    ImguiApp& imgui_;
    uint32_t frameCount_;
    ForwardPass forwardPass_;
    std::vector<FrameContext> frames_;
    vk::Extent2D resourceExtent_{};
};

#endif  // RENDER_HPP
