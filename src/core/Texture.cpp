#include "Texture.hpp"

#include <utility>
Texture::Texture(
    GpuImage image, vk::raii::ImageView view, vk::raii::Sampler sampler
)
    : image_(std::move(image)),
      view_(std::move(view)),
      sampler_(std::move(sampler)) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        sampler_.clear();
        view_.clear();
        image_ = std::move(other.image_);
        view_ = std::move(other.view_);
        sampler_ = std::move(other.sampler_);
    }
    return *this;
}
