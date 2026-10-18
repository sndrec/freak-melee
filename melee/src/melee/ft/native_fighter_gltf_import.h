#ifndef MELEE_FT_NATIVE_FIGHTER_GLTF_IMPORT_H
#define MELEE_FT_NATIVE_FIGHTER_GLTF_IMPORT_H

#ifdef MELEE_NATIVE

#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_skin_weights.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { NATIVE_GLTF_MAX_PRIMITIVES = 128 };

typedef enum NativeGltfBindingMode {
    NATIVE_GLTF_BIND_RIGID,
    NATIVE_GLTF_BIND_AUTOMATIC,
    NATIVE_GLTF_BIND_TRANSFER,
    NATIVE_GLTF_BIND_SOURCE,
} NativeGltfBindingMode;

typedef struct NativeGltfPrimitiveInfo {
    char name[96];
    unsigned vertices;
    unsigned triangles;
    bool has_normals;
    bool has_uv;
    bool has_skin;
    bool has_texture;
} NativeGltfPrimitiveInfo;

typedef struct NativeGltfScan {
    NativeGltfPrimitiveInfo primitives[NATIVE_GLTF_MAX_PRIMITIVES];
    unsigned count;
} NativeGltfScan;

// Parses glTF 2.0/GLB at an explicit import transition. The source file and
// its external buffers are never needed after Apply copies data into the DAT.
bool nativeFighterGltf_Scan(const char* path, NativeGltfScan* scan,
                            char* error, size_t error_size);
// Imports one triangle primitive using rigid, generated, transferred or
// compatible source weights on the fighter's rig. A new mesh receives
// its glTF base-color texture/tint in GX form;
// a replacement changes geometry only and keeps the chosen DObj's material.
// Existing skin data on other meshes is untouched. UINT_MAX appends a DObj.
typedef struct NativeGltfImportRequest {
    unsigned primitive, joint, replacement_mesh;
    NativeGltfBindingMode binding;
} NativeGltfImportRequest;
// One import transaction: a failed primitive publishes none of the batch.
bool nativeFighterGltf_ApplyBatch(NativeFighterProject*, const char* path,
                                  const NativeGltfImportRequest*,
                                  unsigned count, float scale, char* error,
                                  size_t error_size);

// Embeds a PNG/JPEG as a GX RGBA8 texture in an existing shared material.
bool nativeFighterGltf_SetTextureImage(NativeFighterProject* project,
                                       unsigned material, unsigned layer,
                                       uint32_t sequence, unsigned frame,
                                       const char* image_path, char* error,
                                       size_t error_size);

#ifdef __cplusplus
}
#endif
#endif
#endif
