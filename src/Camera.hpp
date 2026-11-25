#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct View {
    glm::fvec3 eye;
    // glm::fvec3 target;
    glm::fvec3 front;
    glm::fvec3 up;
    glm::fmat4x4 calcMat() const {
        return glm::lookAt(eye, eye + front, up);
    }
};

struct Camera {
    float fovy;
    float clipNear;
    float clipFar;
    glm::fmat4x4 calcMat(float aspectRatio) const {
        return glm::perspective(fovy, aspectRatio, clipNear, clipFar);
    }
};

#endif  // CAMERA_HPP
