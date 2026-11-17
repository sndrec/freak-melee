#pragma once
#include <yyjson.h>
#include <string>

namespace NativeAuthoring {
struct Reply {
    yyjson_mut_doc* document;
    yyjson_mut_val* result;
    bool ok = true;
    explicit Reply(yyjson_mut_doc* owner);
    void fail(const char* code, const std::string& message);
};
void add_string(Reply&, yyjson_mut_val*, const char*, const std::string&);
bool validate_arguments(yyjson_val*, yyjson_val* schema, std::string& error,
                        const std::string& path = "arguments");
const char* build_identity();
const char* source_revision();
const char* release_label();
}
