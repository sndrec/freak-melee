#include "native_script_binding_article_combat.h"
#include "native_script_actor_access.h"
#include "types.h"
#include "fighter.h"
#include "ftchangeparam.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/it/itcoll.h>
#include <melee/it/it_2725.h>
#include <melee/it/types.h>
#include <melee/mp/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <math.h>
#include <stdio.h>

typedef struct Source {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    Item* data;
    const NativeScriptItemProgram* spec;
    NativeScriptReceiverView view;
    uint64_t layout,parameters;
} Source;
typedef struct Owner {
    HSD_GObj* object;
    NativeMatchPoolReference reference;
    const void* data;
    uint32_t classifier,generation,kind;
} Owner;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool live(const Source* s)
{
    HSD_GObj* object=s->context.source;NativeScriptRuntimeContext context=s->context;
    NativeScriptReceiverView view;const NativeScriptCompileBank* bank=s->spec->bank;
    return nativeMatchPool_Resolve(s->reference)==object && s->reference.lifetime==context.scene_epoch &&
        object->classifier==HSD_GOBJ_CLASS_ITEM && object->user_data==s->data && nativeScriptBindings_SourceAlive(&context) &&
        nativeScriptItem_Status(object)==NativeScriptVM_Ok && nativeScriptItem_Spec(object)==s->spec && bank->valid &&
        nativeScriptItem_View(object,context.generation,&view) && view.program==&bank->declarations && view.program==s->view.program &&
        view.storage==s->view.storage && view.storage && view.storage->live && view.storage->owner==s->spec->owner &&
        view.storage->generation==context.generation && view.storage->layout==&bank->storage_layout && view.storage->layout->identity==s->layout &&
        view.storage->parameters==&bank->storage_parameters && view.storage->parameters->identity==s->parameters;
}
static bool capture(NativeScriptRuntimeContext* c,Source* s,NativeScriptDiagnostic* d)
{
    if(!c || c->source_class!=NativeCallbackSource_Item || !c->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&s->reference) || s->reference.lifetime!=c->scene_epoch ||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_ITEM || !nativeScriptBindings_SourceAlive(c))
        return error(d,"Article combat source has no live Item identity in its pinned scene");
    s->context=*c;s->data=((HSD_GObj*)c->source)->user_data;s->spec=nativeScriptItem_Spec(c->source);
    if(!s->spec || !s->spec->bank || !nativeScriptItem_View(c->source,c->generation,&s->view) ||
       !s->view.storage || !s->view.storage->layout || !s->view.storage->parameters)
        return error(d,"Article combat source has no exact owned program/storage");
    s->layout=s->view.storage->layout->identity;s->parameters=s->view.storage->parameters->identity;
    return live(s)?true:error(d,"Article combat source is retired, faulted or has changed schema");
}
static bool index(const NativeScriptValue* v,uint32_t* result,bool* inside,NativeScriptDiagnostic* d)
{
    uint64_t number;
    switch(v->type) {
    case NativeScriptType_Bool:number=v->as.u64!=0;break;
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(v->as.i64<INT32_MIN || v->as.i64>UINT32_MAX)return error(d,"Item capsule index exceeds native 32-bit width");
        if(v->as.i64<0){*inside=false;return true;}number=(uint64_t)v->as.i64;break;
    case NativeScriptType_U32:case NativeScriptType_U64:number=v->as.u64;break;
    default:return error(d,"Item capsule index must be integral");
    }
    if(number>UINT32_MAX)return error(d,"Item capsule index exceeds native 32-bit width");
    *inside=number<4;*result=*inside?(uint32_t)number:0;return true;
}
static bool owner_live(const Owner* o,uint64_t epoch)
{
    if(!o->object)return true;
    if(nativeMatchPool_Resolve(o->reference)!=o->object || o->reference.lifetime!=epoch ||
       o->object->classifier!=o->classifier || o->object->user_data!=o->data)return false;
    if(o->classifier!=HSD_GOBJ_CLASS_FIGHTER)return true;
    const Fighter* fighter=o->data;
    return nativeCallbackSource_Alive(o->object,o->generation,NativeCallbackSource_Fighter) &&
        fighter->gobj==o->object && (uint32_t)fighter->kind==o->kind;
}
static bool owner_capture(const Source* s,Owner* o,NativeScriptDiagnostic* d)
{
    o->object=s->data->owner;if(!o->object)return true;
    if(!nativeMatchPool_Reference(&nativeGObjPool,o->object,&o->reference) || o->reference.lifetime!=s->context.scene_epoch)
        return error(d,"Item damage owner has no live identity in this scene");
    o->classifier=o->object->classifier;o->data=o->object->user_data;
    if(o->classifier==HSD_GOBJ_CLASS_FIGHTER) {
        if(!o->data || !nativeCallbackSource_Snapshot(o->object,NativeCallbackSource_Fighter,&o->generation))
            return error(d,"Item Fighter damage owner has retired");
        const Fighter* fighter=o->data;o->kind=(uint32_t)fighter->kind;
        if(fighter->x34_scale.y!=1.0f && (!Fighter_804D6524 || fighter->x34_scale.y==0.0f))
            return error(d,"Item damage owner scale or common scale data is invalid");
    }
    return owner_live(o,s->context.scene_epoch)?true:error(d,"Item damage owner changed during capture");
}
static bool geometry(const Source* s,uint32_t disabled,NativeScriptDiagnostic* d)
{
    // Disabling one capsule advances all remaining Enabled/Unk2/Unk3 slots.
    // Those exact native states require an engine-owned attachment joint.
    for(unsigned i=0;i<4;++i) {
        if(i==disabled)continue;
        const HitCapsule* hit=&s->data->x5D4_hitboxes[i].hit;
        if((hit->state==HitCapsule_Enabled || hit->state==HitCapsule_Unk2 || hit->state==HitCapsule_Unk3) && !hit->jobj)
            return error(d,"Active Item capsule has no native geometry joint");
    }
    return true;
}
bool nativeScriptBindingArticleCombat_Read(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)selectors;uint32_t mask;
    switch(semantic) {
    case NativeScriptBinding_CollisionFloorContact:mask=Collide_FloorMask;break;
    case NativeScriptBinding_CollisionCeilingContact:mask=Collide_CeilingMask;break;
    case NativeScriptBinding_CollisionLeftWall:mask=Collide_LeftWallMask;break;
    case NativeScriptBinding_CollisionRightWall:mask=Collide_RightWallMask;break;
    default:return error(d,"Unsupported Article contact property");
    }
    if(!out || count)return error(d,"Article contact properties have no selectors");
    Source s={0};if(!capture(c,&s,d))return false;
    bool result=(s.data->x378_itemColl.env_flags&mask)!=0;
    if(!live(&s))return error(d,"Article contact source changed before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=result};return true;
}
bool nativeScriptBindingArticleCombat_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool enabled=semantic==NativeScriptBinding_HitboxEnabled,radius=semantic==NativeScriptBinding_HitboxRadius;
    bool damage=semantic==NativeScriptBinding_HitboxSetDamage,set_radius=semantic==NativeScriptBinding_HitboxSetRadius;
    bool all=semantic==NativeScriptBinding_HitboxesDisableAll,disable=semantic==NativeScriptBinding_ItemDisableHitbox;
    if(!out || (!enabled&&!radius&&!damage&&!set_radius&&!all&&!disable) || count!=(all?0u:damage||set_radius?2u:1u) || (count&&!args))
        return error(d,"Item capsule operation or arguments are invalid");
    uint32_t capsule=0;bool inside=true;
    if(!all&&!index(&args[0],&capsule,&inside,d))return false;
    if(!inside&&!disable)return error(d,"Item capsule index is outside four native slots");
    NativeScriptValue converted={0};
    if(damage&&!nativeScriptValue_Convert(&args[1],NativeScriptType_U32,&converted,d))return false;
    if(set_radius&&args[1].type!=NativeScriptType_F32)return error(d,"Item radius argument must be marshalled F32");
    Source s={0};if(!capture(c,&s,d))return false;
    NativeScriptValue result={.type=enabled?NativeScriptType_Bool:radius?NativeScriptType_F32:NativeScriptType_Void};
    Owner owner={0};
    if(enabled)result.as.u64=s.data->x5D4_hitboxes[capsule].hit.state!=HitCapsule_Disabled;
    else if(radius)result.as.f32=s.data->x5D4_hitboxes[capsule].hit.scale;
    else if(set_radius){float value=args[1].as.f32;if(isfinite(value)&&value>=0)s.data->x5D4_hitboxes[capsule].hit.scale=value;}
    else if(damage) {
        if(!owner_capture(&s,&owner,d))return false;
        if(owner.object && owner.classifier==HSD_GOBJ_CLASS_FIGHTER) {
            const Fighter* fighter=owner.data;
            if(fighter->x34_scale.y!=1.0f) {
                // Validate the native branch's otherwise undefined F32->U32
                // conversion with its actual pure math. The real setter still
                // performs ownership scaling and stale-move bookkeeping.
                NativeScriptValue scaled={.type=NativeScriptType_F32,.as.f32=0.999f+
                    ftCo_CalcYScaledKnockback((float)(uint32_t)converted.as.u64,fighter->x34_scale.y,Fighter_804D6524->x4)};
                NativeScriptValue checked;
                if(!nativeScriptValue_Convert(&scaled,NativeScriptType_U32,&checked,d))return false;
            }
        }
        it_80272460(&s.data->x5D4_hitboxes[capsule].hit,(uint32_t)converted.as.u64,c->source);
    } else if(all)it_802725D4(c->source);
    else if(inside){if(!geometry(&s,capsule,d))return false;it_80272560(c->source,(int32_t)capsule);}
    if(!live(&s))return error(d,"Item capsule source retired/faulted before publication");
    if(damage&&(!owner_live(&owner,s.context.scene_epoch)||s.data->owner!=owner.object))return error(d,"Item damage owner changed during setter");
    *out=result;return true;
}
