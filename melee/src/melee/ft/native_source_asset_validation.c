#include "native_source_assets.h"
#include "native_fighter_pose.h"
#include "native_fighter_sound_policy.h"
#include "kinds/ftCommon/forward.h"
#include <stdio.h>
#include <string.h>
static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool name(const char* value,bool optional)
{
    unsigned length=0;
    while(length<NATIVE_SOURCE_NAME_BYTES && value[length]) {
        if((unsigned char)value[length]<33 || (unsigned char)value[length]>126)return false;
        ++length;
    }
    return length<NATIVE_SOURCE_NAME_BYTES && (length||optional);
}
bool nativeSourceAssets_Validate(const NativeSourceAssetFacts* f,NativeScriptDiagnostic* d)
{
    if(!f || !name(f->fighter_root,false) || !name(f->costume_joint,false) ||
       !name(f->costume_matanim,true) || !f->actions || f->actions>1024 ||
       !f->demo_count || f->demo_count>128 || !f->joints ||
       f->joints>MAX_FT_PARTS || !f->bone_count || f->bone_count>FT_COMMON_PART_COUNT ||
       f->omission_count>NATIVE_SOURCE_OMISSIONS || f->joints+f->omission_count>MAX_FT_PARTS ||
       f->article_count>NATIVE_SOURCE_ARTICLES || f->pose_count>NATIVE_SOURCE_POSE_GRAPHS ||
       f->sound_rule_count>NATIVE_FIGHTER_SOUND_RULE_CAPACITY || f->ko_cue_count>NATIVE_FIGHTER_KO_CUE_CAPACITY ||
       f->animation_tag>=Ft_Kind_StockCount || (f->effect_bank!=UINT8_MAX && f->effect_bank>=64) ||
       f->sound_mute_channels>127 || (f->sound_banks>>55) ||
       !ftCharacterAssetLayout_IsValid(&f->layout) || f->moves.common_count!=ftCo_MS_Count ||
       f->moves.count>NATIVE_SOURCE_MOTIONS || !nativeScriptMoves_Validate(&f->moves,d))
        return error(d,"Source asset facts have invalid roots, counts, dependency flags or owned motion layout");
    for(unsigned i=0;i<4;++i)if(!name(f->demos[i],true))return error(d,"Invalid owned demo root name");
    unsigned parts=f->joints+f->omission_count;
    for(unsigned i=0;i<f->bone_count;++i) {
        if((unsigned)f->bones[i].part>=FT_COMMON_PART_COUNT || f->bones[i].joint>=parts)
            return error(d,"Source semantic bone is outside its owned rig");
        for(unsigned j=0;j<i;++j)if(f->bones[j].part==f->bones[i].part)
            return error(d,"Duplicate source semantic bone mapping");
    }
    const unsigned required[]={FtPart_TopN,FtPart_TransN,FtPart_XRotN,FtPart_YRotN,
        FtPart_HipN,FtPart_TransN2,FT_COMMON_PART_COUNT-1};
    for(unsigned i=0;i<sizeof(required)/sizeof(*required);++i) {
        bool present=false;
        for(unsigned j=0;j<f->bone_count;++j)if((unsigned)f->bones[j].part==required[i])present=true;
        if(!present)return error(d,"Source rig is missing a required engine bone role");
    }
    for(unsigned i=0;i<f->omission_count;++i) {
        const NativeFighterPartOmission* row=&f->omissions[i];
        if(row->joint>=parts || row->parent>=parts || row->insertion_mode>3 ||
           (row->donor_joint!=UINT8_MAX && row->donor_joint>=f->joints))return error(d,"Invalid source part insertion");
        for(unsigned j=0;j<i;++j)if(f->omissions[j].joint==row->joint)return error(d,"Duplicate source part insertion");
    }
    for(unsigned i=0;i<f->article_count;++i) {
        const NativeSourceArticleAsset* row=&f->articles[i];
        if(!name(row->declaration,false) || row->kind<0 || row->kind>It_Kind_NativeHoldable || row->slot>=FT_CHARACTER_ITEM_CAPACITY ||
           !(f->layout.article_mask&(1u<<row->slot)) || row->kind_attribute_offset < -1 ||
           (row->kind_attribute_offset>=0 && (row->kind_attribute_offset%4 ||
            row->kind_attribute_offset>FT_CHARACTER_EXT_ATTR_CAPACITY-4)))return error(d,"Invalid source article asset mapping");
        for(unsigned j=0;j<i;++j)if(!strcmp(f->articles[j].declaration,row->declaration))
            return error(d,"Duplicate source article declaration name");
        if(row->kind_attribute_offset>=0)for(unsigned b=0;b<f->layout.byte_range_count;++b) {
            const FtCharacterByteRange* range=&f->layout.byte_ranges[b];unsigned offset=(unsigned)row->kind_attribute_offset;
            if(offset<range->offset+range->size && range->offset<offset+4)
                return error(d,"Article kind selects raw byte data instead of a decoded scalar word");
        }
    }
    for(unsigned i=0;i<f->pose_count;++i) {
        const NativeSourcePoseGraph* row=&f->poses[i];
        if(!row->name || row->slot>=FT_CHARACTER_ITEM_CAPACITY ||
           !(f->layout.joint_graph_mask&(1u<<row->slot)) || !name(row->path,true) ||
           !nativeFighterPose_PathValid(row->path) || !row->bone_count || row->bone_count>MAX_FT_PARTS)
            return error(d,"A pose needs a valid graph and a bone ID for each descriptor.");
        for(unsigned b=0;b<row->bone_count;++b){
            if(row->bone_ids[b]<0)return error(d,"A pose target needs a nonnegative bone ID.");
            for(unsigned j=0;j<b;++j)if(row->bone_ids[b]==row->bone_ids[j])
                return error(d,"A pose cannot target the same bone twice.");
        }
        for(unsigned j=0;j<i;++j)if(f->poses[j].name==row->name)return error(d,"Duplicate source pose graph identity");
    }
    for(unsigned i=0;i<f->moves.count;++i)if(f->motions[i].animation < -1 ||
       f->motions[i].animation>=f->actions || f->motions[i].camera>1)return error(d,"Invalid source numeric motion record");
    NativeFighterSoundPolicy sound={f->sound_rule_count?f->sound_rules:NULL,
        f->ko_cue_count?f->ko_cues:NULL,f->sound_rule_count,f->ko_cue_count};
    if(!nativeFighterSoundPolicy_Validate(&sound))return error(d,"Invalid source sound data policy");
    return true;
}
