#ifndef MELEE_FT_NATIVE_FIGHTER_MODEL_EDITOR_H
#define MELEE_FT_NATIVE_FIGHTER_MODEL_EDITOR_H

#ifdef MELEE_NATIVE

#include <melee/ft/native_fighter_project.h>
#include <sysdolphin/baselib/dat.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NATIVE_MODEL_MAX_JOINTS = 512,
    NATIVE_MODEL_MAX_MESHES = 1024,
    NATIVE_MODEL_MAX_MATERIALS = 512,
};

typedef struct NativeModelJoint {
    u32 offset;
    int16_t parent;
    uint8_t depth;
    u32 flags;
    float rotation[3];
    float scale[3];
    float position[3];
    char name[64];
} NativeModelJoint;

typedef struct NativeModelMesh {
    u32 offset;
    uint16_t joint;
    uint16_t material;
    uint16_t polygon_count;
    char name[64];
} NativeModelMesh;

typedef struct NativeModelMaterial {
    u32 offset;
    u32 color_offset;
    u32 texture_offset;
    u32 render_mode;
    uint8_t ambient[4];
    uint8_t diffuse[4];
    uint8_t specular[4];
    float alpha;
    float shininess;
    char name[64];
} NativeModelMaterial;

enum { NATIVE_MODEL_MAX_ENVELOPE_INFLUENCES = 16 };

typedef struct NativeModelEnvelopeGroup {
    uint16_t polygon;
    uint8_t palette;
    uint8_t influence_count;
    uint8_t truncated;
    struct {
        uint16_t joint;
        u32 weight_offset;
        float weight;
    } influences[NATIVE_MODEL_MAX_ENVELOPE_INFLUENCES];
} NativeModelEnvelopeGroup;

typedef struct NativeFighterModelEditor {
    NativeFighterProject* project;
    HSD_Dat dat;
    NativeModelJoint joints[NATIVE_MODEL_MAX_JOINTS];
    NativeModelMesh meshes[NATIVE_MODEL_MAX_MESHES];
    NativeModelMaterial materials[NATIVE_MODEL_MAX_MATERIALS];
    uint16_t joint_count;
    uint16_t mesh_count;
    uint16_t material_count;
    char root_name[128];
} NativeFighterModelEditor;

bool nativeFighterModel_Open(NativeFighterModelEditor* editor,
                             NativeFighterProject* project);
bool nativeFighterModel_SetJoint(NativeFighterModelEditor* editor,
                                unsigned joint, unsigned channel,
                                unsigned axis, float value);
// Preserve skinned and rigid geometry at rest after changing a joint. Updates
// inverse binds and rigid vertex data in an asset transaction. Success may
// replace/resize costume_dat and reopens the editor; failure leaves the current
// asset unchanged. The caller restores its previous joint edits on failure.
bool nativeFighterModel_CompensateRest(NativeFighterModelEditor* editor,
                                      const uint8_t* previous,
                                      size_t previous_size,
                                      unsigned changed_joint);
bool nativeFighterModel_SetMaterialColor(NativeFighterModelEditor* editor,
                                        unsigned material, unsigned color,
                                        unsigned channel, uint8_t value);
bool nativeFighterModel_SetMaterialFloat(NativeFighterModelEditor* editor,
                                        unsigned material, bool shininess,
                                        float value);
bool nativeFighterModel_SetMeshMaterial(NativeFighterModelEditor* editor,
                                       unsigned mesh, unsigned material);
unsigned nativeFighterModel_EnvelopeGroups(const NativeFighterModelEditor* editor,
                                           unsigned mesh,
                                           NativeModelEnvelopeGroup* output,
                                           unsigned capacity);
bool nativeFighterModel_SetEnvelopeWeight(NativeFighterModelEditor* editor,
                                          u32 weight_offset, float weight);

#ifdef __cplusplus
}
#endif
#endif
#endif
