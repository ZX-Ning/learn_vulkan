#ifndef RENDERER_TEXTURES_HPP
#define RENDERER_TEXTURES_HPP
#include <memory>

#include "../core/Texture.hpp"
class VulkanContext;
inline constexpr vk::Format PING_PONG_COLOR_FORMAT = vk::Format::eR16G16B16A16Sfloat;
inline constexpr vk::Format SCENE_DEPTH_FORMAT = vk::Format::eD32Sfloat;
std::shared_ptr<Texture> createDefaultTexture(const VulkanContext&, const vk::Extent3D&);
std::unique_ptr<Texture> createDepthTexture(const VulkanContext&, const vk::Extent3D&);
std::unique_ptr<Texture> createPingPongTexture(const VulkanContext&, const vk::Extent3D&);
#endif
