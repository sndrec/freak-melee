#pragma once
#include "../../../extern/cgltf/cgltf.h"
#include <stdint.h>

bool nativeGltfIdentity(const cgltf_data*,const cgltf_mesh*,const cgltf_primitive*,char identity[256]);
uint64_t nativeGltfTopology(const cgltf_primitive*);
bool nativeGltfSourcePath(const char*,char path[512]);
