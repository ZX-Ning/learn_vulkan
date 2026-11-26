#ifndef MODELUPLOAD_HPP
#define MODELUPLOAD_HPP

#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

class UploadBatch;
class VulkanContext;
struct Material;
struct Mesh;
struct ModelVertex;

namespace model_upload {

Mesh uploadMesh(
    const VulkanContext& context,
    vk::raii::CommandBuffer& cmd,
    UploadBatch& uploads,
    const std::vector<ModelVertex>& vertices,
    const std::vector<uint32_t>& indices
);

Material uploadMaterial(
    const VulkanContext& context,
    vk::raii::CommandBuffer& cmd,
    UploadBatch& uploads,
    vk::DescriptorSetLayout materialSetLayout,
    vk::Extent3D extent,
    std::span<const uint8_t> pixels
);

}  // namespace model_upload

#endif  // MODELUPLOAD_HPP
