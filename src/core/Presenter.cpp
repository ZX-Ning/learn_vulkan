#include "Presenter.hpp"

#include <stdexcept>
#include <utility>

#include "Swapchain.hpp"
#include "VulkanContext.hpp"
#include "VulkanUtils.hpp"

Presenter::Presenter(
    const VulkanContext& context,
    vk::Extent2D drawableExtent,
    uint32_t framesInFlight
)
    : context_(context) {
    if (framesInFlight == 0) {
        throw std::invalid_argument(
            "Presenter requires at least one frame slot"
        );
    }
    const auto queueFamily = context_.queueFamilyIndex();
    if (!*context_.surface() ||
        !context_.enabledDevice().enabledExtension(
            vk::KHRSwapchainExtensionName
        ) ||
        !context_.enabledDevice().enabled(DeviceCapability::Synchronization) ||
        !(context_.capabilities().queues[queueFamily].queueFlags &
          vk::QueueFlagBits::eGraphics) ||
        !context_.physicalDevice().getSurfaceSupportKHR(
            queueFamily, context_.surface()
        )) {
        throw std::invalid_argument(
            "Presenter requires swapchain support, synchronization2 and a "
            "graphics/present queue"
        );
    }

    auto commands = context_.device().allocateCommandBuffers(
        {.commandPool = context_.commandPool(),
         .level = vk::CommandBufferLevel::ePrimary,
         .commandBufferCount = framesInFlight}
    );
    frames_.reserve(framesInFlight);
    for (auto& cmd : commands) {
        frames_.push_back(
            {std::move(cmd),
             context_.device().createSemaphore({}),
             context_.device().createFence(
                 {.flags = vk::FenceCreateFlagBits::eSignaled}
             )}
        );
    }
    recreate(drawableExtent);
}

Presenter::~Presenter() = default;

uint32_t Presenter::frameCount() const noexcept {
    return static_cast<uint32_t>(frames_.size());
}

vk::Format Presenter::colorFormat() const noexcept {
    return swapChain_ ? swapChain_->surfaceFormat.format
                      : vk::Format::eUndefined;
}

void Presenter::recreate(vk::Extent2D drawableExtent) {
    if (swapChain_ && *swapChain_->vkSwapChain) {
        context_.device().waitIdle();
    }
    recreatePending_ = true;
    auto previous = std::move(swapChain_);
    auto replacement = std::make_unique<SwapChain>(
        context_,
        drawableExtent,
        previous ? *previous->vkSwapChain : vk::SwapchainKHR{}
    );
    std::vector<vk::raii::Semaphore> renderFinished;
    renderFinished.reserve(replacement->images.size());
    for (size_t i = 0; i < replacement->images.size(); ++i) {
        renderFinished.push_back(context_.device().createSemaphore({}));
    }
    recreatePending_ = !*replacement->vkSwapChain;
    swapChain_ = std::move(replacement);
    renderFinished_ = std::move(renderFinished);
    drawableExtent_ = drawableExtent;
}

void Presenter::draw(
    vk::Extent2D drawableExtent,
    const std::function<void(const PresentFrame&)>& record
) {
    if (drawableExtent.width == 0 || drawableExtent.height == 0) {
        recreatePending_ = true;
        return;
    }
    if (recreatePending_ || drawableExtent != drawableExtent_) {
        recreate(drawableExtent);
    }
    if (!*swapChain_->vkSwapChain) {
        return;
    }

    auto& slot = frames_[frameIndex_];
    if (context_.device().waitForFences(*slot.finished, vk::True, UINT64_MAX) !=
        vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for presentation frame fence");
    }

    uint32_t imageIndex;
    try {
        auto [result, index] = swapChain_->vkSwapChain.acquireNextImage(
            UINT64_MAX, *slot.imageAvailable, {}
        );
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreatePending_ = true;
            return;
        }
        if (result == vk::Result::eTimeout || result == vk::Result::eNotReady) {
            return;
        }
        recreatePending_ = result == vk::Result::eSuboptimalKHR;
        imageIndex = index;
    }
    catch (const vk::OutOfDateKHRError&) {
        recreatePending_ = true;
        return;
    }

    const auto& image = swapChain_->images[imageIndex];
    constexpr auto colorOutput =
        vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    const vk::SemaphoreSubmitInfo acquireWait{
        .semaphore = *slot.imageAvailable, .stageMask = colorOutput
    };
    auto& cmd = slot.cmd;
    try {
        cmd.reset();
        cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        // Match the semaphore wait stage to order the layout change after
        // acquire.
        transitionImageLayout(
            cmd,
            image.image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            colorOutput,
            {},
            colorOutput,
            vk::AccessFlagBits2::eColorAttachmentRead |
                vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::ImageAspectFlagBits::eColor
        );
        record({
            cmd,
            frameIndex_,
            *image.imageView,
            swapChain_->extent,
            colorFormat(),
        });
        transitionImageLayout(
            cmd,
            image.image,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            colorOutput,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eNone,
            {},
            vk::ImageAspectFlagBits::eColor
        );
        cmd.end();
    }
    catch (...) {
        // Consume the acquire semaphore and retire this swapchain on retry.
        recreatePending_ = true;
        context_.queue().submit2(
            vk::SubmitInfo2{}.setWaitSemaphoreInfos(acquireWait), {}
        );
        throw;
    }

    const vk::CommandBufferSubmitInfo command{.commandBuffer = *cmd};
    // Presentation consumes this semaphore independently of the frame-slot
    // fence.
    const vk::SemaphoreSubmitInfo presentSignal{
        .semaphore = *renderFinished_[imageIndex],
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands
    };
    context_.device().resetFences(*slot.finished);
    context_.queue().submit2(
        vk::SubmitInfo2{}
            .setWaitSemaphoreInfos(acquireWait)
            .setCommandBufferInfos(command)
            .setSignalSemaphoreInfos(presentSignal),
        *slot.finished
    );
    frameIndex_ = (frameIndex_ + 1) % frameCount();

    try {
        auto result = context_.queue().presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &presentSignal.semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*swapChain_->vkSwapChain,
            .pImageIndices = &imageIndex,
        });
        recreatePending_ |= result != vk::Result::eSuccess;
    }
    catch (const vk::OutOfDateKHRError&) {
        recreatePending_ = true;
    }
}
