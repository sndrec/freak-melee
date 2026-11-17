#include "native_workshop_agent_catalog.h"
#include <cstring>
namespace NativeWorkshopAgent {
#define INSTANCE R"("instance":{"type":"string","minLength":34,"maxLength":43})"
#define REVISION R"("revision":{"type":"string","minLength":64,"maxLength":64})"
static const Command commands[] = {
    {"session.inspect", "Inspect the running Workshop, selected member, live revision, unsaved state and undo availability.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "},\"required\":[\"instance\"],\"additionalProperties\":false}", true},
    {"session.open", "Open a saved draft in this instance's Workshop. Requires the current live revision and refuses to discard unsaved edits or interrupt an online match.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"project":{"type":"string","minLength":1,"maxLength":500}},"required":["instance","revision","project"],"additionalProperties":false})", false},
    {"session.member.select", "Select the family member to inspect or edit. Returns the new live view and revision.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"member":{"type":"string","minLength":1,"maxLength":31}},"required":["instance","revision","member"],"additionalProperties":false})", false},
    {"session.workspace.select", "Select the Action, Scripting, Animation or Model workspace in the current editor.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"workspace":{"type":"string","enum":["action","scripting","animation","model"]}},"required":["instance","revision","workspace"],"additionalProperties":false})", false},
    {"session.scripts.list", "List the live member's editable source files and parameters, including unsaved Workshop changes.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "},\"required\":[\"instance\"],\"additionalProperties\":false}", true},
    {"session.scripts.read", "Read one live editable move or fighter-state file by its relative path from session.scripts.list.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE R"(,"file":{"type":"string","minLength":1,"maxLength":400}},"required":["instance","file"],"additionalProperties":false})", true},
    {"session.scripts.select", "Show one editable source file in the Workshop's Scripting pane. Resolve the file through session.scripts.list first.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION R"(,"file":{"type":"string","minLength":1,"maxLength":400}},"required":["instance","revision","file"],"additionalProperties":false})", false},
    {"session.scripts.update", "Change live source files and optional parameter text as one Workshop undo step. Does not save, validate, install or restart preview.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION
        R"(,"summary":{"type":"string","minLength":1,"maxLength":200},"files":{"type":"array","maxItems":128,"items":{"type":"object","properties":{"file":{"type":"string","minLength":1,"maxLength":400},"text":{"type":"string","maxLength":524288}},"required":["file","text"],"additionalProperties":false}},"parameters":{"type":"string","maxLength":65536}},"required":["instance","revision","summary","files"],"additionalProperties":false})", false},
    {"session.undo", "Undo one edit in the current Workshop member and workspace. Requires the inspected live revision.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.redo", "Redo one edit in the current Workshop member and workspace. Requires the inspected live revision.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "," REVISION "},\"required\":[\"instance\",\"revision\"],\"additionalProperties\":false}", false},
    {"session.disconnect", "Disconnect agent access to this running instance. Leaves its Workshop draft open and preserves edits. Access can be enabled again in Agent tools.",
        "{\"type\":\"object\",\"properties\":{" INSTANCE "},\"required\":[\"instance\"],\"additionalProperties\":false}", false},
};
#undef INSTANCE
#undef REVISION
const Command* catalog(size_t& count) { count = sizeof(commands) / sizeof(*commands); return commands; }
const Command* find(const char* name)
{
    for (const auto& command : commands) if (!std::strcmp(name, command.name)) return &command;
    return nullptr;
}
bool validate(const Command& command, yyjson_val* arguments, NativeAuthoring::Reply& reply)
{
    auto* schema = yyjson_read(command.schema, std::strlen(command.schema), 0);
    std::string error;
    bool ok = schema && NativeAuthoring::validate_arguments(arguments, yyjson_doc_get_root(schema), error);
    yyjson_doc_free(schema);
    if (!ok) reply.fail("invalid_arguments", error.empty() ? "Could not read the operation schema." : error);
    return ok;
}
}
