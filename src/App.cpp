#include "App.hpp"

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_scancode.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <print>

#include "Consts.hpp"
#include "ModelLoader.hpp"
#include "VulkanSetup.hpp"

static const std::unordered_map<int, glm::vec<3, int8_t>> KEY_MOVE{
    {SDL_SCANCODE_W, {0, 0, -1}},
    {SDL_SCANCODE_A, {-1, 0, 0}},
    {SDL_SCANCODE_S, {0, 0, 1}},
    {SDL_SCANCODE_D, {1, 0, 0}},
    {SDL_SCANCODE_SPACE, {0, 1, 0}},
    {SDL_SCANCODE_LSHIFT, {0, -1, 0}}
};

App::App(bool validation) {
    state = std::make_unique<AppState>();
    window = std::make_unique<WindowApp>(
        Consts::WIDTH, Consts::HEIGHT, Consts::TITLE, *state
    );
    context = std::make_unique<VulkanContext>(
        rendererContextDesc(*window, validation)
    );
    const Size2D<uint32_t> size = window->getFrameSize();
    presenter = std::make_unique<Presenter>(
        *context, vk::Extent2D{size.width, size.height}
    );
    colorFormat = presenter->colorFormat();
    imgui = std::make_unique<ImguiApp>(
        *window, *context, colorFormat, presenter->frameCount()
    );
    forwardLayout = std::make_unique<ForwardRenderLayout>(*context);
    defaultModel =
        ModelLoader::loadSimpleCubeModel(*context, *forwardLayout, colorFormat);
    defaultRenderobj.reset(new RenderObject{
        .model = defaultModel,
        .position = {0.f, 0.f, 1.f},
        .scale = {1.f, 1.f, 1.f},
        .angle = 0.f,
        .axis = {0.f, 1.f, 0.f}
    });
    std::vector<Ref<RenderObject>> objs{*defaultRenderobj};
    scene = std::make_unique<Scene>(objs, *state);
    renderApp = std::make_unique<RenderApp>(
        *state, *context, *forwardLayout, *imgui, presenter->frameCount()
    );
}

void App::setupCallback() {
    (*window).cleanupCallBack = [this]() {
        this->context->device().waitIdle();
    };
    (*window).drawFrameCallBack = std::bind(&App::onDraw, this);
    (*window).keyCallback = ([this](int key, int action) {
        switch (key) {
            case SDLK_ESCAPE: {
                if (action) {
                    this->state->showImGui = !this->state->showImGui;
                }
                break;
            }
            default: {
                break;
            }
        }
    });
}

void App::onDraw() {
    if (state->rotationSpeed > 1.f) {
        auto& obj = *defaultRenderobj;
        float dAngle =
            degreeToRadian(state->rotationSpeed / 1000 * state->frameTime);
        float angle = obj.angle + dAngle;
        obj.angle = std::fmod(angle, std::numbers::pi * 2);
    }
    uint64_t timeNow = getTimestampMs();
    state->frameTime = timeNow - state->lastRenderTimestamp;
    state->lastRenderTimestamp = timeNow;
    if (!state->currentScene.has_value()) {
        return;
    }
    Scene& scene = (*state->currentScene);

    float displacement = (state->frameTime / 1000.0) * state->moveSpeed;
    for (auto& [k, v] : KEY_MOVE) {
        glm::fvec3 right = glm::cross(scene.view.front, scene.view.up);
        glm::fvec3 forward = glm::cross(right, scene.view.up);
        if (window->getKeyState(k)) {
            glm::fvec3 move = v;
            if (move.x != 0) {  // left-right
                move = move.x * right;
            }
            else if (move.z != 0) {  // forward-backward
                move = move.z * forward;
            }
            scene.view.eye += glm::normalize(move) * displacement;
        }
    }

    if (!state->showImGui ||
        (state->inputs.mouseState && !imgui->wantMouse())) {
        glm::fvec3 move(
            state->inputs.lastMousePos - state->inputs.mousePos, 0.f
        );
        move.y = -move.y;  // flip y
        float moveNorm = glm::length(move);
        if (moveNorm < 40.f && moveNorm > 0.1f) {
            auto viewMatInv = glm::inverse(glm::mat3x3(scene.view.calcMat()));
            move = viewMatInv * move;
            float angle = degreeToRadian(
                std::clamp(state->mouseSpeed * moveNorm, 0.f, 89.f)
            );
            glm::fvec3 axis = glm::cross(move, scene.view.front);
            glm::mat3x3 rMat =
                glm::mat3_cast(glm::angleAxis(angle, glm::normalize(axis)));
            // scene.view.up = rMat * scene.view.up;
            glm::fvec3 newFront = glm::normalize(rMat * scene.view.front);
            float iAngle = radianToDegree(glm::angle(scene.view.up, newFront));
            // std::println("iAngle: {}", iAngle);
            if (iAngle > 1.f && iAngle < 179.f) {
                scene.view.front = newFront;
            }
        }
    }

    if (!state->quit) {
        const Size2D<uint32_t> size =
            window->isMinimized()
                ? Size2D<uint32_t>{}
                : static_cast<Size2D<uint32_t>>(window->getFrameSize());
        presenter->draw(
            {
                size.width,
                size.height,
            },
            [&](const PresentFrame& frame) {
                if (colorFormat != frame.colorFormat) {
                    defaultModel.material.pipeline =
                        ModelLoader::createSimpleCubePipeline(
                            *context, *forwardLayout, frame.colorFormat
                        );
                    imgui->setColorFormat(frame.colorFormat);
                    colorFormat = frame.colorFormat;
                }
                renderApp->recordFrame(frame);
            }
        );
    }
}

void App::run() {
    setupCallback();
    std::println("{}", Consts::HELP_MSG);
    try {
        window->run();
    }
    catch (...) {
        // Drain submitted frames before the scene resources unwind.
        try {
            context->device().waitIdle();
        }
        catch (const vk::SystemError&) {
        }
        throw;
    }
}
