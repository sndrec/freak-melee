#include "native_shell_workshop_editor_internal.h"
#include "native_shell_workshop_model_internal.h"

void nativeShellWorkshopEditor_Resources(const char* method,yyjson_val* args,NativeAuthoring::Reply& reply)
{
    using namespace WorkshopEditor;
    if(!session->project) {reply.fail("no_draft","Open a fighter draft first.");return;}
    if(!std::strcmp(method,"read")) {NativeResourceAuthoring::read(*session->project,args,reply);return;}
    if(!std::strcmp(method,"export")) {NativeResourceAuthoring::export_resource(*session->project,args,reply);return;}
    auto history=std::make_unique<NativeResourceAuthoring::History>();
    const char* summary=yyjson_get_str(yyjson_obj_get(args,"summary"));
    const std::string label=summary?summary:"Edit fighter resources";
    const bool model_mode=session->mode==WorkshopMode::Model;
    EditHistoryEntry editor_entry{};WorkshopModelInternal::Change model_entry{};
    if(model_mode) {
        model_entry.summary=label;
        WorkshopModelInternal::active->changes.reserve(64);
    } else {
        editor_entry.summary=label;
        session->edit_history.reserve(32);
    }
    if(!NativeResourceAuthoring::edit(*session->project,args,reply,history.get()))return;
    if(yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result,"changed"))) {
        if(model_mode) {
            auto& model=*WorkshopModelInternal::active;
            model_entry.resources=std::move(history);
            model.changes.erase(model.changes.begin()+model.change_cursor,model.changes.end());
            if(model.changes.size()==64)model.changes.erase(model.changes.begin());
            model.changes.push_back(std::move(model_entry));model.change_cursor=model.changes.size();
            WorkshopModelInternal::buttons();
        } else {
            editor_entry.resources=std::move(history);
            session->edit_history.erase(session->edit_history.begin()+session->edit_cursor,session->edit_history.end());
            if(session->edit_history.size()==32)session->edit_history.erase(session->edit_history.begin());
            session->edit_history.push_back(std::move(editor_entry));session->edit_cursor=session->edit_history.size();
            update_history_buttons();
        }
    }
    NativeAuthoring::add_string(reply,reply.result,"preview_status","refresh_required");
    set_text("workshop-editor-status","Resources updated. Save to keep them; refresh preview to test them.");
}
