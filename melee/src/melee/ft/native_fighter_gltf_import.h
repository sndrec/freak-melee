#ifndef MELEE_FT_NATIVE_FIGHTER_GLTF_IMPORT_H
#define MELEE_FT_NATIVE_FIGHTER_GLTF_IMPORT_H

#ifdef MELEE_NATIVE

#include <melee/ft/native_fighter_project.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { NATIVE_GLTF_MAX_PRIMITIVES = 128 };

typedef enum NativeGltfBindingMode {
    NATIVE_GLTF_BIND_RIGID,
    NATIVE_GLTF_BIND_AUTOMATIC,
    NATIVE_GLTF_BIND_TRANSFER,
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
// Imports one triangle primitive, using rigid attachment or generated
// envelope weights on the fighter's compatible rig. A new mesh receives
// its glTF base-color texture/tint in GX form;
// a replacement changes geometry only and keeps the chosen DObj's material.
// Existing skin data on other meshes is untouched. UINT_MAX appends a DObj.
bool nativeFighterGltf_Apply(NativeFighterProject* project, const char* path,
                             unsigned primitive, unsigned joint,
                             unsigned replacement_mesh, float scale,
                             NativeGltfBindingMode binding,
                             char* error, size_t error_size);

typedef struct NativeImportedSkinVertex {
    float position[3];
    uint16_t joints[2];
    float weight;
    bool used;
} NativeImportedSkinVertex;
typedef struct NativeSkinBrushPoint { float position[3]; } NativeSkinBrushPoint;

// Returns zero for meshes outside the imported-envelope layout. When output
// is null, returns the required number of vertices. Positions are in the
// model's bind space, and each vertex has two editable influences.
unsigned nativeFighterGltf_SkinVertices(const NativeFighterProject* project,
                                        unsigned mesh,
                                        NativeImportedSkinVertex* output,
                                        unsigned capacity);
bool nativeFighterGltf_PaintWeights(NativeFighterProject* project,
                                    unsigned mesh, unsigned joint,
                                    const NativeSkinBrushPoint* centers,
                                    unsigned center_count, float radius,
                                    float strength, bool subtract,
                                    unsigned* affected,
                                    char* error, size_t error_size);

// Embeds a PNG/JPEG as a GX RGBA8 texture in an existing shared material.
bool nativeFighterGltf_SetMaterialTexture(NativeFighterProject* project,
                                          unsigned material,
                                          const char* image_path,
                                          char* error, size_t error_size);

#ifdef __cplusplus
}
#endif
#endif
#endif
