#pragma once
#include "../webgpu/gpu.hpp"
#include <array>
#include <functional>
#include <vector>

namespace aurora::gfx::video_capture {
struct Command { unsigned operation = 0; std::array<char, 1024> path{}; };
// Game-thread commands become immutable parts of the submitted frame.
bool request(const char* path);
Command take_request();
void encode(const wgpu::CommandEncoder&, const Command&, const wgpu::Texture&,
            wgpu::TextureFormat, const wgpu::Extent3D&,
            std::vector<std::function<void()>>& afterSubmit);
// After render_worker::synchronize; drains the last readback and closes the sink.
void shutdown();
}
