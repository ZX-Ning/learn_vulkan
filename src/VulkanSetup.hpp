#ifndef VULKAN_SETUP_HPP
#define VULKAN_SETUP_HPP
#include "core/VulkanContext.hpp"
#include "utils.hpp"
struct WindowApp;
ContextDesc rendererContextDesc(WindowApp& window, bool validation = !IS_RELEASE);
#endif
