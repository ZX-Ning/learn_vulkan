#include "ImguiApp.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "AppControls.hpp"
#include "Consts.hpp"
#include "WindowApp.hpp"
#include "core/VulkanContext.hpp"
#include "renderer/RenderSettings.hpp"
#include "renderer/Scene.hpp"

namespace {
ImGui_ImplVulkan_PipelineInfo pipelineInfo(const vk::Format& format) {
    const vk::PipelineRenderingCreateInfo rendering{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
    };
    ImGui_ImplVulkan_PipelineInfo info{};
    info.PipelineRenderingCreateInfo = *rendering;
    return info;
}
}  // namespace

void ImguiApp::initImguiForVk(
    WindowApp& windowApp,
    const VulkanContext& context,
    vk::Format format,
    uint32_t frameCount
) {
    ImGuiIO* imguiIo = &ImGui::GetIO();
    imguiIo->ConfigFlags |= ImGuiConfigFlags_IsSRGB;
    // imguiIo->IniFilename = NULL;

    // font
    ImFontConfig fontcfg;
    fontcfg.OversampleH = 2;
    fontcfg.OversampleV = 2;
    fontcfg.PixelSnapH = true;
    fontcfg.PixelSnapV = true;
    fontcfg.RasterizerDensity = 0.86f;
    imguiIo->Fonts->AddFontFromFileTTF(Consts::FONT, 16.f, &fontcfg);
    ImGuiStyle* style = &ImGui::GetStyle();

    // set style
    ImGui::StyleColorsDark();
    style->WindowRounding = 5.f;
    style->FrameRounding = 5.f;
    style->WindowPadding = {8, 5};
    style->FramePadding = {3, 2};
    auto [scaleType, scale] = windowApp.getScale();
    if (scaleType == WindowApp::WINDOWS_OR_X11) {
        style->FontScaleDpi = scale;
        style->ScaleAllSizes(scale);
    }

    auto vert = readFile(Consts::SHADER_IMGUI_VERT_PATH);
    vk::ShaderModuleCreateInfo vertInfo{
        .codeSize = getVectorSize(vert),
        .pCode = reinterpret_cast<uint32_t*>(vert.data())
    };
    auto frag = readFile(Consts::SHADER_IMGUI_FRAG_PATH);
    vk::ShaderModuleCreateInfo fragInfo{
        .codeSize = getVectorSize(frag),
        .pCode = reinterpret_cast<uint32_t*>(frag.data())
    };

    const auto renderBufferCount = std::max(2u, frameCount);
    ImGui_ImplVulkan_InitInfo initInfo{
        .ApiVersion = VK_API_VERSION_1_3,
        .Instance = *context.instance(),
        .PhysicalDevice = *context.physicalDevice(),
        .Device = *context.device(),
        .QueueFamily = context.queueFamilyIndex(),
        .Queue = *context.queue(),
        .DescriptorPoolSize = 0xff,
        .MinImageCount = renderBufferCount,
        .ImageCount = renderBufferCount,
        .PipelineInfoMain = pipelineInfo(format),
        .UseDynamicRendering = true,
        .CustomShaderVertCreateInfo = vertInfo,
        .CustomShaderFragCreateInfo = fragInfo
    };

    ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_3,
        [](const char* name, void* data) {
            auto instance = reinterpret_cast<const vk::raii::Instance*>(data);
            return instance->getDispatcher()->vkGetInstanceProcAddr(
                **instance, name
            );
        },
        (void*)(&context.instance())  // using c-style cast for const +
                                      // reinterpret cast
    );

    ImGui_ImplVulkan_Init(&initInfo);
    colorFormat_ = format;
}

ImDrawData* ImguiApp::buildFrame(
    Scene& scene,
    RenderSettings& settings,
    AppControls& controls,
    float frameTime,
    bool& quit
) {
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoInputs;
    {
        ImGui::Begin("##Fullscreen", nullptr, window_flags);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float w = viewport->Size.x / 2.f;
        float h = viewport->Size.y / 2.f;
        drawList->AddLine({w, h - 8.f}, {w, h + 8.f}, 0xFFFFFFFF, 2.f);
        drawList->AddLine({w - 10.f, h}, {w + 10.f, h}, 0xFFFFFFFF, 2.f);
        ImGui::End();
    }

    // auto imguiScale = ImGui::GetIO().DisplayFramebufferScale;
    // auto size = ImGui::GetMainViewport()->Size;
    // std::println("Imgui scale: {}x{}", imguiScale.x, imguiScale.y);
    // ImGui::SetNextWindowSize(ImVec2(size.x * 0.75, size.y * 0.15),
    // ImGuiCond_Appearing);
    if (controls.showImGui) {
        ImGui::Begin("Hello, world!");
        ImGui::ColorEdit3("clear color", settings.clearColor.getRaw());
        ImGui::SliderFloat(
            "Gamma",
            &settings.gamma,
            0.1f,
            4.f,
            "%.2f",
            ImGuiSliderFlags_AlwaysClamp
        );
        ImGui::DragFloat(
            "Move Speed (1/s)", &controls.moveSpeed, 0.01f, 0.f, 20.f, "%.2f"
        );
        ImGui::DragFloat(
            "Mouse (deg/px)", &controls.mouseSpeed, 0.002f, 0.f, 1.f, "%.5f"
        );
        const auto& view = scene.view;
        ImGui::Text(
            "View Front: %.1f, %.1f, %.1f",
            view.front.x,
            view.front.y,
            view.front.z
        );
        ImGui::Text(
            "View Up: %.1f, %.1f, %.1f", view.up.x, view.up.y, view.up.z
        );

        float degreeFov = radianToDegree(scene.camera.fovy);
        if (ImGui::DragFloat("Fov", &degreeFov, 0.05f, -360.f, 360.f, "%.1f")) {
            scene.camera.fovy = degreeToRadian(degreeFov);
        }
        ImGui::DragFloat(
            "Rotate Speed", &controls.rotationSpeed, 1.f, 0.f, 720.f, "%.2f"
        );
        if (!scene.objects.empty()) {
            ImGui::SliderFloat(
                "Model Scale",
                &scene.objects.front().uniformScale,
                0.01f,
                100.f,
                "%.3fx",
                ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp
            );
        }
        ImGui::InputFloat3("Coord", glm::value_ptr(scene.view.eye), "%.2f");
        if (ImGui::Button("Reset Camera")) {
            scene.resetCamera();
        }

        const auto now = getTimestampMs();
        if (now - lastShowFpsTime >= FPS_UPDATE_INTERVAL_MS) {
            lastFrameTime = frameTime;
            lastShowFpsTime = now;
        }
        float fps = lastFrameTime > 0.f ? 1000.f / lastFrameTime : 0.f;
        ImGui::Text("Frame time: %.2f ms (%.0f fps)", lastFrameTime, fps);
        if (ImGui::Button("Quit")) {
            quit = true;
        }
        ImGui::End();
    }
    ImGui::Render();
    return ImGui::GetDrawData();
}

bool ImguiApp::wantMouse() {
    return ImGui::GetIO().WantCaptureMouse;
}

void ImguiApp::processEvent(const SDL_Event& event) {
    ImGui_ImplSDL3_ProcessEvent(&event);
}

ImguiApp::ImguiApp(
    WindowApp& window,
    const VulkanContext& context,
    vk::Format colorFormat,
    uint32_t frameCount
) {
    initForSdl(window);
    initImguiForVk(window, context, colorFormat, frameCount);
}

void ImguiApp::setColorFormat(vk::Format colorFormat) {
    if (colorFormat == colorFormat_) {
        return;
    }
    auto info = pipelineInfo(colorFormat);
    ImGui_ImplVulkan_CreateMainPipeline(&info);
    colorFormat_ = colorFormat;
}

void ImguiApp::initForSdl(WindowApp& windowApp) {
    ImGui::CreateContext();
    if (!ImGui_ImplSDL3_InitForVulkan(windowApp.getWindowPtr())) {
        throw std::runtime_error("Failed to init IMGUI for SDL3");
    }
}

ImguiApp::~ImguiApp() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
