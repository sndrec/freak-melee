#include "native_script_binding_item.h"
#include "native_script_bindings.h"
#include "fighter.h"
#include "ftchangeparam.h"
#include "types.h"
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false;}

static bool holdable(Item* item,NativeScriptDiagnostic* diagnostic)
{return item->kind==It_Kind_NativeHoldable?true:error(diagnostic,"This property requires the native holdable storage schema.");}

static bool response_mask(const NativeScriptValue* value,int32_t* output,NativeScriptDiagnostic* d)
{
    uint32_t bits;
    switch(value->type) {
    case NativeScriptType_Bool:
        if(value->as.u64>1)return error(d,"Stage response mask contains a malformed Bool");
        bits=(uint32_t)value->as.u64;break;
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(value->as.i64<INT32_MIN || value->as.i64>UINT32_MAX)
            return error(d,"Stage response mask exceeds native 32-bit width");
        if(value->type==NativeScriptType_I32 && value->as.i64>INT32_MAX)
            return error(d,"Stage response mask contains a malformed Int");
        bits=(uint32_t)value->as.i64;break;
    case NativeScriptType_U32:case NativeScriptType_U64:
        if(value->as.u64>UINT32_MAX)return error(d,"Stage response mask exceeds native 32-bit width");
        bits=(uint32_t)value->as.u64;break;
    default:return error(d,"Stage response mask must be integral");
    }
    memcpy(output,&bits,sizeof(bits));return true;
}

typedef struct ResponseOwner {
    HSD_GObj* object;
    NativeMatchPoolReference reference;
    const void* data;
    uint32_t classifier,generation,kind;
    bool observed;
} ResponseOwner;
static bool response_owner_current(const ResponseOwner* owner,uint64_t epoch)
{
    if(!owner->object)return true;
    if(nativeMatchPool_Resolve(owner->reference)!=owner->object || owner->reference.lifetime!=epoch ||
       owner->object->classifier!=owner->classifier || owner->object->user_data!=owner->data)return false;
    if(owner->classifier!=HSD_GOBJ_CLASS_FIGHTER)return true;
    const Fighter* fighter=owner->data;
    return nativeCallbackSource_Alive(owner->object,owner->generation,NativeCallbackSource_Fighter) &&
        fighter->gobj==owner->object && (uint32_t)fighter->kind==owner->kind;
}
static bool response_damage(const NativeItemLifetime* source,ResponseOwner* owner,NativeScriptDiagnostic* d)
{
    for(unsigned i=0;i<4;++i)if(source->data->x5D4_hitboxes[i].hit.state!=HitCapsule_Disabled) {
        owner->observed=true;break;
    }
    if(!owner->observed)return true;
    owner->object=source->data->owner;
    if(owner->object) {
        if(!nativeMatchPool_Reference(&nativeGObjPool,owner->object,&owner->reference) ||
           owner->reference.lifetime!=source->reference.lifetime)
            return error(d,"Stage response damage owner has no physical identity in this scene");
        owner->classifier=owner->object->classifier;owner->data=owner->object->user_data;
        if(owner->classifier==HSD_GOBJ_CLASS_FIGHTER) {
            if(!nativeCallbackSource_Snapshot(owner->object,NativeCallbackSource_Fighter,&owner->generation))
                return error(d,"Stage response Fighter damage owner is unavailable");
            owner->kind=(uint32_t)((const Fighter*)owner->data)->kind;
        }
        if(!response_owner_current(owner,source->reference.lifetime))
            return error(d,"Stage response damage owner changed during capture");
    }
    for(unsigned i=0;i<4;++i) {
        const HitCapsule* hit=&source->data->x5D4_hitboxes[i].hit;
        if(hit->state==HitCapsule_Disabled)continue;
        // The native kernel first converts attenuated damage to U32, then
        // applies the current Fighter owner's scale. Validate only otherwise
        // undefined conversions; the real setter performs all bookkeeping.
        NativeScriptValue attenuated={.type=NativeScriptType_F32,
            .as.f32=hit->damage*source->attributes->x58},checked;
        NativeScriptDiagnostic conversion={0};
        if(!nativeScriptValue_Convert(&attenuated,NativeScriptType_U32,&checked,d?d:&conversion))return false;
        if(owner->object && owner->classifier==HSD_GOBJ_CLASS_FIGHTER) {
            const Fighter* fighter=owner->data;
            if(fighter->x34_scale.y!=1.0f) {
                if(!Fighter_804D6524 || fighter->x34_scale.y==0.0f)
                    return error(d,"Stage response owner scale data is unavailable");
                NativeScriptValue scaled={.type=NativeScriptType_F32,.as.f32=0.999f+
                    ftCo_CalcYScaledKnockback((float)(uint32_t)checked.as.u64,fighter->x34_scale.y,Fighter_804D6524->x4)};
                if(!nativeScriptValue_Convert(&scaled,NativeScriptType_U32,&checked,d?d:&conversion))return false;
            }
        }
    }
    return true;
}

static bool response(NativeScriptRuntimeContext* c,bool stage,const NativeScriptValue* args,uint32_t count,
    NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    int32_t mask=0;NativeItemLifetime source;
    if(!out || count!=(stage?1u:0u) || (stage&&!args))
        return error(d,"Item response arguments do not match the native operation");
    if(stage&&!response_mask(args,&mask,d))return false;
    if(!c || c->source_class!=NativeCallbackSource_Item || !c->scene_epoch ||
       !nativeItemLifetime_Capture(c->source,&source) || !source.source_owned ||
       source.reference.lifetime!=c->scene_epoch || source.generation!=c->generation ||
       !nativeScriptBindings_SourceAlive(c) || !source.article ||
       (stage&&(!source.attributes||source.article->x0_common_attr!=source.attributes)) ||
       source.program->article!=source.article ||
       source.program->owner!=source.data->native_script_owner || source.program->kind!=source.kind ||
       source.program->author_kind!=source.data->native_article_author_kind)
        return error(d,"Item response requires its exact live owned Article and common attributes");
    const ItemCommonData* common=it_804D6D28;
    if(!stage&&!common)return error(d,"Victim bounce requires common Item response factors");
    ResponseOwner owner={0};bool tail=false;
    if(stage) {
        const CollData* coll=&source.data->x378_itemColl;
        // Preflight only it_80276D9C's tail predicate. The real helper still
        // runs once after the velocity bounce, including false-path ECB and
        // position writes; that path does not access effects/damage owners.
        tail=!((coll->env_flags&Collide_LeftWallMask)&&(coll->env_flags&Collide_RightWallMask)) &&
            !((mask&4)&&(coll->prev_env_flags&Collide_RightWallMask)) &&
            !((mask&8)&&(coll->prev_env_flags&Collide_LeftWallMask)) &&
            !((mask&2)&&(coll->prev_env_flags&Collide_CeilingMask)) &&
            !((mask&1)&&(coll->prev_env_flags&Collide_FloorMask));
        if(tail) {
            if(!source.data->xDCF_flag.b0) {
                const ItemModelDesc* model=source.article->x10_modelDesc;
                if(!model || (model->x4_bone_count&&!source.data->xBBC_dynamicBoneTable))
                    return error(d,"Stage response effect has no native attachment model");
                const HSD_JObj* joint=model->x4_bone_count?source.data->xBBC_dynamicBoneTable->bones[0]:source.model;
                if(source.object_kind!=HSD_GObj_JObjKind || !joint ||
                   !HSD_CLASS_METHOD(joint) || HSD_CLASS_METHOD(joint)->head.obj_size<(int)sizeof(*joint) ||
                   // The native ancestry API reads the object; only its
                   // class metadata may initialize, never the Item/owner.
                   !hsdObjIsDescendantOf((HSD_Obj*)joint,HSD_CLASS_INFO(&hsdJObj)))
                    return error(d,"Stage response effect requires its actual native JObj");
            }
            if(!response_damage(&source,&owner,d))return false;
        }
        // Audited map/vector, sound, queued-effect and hitbox leaves do not
        // synchronously dispatch authored code. Preserve the entire kernel.
        it_80276FC4(c->source,mask);
    } else itColl_BounceOffVictim(c->source);
    if(c->source!=source.object || c->source_class!=NativeCallbackSource_Item ||
       c->generation!=source.generation || c->scene_epoch!=source.reference.lifetime ||
       !nativeItemLifetime_Current(&source) || !nativeScriptBindings_SourceAlive(c) ||
       (!stage&&it_804D6D28!=common))
        return error(d,"Item response source retired/faulted before publication");
    if(owner.observed && (source.data->owner!=owner.object || !response_owner_current(&owner,c->scene_epoch)))
        return error(d,"Stage response damage owner changed before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

typedef struct ItemCallbackSource {
    NativeMatchPoolReference reference;
    NativeScriptRuntimeContext context;
    const NativeScriptItemProgram* program;
    const NativeScriptCompileBank* bank;
    NativeScriptReceiverView view;
    Item* item;
    uint64_t program_identity,layout_identity,parameter_identity;
} ItemCallbackSource;
static bool callback_current(const ItemCallbackSource* s,NativeScriptRuntimeContext* c,
    NativeScriptDiagnostic* d)
{
    NativeScriptReceiverView view;
    if(c->source!=s->context.source||c->source_class!=s->context.source_class||
       c->generation!=s->context.generation||c->scene_epoch!=s->reference.lifetime||
       nativeMatchPool_Resolve(s->reference)!=c->source||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_ITEM||((HSD_GObj*)c->source)->user_data!=s->item||
       !nativeScriptBindings_SourceAlive(c)||nativeScriptItem_Status(c->source)!=NativeScriptVM_Ok||
       nativeScriptItem_Spec(c->source)!=s->program||s->program->identity!=s->program_identity||
       s->program->bank!=s->bank||!s->bank->valid||!nativeScriptItem_View(c->source,c->generation,&view)||
       view.program!=&s->bank->declarations||view.storage!=s->view.storage||!view.storage||!view.storage->live||
       view.storage->owner!=s->program->owner||view.storage->generation!=c->generation||
       view.storage->layout!=&s->bank->storage_layout||view.storage->layout->identity!=s->layout_identity||
       view.storage->parameters!=&s->bank->storage_parameters||view.storage->parameters->identity!=s->parameter_identity)
        return error(d,"Item accessory source, program or storage identity is unavailable");
    return true;
}
static bool accessory(NativeScriptRuntimeContext* c,bool track,uint32_t count,
    NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    ItemCallbackSource s;
    if(!output||count||!c||c->source_class!=NativeCallbackSource_Item||!c->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&s.reference)||s.reference.lifetime!=c->scene_epoch||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_ITEM||
       !nativeScriptBindings_SourceAlive(c)||nativeScriptItem_Status(c->source)!=NativeScriptVM_Ok)
        return error(d,"Item accessory registration requires a live owned Item and zero arguments");
    s.context=*c;s.item=((HSD_GObj*)c->source)->user_data;s.program=nativeScriptItem_Spec(c->source);
    if(!s.program||!s.program->bank||!nativeScriptItem_View(c->source,c->generation,&s.view)||
       !s.view.storage||s.view.storage->layout!=&s.program->bank->storage_layout||
       s.view.storage->parameters!=&s.program->bank->storage_parameters)
        return error(d,"Item accessory registration has no exact owned program view");
    s.bank=s.program->bank;s.program_identity=s.program->identity;
    s.layout_identity=s.view.storage->layout->identity;s.parameter_identity=s.view.storage->parameters->identity;
    if(!callback_current(&s,c,d))return false;
    uint32_t function=UINT32_MAX;
    if(track){
        function=nativeScriptItem_CallbackFunction(c->source,NativeScriptItemCallback_Accessory);
        if(function==UINT32_MAX||function>=s.bank->declarations.function_count||function>=s.bank->code.function_count||
           !s.bank->declarations.functions[function].callback||!s.bank->execution_functions[function].callback||
           function!=s.program->hooks[NativeScriptItemHook_Accessory])
            return error(d,"Item accessory tracking requires its declared owned Void callback");
    }
    if(!nativeScriptItem_BindCallback(c->source,NativeScriptItemCallback_Accessory,function))
        return error(d,"Item accessory registration failed its owned callback contract");
    if(!callback_current(&s,c,d))return false;
    *output=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

bool nativeScriptBindingItem_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    Item* item=((HSD_GObj*)context->source)->user_data;
    switch(semantic) {
    case NativeScriptBinding_ItemLife:result->as.f32=item->xD44_lifeTimer;break;
    case NativeScriptBinding_ECBUp:result->as.f32=item->x378_itemColl.ecb_source.up;break;
    case NativeScriptBinding_ECBDown:result->as.f32=item->x378_itemColl.ecb_source.down;break;
    case NativeScriptBinding_ECBFront:result->as.f32=item->x378_itemColl.ecb_source.front;break;
    case NativeScriptBinding_ECBBack:result->as.f32=item->x378_itemColl.ecb_source.back;break;
    case NativeScriptBinding_ItemHalfLife:result->as.f32=item->xD48_halfLifeTimer;break;
    case NativeScriptBinding_ItemAnimationRate:result->as.f32=item->x5D0_animFrameSpeed;break;
    case NativeScriptBinding_ItemSpinSpeed:result->as.f32=item->xD3C_spinSpeed;break;
    case NativeScriptBinding_ItemKind:result->as.i64=item->kind;break;
    case NativeScriptBinding_ItemCreationID:result->as.u64=(uint32_t)item->x1C;break;
    case NativeScriptBinding_ItemMotionState:result->as.i64=item->msid;break;
    case NativeScriptBinding_ItemAge:
        if(!holdable(item,diagnostic))return false;
        result->as.i64=item->xDD4_itemVar.native_holdable.age;break;
    case NativeScriptBinding_ItemAccumulatedDamage:result->as.i64=item->xC9C;break;
    case NativeScriptBinding_ItemDamageReceived:result->as.i64=item->xCA0;break;
    case NativeScriptBinding_ItemDamageBehavior:result->as.f32=item->xC98;break;
    case NativeScriptBinding_ItemCommand0:result->as.u64=item->xDAC_itcmd_var0;break;
    case NativeScriptBinding_ItemCommand1:result->as.u64=item->xDB0_itcmd_var1;break;
    case NativeScriptBinding_ItemCommand2:result->as.u64=item->xDB4_itcmd_var2;break;
    case NativeScriptBinding_ItemCommand3:result->as.u64=item->xDB8_itcmd_var3;break;
    case NativeScriptBinding_ItemCollisionMode:result->as.i64=item->xD0C;break;
    case NativeScriptBinding_ItemCollisionGroup:result->as.u64=item->xAC4_ignoreItemID;break;
    case NativeScriptBinding_ItemExitCategory:result->as.i64=item->xDCC_flag.b4567;break;
    case NativeScriptBinding_ItemAnimationFrame:result->as.f32=item->x5CC_currentAnimFrame;break;
    case NativeScriptBinding_ItemBaseSpinSpeed:result->as.f32=item->spin_spd;break;
    case NativeScriptBinding_ItemReflectionMultiplier:result->as.f32=item->xC70;break;
    case NativeScriptBinding_ItemScale:result->as.f32=item->scl;break;
    case NativeScriptBinding_ItemThrowScale:result->as.f32=item->throw_scale;break;
    case NativeScriptBinding_ItemDamageDirection:result->as.f32=item->xCCC_incDamageDirection;break;
    case NativeScriptBinding_ItemDropSpeed:case NativeScriptBinding_ItemBaseScale:case NativeScriptBinding_ItemSurfaceRestitution:
        if(!item->xCC_item_attr)return error(diagnostic,"Native item common attributes are absent.");
        result->as.f32=semantic==NativeScriptBinding_ItemDropSpeed?item->xCC_item_attr->x18:
            semantic==NativeScriptBinding_ItemBaseScale?item->xCC_item_attr->x60_scale:item->xCC_item_attr->x58;break;
    case NativeScriptBinding_ItemPhysicsDisabled:result->as.u64=item->xDD0_flag.b3;break;
    case NativeScriptBinding_ItemCustomFloorMotion:result->as.u64=item->xDCD_flag.b3;break;
    case NativeScriptBinding_ItemRemoveAtBounds:result->as.u64=item->xDCC_flag.b3;break;
    case NativeScriptBinding_ItemSuppressDestructionEffects:result->as.u64=item->xDCF_flag.b2;break;
    case NativeScriptBinding_ItemWorldInteraction:result->as.u64=item->xDC8_word.flags.x15;break;
    case NativeScriptBinding_ItemOwnerBound:result->as.u64=item->xDC8_word.flags.x13;break;
    case NativeScriptBinding_ItemReverseSpin:result->as.u64=item->xDC8_word.flags.x19;break;
    case NativeScriptBinding_ItemThrowSpeedDamage:result->as.u64=item->xDC8_word.flags.x14;break;
    case NativeScriptBinding_ItemDamageHitlag:result->as.u64=item->xDC8_word.flags.xD;break;
    case NativeScriptBinding_ItemContactHitlag:result->as.u64=item->xDC8_word.flags.xC;break;
    case NativeScriptBinding_ItemHarmOwner:result->as.u64=item->xDCD_flag.b5;break;
    case NativeScriptBinding_ItemHarmTeammates:result->as.u64=item->xDCD_flag.b6;break;
    case NativeScriptBinding_ItemHarmSiblingItems:result->as.u64=item->xDCD_flag.b7;break;
    case NativeScriptBinding_ItemCanBeHitByOwner:result->as.u64=item->xDCE_flag.b0;break;
    case NativeScriptBinding_ItemCanBeHitByTeammates:result->as.u64=item->xDCE_flag.b1;break;
    case NativeScriptBinding_ItemCanBeHitBySiblingItems:result->as.u64=item->xDCE_flag.b2;break;
    case NativeScriptBinding_ItemSubactionSoundsMuted:result->as.u64=item->xDCD_flag.b2;break;
    case NativeScriptBinding_ItemStopOnSurface:result->as.u64=item->xDCD_flag.b4;break;
    case NativeScriptBinding_ItemDynamicECB:result->as.u64=item->xDCE_flag.b7;break;
    case NativeScriptBinding_ItemGrounded:result->as.u64=item->kind==It_Kind_NativeHoldable?item->msid==1:item->ground_or_air==GA_Ground;break;
    case NativeScriptBinding_ItemAirborne:result->as.u64=item->kind==It_Kind_NativeHoldable?(item->msid==2||item->msid==3):item->ground_or_air==GA_Air;break;
    case NativeScriptBinding_ItemGroundContact:
        if(!holdable(item,diagnostic))return false;
        result->as.u64=(item->xDD4_itemVar.native_holdable.collision_flags&0xE)!=0;break;
    default:return error(diagnostic,"Unsupported native item self property.");
    }
    return true;
}

bool nativeScriptBindingItem_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    if(semantic>=NativeScriptBinding_ECBUp&&semantic<=NativeScriptBinding_ECBBack) {
        if(!context||context->source_class!=NativeCallbackSource_Item||!value||
           value->type!=NativeScriptType_F32||!nativeScriptBindings_SourceAlive(context))
            return error(diagnostic,"Collision-box extent requires a live Item and exact Float");
        Item* item=((HSD_GObj*)context->source)->user_data;
        switch(semantic) {
        case NativeScriptBinding_ECBUp:item->x378_itemColl.ecb_source.up=value->as.f32;break;
        case NativeScriptBinding_ECBDown:item->x378_itemColl.ecb_source.down=value->as.f32;break;
        case NativeScriptBinding_ECBFront:item->x378_itemColl.ecb_source.front=value->as.f32;break;
        case NativeScriptBinding_ECBBack:item->x378_itemColl.ecb_source.back=value->as.f32;break;
        default:return false;
        }
        return true;
    }
    if(semantic==NativeScriptBinding_ItemAirborne||semantic==NativeScriptBinding_ItemGrounded) {
        NativeItemLifetime source;
        if(!context||context->source_class!=NativeCallbackSource_Item||!context->scene_epoch||
           !value||value->type!=NativeScriptType_Bool||value->as.u64>1||
           !nativeItemLifetime_Capture(context->source,&source)||
           source.reference.lifetime!=context->scene_epoch||source.generation!=context->generation||
           !source.source_owned||(semantic==NativeScriptBinding_ItemAirborne&&source.removing)||
           source.program->kind!=source.kind||source.program->owner!=source.data->native_script_owner||
           source.program->author_kind!=source.data->native_article_author_kind)
            return error(diagnostic,"Item ground-state assignment requires its live owned Article and Bool value");
        if(source.kind==It_Kind_NativeHoldable)
            return error(diagnostic,"Workshop holdable ground state is selected by its motion and cannot be assigned");
        if(!source.article||source.program->article!=source.article||
           (semantic==NativeScriptBinding_ItemAirborne&&!source.attributes))
            return error(diagnostic,"Item ground-state assignment has no exact owned Article data");
        if(semantic==NativeScriptBinding_ItemGrounded) {
            if(value->as.u64)it_802762B0(source.data);
            else it_802762BC(source.data);
        } else source.data->ground_or_air=value->as.u64?GA_Air:GA_Ground;
        return nativeItemLifetime_Current(&source)?true:
            error(diagnostic,"Item ground-state source retired before continuation");
    }
    Item* item=((HSD_GObj*)context->source)->user_data;bool flag=value->as.u64!=0;
    switch(semantic) {
    case NativeScriptBinding_ItemLife:item->xD44_lifeTimer=value->as.f32;break;
    case NativeScriptBinding_ItemDamageBehavior:item->xC98=value->as.f32;break;
    case NativeScriptBinding_ItemHalfLife:item->xD48_halfLifeTimer=value->as.f32;break;
    case NativeScriptBinding_ItemAnimationRate:item->x5D0_animFrameSpeed=value->as.f32;break;
    case NativeScriptBinding_ItemSpinSpeed:item->xD3C_spinSpeed=value->as.f32;break;
    case NativeScriptBinding_ItemCommand0:item->xDAC_itcmd_var0=(uint32_t)value->as.u64;break;
    case NativeScriptBinding_ItemCommand1:item->xDB0_itcmd_var1=(uint32_t)value->as.u64;break;
    case NativeScriptBinding_ItemCommand2:item->xDB4_itcmd_var2=(uint32_t)value->as.u64;break;
    case NativeScriptBinding_ItemCommand3:item->xDB8_itcmd_var3=(uint32_t)value->as.u64;break;
    case NativeScriptBinding_ItemCollisionMode:item->xD0C=(int32_t)value->as.i64;break;
    case NativeScriptBinding_ItemCollisionGroup:item->xAC4_ignoreItemID=(uint32_t)value->as.u64;break;
    case NativeScriptBinding_ItemExitCategory:item->xDCC_flag.b4567=(uint32_t)value->as.i64&15;break;
    case NativeScriptBinding_ItemPhysicsDisabled:item->xDD0_flag.b3=flag;break;
    case NativeScriptBinding_ItemCustomFloorMotion:item->xDCD_flag.b3=flag;break;
    case NativeScriptBinding_ItemRemoveAtBounds:item->xDCC_flag.b3=flag;break;
    case NativeScriptBinding_ItemSuppressDestructionEffects:item->xDCF_flag.b2=flag;break;
    case NativeScriptBinding_ItemWorldInteraction:item->xDC8_word.flags.x15=flag;break;
    case NativeScriptBinding_ItemOwnerBound:item->xDC8_word.flags.x13=flag;break;
    case NativeScriptBinding_ItemReverseSpin:item->xDC8_word.flags.x19=flag;break;
    case NativeScriptBinding_ItemThrowSpeedDamage:item->xDC8_word.flags.x14=flag;break;
    case NativeScriptBinding_ItemDamageHitlag:item->xDC8_word.flags.xD=flag;break;
    case NativeScriptBinding_ItemContactHitlag:item->xDC8_word.flags.xC=flag;break;
    case NativeScriptBinding_ItemHarmOwner:item->xDCD_flag.b5=flag;break;
    case NativeScriptBinding_ItemHarmTeammates:item->xDCD_flag.b6=flag;break;
    case NativeScriptBinding_ItemHarmSiblingItems:item->xDCD_flag.b7=flag;break;
    case NativeScriptBinding_ItemCanBeHitByOwner:item->xDCE_flag.b0=flag;break;
    case NativeScriptBinding_ItemCanBeHitByTeammates:item->xDCE_flag.b1=flag;break;
    case NativeScriptBinding_ItemCanBeHitBySiblingItems:item->xDCE_flag.b2=flag;break;
    case NativeScriptBinding_ItemSubactionSoundsMuted:item->xDCD_flag.b2=flag;break;
    case NativeScriptBinding_ItemStopOnSurface:item->xDCD_flag.b4=flag;break;
    case NativeScriptBinding_ItemDynamicECB:item->xDCE_flag.b7=flag;break;
    default:return error(diagnostic,"Unsupported native item self property write.");
    }
    return true;
}

bool nativeScriptBindingItem_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_ItemBounceVictim||semantic==NativeScriptBinding_ItemRespondStageMask)
        return response(context,semantic==NativeScriptBinding_ItemRespondStageMask,values,count,result,diagnostic);
    if(semantic==NativeScriptBinding_ItemTrackAccessory||semantic==NativeScriptBinding_ItemClearAccessory)
        return accessory(context,semantic==NativeScriptBinding_ItemTrackAccessory,count,result,diagnostic);
    (void)count;HSD_GObj* object=context->source;Item* item=object->user_data;
    switch(semantic) {
    case NativeScriptBinding_ItemGravity:case NativeScriptBinding_ItemGravityWithSpin:
        if(!item->xCC_item_attr)return error(diagnostic,"Item gravity requires native common attributes.");
        if(semantic==NativeScriptBinding_ItemGravity||values[0].as.u64) {
            if(!it_804D6D28)return error(diagnostic,"Item spin setup requires common item data.");
            Item_ApplyFallingPhysics(object);
        } else it_80272860(object,item->xCC_item_attr->x10_fall_speed,item->xCC_item_attr->x14_fall_speed_max);
        break;
    case NativeScriptBinding_ItemSetLifetime:
        if(!it_804D6D28)return error(diagnostic,"Item lifetime requires common item data.");
        it_80275158(object,values[0].as.f32);break;
    case NativeScriptBinding_ItemExpireTick:result->as.u64=it_802751D8(object);break;
    case NativeScriptBinding_ItemReflect:result->as.u64=it_80273030(object);break;
    case NativeScriptBinding_ItemShieldBounce:result->as.u64=itColl_BounceOffShield(object);break;
    case NativeScriptBinding_ItemBounceStage:
        if(!item->xCC_item_attr)return error(diagnostic,"Item stage bounce requires common attributes.");
        result->as.u64=it_8027781C(object);break;
    case NativeScriptBinding_ItemPause:it_8026B724(object);break;
    case NativeScriptBinding_ItemResume:it_8026B73C(object);break;
    case NativeScriptBinding_ItemUseManualLifetime:it_8026B3A8(object);break;
    case NativeScriptBinding_ItemMirrorVelocity:lbVector_Mirror(&item->x40_vel,&item->xC58);break;
    case NativeScriptBinding_ItemMirrorVelocityWithNormal: {
        Vec3 normal={values[0].as.f32,values[1].as.f32,values[2].as.f32};lbVector_Mirror(&item->x40_vel,&normal);break;
    }
    case NativeScriptBinding_ItemStopLoopSound:Item_8026B074(item);break;
    case NativeScriptBinding_ItemPlaySound:case NativeScriptBinding_ItemLoopSound:
        if((uint64_t)values[0].as.i64>0xFFFFFF||(uint64_t)values[1].as.i64>255||(uint64_t)values[2].as.i64>255)
            return error(diagnostic,"Item sound ID/byte controls are outside the native script range.");
        if(semantic==NativeScriptBinding_ItemPlaySound)Item_8026AE84(item,(int32_t)values[0].as.i64,(u8)values[1].as.i64,(u8)values[2].as.i64);
        else Item_8026AF0C(item,(int32_t)values[0].as.i64,(u8)values[1].as.i64,(u8)values[2].as.i64);
        break;
    default:return error(diagnostic,"Unsupported native item self operation.");
    }
    return true;
}
