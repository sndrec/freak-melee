#include "native_shell_workshop_agent.h"
#include "native_shell_workshop_model_internal.h"
#include "native_shell_workshop_editor.h"
#include "native_shell_workshop.h"
#include <cstring>

void nativeShellWorkshopModel_Agent(const char* method, yyjson_val* args,
                                    NativeAuthoring::Reply& reply)
{
    using namespace WorkshopModelInternal;
    if (!active->project) {reply.fail("no_draft", "Open a fighter draft first."); return;}
    if (!std::strcmp(method, "read")) {NativeModelAuthoring::read(*active->project, args, reply); return;}
    if (std::strcmp(method, "edit")) {reply.fail("unknown_model_operation", "Unknown live Model operation."); return;}
    // Everything that can allocate for the history entry is reserved before
    // the shared operation publishes its privately validated candidate.
    Change entry{};
    entry.summary = yyjson_get_str(yyjson_obj_get(args, "summary"));
    entry.core = std::make_unique<CoreAssetChange>();
    rig_capture_view(entry.core->before_view);
    entry.core->after_view = entry.core->before_view;
    active->changes.reserve(64);
    auto before = std::make_unique<NativeFighterModelEditor>(active->model);
    if (!NativeModelAuthoring::edit(*active->project, args, reply, entry.core.get())) return;
    if (!yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result, "changed"))) return;
    const bool rig_changed = entry.core->before.rig_revision != entry.core->after.rig_revision;
    active->changes.erase(active->changes.begin() + active->change_cursor, active->changes.end());
    if (active->changes.size() == 64) active->changes.erase(active->changes.begin());
    active->changes.push_back(std::move(entry)); active->change_cursor = active->changes.size();
    playing = false;
    nativeShellWorkshopEditor_AgentWorkspace("model");
    reopen_model(&active->changes.back().core->after_view);
    if (rig_changed) nativeShellWorkshopEditor_RigChanged(*before, active->model);
    nativeHome_WorkshopPoseInvalidate(); buttons();
    const bool refreshing = nativeShellWorkshop_RequestPreview(false);
    yyjson_mut_obj_remove_str(reply.result, "preview_status");
    NativeAuthoring::add_string(reply, reply.result, "preview_status", refreshing ? "loading" : "refresh_required");
    status("Edited model. Save the draft to keep the changes.");
}
