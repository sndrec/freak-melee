#include "native_source_assets.h"
#include "native_source_rig_validation.h"
#include "fighter.h"
#include "ftanimation_bank.h"
#include "ftcamera.h"
#include "native_fighter_pose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Counts from the supported disc also allow validation in the standalone Workshop tool.
static const struct { u8 parts, insertions; } stock_rigs[] = {
#include <melee/ft/stock_rig_sizes.inc>
};
_Static_assert(sizeof(stock_rigs) / sizeof(stock_rigs[0]) == Ft_Kind_None + 1,
               "Include every stock fighter and the shared throw skeleton");

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
void nativeSourceAssets_Release(NativeSourceImportedAssets* assets)
{
    if(!assets)return;
    free(assets->motions);free(assets->costume.arena);free(assets->animation_arena);free(assets->character_arena);
    memset(assets,0,sizeof(*assets));
}
bool nativeSourceAssets_Import(const NativeSourceAssetFacts* facts,const NativeSourceAssetInput* input,
    FighterKind target,NativeSourceImportedAssets* output,NativeScriptDiagnostic* d)
{
    if(!output || output->ready || output->character_arena || output->animation_arena || output->costume.arena || output->motions)
        return error(d,"Source asset import requires a vacant candidate at its stable address");
    if(!input || !input->fighter || !input->animation || !input->costume ||
       (unsigned)target>=Ft_Kind_RuntimeCapacity || target==Ft_Kind_None || !nativeSourceAssets_Validate(facts,d))return false;
    if((unsigned)target>=64)
        return error(d,"Source target does not fit the native six-bit animation family field");
    // All self-referential imported records are constructed in their final
    // owner. A failed candidate cannot overwrite a separately accepted owner.
    output->facts=*facts;facts=&output->facts;
    output->character_arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);
    output->animation_arena=malloc(FT_ANIMATION_IMPORT_ARENA_SIZE);
    output->motions=calloc(facts->moves.count?facts->moves.count:1,sizeof(*output->motions));
    if(!output->character_arena || !output->animation_arena || !output->motions) {
        error(d,"Could not reserve source asset candidate storage");goto failure;
    }
    if(!ftCharacterAssets_Import(input->fighter,input->fighter_bytes,&facts->layout,
            facts->fighter_root,facts->actions,facts->demo_count,1,output->character_arena,
            FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,&output->character)) {
        if(d)snprintf(d->message,sizeof(d->message),"Source fighter DAT graph import failed at 0x%X (status %u)",
            (unsigned)output->character.import.error_offset,(unsigned)output->character.import.status);goto failure;
    }
    char rig_error[192];
    if (!nativeFighterRig_Read(&output->character.dat, facts->joints + facts->omission_count,
                              &output->rig, rig_error, sizeof(rig_error))) {
        error(d,rig_error);goto failure;
    }
    FtAnimationBankStats animation;
    const FtAnimationBankTable tables[]={
        {output->character.data.xC,facts->actions,false},
        {output->character.data.x14,facts->demo_count,true},
    };
    if(!ftAnimationBank_Import(input->animation,input->animation_bytes,tables,2,
            output->animation_arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&animation)) {
        if(d)snprintf(d->message,sizeof(d->message),"Source animation import failed at table %u clip %u (status %u)",
            (unsigned)animation.failed_table,(unsigned)animation.failed_index,(unsigned)animation.status);goto failure;
    }
    if(!ftCostumeAssets_Load(input->costume,input->costume_bytes,facts->costume_joint,
            facts->costume_matanim,&output->costume) || output->costume.joint_count!=facts->joints ||
       !ftCharacterAssets_ValidateVisibility(&output->character,facts->layout.visibility_mask,output->costume.joint)) {
        error(d,"Source costume graph, rig count or visibility data is invalid");goto failure;
    }
    FtCostumeBindings bindings;
    if (!ftCostumeBindings_FromCharacter(&output->character, facts->joints, facts->layout.visibility_mask, 0, &bindings) ||
        !ftCostumeBindings_Fit(&bindings, &output->costume)) {
        error(d,"The fighter's base costume does not contain its required meshes or animated texture layers.");goto failure;
    }
    for(unsigned i=0;i<facts->pose_count;++i)if(!nativeFighterPose_BindGraph(&output->character,
            facts->poses[i].slot,facts->poses[i].path,&output->rig,facts->poses[i].bone_ids,
            facts->poses[i].bone_count,&output->poses[i])) {
        error(d,"A pose's descriptor count or target bone IDs do not match its owned assets.");goto failure;
    }
    u32 root,attribute_begin;const char* external;
    if(!HSD_DatFindRoot(&output->character.dat,facts->fighter_root,&root) ||
       HSD_DatResolve(&output->character.dat,root+4,&attribute_begin,&external)!=HSD_DAT_REF_LOCAL) {
        error(d,"Source article kind has no owned attribute block");goto failure;
    }
    u32 attribute_end=HSD_DatImportNextTarget(&output->character.import,attribute_begin);
    if(attribute_end<attribute_begin){error(d,"Source attribute block bounds are invalid");goto failure;}
    size_t attribute_bytes=attribute_end-attribute_begin;
    if(attribute_bytes>FT_CHARACTER_EXT_ATTR_CAPACITY)attribute_bytes=FT_CHARACTER_EXT_ATTR_CAPACITY;
    attribute_bytes&=~(size_t)3;
    for(unsigned i=0;i<facts->article_count;++i) {
        const NativeSourceArticleAsset* row=&facts->articles[i];ItemKind kind=(ItemKind)row->kind;
        if(row->kind_attribute_offset>=0) {
            size_t offset=(size_t)row->kind_attribute_offset;
            if(offset>attribute_bytes || sizeof(kind)>attribute_bytes-offset) {
                error(d,"Source article kind exceeds this DAT's actual attribute block");goto failure;
            }
            memcpy(&kind,output->character.extended_attributes+offset,sizeof(kind));
        }
        if(kind<0 || kind>It_Kind_NativeHoldable || !output->character.fighter_items[row->slot]) {error(d,"Source article asset or decoded kind is absent");goto failure;}
        output->article_kinds[i]=kind;
    }
    struct Fighter_WaitAnimData* banks[]={output->character.data.xC,output->character.data.x14};
    const unsigned counts[]={facts->actions,facts->demo_count};
    for(unsigned b=0;b<2;++b)for(unsigned i=0;banks[b]&&i<counts[b];++i) {
        struct Fighter_WaitAnimData* clip=&banks[b][i];
        // ftCharacterAssets_Import has already converted the PPC word to the
        // native Fighter union: its six-bit animation family is at bits 26..31.
        if (!clip->x14) continue;
        unsigned family = (u32)clip->x10_animCurrFlags >> 26;
        if (family > Ft_Kind_None) {
            error(d,"Animation refers to a skeleton outside the base game and its own package");goto failure;
        }
        bool own = family == facts->animation_tag;
        unsigned parts = own ? facts->joints + facts->omission_count : stock_rigs[family].parts;
        unsigned insertions = own ? facts->omission_count : stock_rigs[family].insertions;
        if (!own && ftPartsTable) {
            if (!ftPartsTable[family] || !Fighter_804D6540) {
                error(d, "The borrowed skeleton is missing from the loaded fighter tables");
                goto failure;
            }
            parts = ftPartsTable[family]->parts_num;
            insertions = Fighter_804D6540[family] ? Fighter_804D6540[family]->x4 : 0;
        }
        if (!ftAnimationBank_FitsRig(clip, parts, insertions)) {
            if (d) snprintf(d->message, sizeof(d->message), "Animation %.90s does not fit %s skeleton",
                            clip->x0, own ? "this fighter's" : "its borrowed");
            goto failure;
        }
        if (own) {
            clip->x10_animCurrFlags=(int32_t)(((uint32_t)clip->x10_animCurrFlags&UINT32_C(0x03ffffff))|((uint32_t)target<<26));
        }
    }
    for(unsigned i=0;i<facts->moves.count;++i) {
        const NativeSourceMotionData* row=&facts->motions[i];MotionState* motion=&output->motions[i];
        motion->anim_id=row->animation;motion->x4_flags=(enum_t)row->flags;motion->_=row->attack;
        // This is an audited common camera primitive. All authored phases are
        // absent machine pointers and must be selected through the script map.
        if(row->camera)motion->cam_cb=ftCamera_UpdateCameraBox;
    }
    memset(output->joint_to_part,FTPART_INVALID,sizeof(output->joint_to_part));
    memset(output->part_to_joint,FTPART_INVALID,sizeof(output->part_to_joint));
    for(unsigned i=0;i<facts->bone_count;++i) {
        unsigned part=(unsigned)facts->bones[i].part,joint=facts->bones[i].joint;
        output->part_to_joint[part]=(u8)joint;output->joint_to_part[joint]=(u8)part;
    }
    output->parts=(FighterPartsTable){output->joint_to_part,output->part_to_joint,facts->joints+facts->omission_count};
    for(unsigned i=0;i<facts->omission_count;++i) {
        const NativeFighterPartOmission* row=&facts->omissions[i];
        output->omission_rows[i]=(struct Fighter_804D6540_x0_t){row->joint,row->parent,row->insertion_mode,row->donor_joint};
    }
    output->omissions.x0=output->omission_rows;output->omissions.x4=facts->omission_count;
    output->demos=(Fighter_DemoStrings){output->facts.demos[0],output->facts.demos[1],output->facts.demos[2],output->facts.demos[3]};
    if (!nativeSourceAssets_ValidateRig(output, d)) goto failure;
    output->ready=true;return true;
failure:
    nativeSourceAssets_Release(output);return false;
}
