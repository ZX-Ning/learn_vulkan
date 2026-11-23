#ifndef FORWARDPASS_HPP
#define FORWARDPASS_HPP

#include <vulkan/vulkan_raii.hpp>

struct AppState;
struct FrameContext;
struct ForwardRenderLayout;
struct ImguiApp;

struct ForwardPassContext {
    vk::raii::CommandBuffer& cmd;
    ForwardRenderLayout& layout;
    FrameContext& frame;
    vk::ImageView colorView;
    vk::Extent2D extent;
    AppState& state;
    ImguiApp& imgui;
};

struct ForwardPass {
    void record(const ForwardPassContext& ctx);
};

#endif  // FORWARDPASS_HPP
