#ifndef RENDERER_TEXTURES_HPP
#define RENDERER_TEXTURES_HPP
#include <memory>

#include "../core/Texture.hpp"
class VulkanContext;
std::shared_ptr<Texture> createDefaultTexture(const VulkanContext&, const vk::Extent3D&);
std::unique_ptr<Texture> createDepthTexture(const VulkanContext&, const vk::Extent3D&);
std::unique_ptr<Texture> createPingPongTexture(const VulkanContext&, const vk::Extent3D&);
#endif
