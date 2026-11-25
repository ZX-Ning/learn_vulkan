#ifndef MODELLOADER_HPP
#define MODELLOADER_HPP

#include "core/VulkanContext.hpp"
#include "renderer/Model.hpp"

struct ForwardRenderLayout;

struct ModelLoader {
    static Model loadSimpleCubeModel(
        VulkanContext& context,
        const ForwardRenderLayout& layout,
        vk::Format colorFormat
    );
};

#endif // MODELLOADER_HPP
