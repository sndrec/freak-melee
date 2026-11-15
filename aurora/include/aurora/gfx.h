#ifndef AURORA_GFX_H
#define AURORA_GFX_H

#ifdef __cplusplus
#include <cstdint>

extern "C" {
#else
#include "stdint.h"
#endif

#if !defined(NDEBUG) && !defined(AURORA_GFX_DEBUG_GROUPS)
#define AURORA_GFX_DEBUG_GROUPS
#endif

void push_debug_group(const char* label);
void pop_debug_group();

typedef struct {
  uint32_t queuedPipelines;
  uint32_t createdPipelines;
  uint32_t drawCallCount;
  uint32_t mergedDrawCallCount;
  uint32_t lastVertSize;
  uint32_t lastUniformSize;
  uint32_t lastIndexSize;
  uint32_t lastStorageSize;
  uint32_t lastTextureUploadSize;
} AuroraStats;

const AuroraStats* aurora_get_stats();
float aurora_get_fps();

void aurora_enable_vsync(bool enabled);
bool aurora_vsync_enabled(void);

typedef enum AuroraImageExportState {
  AuroraImageExport_Idle, AuroraImageExport_Pending, AuroraImageExport_Complete, AuroraImageExport_Failed
} AuroraImageExportState;
typedef void (*AuroraImageDraw)(void* userdata);
// Render into a cleared transparent target and save an RGB/RGBA PNG.
// draw runs synchronously and emits GX commands. GPU readback and writing are
// asynchronous; one export can be pending at a time.
bool aurora_export_render_png(const char* path, uint32_t width, uint32_t height,
                              bool alpha, AuroraImageDraw draw, void* userdata);
AuroraImageExportState aurora_image_export_state(void);

#ifdef __cplusplus
}
#endif

#endif
