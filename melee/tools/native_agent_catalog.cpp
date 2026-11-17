#include "native_agent_internal.h"
#include <melee/ft/native_workshop_agent_catalog.h>
#include <melee/ft/native_action_authoring.h>
#include <melee/ft/native_animation_authoring.h>
#include <cmath>
#include <cstring>
#include <iterator>

namespace NativeAgent {
#define PAGE_FIELDS R"("query":{"type":"string","maxLength":256},"offset":{"type":"integer","minimum":0,"maximum":1000000},"limit":{"type":"integer","minimum":1,"maximum":200})"
#define PROJECT_FIELDS R"("project":{"type":"string","minLength":1,"maxLength":500},"member":{"type":"string","maxLength":31})"
static const Tool offline_tools[] = {
    {"animations.read", "Inspect clips, bones and rest transforms, clip metadata, tracks, keys, bone-local poses or evaluated samples. IDs come from inspection and belong to its revision. Foreign rig bindings are identified explicitly.", NativeAnimationAuthoring::read_schema(false), read_animations, true},
    {"animations.edit", "Atomically create/duplicate an action clip, change duration, insert bone keys, change/delete/transform selected keys across bones, or copy a pose. Uses native evaluation and encoding limits; returns applied values and preserves a rejected draft.", NativeAnimationAuthoring::edit_schema(false), edit_animations, false},
    {"instances.list", "Discover local running Workshop endpoints. Each returned instance must be chosen explicitly for live requests.",
        R"({"type":"object","properties":{},"additionalProperties":false})", running_instances, true},
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
    {"project.build", "Build every member from its editable sources and owned assets. Requires the saved disk revision, collects member diagnostics, and caches a complete package. Preserves draft files and installed membership.",
        R"({"type":"object","properties":{"project":{"type":"string","minLength":1,"maxLength":500},"revision":{"type":"string","minLength":64,"maxLength":64},"portrait":{"type":"string","minLength":1,"maxLength":500},"content_root":{"type":"string","minLength":1,"maxLength":500}},"required":["project","revision"],"additionalProperties":false})", build_project, false},
    {"packages.export", "Export a verified cached package to a new file. Returns its exact content identity and byte count. Choose an existing output directory; existing files are preserved.",
        R"({"type":"object","properties":{"package_id":{"type":"string","minLength":64,"maxLength":64},"output":{"type":"string","minLength":1,"maxLength":500},"content_root":{"type":"string","minLength":1,"maxLength":500}},"required":["package_id","output"],"additionalProperties":false})", export_package, false},
    {"parameters.read", "Read this member's parameter override text and typed values, including values imported for a stock template. Inline defaults remain visible in the move sources. Invalid overrides are returned for repair.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", parameter_source, true},
    {"attributes.list", "Read named common fighter attributes with their types, groups and descriptions.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "," PAGE_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", attributes, true},
    {"actions.list", "List this fighter's real action IDs and names. Resolve the selected fighter's data before editing an action.",
        "{\"type\":\"object\",\"properties\":{" PROJECT_FIELDS "," PAGE_FIELDS "},\"required\":[\"project\"],\"additionalProperties\":false}", actions, true},
    {"actions.read", "Inspect saved action playback, reachable events, a selected event's typed fields, expanded loop occurrences or creation defaults. Choose view: metadata, events, event, timeline or types. Event handles belong to the inspected action/revision. Timers are hidden unless requested.", NativeActionAuthoring::read_schema(false), read_actions, true},
    {"actions.edit", "Atomically edit a saved action: label/visibility metadata, playback, own_hitboxes conversion, direct event creation, single or grouped event property updates, timing/duration across repetitions, or deletion. updates accepts up to 256 distinct events from the inspected action/revision; returns requested and applied properties. Refuses a live-owned draft.", NativeActionAuthoring::edit_schema(false), edit_actions, false},
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
    static const std::vector<Tool> tools = [] {
        std::vector<Tool> result(std::begin(offline_tools), std::end(offline_tools));
        size_t size;
        const auto* live = NativeWorkshopAgent::catalog(size);
        for (size_t i = 0; i < size; ++i)
            result.push_back({live[i].name, live[i].description, live[i].schema, nullptr, live[i].read_only, true});
        return result;
    }();
    count = tools.size();
    return tools.data();
}

yyjson_mut_val* list_tools(yyjson_mut_doc* document)
{
    auto* result = yyjson_mut_arr(document);
    size_t count;
    const auto* tools = catalog(count);
    for (size_t i = 0; i < count; ++i) {
        const auto& tool = tools[i];
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
        yyjson_mut_obj_add_bool(document, annotations, "destructiveHint", !std::strcmp(tool.name, "feedback.discard"));
        yyjson_mut_obj_add_bool(document, annotations, "openWorldHint", !std::strcmp(tool.name, "feedback.submit"));
        yyjson_mut_obj_add_val(document, item, "annotations", annotations);
        yyjson_mut_arr_add_val(result, item);
    }
    return result;
}

static bool validate(yyjson_val* input, yyjson_val* schema, Reply& reply)
{
    std::string error;
    if (!NativeAuthoring::validate_arguments(input, schema, error)) {
        reply.fail("invalid_arguments", error); return false;
    }
    return true;
}

bool call(const Context& context, const char* name, yyjson_val* arguments, Reply& reply)
{
    size_t count;
    const auto* tools = catalog(count);
    for (size_t i = 0; i < count; ++i) {
        const auto& tool = tools[i];
        if (std::strcmp(tool.name, name)) continue;
        auto* schema = yyjson_read(tool.schema, std::strlen(tool.schema), 0);
        if (!schema) {
            reply.fail("invalid_tool_schema", std::string("Could not load the tool schema: ") + name);
            return false;
        }
        bool valid = schema && validate(arguments, yyjson_doc_get_root(schema), reply);
        yyjson_doc_free(schema);
        if (valid) {
            if (tool.live) live_command(tool.name, arguments, reply);
            else tool.handler(context, arguments, reply);
        }
        return reply.ok;
    }
    reply.fail("unknown_tool", std::string("Unknown authoring tool: ") + name);
    return false;
}
}
