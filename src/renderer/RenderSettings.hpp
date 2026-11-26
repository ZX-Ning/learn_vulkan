#ifndef RENDER_SETTINGS_HPP
#define RENDER_SETTINGS_HPP

#include "../utils.hpp"

struct RenderSettings {
    RGBAColor clearColor{0.12, 0.09, 0.22, 1.f};
    float gamma{1.f};
};

#endif  // RENDER_SETTINGS_HPP
