#include "WindowApp.hpp"

// std c++
#include <functional>
#include <memory>
#include <print>

// vulkan
#include <vulkan/vulkan_raii.hpp>

// glfw
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

void WindowApp::resizeCallBackHelper(GLFWwindow* window, int width, int height) {
    WindowApp* self = reinterpret_cast<WindowApp*>(glfwGetWindowUserPointer(window));
    std::println("Resized: {}x{}", width, height);
    self->resizeCallBack(width, height);
}

WindowApp::WindowApp(int width, int height, std::string_view tittle) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR , GLFW_TRUE);

    window = glfwCreateWindow(
        width,
        height,
        "Learn Vulkan",
        nullptr,
        nullptr
    );
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(
        window,
        &resizeCallBackHelper
    );

    auto fbSize = getFrameSize();
    std::println(
        "Created a GLFW window; Size:{} x {}; Framebuffer Size: {}x{}",
        width,
        height,
        fbSize.width,
        fbSize.height
    );
}

void WindowApp::run() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrameCallBack();
    }
    cleanupCallBack();
    glfwDestroyWindow(window);
    // glfwTerminate();
    // call terminal will cause segfault on linux when cleanup swapchain?!
}

Size2D<int> WindowApp::getFrameSize() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return {width, height};
}

std::unique_ptr<vk::raii::SurfaceKHR> WindowApp::createSurface(const vk::raii::Instance& instance) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &surface) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    return std::make_unique<vk::raii::SurfaceKHR>(instance, surface);
}
