#ifndef WINDOWAPP_HPP
#define WINDOWAPP_HPP

// c++ std
#include <functional>
#include <memory>
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
    GLFWwindow* window;
    static void resizeCallBackHelper(GLFWwindow* window, int width, int height);

public:
    WindowApp(int width, int height, std::string_view tittle);
    std::function<void(int width, int height)> resizeCallBack;
    std::function<void()> drawFrameCallBack;
    std::function<void()> cleanupCallBack;
    void run();
    Size2D<int> getFrameSize();
    std::unique_ptr<vk::raii::SurfaceKHR> createSurface(const vk::raii::Instance& instance);

    DISABLE_COPY(WindowApp)
};

#endif  // WINDOWAPP_HPP
