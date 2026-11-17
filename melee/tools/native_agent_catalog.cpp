#include "native_agent_internal.h"
#include <cmath>
#include <cstring>

namespace NativeAgent {
#define PAGE_FIELDS R"("query":{"type":"string","maxLength":256},"offset":{"type":"integer","minimum":0,"maximum":1000000},"limit":{"type":"integer","minimum":1,"maximum":200})"
#define PROJECT_FIELDS R"("project":{"type":"string","minLength":1,"maxLength":500},"member":{"type":"string","maxLength":31})"
static const Tool tools[] = {
    {"environment.describe", "Locate this installation, local drafts, build identity and available authoring tools.",
        R"({"type":"object","properties":{},"additionalProperties":false})", environment, true},
    {"projects.list", "List valid local fighter drafts. Supports case-insensitive title search and pagination.",
        "{\"type\":\"object\",\"properties\":{" PAGE_FIELDS "},\"additionalProperties\":false}", projects, true},
    {"project.inspect", "Inspect a draft's members, owned assets and script status. A project is an absolute draft folder; member is an optional member key.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", project, true},
    {"scripts.list", "List the editable fighter-state and move sources used by the Workshop, separately from validated behavior.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", sources, true},
    {"scripts.read", "Read one editable source by the relative file path returned by scripts.list.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS ",\"file\":{\"type\":\"string\",\"minLength\":1,\"maxLength\":500}},\"required\":[\"project\",\"file\"],\"additionalProperties\":false}", source, true},
    {"scripts.check", "Validate the current editable move files and owned parameters against this member's assets. Returns independent errors with file/line locations; leaves the draft and accepted behavior unchanged.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", check_scripts, true},
    {"scripts.update", "Save a batch of editable source files and optional parameter overrides. Requires the disk revision returned by inspection; refuses drafts owned by a live Workshop. Source checking and installation are separate operations. Existing files only; first save materializes the template's split files.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS
        R"(,"revision":{"type":"string","minLength":64,"maxLength":64},"files":{"type":"array","maxItems":128,"items":{"type":"object","properties":{"file":{"type":"string","minLength":1,"maxLength":400},"text":{"type":"string","maxLength":524288}},"required":["file","text"],"additionalProperties":false}},"parameters":{"type":"string","maxLength":65536}},"required":["project","revision","files"],"additionalProperties":false})", update_scripts, false},
    {"parameters.read", "Read this member's parameter override text and typed values, including values imported for a stock template. Inline defaults remain visible in the move sources. Invalid overrides are returned for repair.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", parameter_source, true},
    {"attributes.list", "Read named common fighter attributes with their types, groups and descriptions.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "," PAGE_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", attributes, true},
    {"actions.list", "List this fighter's real action IDs and names. Resolve the selected fighter's data before editing an action.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "," PAGE_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", actions, true},
    {"docs.search", "Search the installed scripting reference by filename or content; returns page IDs and short excerpts.",
        "{\"type\":\"object\",\"properties\":{" PAGE_FIELDS "},\"additionalProperties\":false}", documentation, true},
    {"docs.read", "Read a bundled reference page using its page ID from docs.search.",
        R"({"type":"object","properties":{"page":{"type":"string","minLength":1,"maxLength":128}},"required":["page"],"additionalProperties":false})", documentation_page, true},
    {"examples.list", "List bundled fighter script templates and supporting source examples. Use project inspection for an installed custom fighter's own scripts.",
        "{\"type\":\"object\",\"properties\":{" PAGE_FIELDS "},\"additionalProperties\":false}", examples, true},
    {"examples.read", "Read a bundled FMS source example by its filename from examples.list.",
        R"({"type":"object","properties":{"example":{"type":"string","minLength":1,"maxLength":128}},"required":["example"],"additionalProperties":false})", example, true},
};
#undef PROJECT_FIELDS
#undef PAGE_FIELDS

const Tool* catalog(size_t& count)
{
    count = sizeof(tools) / sizeof(*tools);
    return tools;
}

yyjson_mut_val* list_tools(yyjson_mut_doc* document)
{
    auto* result = yyjson_mut_arr(document);
    for (const auto& tool : tools) {
        auto* item = yyjson_mut_obj(document);
        yyjson_mut_obj_add_str(document, item, "name", tool.name);
        yyjson_mut_obj_add_str(document, item, "description", tool.description);
        auto* schema = yyjson_read(tool.schema, std::strlen(tool.schema), 0);
        yyjson_mut_obj_add_val(document, item, "inputSchema", yyjson_val_mut_copy(document, yyjson_doc_get_root(schema)));
        yyjson_doc_free(schema);
        auto* output = yyjson_mut_obj(document);
        yyjson_mut_obj_add_str(document, output, "type", "object");
        yyjson_mut_obj_add_val(document, item, "outputSchema", output);
        auto* annotations = yyjson_mut_obj(document);
        yyjson_mut_obj_add_bool(document, annotations, "readOnlyHint", tool.read_only);
        yyjson_mut_obj_add_bool(document, annotations, "destructiveHint", false);
        yyjson_mut_obj_add_bool(document, annotations, "openWorldHint", false);
        yyjson_mut_obj_add_val(document, item, "annotations", annotations);
        yyjson_mut_arr_add_val(result, item);
    }
    return result;
}

static bool validate_value(yyjson_val* input, yyjson_val* schema, std::string& error,
                            const std::string& path = "arguments")
{
    const char* type = yyjson_get_str(yyjson_obj_get(schema, "type"));
    bool valid = false;
    if (type && !std::strcmp(type, "object") && yyjson_is_obj(input)) {
        size_t index, count; yyjson_val *key, *value;
        auto* properties = yyjson_obj_get(schema, "properties");
        yyjson_obj_foreach(input, index, count, key, value) {
            const char* name = yyjson_get_str(key);
            auto* field = yyjson_obj_get(properties, name);
            if (!field) { error = "Unknown field: " + path + "." + name; return false; }
            if (!validate_value(value, field, error, path + "." + name)) return false;
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
                if (!validate_value(value, yyjson_obj_get(schema, "items"), error, path + "[" + std::to_string(index) + "]")) return false;
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
    if (!valid) error = "Invalid value: " + path;
    return valid;
}

static bool validate(yyjson_val* input, yyjson_val* schema, Reply& reply)
{
    std::string error;
    if (!validate_value(input, schema, error)) {
        reply.fail("invalid_arguments", error); return false;
    }
    return true;
}

bool call(const Context& context, const char* name, yyjson_val* arguments, Reply& reply)
{
    for (const auto& tool : tools) {
        if (std::strcmp(tool.name, name)) continue;
        auto* schema = yyjson_read(tool.schema, std::strlen(tool.schema), 0);
        bool valid = schema && validate(arguments, yyjson_doc_get_root(schema), reply);
        yyjson_doc_free(schema);
        if (valid) tool.handler(context, arguments, reply);
        return reply.ok;
    }
    reply.fail("unknown_tool", std::string("Unknown authoring tool: ") + name);
    return false;
}
}
