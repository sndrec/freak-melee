#include "native_shell_workshop_model_internal.h"
#include "native_shell_workshop_editor.h"
#include "native_shell_workshop.h"
#include <melee/ft/native_fighter_rig.h>
#include <melee/ft/native_fighter_rig_edit.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace WorkshopModelInternal {
namespace {
bool read_ids(const NativeFighterProject& project, int32_t* ids, bool* extra)
{
    HSD_Dat dat;
    NativeFighterRig rig;
    char error[192];
    const unsigned parts=project.facts.joints+project.facts.omission_count;
    if(HSD_DatOpen(&dat,project.fighter_dat,project.fighter_size)!=HSD_DAT_OK ||
       !nativeFighterRig_Read(&dat,parts,&rig,error,sizeof(error))) return false;
    for(unsigned part=0, ordinal=0;part<parts;++part) {
        bool omitted=false;
        for(unsigned i=0;i<project.facts.omission_count;++i)
            omitted |= project.facts.omissions[i].joint==part;
        if(omitted) continue;
        if(ordinal>=NATIVE_MODEL_MAX_JOINTS) return false;
        ids[ordinal]=nativeFighterRig_Id(&rig,part);
        extra[ordinal++]=(rig.flags[part]&NATIVE_RIG_EXTRA)!=0;
    }
    return true;
}

// Own only the four assets. Scripts, parameters and authoring names stay with
// the open draft and are borrowed during admission of this private candidate.
struct Candidate {
    NativeFighterProject project;
    explicit Candidate(const NativeFighterProject& source):project(source) {
        project.fighter_dat=project.animation_dat=project.costume_dat=project.asset_record=nullptr;
    }
    ~Candidate() {
        std::free(project.fighter_dat);std::free(project.animation_dat);
        std::free(project.costume_dat);std::free(project.asset_record);
    }
    bool copy(const NativeFighterCoreAssetState& state) {
        uint8_t** assets[]={&project.fighter_dat,&project.animation_dat,&project.costume_dat,&project.asset_record};
        for(unsigned i=0;i<4;++i) {
            *assets[i]=(uint8_t*)std::malloc(state.bytes[i].size());
            if(!*assets[i]) return false;
            std::memcpy(*assets[i],state.bytes[i].data(),state.bytes[i].size());
        }
        return true;
    }
    void publish(NativeFighterProject& destination) {
        std::swap(project.fighter_dat,destination.fighter_dat);
        std::swap(project.animation_dat,destination.animation_dat);
        std::swap(project.costume_dat,destination.costume_dat);
        std::swap(project.asset_record,destination.asset_record);
        destination.fighter_size=project.fighter_size;destination.animation_size=project.animation_size;
        destination.costume_size=project.costume_size;destination.asset_record_size=project.asset_record_size;
        destination.facts=project.facts;
        destination.rig_edit_revision=project.rig_edit_revision;
        destination.rig_edit_serial=project.rig_edit_serial;
    }
};

void edit_rig(const NativeFighterRigEdit& edit) try
{
    auto change=std::make_unique<RigChange>();
    if(!nativeFighterCoreAssets_Capture(*active->project,change->before)) {
        status("Could not reserve storage for bone Undo.");return;
    }
    rig_capture_view(change->before_view);
    Candidate candidate(*active->project);
    if(!candidate.copy(change->before)) {status("Could not allocate the bone edit.");return;}
    int32_t result=-1;
    char error[256]={};
    if(!nativeFighterRig_Edit(&candidate.project,&edit,&result,error,sizeof(error))) {status(error);return;}
    if(!nativeFighterCoreAssets_Capture(candidate.project,change->after)) {
        status("Could not reserve storage for bone Redo. The draft is unchanged.");return;
    }
    auto next=std::make_unique<NativeFighterModelEditor>();
    int32_t ids[NATIVE_MODEL_MAX_JOINTS];bool extra[NATIVE_MODEL_MAX_JOINTS];
    if(!nativeFighterModel_Open(next.get(),&candidate.project)||!read_ids(candidate.project,ids,extra)) {
        status("Could not open the edited skeleton. The draft is unchanged.");return;
    }
    // Keep the old offset identities in the view; reopen_model remaps retained
    // selections and temporary local poses after publication.
    change->after_view=change->before_view;
    for(unsigned i=0;i<next->joint_count;++i) if(ids[i]==result) {
        const uint32_t offset=next->joints[i].offset;
        unsigned selected=change->after_view.count;
        for(unsigned old=0;old<change->after_view.count;++old)
            if(change->after_view.offsets[old]==offset) selected=old;
        if(selected==change->after_view.count && selected<NATIVE_MODEL_MAX_JOINTS) {
            ++change->after_view.count;
            change->after_view.offsets[selected]=offset;
            std::memcpy(change->after_view.pose[selected].position,next->joints[i].position,3*sizeof(float));
            std::memcpy(change->after_view.pose[selected].rotation,next->joints[i].rotation,3*sizeof(float));
            std::memcpy(change->after_view.pose[selected].scale,next->joints[i].scale,3*sizeof(float));
        }
        change->after_view.selected=(int)selected;
        std::fill_n(change->after_view.selected_joints,NATIVE_MODEL_MAX_JOINTS,false);
        change->after_view.selected_joints[selected]=true;
        break;
    }
    // Reserve history before publishing, so allocation failure cannot leave a
    // successful hierarchy edit without its Undo entry.
    active->changes.reserve(64);
    auto before_model=std::make_unique<NativeFighterModelEditor>(active->model);
    Change entry={};entry.rig=std::move(change);
    active->changes.erase(active->changes.begin()+active->change_cursor,active->changes.end());
    if(active->changes.size()==64) active->changes.erase(active->changes.begin());
    active->changes.push_back(std::move(entry));active->change_cursor=active->changes.size();
    candidate.publish(*active->project);
    playing=false;
    reopen_model(&active->changes.back().rig->after_view);
    nativeShellWorkshopEditor_RigChanged(*before_model,active->model);
    nativeShellWorkshop_RequestPreview(false);nativeHome_WorkshopPoseInvalidate();buttons();
    status(edit.kind==NativeRigEdit_Add?"Added bone. Save the draft to keep it.":
           edit.kind==NativeRigEdit_Reparent?"Changed bone parent. Save the draft to keep it.":
           "Removed bone. Its children keep their rest positions.");
}
catch(const std::bad_alloc&) {status("Could not allocate storage for the bone edit.");}
}

void rig_capture_view(RigView& view)
{
    view.count=active->model.joint_count;view.selected=active->selected_joint;
    view.initialized=active->pose_initialized;
    for(unsigned i=0;i<view.count;++i) {
        view.offsets[i]=active->model.joints[i].offset;
        view.selected_joints[i]=active->selected_joints[i];
        view.pose_changed[i]=active->pose_changed[i];view.pose[i]=active->pose[i];
    }
}

void rig_restore_view(const RigView& view)
{
    active->selected_joint=0;
    std::fill_n(active->selected_joints,NATIVE_MODEL_MAX_JOINTS,false);
    std::fill_n(active->pose_changed,NATIVE_MODEL_MAX_JOINTS,false);
    for(unsigned i=0;i<active->model.joint_count;++i) {
        const auto& joint=active->model.joints[i];
        auto& pose=active->pose[i];
        std::memcpy(pose.position,joint.position,3*sizeof(float));
        std::memcpy(pose.rotation,joint.rotation,3*sizeof(float));
        std::memcpy(pose.scale,joint.scale,3*sizeof(float));
        for(unsigned old=0;old<view.count;++old) if(joint.offset==view.offsets[old]) {
            active->selected_joints[i]=view.selected_joints[old];
            if((int)old==view.selected) active->selected_joint=(int)i;
            if(view.initialized) {pose=view.pose[old];active->pose_changed[i]=view.pose_changed[old];}
            break;
        }
    }
    active->pose_initialized=view.initialized;
}

std::string rig_tree_controls()
{
    if(active->rig_mode!=RigMode::Edit) return "";
    std::string name="Extra bone";
    for(unsigned suffix=2;;++suffix) {
        bool used=false;
        for(unsigned i=0;i<active->model.joint_count;++i) used|=name==active->model.joints[i].name;
        if(!used) break;
        name="Extra bone "+std::to_string(suffix);
    }
    return "<label>New child bone</label><input id='workshop-model-bone-name' class='room-input' "
        "type='text' maxlength='63' value='"+escape(name)+"'/><button "
        "id='workshop-model-bone-add' class='room-button'>Add to selected bone</button>";
}

std::string rig_inspector()
{
    if(active->rig_mode!=RigMode::Edit || active->selected_joint<0 ||
       active->selected_joint>=active->model.joint_count) return "";
    int32_t ids[NATIVE_MODEL_MAX_JOINTS];bool extra[NATIVE_MODEL_MAX_JOINTS];
    if(!read_ids(*active->project,ids,extra)) return "";
    if(!extra[active->selected_joint]) return "<div class='workshop-model-inspector-section'>"
        "<p>This bone belongs to the fighter's base skeleton. Extra bones can be added as children.</p></div>";
    std::string markup="<div class='workshop-model-inspector-section'><div class='eyebrow'>HIERARCHY</div>"
        "<label>Parent</label><select id='workshop-model-bone-parent' class='room-input'>";
    for(unsigned i=0;i<active->model.joint_count;++i) {
        bool descendant=false;
        for(int parent=(int)i;parent>=0;parent=active->model.joints[parent].parent)
            descendant|=parent==active->selected_joint;
        if(descendant) continue;
        markup+="<option value='"+std::to_string(ids[i])+"'"+
            (active->model.joints[active->selected_joint].parent==(int)i?" selected":"")+">"+
            escape(active->model.joints[i].name)+"</option>";
    }
    return markup+"</select><button id='workshop-model-bone-reparent' class='room-button'>Change parent</button>"
        "<p>Keeps the bone and its children in place in the rest pose.</p>"
        "<button id='workshop-model-bone-remove' class='room-button'>Remove bone</button>"
        "<p>A bone used by a mesh, constraint or fighter behavior must be unlinked before removal.</p></div>";
}

bool rig_click(const char* id)
{
    NativeFighterRigEdit edit={};
    if(!std::strcmp(id,"workshop-model-bone-add")) edit.kind=NativeRigEdit_Add;
    else if(!std::strcmp(id,"workshop-model-bone-reparent")) edit.kind=NativeRigEdit_Reparent;
    else if(!std::strcmp(id,"workshop-model-bone-remove")) edit.kind=NativeRigEdit_Remove;
    else return false;
    if(!visible||!active->project||active->context!=Context::Skeleton||active->rig_mode!=RigMode::Edit ||
       active->selected_joint<0||active->selected_joint>=active->model.joint_count) return true;
    int32_t ids[NATIVE_MODEL_MAX_JOINTS];bool extra[NATIVE_MODEL_MAX_JOINTS];
    if(!read_ids(*active->project,ids,extra)) {status("Could not read the fighter's bone identities.");return true;}
    edit.bone_id=edit.parent_id=ids[active->selected_joint];
    if(edit.kind==NativeRigEdit_Add) {
        std::snprintf(edit.name,sizeof(edit.name),"%s",input("workshop-model-bone-name")->GetValue().c_str());
    } else if(edit.kind==NativeRigEdit_Reparent) {
        auto* parent=static_cast<Rml::ElementFormControlSelect*>(element("workshop-model-bone-parent"));
        char* end;const std::string value=parent->GetValue();
        const long parsed=std::strtol(value.c_str(),&end,10);
        if(value.empty()||*end||parsed<0||parsed>INT32_MAX) {status("Choose a parent bone.");return true;}
        edit.parent_id=(int32_t)parsed;
    }
    edit_rig(edit);return true;
}
}
