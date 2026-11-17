#pragma once
#include "native_authoring_reply.h"
#include "native_fighter_project.h"

namespace NativeAttributeAuthoring {
void list(const NativeFighterProject&, yyjson_val*, NativeAuthoring::Reply&);
bool edit(NativeFighterProject&, yyjson_val*, NativeAuthoring::Reply&);
const char* list_schema(bool live);
const char* edit_schema(bool live);
}
