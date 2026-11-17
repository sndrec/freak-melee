#pragma once
#include "native_authoring_reply.h"
#include "native_fighter_animation_editor.h"

namespace NativeAnimationAuthoring {
    void read(const NativeFighterProject&, yyjson_val*,
              NativeAuthoring::Reply&);
    bool edit(NativeFighterProject&, yyjson_val*, NativeAuthoring::Reply&);
    const char* read_schema(bool live);
    const char* edit_schema(bool live);
}
