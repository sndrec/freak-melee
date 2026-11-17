#include "native_shell_workshop_agent.h"
#include "native_shell_workshop_editor_internal.h"
#include "native_shell_workshop_model_internal.h"
#include "native_shell_workshop_model.h"
#include "native_shell_workshop_preview_agent.h"
#include <melee/gm/native_content_store.h>

using NativeAuthoring::Reply;
using NativeAuthoring::add_string;

namespace {
const char* value(yyjson_val* object, const char* field)
{ const char* result = yyjson_get_str(yyjson_obj_get(object, field)); return result ? result : ""; }
void append(std::string& result, const std::string& text)
{ result += std::to_string(text.size()) + ':'; result += text; }
}

namespace WorkshopEditor {
bool apply_source_history(const EditHistoryEntry& edit, bool redo)
{
    combined_script();
    const auto& expected = redo ? edit.before_sources : edit.after_sources;
    const auto& replacement = redo ? edit.after_sources : edit.before_sources;
    const auto& expected_parameters = redo ? edit.before_parameters : edit.after_parameters;
    if (expected.size() != session->script_files.files.size() || replacement.size() != expected.size() ||
        nativeShellWorkshopParameters_Text() != expected_parameters) {
        set_text("workshop-editor-status", "Source changed since this edit. Undo intervening text edits first."); return false;
    }
    for (size_t i = 0; i < expected.size(); ++i)
        if (session->script_files.files[i].text != expected[i]) {
            set_text("workshop-editor-status", "Source changed since this edit. Undo intervening text edits first."); return false;
        }
    capture_script_view();
    for (size_t i = 0; i < replacement.size(); ++i) session->script_files.files[i].text = replacement[i];
    session->parameter_source = redo ? edit.after_parameters : edit.before_parameters;
    session->script_files.parameters = session->parameter_source;
    nativeShellWorkshopParameters_SetText(document, session->parameter_source);
    static_cast<Rml::ElementFormControlTextArea*>(document->GetElementById("workshop-script-source"))
        ->SetValue(session->script_files.files[session->script_file_index].text);
    show_script_files();
    nativeShellWorkshopEditor_RefreshScriptOutline();
    restore_script_view();
    set_text("workshop-editor-status", (redo ? "Redid: " : "Undid: ") + edit.summary);
    return true;
}
}

bool nativeShellWorkshopEditor_AgentBusy()
{
    using namespace WorkshopEditor;
    return session->graph_drag_active || action_timeline_events.Active() || nativeWorkshopTransform_IsActive() ||
        nativeShellWorkshopModel_IsBusy();
}

void nativeShellWorkshopEditor_AgentWorkspace(const char* workspace)
{
    using namespace WorkshopEditor;
    if (!std::strcmp(workspace, "action")) select_mode(WorkshopMode::Action);
    else if (!std::strcmp(workspace, "scripting")) select_mode(WorkshopMode::Script);
    else if (!std::strcmp(workspace, "animation")) select_mode(WorkshopMode::Animation);
    else if (!std::strcmp(workspace, "model")) select_mode(WorkshopMode::Model);
}

bool nativeShellWorkshopEditor_AgentState(std::string& fingerprint, Reply* details)
{
    using namespace WorkshopEditor;
    if (!session->project) return true;
    show_script();
    if (!session->script_loaded) {
        if (details) details->fail("source_unavailable", session->script_files.error);
        return false;
    }
    combined_script();
    session->parameter_source = nativeShellWorkshopParameters_Text();
    fingerprint += "member:" + std::to_string(selected_member) + ";mode:" + std::to_string((int)session->mode) + ';';
    fingerprint += "source:" + std::to_string(session->script_file_index) + ';';
    NativeHomeWorkshopCamera camera{};
    if (nativeHome_WorkshopCameraRead(&camera)) fingerprint.append((const char*)&camera, sizeof(camera));
    const int active_view[] = {session->selected_action, (int) session->animation_clip.bank,
        (int) session->animation_clip.index,
        session->action_preview_playing ? -1 : session->timeline_frame,
        session->action_preview_playing ? -1 : session->animation_frame,
        session->action_preview_playing, session->show_hitboxes, session->show_hurtboxes,
        session->show_action_bones, session->show_animation_bones, session->graph_focused};
    fingerprint.append((const char*)active_view, sizeof(active_view));
    for (unsigned member = 0; member < 2; ++member) {
        auto& editor = sessions[member];
        if (!editor.project) continue;
        fingerprint += std::to_string(member) + ':' + std::to_string(editor.edit_cursor) + ':' + std::to_string(editor.edit_history.size()) + ';';
        fingerprint += std::to_string(editor.action_dirty) + std::to_string(editor.animation_dirty);
        append(fingerprint, NativeFighterSources::compose(editor.script_files));
        append(fingerprint, editor.parameter_source);
        for (const auto& file : editor.script_files.files) append(fingerprint, file.saved);
        auto& model = WorkshopModelInternal::sessions[member];
        fingerprint += std::to_string(model.change_cursor) + ':' + std::to_string(model.changes.size()) + ';';
        fingerprint.append((const char*)model.pose, sizeof(model.pose));
        WorkshopModelInternal::view_fingerprint(fingerprint,member);
    }
    if (details) {
        nativeWorkshopPreview_Describe(*details, details->result);
        const char* names[] = {"action", "scripting", "animation", "model"};
        add_string(*details, details->result, "workspace", names[(unsigned)session->mode]);
        auto source_path = session->script_files.files[session->script_file_index].path.substr(session->script_files.root.size() + 1);
        std::replace(source_path.begin(), source_path.end(), '\\', '/');
        add_string(*details, details->result, "selected_source", source_path);
        yyjson_mut_obj_add_bool(details->document, details->result, "gesture_active", nativeShellWorkshopEditor_AgentBusy());
        const auto& model = *WorkshopModelInternal::active;
        auto* model_view=yyjson_mut_obj(details->document);
        yyjson_mut_obj_add_val(details->document,details->result,"model_view",model_view);
        WorkshopModelInternal::describe_view(*details,model_view);
        bool model_mode = session->mode == WorkshopMode::Model;
        yyjson_mut_obj_add_bool(details->document, details->result, "can_undo", model_mode ? model.change_cursor > 0 : session->edit_cursor > 0);
        yyjson_mut_obj_add_bool(details->document, details->result, "can_redo", model_mode ? model.change_cursor < model.changes.size() : session->edit_cursor < session->edit_history.size());
        yyjson_mut_obj_add_int(details->document, details->result, "selected_action", session->selected_action);
        yyjson_mut_obj_add_int(details->document, details->result, "frame", session->mode == WorkshopMode::Animation ? session->animation_frame : session->mode==WorkshopMode::Model ? model.frame : session->timeline_frame);
    }
    return true;
}

void nativeShellWorkshopEditor_AgentSources(const char* requested, Reply& reply)
{
    using namespace WorkshopEditor;
    if (!session->project || !session->script_loaded) { reply.fail("no_draft", "Open a fighter draft first."); return; }
    combined_script();
    auto* files = yyjson_mut_arr(reply.document);
    if (!requested) yyjson_mut_obj_add_val(reply.document, reply.result, "files", files);
    for (const auto& file : session->script_files.files) {
        auto relative = file.path.substr(session->script_files.root.size() + 1);
        std::replace(relative.begin(), relative.end(), '\\', '/');
        if (requested) {
            if (relative != requested) continue;
            add_string(reply, reply.result, "file", relative);
            add_string(reply, reply.result, "text", file.text); return;
        }
        auto* row = yyjson_mut_obj(reply.document);
        add_string(reply, row, "file", relative); add_string(reply, row, "name", file.name);
        yyjson_mut_obj_add_uint(reply.document, row, "bytes", file.text.size());
        yyjson_mut_obj_add_bool(reply.document, row, "dirty", file.text != file.saved);
        yyjson_mut_arr_add_val(files, row);
    }
    if (requested) reply.fail("unknown_source", "Use a file returned by session.scripts.list.");
    else add_string(reply, reply.result, "parameters", nativeShellWorkshopParameters_Text());
}

void nativeShellWorkshopEditor_AgentSelectSource(const char* requested, Reply& reply)
{
    using namespace WorkshopEditor;
    std::string normalized = requested;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    for (size_t i = 0; i < session->script_files.files.size(); ++i) {
        auto path = session->script_files.files[i].path.substr(session->script_files.root.size() + 1);
        std::replace(path.begin(), path.end(), '\\', '/');
        if (path == normalized) { select_script_file(i); return; }
    }
    reply.fail("unknown_source", "Use a file returned by session.scripts.list.");
}

void nativeShellWorkshopEditor_AgentUpdateSources(yyjson_val* arguments, Reply& reply)
{
    using namespace WorkshopEditor;
    combined_script();
    session->script_files.parameters = nativeShellWorkshopParameters_Text();
    auto candidate = session->script_files;
    std::vector<NativeFighterSources::SourceEdit> edits;
    size_t index, count; yyjson_val* item;
    yyjson_arr_foreach(yyjson_obj_get(arguments, "files"), index, count, item)
        edits.push_back({value(item, "file"), value(item, "text")});
    std::string parameters = value(arguments, "parameters");
    auto staged = NativeFighterSources::edit(candidate, *session->project, edits,
        yyjson_obj_get(arguments, "parameters") ? &parameters : nullptr);
    if (!staged.ok) { reply.fail(staged.code, staged.message); return; }
    EditHistoryEntry history{};
    history.source_edit = true; history.rig_revision = session->project->rig_edit_revision;
    history.summary = value(arguments, "summary");
    history.before_parameters = session->script_files.parameters;
    history.after_parameters = candidate.parameters;
    for (const auto& file : session->script_files.files) history.before_sources.push_back(file.text);
    for (const auto& file : candidate.files) history.after_sources.push_back(file.text);
    if (history.before_sources == history.after_sources && history.before_parameters == history.after_parameters) {
        add_string(reply, reply.result, "status", "unchanged"); return;
    }
    capture_script_view();
    session->script_files = std::move(candidate);
    session->parameter_source = session->script_files.parameters;
    nativeShellWorkshopParameters_SetText(document, session->parameter_source);
    static_cast<Rml::ElementFormControlTextArea*>(document->GetElementById("workshop-script-source"))
        ->SetValue(session->script_files.files[session->script_file_index].text);
    show_script_files(); nativeShellWorkshopEditor_RefreshScriptOutline(); restore_script_view();
    session->edit_history.erase(session->edit_history.begin() + session->edit_cursor, session->edit_history.end());
    if (session->edit_history.size() == 32) session->edit_history.erase(session->edit_history.begin());
    session->edit_history.push_back(std::move(history));
    session->edit_cursor = session->edit_history.size(); update_history_buttons();
    set_text("workshop-editor-status", "Agent edit: " + std::string(value(arguments, "summary")));
    add_string(reply, reply.result, "status", "edited_draft");
    add_string(reply, reply.result, "preview_status", "refresh_required");
    yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", false);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
}

bool nativeShellWorkshopEditor_AgentUndo(bool redo, Reply& reply)
{
    using namespace WorkshopEditor;
    if (session->mode == WorkshopMode::Model) {
        if (nativeShellWorkshopModel_Undo(redo)) return true;
    } else {
        const size_t before = session->edit_cursor;
        apply_history(redo);
        if (before != session->edit_cursor) return true;
    }
    reply.fail("history_unavailable", "This history step is unavailable or conflicts with intervening edits. Inspect the Workshop before retrying.");
    return false;
}
