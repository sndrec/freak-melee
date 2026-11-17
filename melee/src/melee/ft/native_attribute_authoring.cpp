#include "native_attribute_authoring.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace NativeAttributeAuthoring {
namespace {
const char* text(yyjson_val* object, const char* name)
{
    const char* value = yyjson_get_str(yyjson_obj_get(object, name));
    return value ? value : "";
}
bool matches(const char* value, const char* query)
{
    std::string a = value, b = query;
    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
    std::transform(a.begin(), a.end(), a.begin(), lower);
    std::transform(b.begin(), b.end(), b.begin(), lower);
    return a.find(b) != std::string::npos;
}
std::string schema(bool edit, bool live)
{
    std::string fields = live
        ? R"("instance":{"type":"string","minLength":34,"maxLength":43})"
        : R"("project":{"type":"string","minLength":1,"maxLength":500},"member":{"type":"string","maxLength":31})";
    std::string required = live ? "\"instance\"" : "\"project\"";
    if (edit) {
        fields += R"(,"revision":{"type":"string","minLength":64,"maxLength":64},"summary":{"type":"string","minLength":1,"maxLength":300},"changes":{"type":"array","minItems":1,"maxItems":97,"items":{"type":"object","properties":{"name":{"type":"string","minLength":1,"maxLength":64},"value":{"type":"number"}},"required":["name","value"],"additionalProperties":false}})";
        required += ",\"revision\",\"changes\"";
        if (live) required += ",\"summary\"";
    } else {
        fields += R"(,"query":{"type":"string","maxLength":256},"offset":{"type":"integer","minimum":0,"maximum":1000000},"limit":{"type":"integer","minimum":1,"maximum":200})";
    }
    return "{\"type\":\"object\",\"properties\":{" + fields + "},\"required\":[" + required + "],\"additionalProperties\":false}";
}
}
const char* list_schema(bool live)
{
    static const auto saved = schema(false, false), running = schema(false, true);
    return (live ? running : saved).c_str();
}
const char* edit_schema(bool live)
{
    static const auto saved = schema(true, false), running = schema(true, true);
    return (live ? running : saved).c_str();
}
void list(const NativeFighterProject& project, yyjson_val* args, NativeAuthoring::Reply& reply)
{
    auto* rows = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "attributes", rows);
    const unsigned offset = (unsigned) yyjson_get_uint(yyjson_obj_get(args, "offset"));
    const unsigned limit = yyjson_obj_get(args, "limit") ? (unsigned) yyjson_get_uint(yyjson_obj_get(args, "limit")) : 50;
    size_t total = 0;
    for (unsigned i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i) {
        const auto& field = nativeFighterCommonAttributes[i];
        const char* query = text(args, "query");
        if (!matches(field.name, query) && !matches(field.label, query) && !matches(field.description, query) && !matches(field.group, query)) continue;
        if (total++ < offset || yyjson_mut_arr_size(rows) >= limit) continue;
        double value;
        if (!nativeFighterProject_GetAttribute(&project, i, &value)) {
            reply.fail("invalid_attribute", std::string("Could not read attribute: ") + field.name); return;
        }
        auto* row = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_append(rows, row);
        using NativeAuthoring::add_string;
        add_string(reply, row, "name", field.name);
        add_string(reply, row, "label", field.label);
        add_string(reply, row, "description", field.description);
        add_string(reply, row, "group", field.group);
        const char* types[] = {"float", "integer", "flags"};
        add_string(reply, row, "type", types[field.type]);
        yyjson_mut_obj_add_real(reply.document, row, "value", value);
        NativeFighterAttributeLimits limits = nativeFighterAttribute_Limits(i);
        yyjson_mut_obj_add_real(reply.document, row, "minimum", limits.minimum);
        yyjson_mut_obj_add_real(reply.document, row, "maximum", limits.maximum);
        yyjson_mut_obj_add_bool(reply.document, row, "exclusive_minimum", limits.exclusive_minimum);
    }
    yyjson_mut_obj_add_uint(reply.document, reply.result, "total", total);
    if (offset + limit < total) yyjson_mut_obj_add_uint(reply.document, reply.result, "next_offset", offset + limit);
    else yyjson_mut_obj_add_null(reply.document, reply.result, "next_offset");
}
bool edit(NativeFighterProject& project, yyjson_val* args, NativeAuthoring::Reply& reply)
{
    std::vector<uint8_t> bytes(project.fighter_dat, project.fighter_dat + project.fighter_size);
    NativeFighterProject candidate = project;
    candidate.fighter_dat = bytes.data();
    bool selected[NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT]{};
    auto* applied = yyjson_mut_arr(reply.document);
    size_t index, count; yyjson_val* change;
    yyjson_arr_foreach(yyjson_obj_get(args, "changes"), index, count, change) {
        const char* name = text(change, "name");
        unsigned field = 0;
        for (; field < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++field)
            if (!std::strcmp(nativeFighterCommonAttributes[field].name, name)) break;
        if (field == NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT) {
            reply.fail("unknown_attribute", std::string("Choose a name returned by attributes.list: ") + name); return false;
        }
        if (selected[field]) {
            reply.fail("duplicate_attribute", std::string("Supply each attribute once: ") + name); return false;
        }
        selected[field] = true;
        const double requested = yyjson_get_num(yyjson_obj_get(change, "value"));
        double value;
        if (!nativeFighterProject_SetAttribute(&candidate, field, requested) ||
            !nativeFighterProject_GetAttribute(&candidate, field, &value)) {
            reply.fail("attribute_edit_rejected", std::string("The value exceeds the attribute's type or valid range: ") + name); return false;
        }
        auto* row = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_append(applied, row);
        NativeAuthoring::add_string(reply, row, "name", name);
        yyjson_mut_obj_add_real(reply.document, row, "requested", requested);
        yyjson_mut_obj_add_real(reply.document, row, "applied", value);
    }
    const bool changed = std::memcmp(bytes.data(), project.fighter_dat, bytes.size()) != 0;
    if (changed) std::memcpy(project.fighter_dat, bytes.data(), bytes.size());
    yyjson_mut_obj_add_val(reply.document, reply.result, "changes", applied);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "changed", changed);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", false);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
    return true;
}
}
