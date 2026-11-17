#include "native_shell_workshop_model_internal.h"
#include "native_shell_workshop_transform.h"
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace WorkshopModelInternal {
extern std::chrono::steady_clock::time_point play_started;
extern int play_start_frame;
namespace {
const char* contexts[]={"meshes","materials","skeleton","skinning"};
const char* modes[]={"edit","pose","animation"};
void base_pose(NativeHomeWorkshopBoneLocal* pose,bool* changed)
{
    for(unsigned bone=0;bone<active->model.joint_count;++bone) {
        pose[bone]={};
        changed[bone]=nativeHome_WorkshopBoneLocal(bone,&pose[bone]);
        if(!changed[bone]) {
            std::memcpy(pose[bone].position,active->model.joints[bone].position,3*sizeof(float));
            std::memcpy(pose[bone].rotation,active->model.joints[bone].rotation,3*sizeof(float));
            std::memcpy(pose[bone].scale,active->model.joints[bone].scale,3*sizeof(float));
        }
    }
}
}
View get_view()
{
    View result;
    result.context=active->context;result.mode=active->rig_mode;
    result.mesh=active->selected_mesh;result.material=active->selected_material;result.bone=active->selected_joint;
    result.clip=active->selected_clip;result.frame=active->frame;result.bones_visible=active->bones_visible;result.playing=playing;
    std::copy_n(active->selected_joints,NATIVE_MODEL_MAX_JOINTS,result.selected_bones);
    std::copy_n(active->hidden_meshes,NATIVE_MODEL_MAX_MESHES,result.hidden_meshes);
    return result;
}
bool set_view(View next,std::string& error)
{
    if(!active->project || !active->model_available) {error="Open a readable costume model first.";return false;}
    if((unsigned)next.context>=4 || (unsigned)next.mode>=3 || next.mesh < -1 || next.material < -1 || next.bone < -1 || next.clip < -1 || next.frame<0 ||
       next.mesh>=(int)active->model.mesh_count || next.material>=(int)active->model.material_count || next.bone>=(int)active->model.joint_count ||
       (next.clip>=0 && ((size_t)next.clip>=active->action_count || !active->has_clip[next.clip]))) {
        error="Choose mesh, material, bone and clip IDs present in this member.";return false;
    }
    for(unsigned bone=active->model.joint_count;bone<NATIVE_MODEL_MAX_JOINTS;++bone)
        if(next.selected_bones[bone]) {error="The bone selection includes an unavailable bone.";return false;}
    for(unsigned mesh=active->model.mesh_count;mesh<NATIVE_MODEL_MAX_MESHES;++mesh)
        if(next.hidden_meshes[mesh]) {error="The hidden mesh list includes an unavailable mesh.";return false;}
    if((next.mode==RigMode::Animation && next.clip<0) || (next.playing && next.mode!=RigMode::Animation)) {
        error="Choose an available action clip in Animation Mode before playing it.";return false;
    }
    if(next.context==Context::Skinning && (next.bone<0 || !(active->model.joints[next.bone].flags&1))) {
        for(unsigned bone=0;bone<active->model.joint_count;++bone) if(active->model.joints[bone].flags&1) {
            next.bone=(int)bone;std::fill_n(next.selected_bones,NATIVE_MODEL_MAX_JOINTS,false);next.selected_bones[bone]=true;break;
        }
    }
    const auto before=get_view();
    if(next==before) return true;
    nativeWorkshopTransform_Clear();
    if(next.mode==RigMode::Pose && !active->pose_initialized) {
        base_pose(active->pose,active->pose_changed);active->pose_initialized=true;
    }
    active->context=next.context;active->rig_mode=next.mode;active->selected_mesh=next.mesh;active->selected_material=next.material;
    active->selected_joint=next.bone;active->selected_clip=next.clip;active->bones_visible=next.bones_visible;
    std::copy_n(next.selected_bones,NATIVE_MODEL_MAX_JOINTS,active->selected_joints);
    std::copy_n(next.hidden_meshes,NATIVE_MODEL_MAX_MESHES,active->hidden_meshes);
    hovered_mesh=-1;
    reveal_material=next.context==Context::Materials?2:0;
    if(before.clip!=next.clip) build_clips();
    const int max_frame=next.clip>=0?(int)std::max(0.0f,std::floor(active->clip_frames[next.clip])):0;
    element("workshop-model-frame")->SetAttribute("max",std::to_string(max_frame));
    set_frame(next.frame);
    if(next.playing && (!before.playing || before.clip!=next.clip || before.frame!=active->frame)) {
        play_started=std::chrono::steady_clock::now();play_start_frame=active->frame;
    }
    playing=next.playing;text("workshop-model-play",playing?"Pause":"Play");
    render();update_mesh_view();nativeHome_WorkshopPoseInvalidate();
    return true;
}
bool set_pose(const PoseChange* changes,unsigned count,bool reset,std::string& error)
{
    if(!active->project || !active->model_available || count>NATIVE_MODEL_MAX_JOINTS || (count && !changes) || (!count && !reset)) {
        error="Choose a readable model and supply temporary pose changes or reset.";return false;
    }
    bool selected[NATIVE_MODEL_MAX_JOINTS]={};
    for(unsigned i=0;i<count;++i) {
        const auto& change=changes[i];
        if(change.bone>=active->model.joint_count || selected[change.bone] || !change.channels || (change.channels&~7u)) {error="Use each inspected bone once and supply position, rotation or scale.";return false;}
        selected[change.bone]=true;
        const float* values[]={change.value.position,change.value.rotation,change.value.scale};
        for(unsigned c=0;c<3;++c) if(change.channels&(1u<<c)) for(unsigned axis=0;axis<3;++axis) {
            const float v=values[c][axis];
            if(!std::isfinite(v) || (c==0 && std::fabs(v)>10000) || (c==1 && std::fabs(v)>100) || (c==2 && (v<.001f || v>100))) {
                error="Use finite positions within 10000 units, rotations within 100 radians and scales from 0.001 to 100.";return false;
            }
        }
    }
    NativeHomeWorkshopBoneLocal pose[NATIVE_MODEL_MAX_JOINTS];bool changed[NATIVE_MODEL_MAX_JOINTS];
    std::copy_n(active->pose,NATIVE_MODEL_MAX_JOINTS,pose);std::copy_n(active->pose_changed,NATIVE_MODEL_MAX_JOINTS,changed);
    if(!active->pose_initialized) base_pose(pose,changed);
    if(reset) for(unsigned bone=0;bone<active->model.joint_count;++bone) {
        std::memcpy(pose[bone].position,active->model.joints[bone].position,3*sizeof(float));
        std::memcpy(pose[bone].rotation,active->model.joints[bone].rotation,3*sizeof(float));
        std::memcpy(pose[bone].scale,active->model.joints[bone].scale,3*sizeof(float));changed[bone]=true;
    }
    for(unsigned i=0;i<count;++i) {
        const auto& change=changes[i];auto& target=pose[change.bone];
        if(change.channels&1) std::memcpy(target.position,change.value.position,3*sizeof(float));
        if(change.channels&2) std::memcpy(target.rotation,change.value.rotation,3*sizeof(float));
        if(change.channels&4) std::memcpy(target.scale,change.value.scale,3*sizeof(float));
        changed[change.bone]=true;
    }
    std::copy_n(pose,NATIVE_MODEL_MAX_JOINTS,active->pose);std::copy_n(changed,NATIVE_MODEL_MAX_JOINTS,active->pose_changed);
    active->pose_initialized=true;active->rig_mode=RigMode::Pose;active->context=Context::Skeleton;
    playing=false;nativeWorkshopTransform_Clear();text("workshop-model-play","Play");
    render();update_mesh_view();nativeHome_WorkshopPoseInvalidate();return true;
}
void describe_view(NativeAuthoring::Reply& r,yyjson_mut_val* out,bool pose_details)
{
    const auto view=get_view();
    NativeAuthoring::add_string(r,out,"context",contexts[(unsigned)view.context]);NativeAuthoring::add_string(r,out,"mode",modes[(unsigned)view.mode]);
    yyjson_mut_obj_add_bool(r.document,out,"visible",visible);yyjson_mut_obj_add_int(r.document,out,"mesh",view.mesh);yyjson_mut_obj_add_int(r.document,out,"material",view.material);yyjson_mut_obj_add_int(r.document,out,"active_bone",view.bone);
    yyjson_mut_obj_add_int(r.document,out,"action_clip",view.clip);yyjson_mut_obj_add_int(r.document,out,"frame",view.frame);yyjson_mut_obj_add_bool(r.document,out,"playing",view.playing);
    yyjson_mut_obj_add_bool(r.document,out,"bones_visible",view.bones_visible);yyjson_mut_obj_add_bool(r.document,out,"bones_drawn",view.bones_visible || view.context==Context::Skeleton);
    auto* bones=yyjson_mut_arr(r.document);auto* hidden=yyjson_mut_arr(r.document);auto* posed=yyjson_mut_arr(r.document);
    yyjson_mut_obj_add_val(r.document,out,"bones",bones);yyjson_mut_obj_add_val(r.document,out,"hidden_meshes",hidden);yyjson_mut_obj_add_val(r.document,out,"temporary_pose_bones",posed);
    yyjson_mut_obj_add_bool(r.document,out,"pose_initialized",active->pose_initialized);
    auto* pose=pose_details?yyjson_mut_arr(r.document):nullptr;
    if(pose) yyjson_mut_obj_add_val(r.document,out,"temporary_pose",pose);
    for(unsigned i=0;i<active->model.joint_count;++i) {
        if(view.selected_bones[i]) yyjson_mut_arr_add_uint(r.document,bones,i);
        if(!active->pose_initialized || !active->pose_changed[i]) continue;
        yyjson_mut_arr_add_uint(r.document,posed,i);
        if(!pose) continue;
        auto* row=yyjson_mut_obj(r.document);yyjson_mut_arr_append(pose,row);yyjson_mut_obj_add_uint(r.document,row,"bone",i);
        const char* names[]={"position","rotation","scale"};const float* values[]={active->pose[i].position,active->pose[i].rotation,active->pose[i].scale};
        for(unsigned c=0;c<3;++c) {auto* array=yyjson_mut_arr(r.document);yyjson_mut_obj_add_val(r.document,row,names[c],array);for(unsigned a=0;a<3;++a) yyjson_mut_arr_add_real(r.document,array,values[c][a]);}
    }
    for(unsigned i=0;i<active->model.mesh_count;++i) if(view.hidden_meshes[i]) yyjson_mut_arr_add_uint(r.document,hidden,i);
}
void view_fingerprint(std::string& fingerprint,unsigned member)
{
    const auto& view=sessions[member];
    const int fields[]={(int)view.context,(int)view.rig_mode,view.selected_mesh,view.selected_material,view.selected_joint,view.selected_clip,
        active==&view && playing?-1:view.frame,(int)view.bones_visible,(int)view.pose_initialized,(int)(active==&view && playing)};
    fingerprint.append((const char*)fields,sizeof(fields));
    fingerprint.append((const char*)view.selected_joints,sizeof(view.selected_joints));fingerprint.append((const char*)view.hidden_meshes,sizeof(view.hidden_meshes));
    fingerprint.append((const char*)view.pose_changed,sizeof(view.pose_changed));
}
}
