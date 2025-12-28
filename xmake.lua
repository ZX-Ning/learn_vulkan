add_rules("mode.debug", "mode.release")

add_requires("glfw")
add_requires("glm", "vulkan-hpp", {system = false})

includes("third_party/")

target("learn_vulkan", function()
    set_kind("binary")
    set_languages("c17", "c++23")
    add_files("src/**.cpp")
    add_packages("glfw", "glm", "vulkan-hpp")
    add_deps("imgui_vulkan_glfw")
    add_defines("GLFW_INCLUDE_VULKAN")
    add_defines("VK_NO_PROTOTYPES")
    add_defines("VULKAN_HPP_NO_CONSTRUCTORS")
    add_defines("VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1")
end)
