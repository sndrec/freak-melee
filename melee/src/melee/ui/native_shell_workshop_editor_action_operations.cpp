#include "native_shell_workshop_editor_internal.h"
#include <melee/ft/native_action_authoring.h>

void nativeShellWorkshopEditor_Actions(const char* method, yyjson_val* args,
                                       NativeAuthoring::Reply& reply)
{
    using namespace WorkshopEditor;
    if (!session->project) {
        reply.fail("no_draft", "Open a fighter draft first.");
        return;
    }
    if (!std::strcmp(method, "list")) {
        NativeActionAuthoring::list(*session->project, args, reply);
        return;
    }
    if (!std::strcmp(method, "read")) {
        NativeActionAuthoring::read(*session->project, args, reply);
        return;
    }
    unsigned action =
        (unsigned) yyjson_get_uint(yyjson_obj_get(args, "action_id"));
    if (action >= session->action_count) {
        reply.fail("unknown_action",
                   "Choose an action_id from this member's action list.");
        return;
    }
    const int frame =
        session->selected_action == (int) action ? session->timeline_frame : 0;
    if (!std::strcmp(method, "edit")) {
        const bool metadata = !std::strcmp(
            yyjson_get_str(yyjson_obj_get(args, "operation")), "metadata");
        EditHistoryEntry label{};
        if (metadata) {
            label.label_edit = true;
            label.label_action = action;
            label.rig_revision = session->project->rig_edit_revision;
            const char* previous = nativeFighterAuthoring_Name(
                session->project, NativeAuthoring_Action, action);
            label.before_name = previous ? previous : "";
            label.before_hidden =
                nativeFighterAuthoring_Hidden(session->project, action);
        }
        const size_t size = session->project->fighter_size;
        std::vector<uint8_t> before(session->project->fighter_dat,
                                    session->project->fighter_dat + size);
        if (!NativeActionAuthoring::edit(*session->project, args, reply)) {
            return;
        }
        if (yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result, "changed"))) {
            if (metadata) {
                const char* current = nativeFighterAuthoring_Name(
                    session->project, NativeAuthoring_Action, action);
                label.after_name = current ? current : "";
                label.after_hidden =
                    nativeFighterAuthoring_Hidden(session->project, action);
                session->edit_history.erase(session->edit_history.begin() +
                                                session->edit_cursor,
                                            session->edit_history.end());
                if (session->edit_history.size() == 32) {
                    session->edit_history.erase(session->edit_history.begin());
                }
                session->edit_history.push_back(std::move(label));
                session->edit_cursor = session->edit_history.size();
            } else {
                record_structural_edit(false, before.data(), size);
            }
            session->action_dirty = true;
            const char* summary =
                yyjson_get_str(yyjson_obj_get(args, "summary"));
            if (summary && !session->edit_history.empty()) {
                session->edit_history.back().summary = summary;
            }
            update_history_buttons();
        }
    }
    select_mode(WorkshopMode::Action);
    select_action(action);
    nativeShellWorkshopEditor_FilterActions();
    nativeShellWorkshopEditor_FilterClips();
    session->timeline_frame =
        std::clamp(yyjson_obj_get(args, "frame")
                       ? (int) yyjson_get_uint(yyjson_obj_get(args, "frame"))
                       : frame,
                   0, session->timeline_end_frame);
    field("workshop-current-frame")->SetValue(number(session->timeline_frame));
    update_timeline_cursor();
    const char* wanted =
        yyjson_mut_get_str(yyjson_mut_obj_get(reply.result, "event_id"));
    if (wanted) {
        for (size_t i = 0; i < session->command_count; ++i) {
            char id[24];
            std::snprintf(id, sizeof(id), "event-%08x",
                          session->commands[i].source_offset);
            if (!std::strcmp(id, wanted)) {
                select_command((unsigned) i);
                break;
            }
        }
    }
    nativeHome_WorkshopPoseInvalidate();
    show_action_timeline();
    set_text("workshop-editor-status",
             wanted ? "Action event changed in the draft."
             : yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result, "changed"))
                 ? "Action updated."
                 : "Action selected.");
}

namespace WorkshopEditor {
    void create_action_event(const char* kind, const char* function)
    {
        if (!session->project || session->selected_action < 0) {
            return;
        }
        auto* doc = yyjson_mut_doc_new(nullptr);
        if (!doc) {
            return;
        }
        auto* args = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, args);
        yyjson_mut_obj_add_uint(doc, args, "action_id",
                                (unsigned) session->selected_action);
        yyjson_mut_obj_add_uint(doc, args, "frame",
                                (unsigned) session->timeline_frame);
        yyjson_mut_obj_add_str(doc, args, "operation", "create");
        yyjson_mut_obj_add_strcpy(doc, args, "kind", kind);
        yyjson_mut_obj_add_strcpy(doc, args, "summary",
                                  (std::string("Add ") + kind).c_str());
        if (function) {
            auto* props = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_strcpy(doc, props, "function", function);
            yyjson_mut_obj_add_val(doc, args, "properties", props);
        }
        auto* input = yyjson_mut_doc_imut_copy(doc, nullptr);
        NativeAuthoring::Reply reply(doc);
        if (input) {
            nativeShellWorkshopEditor_Actions(
                "edit", yyjson_doc_get_root(input), reply);
        }
        if (!reply.ok) {
            const char* error = yyjson_mut_get_str(
                yyjson_mut_obj_get(reply.result, "message"));
            set_text("workshop-editor-status",
                     error ? error : "Could not create this event.");
        }
        yyjson_doc_free(input);
        yyjson_mut_doc_free(doc);
    }
}
