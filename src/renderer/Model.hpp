#ifndef MODEL_HPP
#define MODEL_HPP

#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "../core/Buffer.hpp"

class Texture;

struct Mesh {
    std::shared_ptr<Buffer> vertexBuffer;
    std::shared_ptr<Buffer> indexBuffer;
    uint32_t indexCount;
};

struct Material {
    std::shared_ptr<Texture> texture;
    std::shared_ptr<vk::raii::DescriptorSet> descriptorSet;
};

struct ModelPrimitive {
    Mesh mesh;
    Material material;
    void bind(const vk::raii::PipelineLayout&, vk::raii::CommandBuffer&) const;
    void render(vk::raii::CommandBuffer&) const;
};

struct Model {
    std::vector<ModelPrimitive> primitives;
};

#endif  // MODEL_HPP
