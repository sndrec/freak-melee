#pragma once
#include "native_fighter_project.h"
#include <stdio.h>

enum { NATIVE_MESH_SOURCE_CAPACITY=1024 };
typedef struct NativeFighterMeshSource {
    uint32_t mesh, vertices, corners;
    uint64_t topology;
    unsigned binding;
    float scale;
    bool used, mirrored, uses_bind_shape;
    float transform[16];
    char path[512], identity[256];
} NativeFighterMeshSource;

#ifdef __cplusplus
extern "C" {
#endif
// Mesh keys are persistent costume DObj offsets. This metadata belongs to the
// editable project; local source paths are excluded from fighter packages.
const NativeFighterMeshSource* nativeFighterMeshSource_Get(const NativeFighterProject*,uint32_t mesh);
bool nativeFighterMeshSource_Set(NativeFighterProject*,const NativeFighterMeshSource*);
void nativeFighterMeshSource_Clear(NativeFighterProject*,uint32_t mesh);
// Tagged rows in the existing Workshop metadata file. Parsing a bad row leaves
// the project unchanged; percent characters are escaped in text fields.
bool nativeFighterMeshSource_Parse(NativeFighterProject*,char* row);
bool nativeFighterMeshSource_Write(const NativeFighterProject*,FILE*);
#ifdef __cplusplus
}
#endif
