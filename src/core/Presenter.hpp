#ifndef PRESENTER_HPP
#define PRESENTER_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

class VulkanContext;
struct SwapChain;

struct PresentFrame {
    vk::raii::CommandBuffer& cmd;
    uint32_t frameIndex;
    vk::ImageView colorView;
    vk::Extent2D extent;
    vk::Format colorFormat;
};

// Presents on the context's graphics queue. The context must outlive this
// object, and the caller must drain its queue before destroying the presenter.
class Presenter {
public:
    Presenter(
        const VulkanContext& context,
        vk::Extent2D drawableExtent,
        uint32_t framesInFlight = 2
    );
    ~Presenter();
    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    uint32_t frameCount() const noexcept;
    vk::Format colorFormat() const noexcept;

    // A zero extent skips recording. The callback runs after the frame slot is
    // idle, with the swapchain image ready to use as a color attachment. Its
    // contents are discarded at the start of each frame. End any rendering
    // scope before returning and leave the swapchain image in attachment
    // layout. Keep resources used by the callback alive until their frame slot
    // is idle. Queue and command-pool access must be externally synchronized.
    void draw(
        vk::Extent2D drawableExtent,
        const std::function<void(const PresentFrame&)>& record
    );

private:
    struct FrameSlot {
        vk::raii::CommandBuffer cmd;
        vk::raii::Semaphore imageAvailable;
        vk::raii::Fence finished;
    };

    void recreate(vk::Extent2D drawableExtent);

    const VulkanContext& context_;
    std::unique_ptr<SwapChain> swapChain_;
    std::vector<FrameSlot> frames_;
    std::vector<vk::raii::Semaphore> renderFinished_;
    vk::Extent2D drawableExtent_{};
    uint32_t frameIndex_ = 0;
    bool recreatePending_ = false;
};

#endif  // PRESENTER_HPP
