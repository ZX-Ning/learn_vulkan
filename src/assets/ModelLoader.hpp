#ifndef MODELLOADER_HPP
#define MODELLOADER_HPP

#include <filesystem>

#include "../renderer/Model.hpp"

class VulkanContext;

struct ModelLoader {
    static Model loadSimpleCubeModel(
        const VulkanContext& context,
        vk::DescriptorSetLayout materialSetLayout
    );
    static Model loadGltfModel(
        const VulkanContext& context,
        vk::DescriptorSetLayout materialSetLayout,
        const std::filesystem::path& path
    );
};

#endif // MODELLOADER_HPP
