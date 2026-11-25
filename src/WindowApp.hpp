#ifndef WINDOWAPP_HPP
#define WINDOWAPP_HPP

#include <SDL3/SDL_video.h>

#include <memory>
#include <string_view>
#include <tuple>

#include "utils.hpp"

namespace vk::raii {
class SurfaceKHR;
class Instance;
}  // namespace vk::raii

struct WindowDeleter {
    void operator()(SDL_Window* window) const noexcept;
};
using SDLWindowWrapper = std::unique_ptr<SDL_Window, WindowDeleter>;

struct WindowApp {
private:
    SDLWindowWrapper window;

public:
    enum ScalingType {
        MAC_OR_WAYLAND,
        WINDOWS_OR_X11,
    } scalingType;
    explicit WindowApp(int width, int height, std::string_view title);

    WindowApp(const WindowApp&) = delete;
    WindowApp& operator=(const WindowApp&) = delete;
    WindowApp(WindowApp&&) = delete;
    WindowApp& operator=(WindowApp&&) = delete;
    ~WindowApp();

    Size2D<int> getWindowSize() const;
    Size2D<int> getFrameSize() const;
    SDL_Window* getWindowPtr();
    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance);
    std::tuple<ScalingType, float> getScale();
    int getKeyState(int key);
};

#endif  // WINDOWAPP_HPP
