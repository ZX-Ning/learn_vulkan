#include "ComputeTest.hpp"

#include <array>
#include <cstdint>
#include <print>
#include <vector>

#include "core/BindingLayout.hpp"
#include "core/RenderPipeline.hpp"
#include "core/Transfer.hpp"
#include "core/VulkanContext.hpp"
#include "utils.hpp"

int runComputeTest() {
    constexpr uint32_t elementCount = 10240;
    constexpr uint32_t localSizeX = 64;  // Matches numthreads in compute.slang.
    constexpr uint32_t groupCount =
        (elementCount + localSizeX - 1) / localSizeX;
    constexpr auto computeStage = vk::ShaderStageFlagBits::eCompute;
    constexpr auto storageUsage = vk::BufferUsageFlagBits::eStorageBuffer;
    constexpr auto descriptorType = vk::DescriptorType::eStorageBuffer;

    ContextDesc contextDesc{
        .applicationName = "Vector addition",
        .validation = !IS_RELEASE,
        .device = {
            .queueFlags = vk::QueueFlagBits::eCompute,
            .requiredCapabilities = {DeviceCapability::Synchronization}
        },
    };
    const VulkanContext context(contextDesc);
    const auto& device = context.device();

    std::vector<float> inputA(elementCount);
    std::vector<float> inputB(elementCount);
    std::vector<float> output(elementCount);
    for (uint32_t i = 0; i < elementCount; ++i) {
        inputA[i] = static_cast<float>(i);
        inputB[i] = 0.5f * static_cast<float>(elementCount - i);
    }
    const auto bufferSize = getVectorSize(output);
    const auto inputUsage =
        storageUsage | vk::BufferUsageFlagBits::eTransferDst;
    const auto outputUsage =
        storageUsage | vk::BufferUsageFlagBits::eTransferSrc;
    const auto bufferA = context.createBuffer({bufferSize, inputUsage});
    const auto bufferB = context.createBuffer({bufferSize, inputUsage});
    const auto bufferC = context.createBuffer({bufferSize, outputUsage});

    const std::array declarations{
        DescriptorBinding{0, descriptorType, computeStage},
        DescriptorBinding{1, descriptorType, computeStage},
        DescriptorBinding{2, descriptorType, computeStage},
    };
    const BindingLayout bindings(device, declarations);
    const auto setLayout = bindings.handle();
    const vk::PushConstantRange pushConstant{
        .stageFlags = computeStage,
        .offset = 0,
        .size = sizeof(elementCount),
    };
    const auto pipelineLayout = device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo{}
            .setSetLayouts(setLayout)
            .setPushConstantRanges(pushConstant)
    );
    const auto spirv = decodeSpirv(readFile("shaders/compute.spv"));
    const auto pipeline = createComputePipeline(
        device,
        {.layout = *pipelineLayout,
         .shader = {.spirv = spirv, .specialization = {}}}
    );

    const auto set = bindings.allocate(*context.descriptorPool());
    const std::array bufferBindings{
        BufferBinding{0, bufferA},
        BufferBinding{1, bufferB},
        BufferBinding{2, bufferC},
    };
    bindings.updateBufferBindings(*set, bufferBindings);

    const auto inputABytes = asRawBytes(inputA);
    const auto inputBBytes = asRawBytes(inputB);
    {
        UploadBatch uploads(context.allocator());
        context.submitAndWait([&](vk::raii::CommandBuffer& cmd) {
            uploads.upload(cmd, bufferA, inputABytes);
            uploads.upload(cmd, bufferB, inputBBytes);
            const vk::MemoryBarrier2 uploaded{
                .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
                .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
            };
            cmd.pipelineBarrier2(
                vk::DependencyInfo{}.setMemoryBarriers(uploaded)
            );

            cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
            cmd.bindDescriptorSets(
                vk::PipelineBindPoint::eCompute, *pipelineLayout, 0, *set, {}
            );
            cmd.pushConstants<uint32_t>(
                *pipelineLayout, computeStage, 0, elementCount
            );
            cmd.dispatch(groupCount, 1, 1);
        });
    }

    readBufferSync(
        context,
        bufferC,
        {reinterpret_cast<uint8_t*>(output.data()), bufferSize}
    );
    for (uint32_t i = 0; i < elementCount; ++i) {
        const float expected = inputA[i] + inputB[i];
        if (output[i] != expected) {
            std::println(
                stderr,
                "Mismatch at {} (GPU {}, CPU {})",
                i,
                output[i],
                expected
            );
            return 1;
        }
    }
    std::println(
        "Vector addition passed ({} elements, {} workgroups).",
        elementCount,
        groupCount
    );
    return 0;
}
