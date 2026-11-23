#include "RenderApp.hpp"

#include "../AppState.hpp"
#include "../core/Presenter.hpp"
#include "../utils.hpp"
#include "ForwardRenderLayout.hpp"

RenderApp::RenderApp(
    AppState& state,
    VulkanContext& context,
    ForwardRenderLayout& layout,
    ImguiApp& imgui,
    uint32_t frameCount
)
    : state_(state),
      context_(context),
      layout_(layout),
      imgui_(imgui),
      frameCount_(frameCount) {
    state_.lastRenderTimestamp = getTimestampMs();
}

void RenderApp::recordFrame(const PresentFrame& frame) {
    if (frames_.empty()) {
        frames_ = FrameContext::createFrameInFlights(
            context_, frameCount_, frame.extent, *layout_.sceneSetLayout
        );
    }
    else if (resourceExtent_ != frame.extent) {
        // Presenter drains the old frames before recreating the swapchain.
        for (auto& resources : frames_) {
            resources.recreateTextures(context_, frame.extent);
        }
    }
    resourceExtent_ = frame.extent;

    forwardPass_.record({
        .cmd = frame.cmd,
        .layout = layout_,
        .frame = frames_[frame.frameIndex],
        .colorView = frame.colorView,
        .extent = frame.extent,
        .state = state_,
        .imgui = imgui_,
    });
}
