#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <array>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

struct SimpleVertex {
    glm::fvec3 pos;
    glm::fvec4 color;
    glm::fvec2 texcoord;

    static vk::VertexInputBindingDescription bindingDescription() {
        return {0, sizeof(SimpleVertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription{
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(SimpleVertex, pos)
            },
            vk::VertexInputAttributeDescription{
                1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(SimpleVertex, color)
            },
            vk::VertexInputAttributeDescription{
                2, 0, vk::Format::eR32G32Sfloat, offsetof(SimpleVertex, texcoord)
            }
        };
    }
};

struct ModelVertex {
    glm::vec3 pos{0.f};
    glm::vec4 color{1.f};
    glm::vec2 texcoord{0.f};
    glm::vec3 normal{0.f, 0.f, 1.f};

    static vk::VertexInputBindingDescription bindingDescription() {
        return {0, sizeof(ModelVertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription{
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(ModelVertex, pos)
            },
            vk::VertexInputAttributeDescription{
                1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(ModelVertex, color)
            },
            vk::VertexInputAttributeDescription{
                2, 0, vk::Format::eR32G32Sfloat, offsetof(ModelVertex, texcoord)
            },
            vk::VertexInputAttributeDescription{
                3, 0, vk::Format::eR32G32B32Sfloat, offsetof(ModelVertex, normal)
            }
        };
    }
};

#endif  // VERTEX_HPP
