#include "native_shell_workshop_editor.h"
#include "native_shell_workshop_editor_internal.h"
#include "native_shell_workshop_bones.h"
#include "native_shell_workshop_pose.h"
#include <melee/ft/native_fighter_model_editor.h>
#include <algorithm>

using namespace WorkshopEditor;

void nativeShellWorkshopEditor_RigChanged(const NativeFighterModelEditor& before,
                                         const NativeFighterModelEditor& after)
{
    if(!session->project || session->project!=after.project) return;
    const NativeFighterAnimationClip clip=session->animation_clip;
    const std::string clip_name=session->animation_name;
    const int timeline_frame=session->timeline_frame;
    const float graph_start=session->graph_view_start,graph_end=session->graph_view_end;
    NativeFighterAnimationTrack previous[1024];
    bool selected[512];
    const size_t count=session->track_count;
    std::copy_n(session->tracks,count,previous);
    std::copy_n(session->selected_bones,std::size(selected),selected);
    const int selected_track=session->selected_track;
    const int outliner=session->selected_outliner_joint;
    const auto graph_selection=session->graph_selection;
    unsigned mapping[1024];std::fill_n(mapping,1024,UINT_MAX);
    if(session->pose_copied) {
        NativeFighterPose remapped=session->copied_pose;
        std::memset(remapped.present,0,sizeof(remapped.present));
        remapped.sampled_channels=0;remapped.root_joint=UINT_MAX;
        for(unsigned i=0;i<std::min<unsigned>(after.joint_count,NATIVE_FIGHTER_POSE_JOINTS);++i)
            for(unsigned old=0;old<std::min<unsigned>(before.joint_count,NATIVE_FIGHTER_POSE_JOINTS);++old)
                if(after.joints[i].offset==before.joints[old].offset) {
                    for(unsigned channel=0;channel<NATIVE_FIGHTER_POSE_CHANNELS;++channel) {
                        remapped.present[i][channel]=session->copied_pose.present[old][channel];
                        remapped.values[i][channel]=session->copied_pose.values[old][channel];
                        remapped.sampled_channels+=remapped.present[i][channel]!=0;
                    }
                    if(old==session->copied_pose.root_joint) remapped.root_joint=i;
                    break;
                }
        session->copied_pose=remapped;
        session->pose_copied=remapped.sampled_channels!=0;
    }
    nativeShellWorkshopBones_Reset();
    for(unsigned i=0;i<512;++i)
        session->bone_names[i]=i<after.joint_count?after.joints[i].name:"";
    session->preview_hitbox_cached_frame=session->preview_hitbox_cached_action=-1;
    if(!nativeFighterAction_List(session->project,session->actions,1024,&session->action_count))
        session->action_count=0;
    for(size_t i=0;i<session->action_count;++i) {
        NativeFighterAnimationInfo info={};
        session->action_has_clip[i]=nativeFighterAnimation_Info(session->project,
            {NativeFighterAnimation_Action,(unsigned)i},&info);
        session->action_clip_frames[i]=session->action_has_clip[i]?info.frames:0;
    }
    read_demo_clips();
    if(session->selected_action>=0 && (size_t)session->selected_action<session->action_count)
        select_action((unsigned)session->selected_action);
    session->timeline_frame=timeline_frame;
    field("workshop-current-frame")->SetValue(number((unsigned)timeline_frame));
    update_timeline_cursor();
    session->animation_clip=clip;session->animation_name=clip_name;
    session->displayed_animation_clip.index=UINT_MAX;
    session->skeleton_hierarchy_available=false;
    show_animation();
    std::fill_n(session->selected_bones,std::size(selected),false);
    for(unsigned i=0;i<std::min<size_t>(after.joint_count,std::size(selected));++i)
        for(unsigned old=0;old<std::min<size_t>(before.joint_count,std::size(selected));++old)
            if(after.joints[i].offset==before.joints[old].offset) {
                session->selected_bones[i]=selected[old];
                break;
            }
    session->selected_track=-1;session->selected_outliner_joint=-1;
    for(size_t i=0;i<count;++i) {
        if(previous[i].joint>=before.joint_count) continue;
        const uint32_t offset=before.joints[previous[i].joint].offset;
        for(size_t next=0;next<session->track_count;++next) {
            const auto& track=session->tracks[next];
            if(track.joint>=after.joint_count||track.channel!=previous[i].channel||
               after.joints[track.joint].offset!=offset) continue;
            mapping[i]=(unsigned)next;
            if((int)i==selected_track) session->selected_track=(int)next;
            break;
        }
    }
    if(outliner>=0 && outliner<before.joint_count)
        for(unsigned i=0;i<after.joint_count;++i)
            if(after.joints[i].offset==before.joints[outliner].offset) session->selected_outliner_joint=(int)i;
    if(session->selected_track<0)
        for(size_t i=0;i<session->track_count;++i)
            if(track_selected((unsigned) i)) {session->selected_track=(int)i;break;}
    if(session->selected_track<0 && session->selected_outliner_joint<0 && session->track_count) {
        // A fresh view gets the same first-bone selection as show_animation.
        // A retained unanimated bone keeps its explicit empty selection.
        session->selected_track=0;
        session->selected_outliner_joint=(int)session->tracks[0].joint;
        if(session->tracks[0].joint<std::size(selected))
            session->selected_bones[session->tracks[0].joint]=true;
    }
    if(session->selected_track>=0) refresh_track_selection();
    else {
        session->key_count=0;session->selected_key=-1;
        show_track_browser();show_animation_keys();show_bone_transform();
        show_animation_graph();show_animation_dopesheet();
    }
    session->graph_selection.clear();
    for(const auto& key:graph_selection)
        if(key.track<count && mapping[key.track]!=UINT_MAX)
            session->graph_selection.push_back({mapping[key.track],key.key});
    if(count && graph_end>graph_start) {
        session->graph_view_start=graph_start;session->graph_view_end=graph_end;
    }
    session->graph_width=0;session->graph_refresh=true;session->bone_pose_refresh=true;
    nativeShellWorkshopEditor_FilterClips();
    // Model owns the visible widgets. The refreshed lists will be used when
    // switching back to Action or Animation without reopening the draft.
    set_text("workshop-context-action","Model");
    set_text("workshop-context-clip","Compatible fighter rig");
}
