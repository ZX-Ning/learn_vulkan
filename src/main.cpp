#include <print>

#include "VulkanApp.hpp"
#include "WindowApp.hpp"

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
const char* TITTLE = "Learn Vulkan";

int main() {
    try {
        VulkanApp app(WindowApp(WIDTH, HEIGHT, TITTLE));
        app.run();
    }
    catch (const std::exception& e) {
        std::println(stderr, "Error: {}", e.what());
        return 1;
    }
    return 0;
}
