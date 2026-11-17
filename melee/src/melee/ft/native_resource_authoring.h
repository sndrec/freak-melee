#pragma once
#include "native_authoring_reply.h"
#include "native_fighter_project.h"
#include <vector>

namespace NativeResourceAuthoring {
struct History {std::vector<uint8_t> before,after;};
// Resource views borrow project bytes. Edits build and validate a private bundle
// before publishing it; callers own live history or the saved-file transaction.
void read(const NativeFighterProject&, yyjson_val*, NativeAuthoring::Reply&);
bool edit(NativeFighterProject&, yyjson_val*, NativeAuthoring::Reply&, History* = nullptr);
bool restore(NativeFighterProject&, const History&, bool redo, std::string& error);
void export_resource(const NativeFighterProject&, yyjson_val*, NativeAuthoring::Reply&);
// Standalone authoring from an explicit DAT/Article offset or a common ISO item.
void blueprint(yyjson_val*, NativeAuthoring::Reply&);
const char* read_schema(bool live);
const char* edit_schema(bool live);
const char* export_schema(bool live);
const char* blueprint_schema();
}
