#pragma once
#include "native_authoring_reply.h"
#include <cstddef>
namespace NativeWorkshopAgent {
struct Command {
    const char* name;
    const char* description;
    const char* schema;
    bool read_only;
    const char* output_schema = nullptr;
};
const Command* catalog(size_t& count);
const Command* find(const char* name);
bool validate(const Command&, yyjson_val*, NativeAuthoring::Reply&);
}
