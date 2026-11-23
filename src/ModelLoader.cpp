#include "ModelLoader.hpp"
#include "utils.hpp"

#include <vector>
#include <vulkan/vulkan.hpp>

#include "ImageLoader.hpp"
#include "core/RenderPipeline.hpp"
#include "core/Transfer.hpp"
#include "core/Texture.hpp"
#include "renderer/Textures.hpp"
#include "core/vertex.hpp"
#include "Consts.hpp"
#include "CubeMesh.hpp"
#include "renderer/ForwardRenderLayout.hpp"
#include "renderer/Model.hpp"

namespace {
Mesh createMesh(VulkanContext& context) {
    // create buffers
    auto vertBuf = std::make_shared<Buffer>(context.allocator(), BufferDesc{
        .size = getVectorSize(CUBE_VERTICES),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });
    auto indexBuf = std::make_shared<Buffer>(context.allocator(), BufferDesc{
        .size = getVectorSize(CUBE_INDICES),
        .usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst
    });

    return {
        std::move(vertBuf),
        std::move(indexBuf),
        static_cast<uint32_t>(CUBE_INDICES.size() * 3)
    };
}

}  // namespace

std::shared_ptr<vk::raii::Pipeline> ModelLoader::createSimpleCubePipeline(
    const VulkanContext& context,
    const ForwardRenderLayout& layout,
    vk::Format colorFormat
) {
    auto shader = decodeSpirv(readFile(Consts::SHADER_SPV_PATH));
    vk::VertexInputBindingDescription bindingDescs[] = {
        SimpleVertex::bindingDescription()
    };
    auto attributeDescs = SimpleVertex::attributeDescriptions();
    std::array colorTargets{ColorTargetDesc{.format = colorFormat}};
    return std::make_shared<vk::raii::Pipeline>(createGraphicsPipeline(
        context.device(),
        GraphicsPipelineDesc{
            .layout = *layout.pipelineLayout,
            .vertex = {.spirv = shader, .entryPoint = "vertMain"},
            .fragment = {.spirv = shader, .entryPoint = "fragMain"},
            .vertexBindings = bindingDescs,
            .vertexAttributes = attributeDescs,
            .rasterization = {
                .polygonMode = vk::PolygonMode::eFill,
                .cullMode = vk::CullModeFlagBits::eBack,
                .frontFace = vk::FrontFace::eCounterClockwise,
                .lineWidth = 1.0f
            },
            .depthStencil = {
                .depthTestEnable = vk::True,
                .depthWriteEnable = vk::True,
                .depthCompareOp = vk::CompareOp::eLess
            },
            .colorTargets = colorTargets,
            .depthFormat = vk::Format::eD32Sfloat
        }
    ));
}

namespace {
Material createMaterial(
    VulkanContext& context,
    const ForwardRenderLayout& layout,
    Image& img, vk::Format colorFormat
) {
    auto pipeline = ModelLoader::createSimpleCubePipeline(context, layout, colorFormat);

    // create texture (with sampler)

    auto texture = createDefaultTexture(
        context,
        vk::Extent3D{
            static_cast<uint32_t>(img.width),
            static_cast<uint32_t>(img.height),
            1
        }
    );

    // create descriptor set
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = context.descriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &*layout.materialSetLayout
    };
    auto sets =
        context.device().allocateDescriptorSets(allocInfo);
    assert(sets.size() == 1);
    auto set = std::make_shared<vk::raii::DescriptorSet>(std::move(sets[0]));
    sets.clear();

    vk::DescriptorImageInfo imageInfo{
        .sampler = texture->sampler(),
        .imageView = texture->view(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    std::array descriptorWrites{
        vk::WriteDescriptorSet{
            .dstSet = *set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        }
    };
    context.device().updateDescriptorSets(descriptorWrites, {});

    return {
        std::move(pipeline),
        std::move(texture),
        std::move(set)
    };
}

}  // namespace

Model ModelLoader::loadSimpleCubeModel(
    VulkanContext& context,
    const ForwardRenderLayout& layout, vk::Format colorFormat
) {
    auto imgFile = readFile(Consts::DOG_TEXTURE_PATH);
    auto loadedImg = Image::readImage(imgFile);

    Mesh mesh = createMesh(context);
    Material material = createMaterial(context, layout, loadedImg, colorFormat);

    // Write buffer
    UploadBatch uploads(context.allocator());
    context.submitAndWait([&](vk::raii::CommandBuffer& cmd) {
        uploads.upload(cmd, *mesh.vertexBuffer, asRawBytes(CUBE_VERTICES));
        uploads.upload(cmd, *mesh.indexBuffer, asRawBytes(CUBE_INDICES));
        uploads.upload(cmd, material.texture->image(), loadedImg.rawData(), ImageUploadDesc{
            .region = {
                .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .imageExtent = material.texture->image().desc().extent
            },
            .after = {vk::ImageLayout::eShaderReadOnlyOptimal,
                      vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead}
        });
        vk::MemoryBarrier2 uploaded{
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eVertexInput,
            .dstAccessMask = vk::AccessFlagBits2::eVertexAttributeRead | vk::AccessFlagBits2::eIndexRead
        };
        cmd.pipelineBarrier2(vk::DependencyInfo{}.setMemoryBarriers(uploaded));
    });
    return Model{
        std::move(mesh),
        std::move(material)
    };
}
