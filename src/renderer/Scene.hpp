#ifndef SCENE_HPP
#define SCENE_HPP

#include <memory>
#include <vector>

#include "../Camera.hpp"

struct Model;

struct RenderObject {
    std::shared_ptr<const Model> model;
    glm::fvec3 position;
    glm::fvec3 scale;
    float uniformScale{4.f};
    // rotation
    float angle;
    glm::vec3 axis;

    glm::fmat4x4 calcModelMatrix() const;
};

struct Scene {
    std::vector<RenderObject> objects;
    View view;
    Camera camera;

    Scene();
    void resetCamera();
};

#endif  // SCENE_HPP
