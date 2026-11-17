#pragma once
#include "native_authoring_reply.h"
#include "native_fighter_project.h"
#include "native_fighter_core_asset_state.h"

namespace NativeModelAuthoring {
struct History {
    NativeFighterCoreAssetState before, after;
    bool labels_saved = false;
    std::vector<uint8_t> before_labels, after_labels;
};
void read(const NativeFighterProject&, yyjson_val*, NativeAuthoring::Reply&);
void scan(yyjson_val*, NativeAuthoring::Reply&);
bool edit(NativeFighterProject&, yyjson_val*, NativeAuthoring::Reply&, History* = nullptr);
const char* read_schema(bool live);
const char* edit_schema(bool live);
const char* scan_schema();
}
