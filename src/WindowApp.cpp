#include "WindowApp.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_vulkan.h>

#include <format>
#include <print>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vulkan/vulkan_raii.hpp>

void WindowDeleter::operator()(SDL_Window* window) const noexcept {
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

WindowApp::WindowApp(int width, int height, std::string_view title) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(
            std::format("failed to init SDL: {}", SDL_GetError())
        );
    }

    window.reset(SDL_CreateWindow(
        std::string(title).c_str(),
        width,
        height,
        SDL_WINDOW_VULKAN |
            SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_HIGH_PIXEL_DENSITY
    ));
    if (!window) {
        std::string error = SDL_GetError();
        SDL_Quit();
        throw std::runtime_error(
            std::format("failed to create SDL window: {}", error)
        );
    }
}

WindowApp::~WindowApp() = default;

Size2D<int> WindowApp::getWindowSize() const {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window.get(), &width, &height);
    return {width, height};
};

Size2D<int> WindowApp::getFrameSize() const {
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window.get(), &width, &height);
    return {width, height};
}

SDL_Window* WindowApp::getWindowPtr() {
    return window.get();
};

vk::raii::SurfaceKHR WindowApp::createSurface(const vk::raii::Instance& instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window.get(), *instance, nullptr, &surface)) {
        throw std::runtime_error(
            std::format("failed to create window surface: {}", SDL_GetError())
        );
    }
    return {instance, surface};
}

std::tuple<WindowApp::ScalingType, float> WindowApp::getScale() {
    auto winSize = getWindowSize();
    auto fbSize = getFrameSize();
    scalingType = fbSize.height == winSize.height ? WINDOWS_OR_X11 : MAC_OR_WAYLAND;
    float scale = SDL_GetWindowDisplayScale(window.get());

    std::println(
        "[Window] Size:{} x {}; Framebuffer Size: {}x{}; scale: {}; Scale method: {}.",
        winSize.width,
        winSize.height,
        fbSize.width,
        fbSize.height,
        scale,
        scalingType == WINDOWS_OR_X11 ? "win/x11" : "mac/wayland"
    );

    return {scalingType, scale};
};

int WindowApp::getKeyState(int key) {
    int numKeys = 0;
    const bool* keys = SDL_GetKeyboardState(&numKeys);
    if (key < 0 || key >= numKeys) {
        return 0;
    }
    return keys[key] ? 1 : 0;
}
