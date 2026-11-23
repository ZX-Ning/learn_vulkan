#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include "Image.hpp"

// Optional convenience composition. Declaration order ensures views are destroyed
// before the image, including during exception unwinding.
class Texture {
public:
    Texture(GpuImage image, vk::raii::ImageView view, vk::raii::Sampler sampler = nullptr);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) noexcept = default;
    Texture& operator=(Texture&& other) noexcept;
    const GpuImage& image() const noexcept {
        return image_;
    }
    const vk::raii::ImageView& view() const noexcept {
        return view_;
    }
    const vk::raii::Sampler& sampler() const noexcept {
        return sampler_;
    }

private:
    GpuImage image_;
    vk::raii::ImageView view_;
    vk::raii::Sampler sampler_;
};
#endif
