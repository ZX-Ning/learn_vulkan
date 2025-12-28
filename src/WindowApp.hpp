#ifndef WINDOWAPP_HPP
#define WINDOWAPP_HPP

// c++ std
#include <GLFW/glfw3.h>

#include <functional>
#include <string_view>

#include "utils.hpp"

// forward declaration to avoid including heavy headers
class GLFWwindow;
namespace vk::raii {
class SurfaceKHR;
class Instance;
}  // namespace vk::raii

class WindowApp {
private:
    // unique_ptr make WindowApp moveable but not copyable
    GLFWwindowWrapper window;
    static void resizeCallBackHelper(GLFWwindow* window, int width, int height);

public:
    explicit WindowApp(int width, int height, std::string_view tittle);

    WindowApp(const WindowApp&) = delete;
    WindowApp& operator=(const WindowApp&) = delete;    
    WindowApp(WindowApp&&) = delete;
    WindowApp& operator=(WindowApp&&) = delete;

    std::function<void(int width, int height)> resizeCallBack;
    std::function<void()> drawFrameCallBack;
    std::function<void()> cleanupCallBack;
    void run();
    Size2D<int> getWindowSize();
    Size2D<int> getFrameSize();
    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance);
    float getScale();
};

#endif  // WINDOWAPP_HPP
