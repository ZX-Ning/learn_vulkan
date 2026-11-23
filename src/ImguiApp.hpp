// imgui
#ifndef IMGUI_HPP
#define IMGUI_HPP

#include <imgui.h>

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "utils.hpp"

struct WindowApp;
struct AppState;
class VulkanContext;
struct Scene;
struct SDL_Window;
namespace vk::raii {
class CommandBuffer;
}

struct ImguiApp {
    ImguiApp(
        WindowApp& window,
        const VulkanContext& context,
        vk::Format colorFormat,
        uint32_t frameCount
    );
    void initForSdl(WindowApp& windowApp);
    static constexpr int FPS_UPDATE_INTERVAL_MS = 200;
    uint64_t lastShowFpsTime;
    float lastframeTime;
    void initImguiForVk(
        WindowApp& windowApp,
        const VulkanContext& context,
        vk::Format format,
        uint32_t frameCount
    );
    void setColorFormat(vk::Format colorFormat);
    ImDrawData* drawImgui(AppState& state);
    bool wantMouse();
    void renderVk(ImDrawData* drawData, vk::raii::CommandBuffer& cmd);
    ~ImguiApp();
    DISABLE_COPY(ImguiApp)
};

#endif  // IMGUI_HPP
