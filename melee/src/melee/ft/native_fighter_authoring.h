#ifndef NATIVE_FIGHTER_AUTHORING_H
#define NATIVE_FIGHTER_AUTHORING_H
#include "native_fighter_project.h"
#include "native_fighter_mesh_source.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {
    NativeAuthoring_Bone,
    NativeAuthoring_Mesh,
    NativeAuthoring_Material,
    NativeAuthoring_Action,
    NativeAuthoring_ActionGroup,
    NativeAuthoring_CategoryCount
};
typedef struct NativeFighterAuthoring {
    struct {
        uint32_t key;
        uint8_t category;
        bool used;
        char name[64];
    } labels[4096];
    bool hidden_actions[1024];
    NativeFighterMeshSource sources[NATIVE_MESH_SOURCE_CAPACITY];
} NativeFighterAuthoring;
const char* nativeFighterAuthoring_Name(const NativeFighterProject*,
                                        unsigned category, unsigned index);
bool nativeFighterAuthoring_SetName(NativeFighterProject*, unsigned category,
                                    unsigned index, const char*);
const char* nativeFighterAuthoring_BoneRole(const NativeFighterProject*,
                                            unsigned joint);
bool nativeFighterAuthoring_Hidden(const NativeFighterProject*,
                                   unsigned action);
bool nativeFighterAuthoring_SetHidden(NativeFighterProject*, unsigned action,
                                      bool);
bool nativeFighterAuthoring_Load(const char* path, NativeFighterProject*);
// Encoded storage belongs to the caller. Decode publishes only complete valid data.
bool nativeFighterAuthoring_Encode(const NativeFighterProject*,uint8_t** bytes,size_t* size);
bool nativeFighterAuthoring_Decode(const uint8_t* bytes,size_t size,NativeFighterProject*);
#ifdef __cplusplus
}
#endif
#endif
