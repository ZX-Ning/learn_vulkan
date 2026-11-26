#include "App.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <print>
#include <unordered_map>

#include "Consts.hpp"
#include "assets/ModelLoader.hpp"
#include "VulkanSetup.hpp"

static const std::unordered_map<int, glm::vec<3, int8_t>> KEY_MOVE{
    {SDL_SCANCODE_W, {0, 0, -1}},
    {SDL_SCANCODE_A, {-1, 0, 0}},
    {SDL_SCANCODE_S, {0, 0, 1}},
    {SDL_SCANCODE_D, {1, 0, 0}},
    {SDL_SCANCODE_SPACE, {0, 1, 0}},
    {SDL_SCANCODE_LSHIFT, {0, -1, 0}}
};

App::App(bool validation, const std::filesystem::path& modelPath) {
    window = std::make_unique<WindowApp>(
        Consts::WIDTH, Consts::HEIGHT, Consts::TITLE
    );
    context = std::make_unique<VulkanContext>(
        rendererContextDesc(*window, validation)
    );
    presenter = std::make_unique<Presenter>(*context, *window->getWindowPtr());
    imgui = std::make_unique<ImguiApp>(
        *window, *context, presenter->colorFormat(), presenter->frameCount()
    );
    forwardLayout = std::make_unique<ForwardRenderLayout>(*context);
    const auto materialSetLayout = *forwardLayout->materialSetLayout;
    auto model = modelPath.empty()
        ? ModelLoader::loadSimpleCubeModel(*context, materialSetLayout)
        : ModelLoader::loadGltfModel(*context, materialSetLayout, modelPath);
    scene.objects.push_back(
        {.model = std::make_shared<const Model>(std::move(model)),
         .position = {0.f, 0.f, 1.f},
         .scale = {1.f, 1.f, 1.f},
         .angle = 0.f,
         .axis = {0.f, 1.f, 0.f}}
    );
    renderer = std::make_unique<Renderer>(
        *context, *forwardLayout, presenter->frameCount()
    );
}

void App::processEvents() {
    const auto windowId = SDL_GetWindowID(window->getWindowPtr());
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        imgui->processEvent(event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit = true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (event.window.windowID == windowId) {
                    quit = true;
                }
                break;
            case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_RESIZED:
                if (event.window.windowID == windowId) {
                    window->getScale();
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.windowID == windowId &&
                    event.key.key == SDLK_ESCAPE) {
                    controls.showImGui = !controls.showImGui;
                }
                break;
            default:
                break;
        }
    }
}

void App::updateInput() {
    auto* sdlWindow = window->getWindowPtr();
    const bool grabMouse = mouseDown || !controls.showImGui;
    if (SDL_GetWindowRelativeMouseMode(sdlWindow) != grabMouse) {
        SDL_SetWindowRelativeMouseMode(sdlWindow, grabMouse);
    }

    glm::vec2 mouse{0.f};
    const SDL_MouseButtonFlags buttons =
        grabMouse ? SDL_GetRelativeMouseState(&mouse.x, &mouse.y)
                  : SDL_GetMouseState(&mouse.x, &mouse.y);
    mouseDown = (buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;
    if (grabMouse) {
        mouseDelta = mouse;
        mousePosition += mouse;
    }
    else {
        mouseDelta = mouse - mousePosition;
        mousePosition = mouse;
    }
}

void App::onDraw(float frameTime) {
    if (controls.rotationSpeed > 1.f && !scene.objects.empty()) {
        auto& obj = scene.objects.front();
        float dAngle =
            degreeToRadian(controls.rotationSpeed / 1000 * frameTime);
        float angle = obj.angle + dAngle;
        obj.angle = std::fmod(angle, std::numbers::pi * 2);
    }
    float displacement = (frameTime / 1000.f) * controls.moveSpeed;
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

    if (!controls.showImGui || (mouseDown && !imgui->wantMouse())) {
        glm::fvec3 move(-mouseDelta, 0.f);
        move.y = -move.y;  // flip y
        float moveNorm = glm::length(move);
        if (moveNorm < 40.f && moveNorm > 0.1f) {
            auto viewMatInv = glm::inverse(glm::mat3x3(scene.view.calcMat()));
            move = viewMatInv * move;
            float angle = degreeToRadian(
                std::clamp(controls.mouseSpeed * moveNorm, 0.f, 89.f)
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

    auto drawData =
        imgui->buildFrame(scene, renderSettings, controls, frameTime, quit);
    if (quit) {
        return;
    }
    presenter->draw([&](const PresentFrame& frame) {
        imgui->setColorFormat(frame.colorFormat);
        renderer->recordFrame(frame, scene, renderSettings, drawData);
    });
}

void App::run() {
    std::println("{}", Consts::HELP_MSG);
    try {
        auto previousFrame = std::chrono::steady_clock::now();
        while (!quit) {
            processEvents();
            if (quit) {
                break;
            }
            updateInput();
            auto now = std::chrono::steady_clock::now();
            float frameTime =
                std::chrono::duration<float, std::milli>(now - previousFrame)
                    .count();
            previousFrame = now;
            onDraw(frameTime);
        }
        context->device().waitIdle();
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
