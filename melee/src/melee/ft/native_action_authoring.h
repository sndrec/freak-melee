#pragma once
#include "native_action_properties.h"
namespace NativeActionAuthoring {
    void list(const NativeFighterProject&, yyjson_val*,
              NativeAuthoring::Reply&);
    void read(const NativeFighterProject&, yyjson_val*,
              NativeAuthoring::Reply&);
    // Owns an independent archive candidate. A rejected operation preserves p.
    bool edit(NativeFighterProject& p, yyjson_val*, NativeAuthoring::Reply&);
    const char* read_schema(bool live);
    const char* edit_schema(bool live);
}
