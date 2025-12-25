#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

struct SimpleVertex {
    glm::fvec3 pos;
    glm::fvec3 color;

    static vk::VertexInputBindingDescription bindingDescription() {
        return {0, sizeof(SimpleVertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, pos)
            ),
            vk::VertexInputAttributeDescription(
                1, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, color)
            )
        };
    }
};

const std::vector<SimpleVertex> TRAINGLE = {
    {{0.0f, -0.5f, 0.f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}}
};

#endif  // VERTEX_HPP
