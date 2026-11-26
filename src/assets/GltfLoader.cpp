#include <array>
#include <cmath>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <format>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include "../core/Transfer.hpp"
#include "../core/VulkanContext.hpp"
#include "../core/vertex.hpp"
#include "ImageLoader.hpp"
#include "ModelLoader.hpp"
#include "ModelUpload.hpp"

// GLFT loader is AI generated and not fully viewed.

namespace {

std::span<const uint8_t> imageBytes(
    const fastgltf::Asset& asset, std::size_t imageIndex
) {
    const auto bytes = std::visit(
        [&asset]<class Source>(const Source& source)
            -> std::span<const std::byte> {
            if constexpr (
                std::is_same_v<Source, fastgltf::sources::BufferView>
            ) {
                return fastgltf::DefaultBufferDataAdapter{}(
                    asset, source.bufferViewIndex
                );
            }
            else if constexpr (
                std::is_same_v<Source, fastgltf::sources::Array>
            ) {
                return {source.bytes.data(), source.bytes.size()};
            }
            else if constexpr (
                std::is_same_v<Source, fastgltf::sources::ByteView>
            ) {
                return source.bytes;
            }
            else {
                throw std::runtime_error("Unsupported glTF image source");
            }
        },
        asset.images.at(imageIndex).data
    );
    return {reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()};
}

const fastgltf::Accessor* findAttribute(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive,
    std::string_view name
) {
    const auto attribute = primitive.findAttribute(name);
    return attribute == primitive.attributes.end()
             ? nullptr
             : &asset.accessors.at(attribute->accessorIndex);
}

template <class T>
void validateAttribute(
    const fastgltf::Accessor& accessor, std::size_t vertexCount
) {
    if (accessor.count != vertexCount ||
        accessor.type != fastgltf::ElementTraits<T>::type) {
        throw std::runtime_error("Invalid glTF vertex attribute");
    }
}

std::vector<uint32_t> readIndices(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive,
    std::size_t vertexCount
) {
    std::vector<uint32_t> indices;
    if (primitive.indicesAccessor) {
        const auto& accessor = asset.accessors.at(*primitive.indicesAccessor);
        if (accessor.type != fastgltf::AccessorType::Scalar ||
            accessor.count > std::numeric_limits<uint32_t>::max()) {
            throw std::runtime_error("Invalid glTF index accessor");
        }
        indices.reserve(accessor.count);
        for (std::size_t i = 0; i < accessor.count; ++i) {
            indices.push_back(
                fastgltf::getAccessorElement<uint32_t>(asset, accessor, i)
            );
        }
    }
    else {
        indices.resize(vertexCount);
        std::iota(indices.begin(), indices.end(), 0u);
    }
    if (indices.empty() || indices.size() % 3 != 0) {
        throw std::runtime_error("Invalid glTF triangle indices");
    }
    for (uint32_t index : indices) {
        if (index >= vertexCount) {
            throw std::runtime_error("Invalid glTF triangle indices");
        }
    }
    return indices;
}

glm::vec3 unitNormal(const glm::vec3& normal) {
    const float lengthSquared = glm::dot(normal, normal);
    return lengthSquared > 0.f && std::isfinite(lengthSquared)
             ? normal / std::sqrt(lengthSquared)
             : glm::vec3{0.f, 0.f, 1.f};
}

void generateFlatNormals(
    std::vector<ModelVertex>& vertices, std::vector<uint32_t>& indices
) {
    std::vector<ModelVertex> expanded;
    expanded.reserve(indices.size());
    for (std::size_t i = 0; i < indices.size(); i += 3) {
        auto a = vertices[indices[i]];
        auto b = vertices[indices[i + 1]];
        auto c = vertices[indices[i + 2]];
        a.normal = b.normal = c.normal =
            unitNormal(glm::cross(b.pos - a.pos, c.pos - a.pos));
        expanded.insert(expanded.end(), {a, b, c});
    }
    vertices = std::move(expanded);
    std::iota(indices.begin(), indices.end(), 0u);
}

struct GltfGeometry {
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
};

GltfGeometry readGeometry(
    const fastgltf::Asset& asset,
    const fastgltf::Primitive& primitive,
    const fastgltf::PBRData& pbr,
    const glm::mat4& transform
) {
    if (primitive.type != fastgltf::PrimitiveType::Triangles ||
        !primitive.targets.empty()) {
        throw std::runtime_error(
            "glTF loader requires static triangle primitives"
        );
    }
    const auto* positions = findAttribute(asset, primitive, "POSITION");
    if (!positions || positions->count == 0 ||
        positions->count > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("Invalid glTF POSITION accessor");
    }
    GltfGeometry geometry;
    auto& vertices = geometry.vertices;
    auto& indices = geometry.indices;
    vertices.resize(positions->count);
    indices = readIndices(asset, primitive, vertices.size());
    const auto& factor = pbr.baseColorFactor;
    const glm::vec4 baseColor{factor[0], factor[1], factor[2], factor[3]};
    validateAttribute<glm::vec3>(*positions, vertices.size());
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const auto pos =
            fastgltf::getAccessorElement<glm::vec3>(asset, *positions, i);
        vertices[i].pos = glm::vec3(transform * glm::vec4(pos, 1.f));
        vertices[i].color = baseColor;
    }

    const auto* color = findAttribute(asset, primitive, "COLOR_0");
    if (color && color->type == fastgltf::AccessorType::Vec3) {
        validateAttribute<glm::vec3>(*color, vertices.size());
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const auto value =
                fastgltf::getAccessorElement<glm::vec3>(asset, *color, i);
            vertices[i].color *= glm::vec4(value, 1.f);
        }
    }
    else if (color) {
        validateAttribute<glm::vec4>(*color, vertices.size());
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            vertices[i].color *=
                fastgltf::getAccessorElement<glm::vec4>(asset, *color, i);
        }
    }

    const auto texcoord =
        pbr.baseColorTexture ? pbr.baseColorTexture->texCoordIndex : 0;
    if (const auto* uv = findAttribute(
            asset, primitive, std::format("TEXCOORD_{}", texcoord)
        )) {
        validateAttribute<glm::vec2>(*uv, vertices.size());
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            vertices[i].texcoord =
                fastgltf::getAccessorElement<glm::vec2>(asset, *uv, i);
        }
    }
    else if (pbr.baseColorTexture) {
        throw std::runtime_error(
            "glTF primitive lacks UVs for its base color texture"
        );
    }

    const glm::mat3 linearTransform{transform};
    const float determinant = glm::determinant(linearTransform);
    if (determinant < 0.f) {
        for (std::size_t i = 0; i < indices.size(); i += 3) {
            std::swap(indices[i + 1], indices[i + 2]);
        }
    }
    if (const auto* normals = findAttribute(asset, primitive, "NORMAL");
        normals && determinant != 0.f) {
        const glm::mat3 normalTransform =
            glm::transpose(glm::inverse(linearTransform));
        validateAttribute<glm::vec3>(*normals, vertices.size());
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const auto normal =
                fastgltf::getAccessorElement<glm::vec3>(asset, *normals, i);
            vertices[i].normal = unitNormal(normalTransform * normal);
        }
    }
    else {
        generateFlatNormals(vertices, indices);
    }
    return geometry;
}

struct GltfPrimitiveInstance {
    const fastgltf::Primitive& primitive;
    glm::mat4 transform;
};

std::vector<GltfPrimitiveInstance> collectPrimitives(
    const fastgltf::Asset& asset, std::size_t sceneIndex
) {
    std::vector<GltfPrimitiveInstance> primitives;
    fastgltf::iterateSceneNodes(
        asset,
        sceneIndex,
        fastgltf::math::nmat4x4{1.f},
        [&](const fastgltf::Node& node, const fastgltf::math::nmat4x4& matrix) {
            if (!node.meshIndex) {
                return;
            }
            if (node.skinIndex) {
                throw std::runtime_error(
                    "glTF loader requires unskinned meshes"
                );
            }

            glm::mat4 transform{1.f};
            for (glm::length_t col = 0; col < 4; ++col) {
                const auto& column = matrix[col];
                transform[col] =
                    glm::vec4(column[0], column[1], column[2], column[3]);
            }
            for (const auto& primitive :
                 asset.meshes.at(*node.meshIndex).primitives) {
                primitives.push_back({primitive, transform});
            }
        }
    );
    return primitives;
}

const Material& getOrUploadGltfMaterial(
    const VulkanContext& context,
    vk::raii::CommandBuffer& cmd,
    UploadBatch& uploads,
    vk::DescriptorSetLayout materialSetLayout,
    const fastgltf::Asset& asset,
    const fastgltf::PBRData& pbr,
    std::vector<Material>& materials
) {
    const auto whiteImage = asset.images.size();
    auto imageIndex = whiteImage;
    if (pbr.baseColorTexture) {
        const auto& texture =
            asset.textures.at(pbr.baseColorTexture->textureIndex);
        if (!texture.imageIndex) {
            throw std::runtime_error(
                "glTF base color texture requires a PNG or JPEG image"
            );
        }
        imageIndex = *texture.imageIndex;
    }

    auto& material = materials.at(imageIndex);
    if (material.texture) {
        return material;
    }
    if (imageIndex == whiteImage) {
        constexpr std::array<uint8_t, 4> white{255, 255, 255, 255};
        material = model_upload::uploadMaterial(
            context, cmd, uploads, materialSetLayout, {1, 1, 1}, white
        );
        return material;
    }

    const auto image = Image::readImage(imageBytes(asset, imageIndex));
    material = model_upload::uploadMaterial(
        context,
        cmd,
        uploads,
        materialSetLayout,
        {static_cast<uint32_t>(image.width),
         static_cast<uint32_t>(image.height),
         1},
        image.rawData()
    );
    return material;
}

}  // namespace

Model ModelLoader::loadGltfModel(
    const VulkanContext& context,
    vk::DescriptorSetLayout materialSetLayout,
    const std::filesystem::path& path
) {
    const auto absolutePath = std::filesystem::absolute(path);
    auto file = fastgltf::GltfDataBuffer::FromPath(absolutePath);
    if (file.error() != fastgltf::Error::None) {
        throw std::runtime_error(
            std::format(
                "Cannot open {}: {}",
                path.string(),
                fastgltf::getErrorMessage(file.error())
            )
        );
    }
    fastgltf::Parser parser(fastgltf::Extensions::KHR_materials_unlit);
    auto result = parser.loadGltf(
        file.get(),
        absolutePath.parent_path(),
        fastgltf::Options::LoadExternalBuffers |
            fastgltf::Options::LoadExternalImages
    );
    if (result.error() != fastgltf::Error::None) {
        throw std::runtime_error(
            std::format(
                "Cannot load {}: {}",
                path.string(),
                fastgltf::getErrorMessage(result.error())
            )
        );
    }
    const auto& asset = result.get();
    if (const auto error = fastgltf::validate(asset);
        error != fastgltf::Error::None) {
        throw std::runtime_error(
            std::format(
                "Invalid glTF {}: {}",
                path.string(),
                fastgltf::getErrorMessage(error)
            )
        );
    }
    const auto sceneIndex = asset.defaultScene.value_or(0);
    if (sceneIndex >= asset.scenes.size()) {
        throw std::runtime_error("glTF file has no valid scene");
    }

    const auto primitives = collectPrimitives(asset, sceneIndex);
    if (primitives.empty()) {
        throw std::runtime_error("glTF scene has no triangle meshes");
    }

    Model model;
    model.primitives.reserve(primitives.size());
    UploadBatch uploads(context.allocator());
    std::vector<Material> materials(asset.images.size() + 1);
    const fastgltf::PBRData defaultPbr;
    context.submitAndWait([&](vk::raii::CommandBuffer& cmd) {
        for (const auto& [primitive, transform] : primitives) {
            const auto& pbr =
                primitive.materialIndex
                    ? asset.materials.at(*primitive.materialIndex).pbrData
                    : defaultPbr;
            const auto geometry =
                readGeometry(asset, primitive, pbr, transform);
            const auto& material = getOrUploadGltfMaterial(
                context, cmd, uploads, materialSetLayout, asset, pbr, materials
            );
            model.primitives.push_back({
                .mesh = model_upload::uploadMesh(
                    context, cmd, uploads, geometry.vertices, geometry.indices
                ),
                .material = material,
            });
        }
    });
    return model;
}
