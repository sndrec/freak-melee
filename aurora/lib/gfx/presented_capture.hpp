#pragma once
#include "../webgpu/gpu.hpp"

namespace aurora::gfx::presented_capture {
// Render-worker operations. Retention copies into a free fixed slot; publish
// happens only after the corresponding surface Present succeeds.
int retain(const wgpu::CommandEncoder&, const wgpu::Texture&, wgpu::TextureFormat, const wgpu::Extent3D&);
void publish(int slot);
// Called while recording a frame. Returned handles own their GPU references
// until that submitted frame has finished with them.
struct Backdrop { wgpu::BindGroup bind_group; wgpu::Extent3D extent; };
Backdrop backdrop();
void shutdown();
}
