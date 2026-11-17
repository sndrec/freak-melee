#include "native_shell_workshop_agent.h"
#include "native_shell_workshop.h"
#include "native_shell_workshop_editor.h"
#include "native_hub.h"
#include <melee/ft/native_workshop_connection.h>
#include <melee/ft/native_workshop_agent_catalog.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/gm/native_content_store.h>
#include <melee/gm/native_home.h>
#include <RmlUi/Core.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
using NativeAuthoring::Reply;
using NativeAuthoring::add_string;
NativeWorkshopConnection::Server server;
Rml::ElementDocument* document;
std::array<std::string, 16> recent;
unsigned recent_count;
bool panel_open;
std::string connection_error;

const char* string(yyjson_val* object, const char* key)
{ const char* result = yyjson_get_str(yyjson_obj_get(object, key)); return result ? result : ""; }
void text(const char* id, const std::string& value)
{ document->GetElementById(id)->SetInnerRML(Rml::StringUtilities::EncodeRml(value)); }

bool piece(std::string& fingerprint, const void* bytes, size_t size)
{
    NativeContentId id{};
    if (!size) bytes = "";
    if (!nativeContent_Hash(bytes, size, &id)) return false;
    fingerprint.append((const char*)id.bytes, sizeof(id.bytes)); return true;
}

bool describe(Reply& reply)
{
    const auto state = nativeShellWorkshop_LiveState();
    std::string fingerprint = std::to_string(state.generation) + ':' + std::to_string(state.member) + ':' +
        std::to_string(state.open) + std::to_string(state.testing) + std::to_string(state.loading) + ';';
    add_string(reply, reply.result, "instance", server.instance);
    add_string(reply, reply.result, "build_identity", NativeAuthoring::build_identity());
    add_string(reply, reply.result, "source_revision", NativeAuthoring::source_revision());
    add_string(reply, reply.result, "build", NativeAuthoring::release_label());
    yyjson_mut_obj_add_bool(reply.document, reply.result, "workshop_open", state.open);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "testing", state.testing);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "preview_loading", state.loading);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "editing_allowed", state.allowed);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "agent_access_enabled", server.enabled);
    add_string(reply, reply.result, "preview_scene", state.loading ? "loading" :
        nativeHome_IsPractice() ? "practice" : nativeHome_IsSelection() ? "character_select" : "other");
    if (state.family) {
        add_string(reply, reply.result, "project", state.family->paths[0]);
        add_string(reply, reply.result, "member", state.family->keys[state.member]);
        auto* members = yyjson_mut_arr(reply.document);
        yyjson_mut_obj_add_val(reply.document, reply.result, "members", members);
        for (unsigned i = 0; i < state.family->member_count; ++i) {
            const auto& member = state.family->members[i];
            auto* row = yyjson_mut_obj(reply.document);
            add_string(reply, row, "key", state.family->keys[i]);
            add_string(reply, row, "title", member.title);
            yyjson_mut_arr_add_val(members, row);
            bool hashed = piece(fingerprint, member.fighter_dat, member.fighter_size) &&
                piece(fingerprint, member.animation_dat, member.animation_size) &&
                piece(fingerprint, member.costume_dat, member.costume_size) &&
                piece(fingerprint, member.asset_record, member.asset_record_size) &&
                piece(fingerprint, member.resources, member.resource_size) &&
                piece(fingerprint, member.script_text, member.script_size) &&
                piece(fingerprint, member.parameter_text, member.parameter_size) &&
                piece(fingerprint, member.cpu_text, member.cpu_size);
            uint8_t* authoring = nullptr; size_t size = 0;
            hashed = hashed && nativeFighterAuthoring_Encode(&member, &authoring, &size) && piece(fingerprint, authoring, size);
            std::free(authoring);
            if (!hashed) { reply.fail("revision_unavailable", "Could not fingerprint the live fighter state."); return false; }
            fingerprint += member.title; fingerprint += '\0'; fingerprint += member.tile_name; fingerprint += '\0';
        }
        if (!nativeShellWorkshopEditor_AgentState(fingerprint, &reply)) return false;
    } else yyjson_mut_obj_add_null(reply.document, reply.result, "project");
    NativeContentId digest{}; char hex[65];
    if (!nativeContent_Hash(fingerprint.data(), fingerprint.size(), &digest)) {
        reply.fail("revision_unavailable", "Could not fingerprint the editor state."); return false;
    }
    nativeContentStore_IdHex(&digest, hex);
    add_string(reply, reply.result, "revision", hex);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "dirty", nativeShellWorkshop_LiveState().dirty);
    return true;
}

void render_panel()
{
    if (!panel_open || !document) return;
    text("workshop-agent-status", server.enabled ? "Agent access enabled" : "Agent access disconnected");
    text("workshop-agent-instance", server.enabled ? server.instance : connection_error);
    document->GetElementById("workshop-agent-enable")->SetProperty("display", server.enabled ? "none" : "block");
    document->GetElementById("workshop-agent-disable")->SetProperty("display", server.enabled ? "block" : "none");
    std::string rows;
    for (unsigned i = 0; i < std::min(recent_count, 16u); ++i)
        rows += "<div>" + Rml::StringUtilities::EncodeRml(recent[(recent_count - 1 - i) % 16]) + "</div>";
    document->GetElementById("workshop-agent-recent")->SetInnerRML(rows.empty() ? "No requests yet." : rows);
}

void handle(const char* method, yyjson_val* arguments, Reply& reply, void*)
{
    const auto* command = NativeWorkshopAgent::find(method);
    if (!command) { reply.fail("unknown_tool", "Unknown live Workshop operation."); return; }
    if (!NativeWorkshopAgent::validate(*command, arguments, reply)) return;
    // State replies and their revision are produced from the live owner rather
    // than reopening its disk files. A completed reply describes this boundary.
    if (!describe(reply)) return;
    const auto state = nativeShellWorkshop_LiveState();
    const std::string revision = yyjson_mut_get_str(yyjson_mut_obj_get(reply.result, "revision"));
    if (!command->read_only) reply.result = yyjson_mut_obj(reply.document);
    if (yyjson_obj_get(arguments, "revision") && revision != string(arguments, "revision")) {
        reply.fail("revision_conflict", "The live Workshop changed. Inspect it before editing.");
        add_string(reply, reply.result, "revision", revision);
    } else if (!command->read_only && std::strcmp(method, "session.disconnect") &&
               (!state.allowed || nativeShellWorkshopEditor_AgentBusy())) {
        reply.fail("editor_busy", "Editing is unavailable during gameplay, preview transitions or an active edit gesture.");
    } else if (!std::strcmp(method, "session.inspect")) {
        // The description above is the whole result.
    } else if (!std::strcmp(method, "session.disconnect")) {
        // Finish this reply and its acknowledgment before closing the pipe.
        server.enabled = false;
        add_string(reply, reply.result, "status", "disconnected");
    } else if (!std::strcmp(method, "session.open")) {
        if (state.dirty) reply.fail("unsaved_edits", "Save or discard the current Workshop edits before opening another draft.");
        if (reply.ok) {
            nativeHub_OpenWorkshop(string(arguments, "project"));
            const auto opened = nativeShellWorkshop_LiveState();
            char full[512];
            DWORD size = GetFullPathNameA(string(arguments, "project"), sizeof(full), full, nullptr);
            if (!opened.family || !size || size >= sizeof(full) || _stricmp(opened.family->paths[0], full))
                reply.fail("open_failed", "Could not open that draft in the Workshop. Its previous edits were preserved.");
        }
    } else if (!state.family) reply.fail("no_draft", "Open a fighter draft first.");
    else if (!std::strcmp(method, "session.workspace.select")) nativeShellWorkshopEditor_AgentWorkspace(string(arguments, "workspace"));
    else if (!std::strcmp(method, "session.member.select")) {
        unsigned member = 0;
        for (; member < state.family->member_count; ++member)
            if (!std::strcmp(state.family->keys[member], string(arguments, "member"))) break;
        if (member == state.family->member_count) reply.fail("unknown_member", "Choose a member key returned by session.inspect.");
        else if (!nativeShellWorkshop_SelectMember(member)) reply.fail("member_unavailable", "Could not open this member's editor.");
    } else if (!std::strcmp(method, "session.scripts.list")) nativeShellWorkshopEditor_AgentSources(nullptr, reply);
    else if (!std::strcmp(method, "session.scripts.read")) nativeShellWorkshopEditor_AgentSources(string(arguments, "file"), reply);
    else if (!std::strcmp(method, "session.scripts.select")) nativeShellWorkshopEditor_AgentSelectSource(string(arguments, "file"), reply);
    else if (!std::strcmp(method, "session.scripts.update")) nativeShellWorkshopEditor_AgentUpdateSources(arguments, reply);
    else if (!std::strcmp(method, "session.undo") || !std::strcmp(method, "session.redo"))
        nativeShellWorkshopEditor_AgentUndo(!std::strcmp(method, "session.redo"), reply);
    else reply.fail("unknown_tool", "Unknown live Workshop operation.");
    // Preserve operation results while refreshing their state/revision fields.
    if (reply.ok && !command->read_only) {
        yyjson_mut_obj_remove_str(reply.result, "revision");
        yyjson_mut_obj_add_bool(reply.document, reply.result, "committed", true);
        auto* state_doc = yyjson_mut_doc_new(nullptr);
        if (state_doc) {
            Reply after(state_doc);
            if (describe(after)) {
                size_t index, count; yyjson_mut_val *key, *value;
                yyjson_mut_obj_foreach(after.result, index, count, key, value) {
                    const char* name = yyjson_mut_get_str(key);
                    yyjson_mut_obj_remove_str(reply.result, name);
                    yyjson_mut_obj_add_val(reply.document, reply.result, name, yyjson_mut_val_mut_copy(reply.document, value));
                }
            } else {
                yyjson_mut_obj_add_null(reply.document, reply.result, "revision");
                add_string(reply, reply.result, "state_diagnostic", "Edit completed, but state refresh failed. Inspect the instance again.");
            }
            yyjson_mut_doc_free(state_doc);
        } else yyjson_mut_obj_add_null(reply.document, reply.result, "revision");
    }
    recent[recent_count++ % 16] = std::string(method) + (reply.ok ? " — completed" : " — failed") +
        (*string(arguments, "summary") ? " — " + std::string(string(arguments, "summary")) : "") +
        " [" + server.request_id.substr(0, 8) + ']';
    render_panel();
}
}

void nativeWorkshopAgent_Init(Rml::ElementDocument* owner)
{
    document = owner;
    NativeWorkshopConnection::start(server, handle, nullptr, connection_error);
}
extern "C" void nativeShell_UpdateAuthoring(void)
{
    NativeWorkshopConnection::poll(server);
}
void nativeWorkshopAgent_Shutdown() { NativeWorkshopConnection::shutdown(server); document = nullptr; }
bool nativeWorkshopAgent_Click(const char* id)
{
    if (!std::strcmp(id, "workshop-agent-show")) {
        panel_open = true; document->GetElementById("workshop-agent-panel")->SetProperty("display", "block"); render_panel();
    } else if (!std::strcmp(id, "workshop-agent-close")) {
        panel_open = false; document->GetElementById("workshop-agent-panel")->SetProperty("display", "none");
    } else if (!std::strcmp(id, "workshop-agent-disable")) {
        NativeWorkshopConnection::stop(server); render_panel();
    } else if (!std::strcmp(id, "workshop-agent-enable")) {
        if (server.phase == NativeWorkshopConnection::Server::Off)
            NativeWorkshopConnection::start(server, handle, nullptr, connection_error);
        render_panel();
    } else return false;
    return true;
}
