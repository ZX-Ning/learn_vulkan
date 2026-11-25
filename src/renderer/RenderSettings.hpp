#ifndef RENDER_SETTINGS_HPP
#define RENDER_SETTINGS_HPP

#include "../utils.hpp"

struct RenderSettings {
    RGBAColor clearColor{0.45f, 0.53f, 0.65f, 1.f};
    float gamma{1.f};
};

#endif  // RENDER_SETTINGS_HPP
