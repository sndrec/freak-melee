#include "native_shell_workshop_agent.h"
#include "native_shell_workshop_model.h"
#include "native_shell_workshop_model_internal.h"
#include "native_shell_workshop_editor.h"
#include "native_shell_workshop.h"
#include <cstring>
#include <algorithm>

void nativeShellWorkshopModel_Operation(const char* method, yyjson_val* args,
                                    NativeAuthoring::Reply& reply)
{
    using namespace WorkshopModelInternal;
    if (!active->project) {reply.fail("no_draft", "Open a fighter draft first."); return;}
    if(!std::strcmp(method,"view") || !std::strcmp(method,"select") || !std::strcmp(method,"pose")) {
        const auto has=[&](const char* key){return yyjson_obj_get(args,key)!=nullptr;};
        if(!std::strcmp(method,"select")) {
            auto next=get_view();
            const char* contexts[]={"meshes","materials","skeleton","skinning"};const char* modes[]={"edit","pose","animation"};
            if(has("context")) for(unsigned i=0;i<4;++i) if(!std::strcmp(yyjson_get_str(yyjson_obj_get(args,"context")),contexts[i])) next.context=(Context)i;
            if(has("mode")) for(unsigned i=0;i<3;++i) if(!std::strcmp(yyjson_get_str(yyjson_obj_get(args,"mode")),modes[i])) {next.mode=(RigMode)i;next.playing=false;}
            if(has("mesh")) {next.mesh=(int)yyjson_get_sint(yyjson_obj_get(args,"mesh"));if(next.mesh>=0 && next.mesh<active->model.mesh_count) {const unsigned mat=active->model.meshes[next.mesh].material;next.material=mat<active->model.material_count?(int)mat:-1;}}
            if(has("material")) next.material=(int)yyjson_get_sint(yyjson_obj_get(args,"material"));
            for(const char* field:{"bones","hidden_meshes"}) if(auto* values=yyjson_obj_get(args,field)) {
                const bool bones=!std::strcmp(field,"bones");
                bool seen[NATIVE_MODEL_MAX_MESHES]={};
                if(bones) {std::fill_n(next.selected_bones,NATIVE_MODEL_MAX_JOINTS,false);next.bone=-1;}
                else std::fill_n(next.hidden_meshes,NATIVE_MODEL_MAX_MESHES,0);
                size_t i,n;yyjson_val* value;
                yyjson_arr_foreach(values,i,n,value) {
                    const unsigned id=(unsigned)yyjson_get_uint(value);
                    if(id>=(bones?active->model.joint_count:active->model.mesh_count) || seen[id]) {reply.fail("invalid_model_selection","Use each inspected bone/mesh ID once.");return;}
                    seen[id]=true;
                    if(bones) {next.selected_bones[id]=true;next.bone=(int)id;}else next.hidden_meshes[id]=1;
                }
            }
            if(has("active_bone")) next.bone=(int)yyjson_get_sint(yyjson_obj_get(args,"active_bone"));
            if(has("bones_visible")) next.bones_visible=yyjson_get_bool(yyjson_obj_get(args,"bones_visible"));
            if(has("action_clip")) {next.clip=(int)yyjson_get_uint(yyjson_obj_get(args,"action_clip"));next.frame=0;next.playing=false;}
            if(has("frame")) {next.frame=(int)yyjson_get_uint(yyjson_obj_get(args,"frame"));next.playing=false;}
            if(has("playing")) next.playing=yyjson_get_bool(yyjson_obj_get(args,"playing"));
            std::string error;
            if(!set_view(next,error)) {reply.fail("model_view_rejected",error);return;}
            nativeShellWorkshopEditor_AgentWorkspace("model");
        } else if(!std::strcmp(method,"pose")) {
            std::vector<PoseChange> changes;
            size_t i,n;yyjson_val* row;
            yyjson_arr_foreach(yyjson_obj_get(args,"changes"),i,n,row) {
                PoseChange change;change.bone=(unsigned)yyjson_get_uint(yyjson_obj_get(row,"bone"));
                const char* names[]={"position","rotation","scale"};float* vectors[]={change.value.position,change.value.rotation,change.value.scale};
                for(unsigned c=0;c<3;++c) if(auto* values=yyjson_obj_get(row,names[c])) {
                    change.channels|=1u<<c;for(unsigned a=0;a<3;++a) vectors[c][a]=(float)yyjson_get_num(yyjson_arr_get(values,a));
                }
                changes.push_back(change);
            }
            std::string error;
            if(!set_pose(changes.data(),(unsigned)changes.size(),yyjson_get_bool(yyjson_obj_get(args,"reset")),error)) {reply.fail("pose_edit_rejected",error);return;}
            nativeShellWorkshopEditor_AgentWorkspace("model");
        }
        yyjson_mut_obj_remove_str(reply.result,"model_view");
        auto* out=yyjson_mut_obj(reply.document);yyjson_mut_obj_add_val(reply.document,reply.result,"model_view",out);
        describe_view(reply,out,!std::strcmp(method,"view") && yyjson_get_bool(yyjson_obj_get(args,"include_pose")));
        yyjson_mut_obj_add_bool(reply.document,reply.result,"transient",true);
        yyjson_mut_obj_add_bool(reply.document,reply.result,"saved",false);
        yyjson_mut_obj_add_bool(reply.document,reply.result,"installed",false);
        return;
    }
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
    const char* operation=yyjson_get_str(yyjson_obj_get(args,"operation"));
    if(operation && !std::strcmp(operation,"mesh_material") && active->selected_mesh>=0) {
        active->selected_material=active->model.meshes[active->selected_mesh].material;render();
    }
    if (rig_changed) nativeShellWorkshopEditor_RigChanged(*before, active->model);
    nativeHome_WorkshopPoseInvalidate(); buttons();
    const bool refreshing = nativeShellWorkshop_RequestPreview(false);
    yyjson_mut_obj_remove_str(reply.result, "preview_status");
    NativeAuthoring::add_string(reply, reply.result, "preview_status", refreshing ? "loading" : "refresh_required");
    status("Edited model. Save the draft to keep the changes.");
}
