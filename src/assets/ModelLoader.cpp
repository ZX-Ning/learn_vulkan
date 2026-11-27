#include "ModelLoader.hpp"

#include <array>
#include <utility>

#include "../Consts.hpp"
#include "../CubeMesh.hpp"
#include "../core/Transfer.hpp"
#include "../core/VulkanContext.hpp"
#include "../renderer/Textures.hpp"
#include "../utils.hpp"
#include "ImageLoader.hpp"
#include "ModelUpload.hpp"

namespace model_upload {

Mesh uploadMesh(
    const VulkanContext& context,
    vk::raii::CommandBuffer& cmd,
    UploadBatch& uploads,
    const std::vector<ModelVertex>& vertices,
    const std::vector<uint32_t>& indices
) {
    auto vertBuf = std::make_shared<Buffer>(
        context.createBuffer({
            .size = getVectorSize(vertices),
            .usage = vk::BufferUsageFlagBits::eVertexBuffer |
                     vk::BufferUsageFlagBits::eTransferDst
        })
    );
    auto indexBuf = std::make_shared<Buffer>(
        context.createBuffer({
            .size = getVectorSize(indices),
            .usage = vk::BufferUsageFlagBits::eIndexBuffer |
                     vk::BufferUsageFlagBits::eTransferDst
        })
    );
    uploads.upload(cmd, *vertBuf, asRawBytes(vertices));
    uploads.upload(cmd, *indexBuf, asRawBytes(indices));
    const vk::MemoryBarrier2 uploaded{
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eVertexInput,
        .dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead |
                         vk::AccessFlagBits2::eIndexRead,
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{}.setMemoryBarriers(uploaded));
    return {
        std::move(vertBuf),
        std::move(indexBuf),
        static_cast<uint32_t>(indices.size())
    };
}

Material uploadMaterial(
    const VulkanContext& context,
    vk::raii::CommandBuffer& cmd,
    UploadBatch& uploads,
    vk::DescriptorSetLayout materialSetLayout,
    vk::Extent3D extent,
    std::span<const uint8_t> pixels
) {
    auto texture = createDefaultTexture(context, extent);
    uploads.upload(
        cmd,
        texture->image(),
        pixels,
        {
            .region =
                {
                    .imageSubresource =
                        {.aspectMask = vk::ImageAspectFlagBits::eColor,
                         .layerCount = 1},
                    .imageExtent = extent,
                },
            .before = {},
            .after = {
                vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::PipelineStageFlagBits2::eFragmentShader,
                vk::AccessFlagBits2::eShaderSampledRead,
            },
        }
    );

    auto sets = context.device().allocateDescriptorSets({
        .descriptorPool = context.descriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &materialSetLayout,
    });
    const vk::DescriptorImageInfo imageInfo{
        .sampler = *texture->sampler(),
        .imageView = *texture->view(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    const vk::WriteDescriptorSet write{
        .dstSet = *sets.front(),
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo,
    };
    context.device().updateDescriptorSets(write, {});
    return {
        std::move(texture),
        std::make_shared<vk::raii::DescriptorSet>(std::move(sets.front())),
    };
}

}  // namespace model_upload

Model ModelLoader::loadSimpleCubeModel(
    const VulkanContext& context, vk::DescriptorSetLayout materialSetLayout
) {
    const auto image = Image::readImage(readFile(Consts::DOG_TEXTURE_PATH));
    const std::array faceNormals{
        glm::vec3{0.f, 0.f, 1.f},
        glm::vec3{0.f, 0.f, -1.f},
        glm::vec3{-1.f, 0.f, 0.f},
        glm::vec3{1.f, 0.f, 0.f},
        glm::vec3{0.f, 1.f, 0.f},
        glm::vec3{0.f, -1.f, 0.f},
    };
    std::vector<ModelVertex> vertices;
    vertices.reserve(CUBE_VERTICES.size());
    for (const auto& vertex : CUBE_VERTICES) {
        vertices.push_back(
            {vertex.pos,
             vertex.color,
             vertex.texcoord,
             faceNormals[vertices.size() / 4]}
        );
    }
    std::vector<uint32_t> indices;
    indices.reserve(CUBE_INDICES.size() * 3);
    for (const auto& triangle : CUBE_INDICES) {
        indices.insert(indices.end(), {triangle.x, triangle.y, triangle.z});
    }
    Model model;
    UploadBatch uploads(context.allocator());
    context.submitAndWait([&](vk::raii::CommandBuffer& cmd) {
        model.primitives.push_back({
            .mesh = model_upload::uploadMesh(
                context, cmd, uploads, vertices, indices
            ),
            .material = model_upload::uploadMaterial(
                context,
                cmd,
                uploads,
                materialSetLayout,
                {static_cast<uint32_t>(image.width),
                 static_cast<uint32_t>(image.height),
                 1},
                image.rawData()
            ),
        });
    });
    return model;
}
