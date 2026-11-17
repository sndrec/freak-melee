#include "native_agent_internal.h"
#include "native_agent_project.h"
#include <melee/ft/native_fighter_family_project.h>
#include <melee/ft/native_fighter_sources.h>
#include <melee/ft/native_fighter_attributes.h>
#include <melee/ft/native_fighter_action_editor.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_behavior_source.h>
#include <melee/gm/native_content_store.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <memory>

#ifndef NATIVE_AGENT_SOURCE_REVISION
#define NATIVE_AGENT_SOURCE_REVISION "unknown"
#endif
#ifndef NATIVE_NETPLAY_DIAGNOSTICS_BUILD
#define NATIVE_NETPLAY_DIAGNOSTICS_BUILD "development"
#endif

namespace NativeAgent {
static yyjson_mut_val* item(Reply& reply, yyjson_mut_val* array)
{
    auto* result = yyjson_mut_obj(reply.document);
    yyjson_mut_arr_add_val(array, result);
    return result;
}

static bool directory(const std::string& path)
{
    DWORD flags = GetFileAttributesA(path.c_str());
    return flags != INVALID_FILE_ATTRIBUTES && (flags & FILE_ATTRIBUTE_DIRECTORY);
}

static void pagination(Reply& reply, size_t total, unsigned offset, unsigned limit)
{
    yyjson_mut_obj_add_uint(reply.document, reply.result, "total", total);
    if (total > (size_t)offset + limit)
        yyjson_mut_obj_add_uint(reply.document, reply.result, "next_offset", offset + limit);
    else yyjson_mut_obj_add_null(reply.document, reply.result, "next_offset");
}

void environment(const Context& context, yyjson_val*, Reply& reply)
{
    add_string(reply, reply.result, "installation", context.installation);
    add_string(reply, reply.result, "content_root", context.content_root);
    add_string(reply, reply.result, "projects_root", context.projects_root);
    add_string(reply, reply.result, "build", NATIVE_NETPLAY_DIAGNOSTICS_BUILD);
    add_string(reply, reply.result, "source_revision", NATIVE_AGENT_SOURCE_REVISION);
    add_string(reply, reply.result, "authoring_build_identity", NativeAuthoring::build_identity());
    yyjson_mut_obj_add_bool(reply.document, reply.result, "documentation_available", directory(context.installation + "\\ui\\docs"));
    yyjson_mut_obj_add_uint(reply.document, reply.result, "script_source_capacity_bytes", NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "family_member_capacity", NATIVE_FIGHTER_FAMILY_MEMBERS);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "live_session_tools_available", true);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "edit_tools_available", true);
    size_t count;
    const auto* tools = catalog(count);
    auto* names = yyjson_mut_arr(reply.document);
    for (size_t i = 0; i < count; ++i) yyjson_mut_arr_add_str(reply.document, names, tools[i].name);
    yyjson_mut_obj_add_val(reply.document, reply.result, "tools", names);
    std::string iso;
    std::string user_root = context.projects_root.substr(0, context.projects_root.find_last_of("\\/"));
    if (read_text(user_root + "\\iso-path.txt", iso, 4096)) {
        while (!iso.empty() && (iso.back() == '\n' || iso.back() == '\r')) iso.pop_back();
        add_string(reply, reply.result, "configured_iso", iso);
        DWORD flags = GetFileAttributesA(iso.c_str());
        yyjson_mut_obj_add_bool(reply.document, reply.result, "configured_iso_available", flags != INVALID_FILE_ATTRIBUTES && !(flags & FILE_ATTRIBUTE_DIRECTORY));
    } else yyjson_mut_obj_add_null(reply.document, reply.result, "configured_iso");
}

void projects(const Context&, yyjson_val* arguments, Reply& reply)
{
    size_t count = 0;
    if (!nativeFighterFamilyProject_ListLocal(nullptr, 0, &count)) {
        reply.fail("projects_unavailable", "Could not enumerate local fighter drafts."); return;
    }
    std::vector<NativeFighterDraft> drafts(count);
    if (!nativeFighterFamilyProject_ListLocal(drafts.data(), drafts.size(), &count)) {
        reply.fail("projects_changed", "Drafts changed during enumeration; retry the request."); return;
    }
    auto* array = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "projects", array);
    unsigned offset = page_offset(arguments), limit = page_limit(arguments);
    size_t matching = 0;
    for (const auto& draft : drafts) {
        if (!matches(draft.title, string(arguments, "query"))) continue;
        if (matching++ < offset || yyjson_mut_arr_size(array) >= limit) continue;
        auto* object = item(reply, array);
        add_string(reply, object, "title", draft.title);
        add_string(reply, object, "project", draft.path);
    }
    pagination(reply, matching, offset, limit);
}

void project(const Context&, yyjson_val* arguments, Reply& reply)
{
    auto project = open_project(arguments, reply);
    if (!project) return;
    auto* array = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "members", array);
    for (unsigned i = 0; i < project->family.member_count; ++i) {
        const auto& member = project->family.members[i];
        auto* object = item(reply, array);
        add_string(reply, object, "key", project->family.keys[i]);
        add_string(reply, object, "path", project->family.paths[i]);
        add_string(reply, object, "title", member.title);
        yyjson_mut_obj_add_uint(reply.document, object, "actions", member.action_count);
        yyjson_mut_obj_add_uint(reply.document, object, "demos", member.demo_count);
        yyjson_mut_obj_add_uint(reply.document, object, "fighter_bytes", member.fighter_size);
        yyjson_mut_obj_add_uint(reply.document, object, "animation_bytes", member.animation_size);
        yyjson_mut_obj_add_uint(reply.document, object, "costume_bytes", member.costume_size);
        yyjson_mut_obj_add_uint(reply.document, object, "resource_bytes", member.resource_size);
        yyjson_mut_obj_add_bool(reply.document, object, "has_validated_source", member.script_size != 0);
    }
    const char* modes[] = {"single", "alternate_form", "simultaneous_companion"};
    unsigned mode = (unsigned)project->family.mode;
    add_string(reply, reply.result, "family_mode", mode < 3 ? modes[mode] : "unknown");
    NativeFighterSources::Session files;
    if (load_sources(*project, files, reply)) source_status(files, project->family.members[project->member], reply);
}

void sources(const Context&, yyjson_val* arguments, Reply& reply)
{
    auto project = open_project(arguments, reply);
    NativeFighterSources::Session files;
    if (!project || !load_sources(*project, files, reply)) return;
    if (!source_status(files, project->family.members[project->member], reply)) return;
    auto* array = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "files", array);
    for (const auto& file : files.files) {
        auto* object = item(reply, array);
        add_string(reply, object, "name", file.name);
        add_string(reply, object, "file", file.path.substr(files.root.size() + 1));
        add_string(reply, object, "path", file.path);
        yyjson_mut_obj_add_uint(reply.document, object, "bytes", file.text.size());
        yyjson_mut_obj_add_bool(reply.document, object, "saved_as_separate_file", files.disk_sources);
    }
}

void source(const Context&, yyjson_val* arguments, Reply& reply)
{
    auto project = open_project(arguments, reply);
    NativeFighterSources::Session files;
    if (!project || !load_sources(*project, files, reply)) return;
    const char* requested = string(arguments, "file");
    for (const auto& file : files.files) {
        std::string relative = file.path.substr(files.root.size() + 1);
        std::replace(relative.begin(), relative.end(), '\\', '/');
        std::string normalized(requested);
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        if (relative != normalized) continue;
        add_string(reply, reply.result, "file", relative);
        add_string(reply, reply.result, "text", file.text);
        source_status(files, project->family.members[project->member], reply);
        return;
    }
    reply.fail("unknown_source", "Choose a file returned by scripts.list.");
}

void attributes(const Context&, yyjson_val* arguments, Reply& reply)
{
    auto project = open_project(arguments, reply);
    if (!project) return;
    auto* array = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "attributes", array);
    unsigned offset = page_offset(arguments), limit = page_limit(arguments);
    size_t matching = 0;
    for (unsigned i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i) {
        const auto& field = nativeFighterCommonAttributes[i];
        std::string query = string(arguments, "query");
        if (!matches(field.name, query) && !matches(field.label, query) && !matches(field.description, query)) continue;
        if (matching++ < offset || yyjson_mut_arr_size(array) >= limit) continue;
        double value;
        if (!nativeFighterProject_GetAttribute(&project->family.members[project->member], i, &value)) {
            reply.fail("invalid_attribute", std::string("Could not read attribute: ") + field.name); return;
        }
        auto* object = item(reply, array);
        add_string(reply, object, "name", field.name);
        add_string(reply, object, "label", field.label);
        add_string(reply, object, "description", field.description);
        add_string(reply, object, "group", field.group);
        const char* types[] = {"float", "integer", "flags"};
        add_string(reply, object, "type", types[field.type]);
        yyjson_mut_obj_add_real(reply.document, object, "value", value);
    }
    pagination(reply, matching, offset, limit);
}

void actions(const Context&, yyjson_val* arguments, Reply& reply)
{
    auto project = open_project(arguments, reply);
    if (!project) return;
    size_t count = 0;
    const auto* member = &project->family.members[project->member];
    if (!nativeFighterAction_List(member, nullptr, 0, &count)) {
        reply.fail("invalid_actions", "Could not inspect this fighter's action table."); return;
    }
    std::vector<NativeFighterActionInfo> actions(count);
    if (!nativeFighterAction_List(member, actions.data(), actions.size(), &count)) {
        reply.fail("invalid_actions", "Could not read this fighter's action table."); return;
    }
    auto* array = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "actions", array);
    unsigned offset = page_offset(arguments), limit = page_limit(arguments);
    size_t matching = 0;
    for (const auto& action : actions) {
        const char* label = nativeFighterAuthoring_Name(member, NativeAuthoring_Action, action.index);
        const std::string query = string(arguments, "query");
        if (!matches(action.name, query) && !(label && matches(label, query))) continue;
        if (matching++ < offset || yyjson_mut_arr_size(array) >= limit) continue;
        auto* object = item(reply, array);
        yyjson_mut_obj_add_uint(reply.document, object, "id", action.index);
        add_string(reply, object, "name", action.name);
        if (label) add_string(reply, object, "label", label);
        yyjson_mut_obj_add_bool(reply.document, object, "has_events", action.has_script);
    }
    pagination(reply, matching, offset, limit);
}

std::vector<std::string> documentation_pages(const Context& context)
{
    std::vector<std::string> names;
    WIN32_FIND_DATAA file;
    HANDLE search = FindFirstFileA((context.installation + "\\ui\\docs\\*.md").c_str(), &file);
    if (search == INVALID_HANDLE_VALUE) return names;
    do {
        if (!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) names.emplace_back(file.cFileName);
    } while (FindNextFileA(search, &file));
    FindClose(search);
    std::sort(names.begin(), names.end());
    return names;
}

void documentation(const Context& context, yyjson_val* arguments, Reply& reply)
{
    auto* array = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "pages", array);
    unsigned offset = page_offset(arguments), limit = page_limit(arguments);
    size_t matching = 0;
    std::string query = string(arguments, "query");
    for (const auto& page : documentation_pages(context)) {
        std::string text;
        if (!read_text(context.installation + "\\ui\\docs\\" + page, text, 1024 * 1024)) continue;
        if (!matches(page, query) && !matches(text, query)) continue;
        if (matching++ < offset || yyjson_mut_arr_size(array) >= limit) continue;
        auto* object = item(reply, array);
        add_string(reply, object, "page", page);
        add_string(reply, object, "uri", "freak-melee://docs/" + page);
        size_t start = 0;
        if (!query.empty()) {
            auto at = std::search(text.begin(), text.end(), query.begin(), query.end(),
                [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
            if (at != text.end()) start = (size_t)(at - text.begin());
        }
        add_string(reply, object, "excerpt", text.substr(start, 320));
    }
    pagination(reply, matching, offset, limit);
}

void documentation_page(const Context& context, yyjson_val* arguments, Reply& reply)
{
    const std::string page(string(arguments, "page"));
    std::string text;
    if (read_document(context, page, text)) {
        add_string(reply, reply.result, "page", page);
        add_string(reply, reply.result, "uri", "freak-melee://docs/" + page);
        add_string(reply, reply.result, "text", text);
        return;
    }
    reply.fail("unknown_document", "Choose a bundled page returned by docs.search.");
}

bool read_document(const Context& context, const std::string& page, std::string& text)
{
    for (const auto& candidate : documentation_pages(context))
        if (page == candidate)
            return read_text(context.installation + "\\ui\\docs\\" + page, text, 1024 * 1024);
    return false;
}
}
