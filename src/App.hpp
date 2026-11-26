#ifndef APP_HPP
#define APP_HPP

#include <filesystem>
#include <memory>

#include "AppControls.hpp"
#include "ImguiApp.hpp"
#include "WindowApp.hpp"
#include "core/VulkanContext.hpp"
#include "renderer/ForwardRenderLayout.hpp"
#include "renderer/Presenter.hpp"
#include "renderer/RenderSettings.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/Scene.hpp"

struct App {
private:
    std::unique_ptr<WindowApp> window;
    std::unique_ptr<VulkanContext> context;
    std::unique_ptr<Presenter> presenter;
    std::unique_ptr<ImguiApp> imgui;
    std::unique_ptr<ForwardRenderLayout> forwardLayout;
    Scene scene;
    std::unique_ptr<Renderer> renderer;

    AppControls controls;
    RenderSettings renderSettings;
    glm::vec2 mousePosition{0.f};
    glm::vec2 mouseDelta{0.f};
    bool mouseDown{false};
    bool quit{false};

    void processEvents();
    void updateInput();
    void onDraw(float frameTime);

public:
    explicit App(bool validation = !IS_RELEASE, const std::filesystem::path& modelPath = {});
    void run();
};

#endif  // APP_HPP
