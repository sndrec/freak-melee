#pragma once
#include "../webgpu/gpu.hpp"
#include <array>

namespace aurora::gfx::presented_capture {
// Render-worker operations. Retention copies into a free fixed slot; publish
// happens only after the corresponding surface Present succeeds.
int retain(const wgpu::CommandEncoder&, const wgpu::Texture&, wgpu::TextureFormat, const wgpu::Extent3D&);
struct Request {
  uint64_t id = 0;
  bool viewport_only = false;
  std::array<char, 1024> path{};
};
Request take_request();
void publish(int slot, const Request&, const webgpu::Viewport&);
// Called while recording a frame. Returned handles own their GPU references
// until that submitted frame has finished with them.
struct Backdrop { wgpu::BindGroup bind_group; wgpu::Extent3D extent; };
Backdrop backdrop();
void shutdown();
}
