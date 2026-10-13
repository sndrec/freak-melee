#include "native_holdable.h"
#include "native_script_item_actor.h"
#include "inlines.h"
#include "item.h"
#include "it_2725.h"
#include "itcoll.h"
#include "itgroundcoll.h"
#include "itmaplib.h"
#include "kinds/inlines.h"
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

typedef struct HoldableSource {
    Item_GObj* object;
    NativeMatchPoolReference reference;
    const NativeItemDefinition* recipe;
    uint32_t generation;
} HoldableSource;

static NativeScriptVMStatus begin(Item_GObj* object,NativeScriptValue* result,HoldableSource* source)
{
    if(!result)return NativeScriptVM_Error;
    NativeScriptVMStatus status=nativeScriptItem_Status(object);
    if(status!=NativeScriptVM_Ok)return status;
    const NativeScriptItemProgram* program=nativeScriptItem_Spec(object);
    if(!program || program->kind!=It_Kind_NativeHoldable || !program->recipe ||
        !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&source->generation) ||
        !nativeMatchPool_Reference(&nativeGObjPool,object,&source->reference))return NativeScriptVM_Error;
    source->object=object;source->recipe=program->recipe;
    return NativeScriptVM_Ok;
}
static NativeScriptVMStatus current(const HoldableSource* source)
{
    if(nativeMatchPool_Resolve(source->reference)!=source->object ||
       !nativeCallbackSource_Alive(source->object,source->generation,NativeCallbackSource_Item))return NativeScriptVM_SourceRetired;
    return nativeScriptItem_Status(source->object);
}
NativeScriptVMStatus nativeHoldable_Animation(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    Item* item=GET_ITEM(object);bool expire=false;
    if(item->msid!=0) {
        ++item->xDD4_itemVar.native_holdable.age;
        NativeScriptValue value;
        status=nativeScriptItem_RunHook(object,NativeScriptItemHook_WorkshopTick,NULL,0,&value);
        if(status!=NativeScriptVM_Ok)return status;
        status=current(&source);if(status!=NativeScriptVM_Ok)return status;
        expire=source.recipe->lifetime_frames && item->xDD4_itemVar.native_holdable.age>=source.recipe->lifetime_frames;
    }
    *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=expire};return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeHoldable_Physics(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    Item* item=GET_ITEM(object);
    if((item->msid==2 || item->msid==3) && !nativeScriptItem_HookBound(object,NativeScriptItemHook_WorkshopTick))Item_ApplyFallingPhysics(object);
    status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeHoldable_Collision(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    Item* item=GET_ITEM(object);
    // The actual shared state table has no collision callback while held or
    // resting. Source hooks do not make those neutral phases active.
    if(item->msid!=2 && item->msid!=3) {
        *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return NativeScriptVM_Ok;
    }
    int contact=it_8026DAA8(object);
    status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    item->xDD4_itemVar.native_holdable.collision_flags=(u16)contact;
    if(nativeScriptItem_HookBound(object,NativeScriptItemHook_WorkshopGroundCollision))
        return nativeScriptItem_RunHook(object,NativeScriptItemHook_WorkshopGroundCollision,NULL,0,result);
    if(contact&0xE){
        it_80276FC4(object,contact);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
        Item_80268E5C(object,1,ITEM_ANIM_UPDATE);
    }
    status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=(contact&0xE)?0:(contact&3)!=0};return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeHoldable_PickedUp(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    Item_80268E5C(object,0,ITEM_ANIM_UPDATE);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    it_80273670(object,0,0.0f);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    Item_802694CC(object);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeHoldable_Dropped(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    GET_ITEM(object)->xDD4_itemVar.native_holdable.age=0;
    Item_80268E5C(object,3,ITEM_ANIM_UPDATE|ITEM_DROP_UPDATE);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    it_80273670(object,0,0.0f);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeHoldable_Thrown(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    const NativeItemDefinition* definition=source.recipe;
    Item* item=GET_ITEM(object);item->xDD4_itemVar.native_holdable.age=0;
    Item_80268E5C(object,2,ITEM_ANIM_UPDATE|ITEM_DROP_UPDATE);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    it_80273670(object,0,0.0f);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    HitCapsule* hitbox=&item->x5D4_hitboxes[0].hit;
    hitbox->state=HitCapsule_Enabled;hitbox->x4=0;it_8026FCF8(item,hitbox);
    status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    hitbox->jobj=GET_JOBJ(object);hitbox->b_offset=(Vec3){0};hitbox->scale=definition->hitbox_size/item->scl;
    hitbox->kb_angle=definition->angle;hitbox->x24=definition->knockback_growth;hitbox->x28=0;hitbox->x2C=definition->base_knockback;
    hitbox->element=HitElement_Normal;hitbox->x40_b0=1;hitbox->x40_b2=1;hitbox->x40_b3=1;
    hitbox->x42_b5=1;hitbox->x42_b7=1;hitbox->x43_b2=0;
    item->xDC8_word.flags.x16=1;item->xDAA_flag.b2=0;it_80272460(hitbox,definition->damage,object);
    status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    return nativeScriptItem_RunHook(object,NativeScriptItemHook_WorkshopThrown,NULL,0,result);
}
NativeScriptVMStatus nativeHoldable_Hit(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    if(nativeScriptItem_HookBound(object,NativeScriptItemHook_WorkshopHit))
        return nativeScriptItem_RunHook(object,NativeScriptItemHook_WorkshopHit,NULL,0,result);
    itColl_BounceOffVictim(object);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    Item_80268E5C(object,1,ITEM_ANIM_UPDATE);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeHoldable_Clanked(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    itColl_BounceOffVictim(object);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeHoldable_Reflected(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    it_80273030(object);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return NativeScriptVM_Ok;
}
NativeScriptVMStatus nativeHoldable_ShieldBounced(Item_GObj* object,NativeScriptValue* result)
{
    HoldableSource source;NativeScriptVMStatus status=begin(object,result,&source);
    if(status!=NativeScriptVM_Ok)return status;
    itColl_BounceOffShield(object);status=current(&source);if(status!=NativeScriptVM_Ok)return status;
    *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return NativeScriptVM_Ok;
}
