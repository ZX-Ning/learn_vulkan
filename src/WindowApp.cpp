#include "WindowApp.hpp"

// std c++
#include <functional>
#include <memory>
#include <print>

// vulkan
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

// glfw
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

// imgui
#include <backends/imgui_impl_glfw.h>

void WindowApp::resizeCallBackHelper(GLFWwindow* window, int width, int height) {
    WindowApp* self = reinterpret_cast<WindowApp*>(glfwGetWindowUserPointer(window));
    std::println("Resized: {}x{}", width, height);
    self->resizeCallBack(width, height);
}

WindowApp::WindowApp(int width, int height, std::string_view tittle) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    window.reset(glfwCreateWindow(
        width,
        height,
        "Learn Vulkan",
        nullptr,
        nullptr
    ));
    glfwSetWindowUserPointer(window.get(), this);
    glfwSetFramebufferSizeCallback(
        window.get(),
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

    ImGui::CreateContext();
    if (!ImGui_ImplGlfw_InitForVulkan(window.get(), true)) {
        throw std::runtime_error("Failed to init IMGUI for GLFW");
    }
}

void WindowApp::run() {
    while (!glfwWindowShouldClose(window.get())) {
        glfwPollEvents();
        drawFrameCallBack();
    }
    cleanupCallBack();
    // glfwTerminate();
    // call terminal will cause segfault on linux when cleanup swapchain?!
}

Size2D<int> WindowApp::getWindowSize() {
    int width, height;
    glfwGetWindowSize(window.get(), &width, &height);
    return {width, height};
};

Size2D<int> WindowApp::getFrameSize() {
    int width, height;
    glfwGetFramebufferSize(window.get(), &width, &height);
    return {width, height};
}

vk::raii::SurfaceKHR WindowApp::createSurface(const vk::raii::Instance& instance) {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(
            *instance, window.get(), nullptr, &surface
        ) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }
    return {instance, surface};
}

float WindowApp::getScale() {
    auto fbSize = getFrameSize();
    auto winSize = getWindowSize();
    return static_cast<float>(fbSize.width) / winSize.width;
};
