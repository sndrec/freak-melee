#include "native_authoring_reply.h"
#include <cmath>
#include <cstring>

#ifndef NATIVE_AUTHORING_BUILD_ID
#error Authoring tools need a shared build identity.
#endif

namespace NativeAuthoring {
Reply::Reply(yyjson_mut_doc* owner) : document(owner), result(yyjson_mut_obj(owner)) {}
void Reply::fail(const char* code, const std::string& message)
{
    ok = false;
    result = yyjson_mut_obj(document);
    yyjson_mut_obj_add_strcpy(document, result, "code", code);
    yyjson_mut_obj_add_strcpy(document, result, "message", message.c_str());
}
void add_string(Reply& reply, yyjson_mut_val* object, const char* key, const std::string& value)
{
    yyjson_mut_obj_add_strcpy(reply.document, object, key, value.c_str());
}
const char* build_identity() { return NATIVE_AUTHORING_BUILD_ID; }
const char* source_revision() { return NATIVE_AUTHORING_SOURCE_REVISION; }
const char* release_label() { return NATIVE_AUTHORING_RELEASE; }

bool validate_arguments(yyjson_val* input, yyjson_val* schema, std::string& error, const std::string& path)
{
    const char* type = yyjson_get_str(yyjson_obj_get(schema, "type"));
    bool valid = false;
    if (type && !std::strcmp(type, "object") && yyjson_is_obj(input)) {
        size_t index, count; yyjson_val *key, *value;
        auto* properties = yyjson_obj_get(schema, "properties");
        if (yyjson_obj_size(input) > yyjson_obj_size(properties)) {
            error = "Too many fields in " + path; return false;
        }
        yyjson_obj_foreach(input, index, count, key, value) {
            const char* name = yyjson_get_str(key);
            size_t length = yyjson_get_len(key);
            auto* field = yyjson_obj_getn(properties, name, length);
            if (!field) {
                error = length <= 128 && std::strlen(name) == length ?
                    "Unknown field: " + path + "." + name : "Unknown field in " + path;
                return false;
            }
            if (yyjson_obj_getn(input, name, length) != value) {
                error = "Repeated field: " + path + "." + name; return false;
            }
            if (!validate_arguments(value, field, error, path + "." + name)) return false;
        }
        yyjson_arr_foreach(yyjson_obj_get(schema, "required"), index, count, value) {
            const char* name = yyjson_get_str(value);
            if (!yyjson_obj_get(input, name)) { error = "Missing field: " + path + "." + name; return false; }
        }
        valid = true;
    } else if (type && !std::strcmp(type, "array") && yyjson_is_arr(input)) {
        auto* minimum = yyjson_obj_get(schema, "minItems");
        auto* maximum = yyjson_obj_get(schema, "maxItems");
        size_t length = yyjson_arr_size(input);
        valid = (!minimum || length >= yyjson_get_uint(minimum)) && (!maximum || length <= yyjson_get_uint(maximum));
        if (valid) {
            size_t index, count; yyjson_val* value;
            yyjson_arr_foreach(input, index, count, value)
                if (!validate_arguments(value, yyjson_obj_get(schema, "items"), error, path + "[" + std::to_string(index) + "]")) return false;
        }
    } else if (type && !std::strcmp(type, "string") && yyjson_is_str(input)) {
        size_t size = yyjson_get_len(input);
        auto* minimum = yyjson_obj_get(schema, "minLength");
        auto* maximum = yyjson_obj_get(schema, "maxLength");
        valid = std::strlen(yyjson_get_str(input)) == size &&
            (!minimum || size >= yyjson_get_uint(minimum)) && (!maximum || size <= yyjson_get_uint(maximum));
    } else if (type && ((!std::strcmp(type, "integer") && yyjson_is_int(input)) ||
                        (!std::strcmp(type, "number") && yyjson_is_num(input)))) {
        double number = yyjson_get_num(input);
        auto* minimum = yyjson_obj_get(schema, "minimum");
        auto* maximum = yyjson_obj_get(schema, "maximum");
        valid = std::isfinite(number) && (!minimum || number >= yyjson_get_num(minimum)) && (!maximum || number <= yyjson_get_num(maximum));
    } else if (type && !std::strcmp(type, "boolean")) valid = yyjson_is_bool(input);
    if (valid && yyjson_obj_get(schema, "enum")) {
        size_t index, count; yyjson_val* option;
        valid = false;
        yyjson_arr_foreach(yyjson_obj_get(schema, "enum"), index, count, option)
            if (yyjson_equals(input, option)) { valid = true; break; }
    }
    if (!valid) error = "Invalid value: " + path;
    return valid;
}
}
