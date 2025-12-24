add_rules("mode.debug", "mode.release")

add_requires("volk", "glfw", "glm", "vulkan-hpp")

target("learn_vulkan", function()
    set_kind("binary")
    set_languages("c17", "c++23")
    add_files("src/**.cpp")
    add_packages("volk", "glfw", "glm", "vulkan-hpp")
    add_defines("VULKAN_HPP_NO_CONSTRUCTORS")
end)