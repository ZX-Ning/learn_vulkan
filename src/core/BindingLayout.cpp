#include "BindingLayout.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "Buffer.hpp"

BindingLayout::BindingLayout(
    const vk::raii::Device& device,
    std::span<const DescriptorBinding> declarations
)
    : device_(device), declarations_(declarations.begin(), declarations.end()) {
    std::ranges::sort(declarations_, {}, &DescriptorBinding::binding);
    if (std::ranges::adjacent_find(
            declarations_, {}, &DescriptorBinding::binding
        ) != declarations_.end()) {
        throw std::invalid_argument("Duplicate descriptor binding number");
    }

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(declarations_.size());
    for (const auto& declaration : declarations_) {
        bindings.push_back({
            .binding = declaration.binding,
            .descriptorType = declaration.type,
            .descriptorCount = 1,
            .stageFlags = declaration.stages,
        });
    }
    layout_ = device_.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{}.setBindings(bindings)
    );
}

vk::raii::DescriptorSet BindingLayout::allocate(vk::DescriptorPool pool) const {
    const auto layout = handle();
    auto sets = device_.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo{.descriptorPool = pool}.setSetLayouts(
            layout
        )
    );
    return std::move(sets.front());
}

void BindingLayout::updateBufferBindings(
    vk::DescriptorSet set, std::span<const BufferBinding> bindings
) const {
    std::vector<vk::DescriptorBufferInfo> infos(bindings.size());
    std::vector<vk::WriteDescriptorSet> writes;
    writes.reserve(bindings.size());

    for (size_t i = 0; i < bindings.size(); ++i) {
        const auto& [binding, buffer, offset, range] = bindings[i];
        const auto declaration = std::ranges::lower_bound(
            declarations_, binding, {}, &DescriptorBinding::binding
        );
        if (declaration == declarations_.end() ||
            declaration->binding != binding) {
            throw std::invalid_argument("Undeclared descriptor binding");
        }

        vk::BufferUsageFlagBits requiredUsage;
        switch (declaration->type) {
            case vk::DescriptorType::eUniformBuffer:
                requiredUsage = vk::BufferUsageFlagBits::eUniformBuffer;
                break;
            case vk::DescriptorType::eStorageBuffer:
                requiredUsage = vk::BufferUsageFlagBits::eStorageBuffer;
                break;
            default:
                throw std::invalid_argument(
                    "Expected a uniform or storage buffer binding"
                );
        }
        if (!buffer.handle() || !(buffer.usage() & requiredUsage)) {
            throw std::invalid_argument(
                "Invalid buffer or missing descriptor buffer usage"
            );
        }
        if (offset >= buffer.size() || !range ||
            (range != VK_WHOLE_SIZE && range > buffer.size() - offset)) {
            throw std::out_of_range(
                "Descriptor buffer range exceeds buffer bounds or is empty"
            );
        }

        infos[i] = {
            .buffer = buffer.handle(), .offset = offset, .range = range
        };
        writes.push_back({
            .dstSet = set,
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType = declaration->type,
            .pBufferInfo = &infos[i],
        });
    }

    device_.updateDescriptorSets(writes, {});
}
