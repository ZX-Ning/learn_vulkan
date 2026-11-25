// imgui
#ifndef IMGUI_HPP
#define IMGUI_HPP

#include <imgui.h>

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "utils.hpp"

struct WindowApp;
struct AppControls;
class VulkanContext;
struct RenderSettings;
struct Scene;
union SDL_Event;

struct ImguiApp {
    ImguiApp(
        WindowApp& window,
        const VulkanContext& context,
        vk::Format colorFormat,
        uint32_t frameCount
    );
    void processEvent(const SDL_Event& event);
    void setColorFormat(vk::Format colorFormat);
    ImDrawData* buildFrame(
        Scene& scene,
        RenderSettings& settings,
        AppControls& controls,
        float frameTime,
        bool& quit
    );
    bool wantMouse();
    ~ImguiApp();
    DISABLE_COPY(ImguiApp)

private:
    void initForSdl(WindowApp& windowApp);
    void initImguiForVk(
        WindowApp& windowApp,
        const VulkanContext& context,
        vk::Format format,
        uint32_t frameCount
    );
    static constexpr uint64_t FPS_UPDATE_INTERVAL_MS = 200;
    uint64_t lastShowFpsTime = 0;
    float lastFrameTime = 0.f;
    vk::Format colorFormat_ = vk::Format::eUndefined;
};

#endif  // IMGUI_HPP
