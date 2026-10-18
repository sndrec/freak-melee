#include "native_source_assets.h"
#include "ftcamera.h"
#include "native_fighter_pose.h"
#include "native_script_constants.h"
#include "kinds/ftCommon/forward.h"
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool copy_name(char* out,const char* source,bool optional)
{
    if(!source)return optional;
    size_t length=strnlen(source,NATIVE_SOURCE_NAME_BYTES);
    if(length==NATIVE_SOURCE_NAME_BYTES || (!length&&!optional))return false;
    memcpy(out,source,length+1);return true;
}

bool nativeSourceAssets_Copy(const NativeFighterDefinition* base,NativeSourceAssetFacts* output,NativeScriptDiagnostic* d)
{
    if(!base || !output || !base->motion_states || base->motion_state_count>NATIVE_SOURCE_MOTIONS ||
       base->bone_count>FT_COMMON_PART_COUNT || (base->bone_count&&!base->bones) ||
       base->omission_count>NATIVE_SOURCE_OMISSIONS || (base->omission_count&&!base->omissions) ||
       base->article_asset_count>NATIVE_SOURCE_ARTICLES || (base->article_asset_count&&!base->article_assets) ||
       base->pose_graph_count>NATIVE_SOURCE_POSE_GRAPHS || (base->pose_graph_count&&!base->pose_graphs))
        return error(d,"Missing or oversized authoring asset donor");
    NativeSourceAssetFacts f={0};
    if(!copy_name(f.fighter_root,base->fighter_root,false) || !copy_name(f.costume_joint,base->costume_joint,false) ||
       !copy_name(f.costume_matanim,base->costume_matanim,true))return error(d,"Asset donor root name exceeds owned capacity");
    if(base->demo_motion_names) {
        const Fighter_DemoStrings* names=base->demo_motion_names;
        const char* roots[]={names->result_filename,names->intro_filename,names->ending_filename,names->vi_wait_filename};
        for(unsigned i=0;i<4;++i)if(!copy_name(f.demos[i],roots[i],true))return error(d,"Asset donor demo root exceeds capacity");
    }
    f.layout=base->asset_layout;f.actions=base->action_count;f.demo_count=base->demo_count;
    f.joints=base->joint_count;f.bone_count=base->bone_count;f.omission_count=base->omission_count;
    f.article_count=base->article_asset_count;f.pose_count=base->pose_graph_count;
    f.animation_tag=(uint8_t)base->animation_source_kind;f.effect_bank=base->effect_bank;
    f.sound_banks=base->sound_banks;f.sound_mute_channels=base->sound_mute_channels;
    f.ignore_surface_friction=base->ignore_surface_friction;
    f.moves.common_count=ftCo_MS_Count;f.moves.first=base->motion_state_first;f.moves.count=base->motion_state_count;
    for(unsigned i=0;i<NativeScriptMove_Count;++i){f.moves.ground[i]=base->special_ground_state[i];
        f.moves.air[i]=base->special_air_state[i];f.moves.transition_flags[i]=base->special_transition_flags[i];}
    for(unsigned i=0;i<f.moves.count;++i) {
        const MotionState* row=&base->motion_states[i];
        if(row->cam_cb && row->cam_cb!=ftCamera_UpdateCameraBox)return error(d,"Asset donor uses an unaudited camera policy");
        f.motions[i]=(NativeSourceMotionData){row->anim_id,(uint32_t)row->x4_flags,row->_,row->cam_cb?1:0};
    }
    if(f.bone_count)memcpy(f.bones,base->bones,f.bone_count*sizeof(*f.bones));
    if(f.omission_count)memcpy(f.omissions,base->omissions,f.omission_count*sizeof(*f.omissions));
    for(unsigned i=0;i<f.article_count;++i) {
        const NativeFighterArticleAsset* row=&base->article_assets[i];NativeSourceArticleAsset* owned=&f.articles[i];
        owned->kind=row->kind;owned->slot=row->slot;owned->kind_attribute_offset=row->kind_attribute_offset;
        // Initial authoring names come from the immutable enum catalog. Later
        // edits own this name independently; runtime never derives it again.
        for(uint32_t c=0;c<nativeScriptConstants_Count();++c) {
            NativeScriptConstant value;
            if(nativeScriptConstants_At(c,&value) && value.family==NativeScriptConstant_Item &&
               value.bits==(uint64_t)(int64_t)row->kind && !strncmp(value.name,"It_Kind_",8)) {
                if(!copy_name(owned->declaration,value.name,false))return error(d,"Initial article name exceeds capacity");break;
            }
        }
        if(!owned->declaration[0])snprintf(owned->declaration,sizeof(owned->declaration),"asset_%u",row->slot);
    }
    for(unsigned i=0;i<f.pose_count;++i) {
        f.poses[i].name=base->pose_graphs[i].name_hash;f.poses[i].slot=base->pose_graphs[i].slot;
        if(!copy_name(f.poses[i].path,base->pose_graphs[i].path,true))return error(d,"Asset donor pose path exceeds capacity");
        const NativeFighterPoseGraph* pose=&base->pose_graphs[i];
        if(!pose->bone_count || pose->bone_count>MAX_FT_PARTS || !pose->bone_ids)
            return error(d,"A starting pose needs a bone ID for each descriptor.");
        f.poses[i].bone_count=pose->bone_count;
        memcpy(f.poses[i].bone_ids,pose->bone_ids,pose->bone_count*sizeof(*pose->bone_ids));
    }
    if(base->sound_policy) {
        const NativeFighterSoundPolicy* p=base->sound_policy;
        if(!nativeFighterSoundPolicy_Validate(p))return error(d,"Invalid donor sound policy");
        f.sound_rule_count=p->rule_count;f.ko_cue_count=p->ko_cue_count;
        if(p->rule_count)memcpy(f.sound_rules,p->rules,p->rule_count*sizeof(*f.sound_rules));
        if(p->ko_cue_count)memcpy(f.ko_cues,p->ko_cues,p->ko_cue_count*sizeof(*f.ko_cues));
    }
    if(!nativeSourceAssets_Validate(&f,d))return false;
    *output=f;return true;
}
