#include "Scene.hpp"

#include "../utils.hpp"

glm::fmat4x4 RenderObject::calcModelMatrix() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 r = glm::mat4_cast(glm::angleAxis(angle, glm::normalize(axis)));
    glm::mat4 s = glm::scale(glm::mat4(1.f), scale * uniformScale);

    return t * r * s;
}

Scene::Scene() {
    resetCamera();
}

void Scene::resetCamera() {
    view.eye = {1.3f, 0.f, 20.0f};
    view.front = {-0.3, 0.2f, -0.9f};
    view.up = {0.f, 1.f, 0.f};

    camera.fovy = degreeToRadian(37.8);
    camera.clipNear = 0.1f;
    camera.clipFar = 2000.f;
}
