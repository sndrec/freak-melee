#include "native_script_binding_item_observations.h"
#include "native_script_bindings.h"
#include "fighter.h"
#include "ftparts.h"
#include "types.h"
#include <melee/it/native_item_lifetime.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_3F14.h>
#include <melee/it/itmaplib.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool source(NativeScriptRuntimeContext* c,NativeItemLifetime* s,
    NativeScriptDiagnostic* d)
{
    if(!c||c->source_class!=NativeCallbackSource_Item||!c->scene_epoch||
       !nativeItemLifetime_Capture(c->source,s)||s->reference.lifetime!=c->scene_epoch||
       s->generation!=c->generation||!s->source_owned||s->removing||
       !s->program||s->program->article!=s->article||s->program->kind!=s->kind||
       s->program->owner!=s->data->native_script_owner||
       s->program->author_kind!=s->data->native_article_author_kind)
        return error(d,"Item observation requires its live exact owned source");
    return true;
}
// Attachment parts are physical raw indices, not semantic bone names. The
// creator may use a stock or independently published Fighter program.
typedef struct AttachmentMatrix {
    Fighter_GObj* creator;
    Fighter_GObj* owner;
    NativeMatchPoolReference reference;
    Fighter* fighter;
    const ftData* fighter_data;
    FighterPartsTable** tables;
    FighterPartsTable* table;
    FighterBone* parts;
    HSD_JObj* joint;
    HSD_ClassInfo* joint_class;
    uint32_t generation,kind,count,part;
    int extent;
    bool retained;
} AttachmentMatrix;
static bool attachment_current(const NativeItemLifetime* s,const AttachmentMatrix* a)
{
    if(nativeScriptItem_Creator(s->object)!=a->creator||
       nativeScriptItem_AttachmentOwner(s->object)!=a->owner||s->data->xDC4!=a->part)return false;
    if(!a->retained)return true;
    return nativeMatchPool_Resolve(a->reference)==a->creator&&
        a->reference.lifetime==s->reference.lifetime&&
        a->creator->classifier==HSD_GOBJ_CLASS_FIGHTER&&a->creator->user_data==a->fighter&&
        nativeCallbackSource_Alive(a->creator,a->generation,NativeCallbackSource_Fighter)&&
        a->fighter->gobj==a->creator&&(uint32_t)a->fighter->kind==a->kind&&
        a->fighter->ft_data==a->fighter_data&&a->fighter->parts==a->parts&&
        ftPartsTable==a->tables&&(!a->tables||a->tables[a->kind]==a->table)&&
        (!a->table||a->table->parts_num==a->count)&&
        (!a->parts||(uint32_t)a->part>=a->count||a->parts[a->part].joint==a->joint)&&
        (!a->joint||(HSD_CLASS_METHOD(a->joint)==a->joint_class&&a->joint_class->head.obj_size==a->extent));
}
static bool attachment_capture(const NativeItemLifetime* s,AttachmentMatrix* a,
    NativeScriptDiagnostic* d)
{
    *a=(AttachmentMatrix){.creator=nativeScriptItem_Creator(s->object),
        .owner=nativeScriptItem_AttachmentOwner(s->object),.part=s->data->xDC4};
    if(!a->creator||a->creator!=a->owner)return true;
    if(!nativeMatchPool_Reference(&nativeGObjPool,a->creator,&a->reference)||
       a->reference.lifetime!=s->reference.lifetime||
       a->creator->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !nativeCallbackSource_Snapshot(a->creator,NativeCallbackSource_Fighter,&a->generation))
        return error(d,"Attachment rotation creator has no retained physical Fighter");
    a->fighter=a->creator->user_data;a->kind=(uint32_t)a->fighter->kind;
    if(a->fighter->gobj!=a->creator||a->kind>=Ft_Kind_RuntimeCapacity||a->kind==Ft_Kind_None)
        return error(d,"Attachment rotation creator has malformed physical Fighter data");
    a->fighter_data=a->fighter->ft_data;a->parts=a->fighter->parts;a->tables=ftPartsTable;
    a->table=a->tables?a->tables[a->kind]:NULL;a->count=a->table?a->table->parts_num:0;a->retained=true;
    if(a->count>MAX_FT_PARTS)return error(d,"Attachment rotation part count exceeds the native rig");
    if(a->parts&&(uint32_t)a->part<a->count)a->joint=a->parts[a->part].joint;
    if(a->joint) {
        a->joint_class=HSD_CLASS_METHOD(a->joint);a->extent=a->joint_class?a->joint_class->head.obj_size:0;
        if(!a->joint_class||a->extent<(int)sizeof(*a->joint)||
           !hsdObjIsDescendantOf((HSD_Obj*)a->joint,HSD_CLASS_INFO(&hsdJObj)))
            return error(d,"Attachment rotation requires an admitted native JObj");
    }
    return nativeItemLifetime_Current(s)&&attachment_current(s,a)?true:
        error(d,"Attachment rotation source, relation or rig changed during admission");
}
bool nativeScriptBindingItemObservations_Invoke(NativeScriptRuntimeContext* c,
    uint32_t semantic,NativeScriptValue* args,uint32_t count,
    NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_ItemReadAttachmentMatrixRotation) {
        if(!out||!args||count!=3||args[0].type!=NativeScriptType_F32||
           args[1].type!=NativeScriptType_F32||args[2].type!=NativeScriptType_F32)
            return error(d,"Attachment rotation requires three Float outputs");
        NativeItemLifetime s;AttachmentMatrix attachment;Vec3 rotation={0};
        if(!source(c,&s,d)||!attachment_capture(&s,&attachment,d))return false;
        // Sample the existing matrix. This does not evaluate SRT, constraints,
        // animations or dirty matrices, and it performs no matrix setup.
        if(attachment.joint)HSD_MtxGetRotation(attachment.joint->mtx,&rotation);
        if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
           c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||
           c->scene_epoch!=s.reference.lifetime||!attachment_current(&s,&attachment))
            return error(d,"Attachment rotation source, relation or rig retired before publication");
        args[0].as.f32=rotation.x;args[1].as.f32=rotation.y;args[2].as.f32=rotation.z;
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_ItemSweepStage||semantic==NativeScriptBinding_ItemSweepStageNormal) {
        if(!out||!args||count!=3||args[0].type!=NativeScriptType_F32||
           args[1].type!=NativeScriptType_F32||args[2].type!=NativeScriptType_F32)
            return error(d,"Item stage sweep requires three Float coordinates");
        NativeItemLifetime s;if(!source(c,&s,d))return false;
        Vec3 previous={args[0].as.f32,args[1].as.f32,args[2].as.f32};
        if(!isfinite(previous.x)||!isfinite(previous.y)||!isfinite(previous.z)||
           !isfinite(s.data->pos.x)||!isfinite(s.data->pos.y)||!isfinite(s.data->pos.z))
            return error(d,"Item stage sweep requires finite previous/current positions");
        bool collect_normal=semantic==NativeScriptBinding_ItemSweepStageNormal;
        if(collect_normal&&s.program->recipe)return error(d,"Surface normal sweep requires ordinary Article storage");
        Vec3 normal=s.data->xDD4_itemVar.native_article.surface_normal;
        bool hit=it_8026E9A4(s.object,&previous,&s.data->pos,collect_normal?&normal:NULL);
        if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
           c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||
           c->scene_epoch!=s.reference.lifetime)
            return error(d,"Item stage sweep source retired before publication");
        if(collect_normal)s.data->xDD4_itemVar.native_article.surface_normal=normal;
        *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=hit};return true;
    }
    if(!out||count)
        return error(d,"Item observation differs from its zero-argument signature");
    NativeItemLifetime s;NativeScriptValue value={.type=NativeScriptType_Void};
    if(!source(c,&s,d))return false;
    switch(semantic) {
    case NativeScriptBinding_ItemCancelHitlagRequest:
        // Clear the pending Item hitlag request, not an active hitlag timer.
        s.data->xCA8=0;break;
    case NativeScriptBinding_ItemDisableSurfaceResponse:
        s.data->xD5C=0;break;
    case NativeScriptBinding_ItemHitsStage:
        if(s.data->x378_itemColl.x0_gobj!=s.object||s.data->x378_itemColl.x34_flags.b1234==1||
           !s.attributes||!it_804D6D28)return error(d,"Item terrain sweep requires its native collision actor and attributes");
        value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=it_8026E058(c->source)};break;
    case NativeScriptBinding_ItemCheckGround:
        value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=it_8026DA70(c->source)};break;
    case NativeScriptBinding_ItemSweepStageFlags:
        value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=it_8026DAA8(c->source)};
        break;
    case NativeScriptBinding_ItemSweepFloor:
        // mpCollPrev clears the optional fighter floor callback before this
        // native sweep; no authored callback is installed by this operation.
        it_8026E0F4(c->source);break;
    case NativeScriptBinding_ItemWallContactFlags:
        value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=it_80276308(c->source)};
        break;
    case NativeScriptBinding_ItemCeilingContactFlags:
        value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=it_802763E0(c->source)};
        break;
    case NativeScriptBinding_ItemReflectFromWall:
        if((s.data->x378_itemColl.env_flags&(Collide_LeftWallMask|Collide_RightWallMask))&&!s.attributes)
            return error(d,"Wall reflection requires the Item's native attributes");
        value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=it_8027770C(c->source)};
        break;
    case NativeScriptBinding_ItemAlignToFloor:
        if(s.data->x378_itemColl.env_flags&Collide_FloorMask) {
            const ItemModelDesc* model=s.article?s.article->x10_modelDesc:NULL;
            struct DynamicBoneTable* bones=s.data->xBBC_dynamicBoneTable;
            if(!model||(model->x4_bone_count&&!bones))
                return error(d,"Floor alignment requires the Item's native bone table");
            HSD_JObj* joint=it_80272CC0(c->source,0);
            HSD_ClassInfo* joint_class=joint?HSD_CLASS_METHOD(joint):NULL;
            int extent=joint_class?joint_class->head.obj_size:0;
            if(!joint||s.object_kind!=HSD_GObj_JObjKind||!joint_class||
               extent<(int)sizeof(*joint)||
               !hsdObjIsDescendantOf((HSD_Obj*)joint,HSD_CLASS_INFO(&hsdJObj)))
                return error(d,"Floor alignment requires an admitted native JObj");
            if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
               c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||
               c->scene_epoch!=s.reference.lifetime||s.article->x10_modelDesc!=model||
               s.data->xBBC_dynamicBoneTable!=bones||
               it_80272CC0(c->source,0)!=joint||HSD_CLASS_METHOD(joint)!=joint_class||
               joint_class->head.obj_size!=extent)
                return error(d,"Floor alignment graph changed during class admission");
        }
        it_80276CB8(c->source);break;
    case NativeScriptBinding_ItemAlignFacingToVelocity:
        it_80272980(c->source);break;
    default:return error(d,"Unsupported Item observation operation");
    }
    if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
       c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||
       c->scene_epoch!=s.reference.lifetime)
        return error(d,"Item observation source retired before publication");
    *out=value;return true;
}
bool nativeScriptBindingItemObservations_Read(NativeScriptRuntimeContext* c,
    uint32_t semantic,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool floor_line = semantic == NativeScriptBinding_FloorLine;
    bool normal=semantic>=NativeScriptBinding_FloorNormalX && semantic<=NativeScriptBinding_FloorNormalZ;
    bool collision_position=semantic>=NativeScriptBinding_CollisionPositionX && semantic<=NativeScriptBinding_CollisionPositionZ;
    bool ceiling=(semantic>=NativeScriptBinding_CeilingNormalX&&semantic<=NativeScriptBinding_CeilingNormalY)||semantic==NativeScriptBinding_CeilingNormalZ;
    bool left=(semantic>=NativeScriptBinding_WallLeftNormalX&&semantic<=NativeScriptBinding_WallLeftNormalY)||semantic==NativeScriptBinding_WallLeftNormalZ;
    bool right=(semantic>=NativeScriptBinding_WallRightNormalX&&semantic<=NativeScriptBinding_WallRightNormalY)||semantic==NativeScriptBinding_WallRightNormalZ;
    bool launch=semantic>=NativeScriptBinding_ItemLaunchAngle&&semantic<=NativeScriptBinding_ItemLaunchOriginZ;
    bool reflected=semantic==NativeScriptBinding_ItemReflectionFacing;
    bool surface=semantic>=NativeScriptBinding_ItemSurfaceNormalX&&semantic<=NativeScriptBinding_ItemSurfaceNormalZ;
    if (!out || (!floor_line && !normal && !collision_position && !ceiling &&
                 !left && !right && !launch && !reflected && !surface &&
                 semantic != NativeScriptBinding_ItemAnimationFinished &&
                 semantic != NativeScriptBinding_FloorFriction))
        return error(d, "Unsupported Item observation property");
    NativeItemLifetime s;if(!source(c,&s,d))return false;
    if(surface&&s.program->recipe)return error(d,"Surface normal observation requires ordinary Article storage");
    if(launch||reflected||surface) {
        const float* component;
        switch(semantic) {
        case NativeScriptBinding_ItemLaunchAngle:component=&s.data->xDD4_itemVar.native_article.launch_angle;break;
        case NativeScriptBinding_ItemLaunchSpeed:component=&s.data->xDD4_itemVar.native_article.launch_speed;break;
        case NativeScriptBinding_ItemLaunchOriginX:component=&s.data->xDD4_itemVar.native_article.launch_origin.x;break;
        case NativeScriptBinding_ItemLaunchOriginY:component=&s.data->xDD4_itemVar.native_article.launch_origin.y;break;
        case NativeScriptBinding_ItemLaunchOriginZ:component=&s.data->xDD4_itemVar.native_article.launch_origin.z;break;
        case NativeScriptBinding_ItemReflectionFacing:component=&s.data->xC68;break;
        case NativeScriptBinding_ItemSurfaceNormalX:component=&s.data->xDD4_itemVar.native_article.surface_normal.x;break;
        case NativeScriptBinding_ItemSurfaceNormalY:component=&s.data->xDD4_itemVar.native_article.surface_normal.y;break;
        case NativeScriptBinding_ItemSurfaceNormalZ:component=&s.data->xDD4_itemVar.native_article.surface_normal.z;break;
        default:return error(d,"Item launch observation requires a scalar component");
        }
        NativeScriptValue value={.type=NativeScriptType_F32};
        memcpy(&value.as.f32,component,sizeof(value.as.f32));
        if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
           c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||
           c->scene_epoch!=s.reference.lifetime)
            return error(d,"Item launch/reflection source retired before publication");
        *out=value;return true;
    }
    if(semantic==NativeScriptBinding_FloorFriction) {
        uint32_t flags=s.data->x378_itemColl.floor.flags;
        if((uint8_t)flags>=20)return error(d,"Floor material index exceeds its native table");
        NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=mpLib_800569EC(flags)};
        if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
           c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||c->scene_epoch!=s.reference.lifetime)
            return error(d,"Item floor material source retired before publication");
        *out=value;return true;
    }
    if(floor_line||normal||collision_position||ceiling||left||right) {
        const CollData* collision=&s.data->x378_itemColl;
        NativeScriptValue value;
        if (floor_line) {
            value = (NativeScriptValue){.type = NativeScriptType_I32,
                                        .as.i64 = collision->floor.index};
        } else {
            const Vec3* vector=normal?&collision->floor.normal:ceiling?&collision->ceiling.normal:
                left?&collision->left_facing_wall.normal:right?&collision->right_facing_wall.normal:&collision->cur_pos;
            unsigned first=normal?NativeScriptBinding_FloorNormalX:ceiling?NativeScriptBinding_CeilingNormalX:
                left?NativeScriptBinding_WallLeftNormalX:right?NativeScriptBinding_WallRightNormalX:NativeScriptBinding_CollisionPositionX;
            unsigned axis=(semantic==NativeScriptBinding_CeilingNormalZ||semantic==NativeScriptBinding_WallLeftNormalZ||semantic==NativeScriptBinding_WallRightNormalZ)?2u:semantic-first;
            const float* component=axis==0?&vector->x:axis==1?&vector->y:&vector->z;
            value = (NativeScriptValue){.type = NativeScriptType_F32};
            memcpy(&value.as.f32,component,sizeof(value.as.f32));
        }
        if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
           c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||
           c->scene_epoch!=s.reference.lifetime)
            return error(d,"Item collision observation source retired before publication");
        *out=value;return true;
    }
    // The native query treats a missing graph as inactive. Admit actual
    // JObj subclasses with sufficient native object extent when present.
    HSD_JObj* root=(HSD_JObj*)s.model;
    HSD_ClassInfo* root_class=root?HSD_CLASS_METHOD(root):NULL;
    int extent=root_class?root_class->head.obj_size:0;
    if(root&&(s.object_kind!=HSD_GObj_JObjKind||!root_class||
       extent<(int)sizeof(*root)||
       !hsdObjIsDescendantOf((HSD_Obj*)root,HSD_CLASS_INFO(&hsdJObj))))
        return error(d,"Item animation observation has no admitted primary JObj");
    // Class admission can initialize engine class metadata. Recheck source
    // and the admitted class before entering the native graph traversal.
    if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
       c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||
       c->scene_epoch!=s.reference.lifetime||
       (root&&(HSD_CLASS_METHOD(root)!=root_class||root_class->head.obj_size!=extent)))
        return error(d,"Item primary graph changed during class admission");
    bool finished=!it_80272C6C(c->source);
    if(!nativeItemLifetime_Current(&s)||c->source!=s.object||
       c->source_class!=NativeCallbackSource_Item||c->generation!=s.generation||
       c->scene_epoch!=s.reference.lifetime||
       (root&&(HSD_CLASS_METHOD(root)!=root_class||root_class->head.obj_size!=extent)))
        return error(d,"Item animation observation source retired before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=finished};return true;
}

bool nativeScriptBindingItemObservations_Write(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(!value||value->type!=NativeScriptType_F32||semantic<NativeScriptBinding_ItemSurfaceNormalX||
       semantic>NativeScriptBinding_ItemSurfaceNormalZ)return error(d,"Surface normal assignment requires a Float component");
    NativeItemLifetime s;if(!source(c,&s,d))return false;
    if(s.program->recipe)return error(d,"Surface normal assignment requires ordinary Article storage");
    Vec3* normal=&s.data->xDD4_itemVar.native_article.surface_normal;
    float* component=semantic==NativeScriptBinding_ItemSurfaceNormalX?&normal->x:
        semantic==NativeScriptBinding_ItemSurfaceNormalY?&normal->y:&normal->z;
    memcpy(component,&value->as.f32,sizeof(value->as.f32));return true;
}
