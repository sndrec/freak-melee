#pragma once
#include "../webgpu/gpu.hpp"
#include <array>
#include <functional>
#include <vector>

namespace aurora::gfx::screenshot {
using Path = std::array<char, 1024>;
// Request and take_request run on the game thread; encode runs on the render worker.
bool request(const char* path);
Path take_request();
void encode(const wgpu::CommandEncoder& encoder, const Path& path,
            const wgpu::Texture& presentedTexture, wgpu::TextureFormat format,
            const wgpu::Extent3D& extent,
            std::vector<std::function<void()>>& afterSubmit);
}
