#pragma once
#include "native_model_authoring.h"
#include "native_fighter_authoring.h"
#include "native_fighter_core_asset_state.h"
#include "native_fighter_gltf_import.h"
#include "native_fighter_material_render.h"
#include "native_fighter_material_animation.h"
#include "native_fighter_model_editor.h"
#include "native_fighter_rig.h"
#include "native_fighter_rig_edit.h"
#include "native_fighter_skin_edit.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace NativeModelAuthoring {
using NativeAuthoring::Reply;
using NativeAuthoring::add_string;
inline const char* text(yyjson_val* o, const char* key) {const char* s = yyjson_get_str(yyjson_obj_get(o, key)); return s ? s : "";}
inline bool has(yyjson_val* o, const char* key) {return yyjson_obj_get(o, key) != nullptr;}
inline unsigned number(yyjson_val* o, const char* key) {return (unsigned)yyjson_get_uint(yyjson_obj_get(o, key));}
inline double real(yyjson_val* o, const char* key, double fallback = 0) {return has(o, key) ? yyjson_get_num(yyjson_obj_get(o, key)) : fallback;}
inline bool boolean(yyjson_val* o, const char* key) {return yyjson_get_bool(yyjson_obj_get(o, key));}
void vector(Reply&, yyjson_mut_val*, const char* key, const float*, unsigned count = 3);
bool matches(const char*, const char*);
void page(Reply&, size_t total, unsigned offset, unsigned limit);
bool open(const NativeFighterProject&, std::unique_ptr<NativeFighterModelEditor>&, Reply&);
bool bone_ids(const NativeFighterProject&, unsigned count, std::vector<int32_t>&, std::vector<uint8_t>&, Reply&);
const char* binding_name(unsigned);
NativeGltfBindingMode binding(yyjson_val*);
bool encode_names(const NativeFighterProject&, std::vector<uint8_t>&, Reply&);
bool sequences(const NativeFighterModelEditor&, unsigned, std::vector<NativeMaterialTextureSequence>&, Reply&);
void material_properties(const NativeFighterModelEditor&, unsigned, Reply&, yyjson_mut_val*);
bool edit_material(NativeFighterProject&, unsigned, yyjson_val*, Reply&);
const char* material_schema();
}
