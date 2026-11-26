#include "Model.hpp"

void ModelPrimitive::bind(const vk::raii::PipelineLayout& layout, vk::raii::CommandBuffer& cmd) const {
    if (material.descriptorSet != nullptr) {
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            layout,
            1,
            **material.descriptorSet,
            nullptr
        );
    }
    cmd.bindVertexBuffers(
        0,
        mesh.vertexBuffer->handle(),
        {0}
    );
    cmd.bindIndexBuffer(
        mesh.indexBuffer->handle(),
        0,
        vk::IndexType::eUint32
    );
}

void ModelPrimitive::render(vk::raii::CommandBuffer& cmd) const {
    cmd.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
}
