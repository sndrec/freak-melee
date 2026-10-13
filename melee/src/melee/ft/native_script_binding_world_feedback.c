#include "native_script_binding_world_feedback.h"
#include "native_script_actor_access.h"
#include "native_script_fighter.h"
#include "native_callback_lifetime.h"
#include "fighter.h"
#include "ftcommon.h"
#include "types.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/it/it_2725.h>
#include <melee/it/types.h>
#include <melee/cm/camera.h>
#include <melee/cm/types.h>
#include <melee/gm/forward.h>
#include <melee/lb/lb_013B.h>
#include <melee/pl/player.h>
#include <dolphin/pad.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>

typedef struct Source {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    const void* data;
    const NativeScriptCompileBank* bank;
    const NativeScriptFighterProgram* fighter;
    const NativeScriptItemProgram* item;
    NativeScriptReceiverView view;
    uint64_t program_identity,layout,parameters;
    uint32_t owner;
} Source;
typedef struct DamageOwner {
    HSD_GObj* object;
    NativeMatchPoolReference reference;
    const void* data;
    uint32_t classifier,generation,kind;
} DamageOwner;
static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool current(const Source* s)
{
    NativeScriptRuntimeContext c=s->context;HSD_GObj* object=c.source;NativeScriptReceiverView view;
    if(nativeMatchPool_Resolve(s->reference)!=object || s->reference.lifetime!=c.scene_epoch ||
       object->classifier!=(c.source_class==NativeCallbackSource_Item?HSD_GOBJ_CLASS_ITEM:HSD_GOBJ_CLASS_FIGHTER) ||
       object->user_data!=s->data || !nativeScriptBindings_SourceAlive(&c) || !s->bank->valid)return false;
    if(c.source_class==NativeCallbackSource_Item) {
        if(nativeScriptItem_Status(object)!=NativeScriptVM_Ok || nativeScriptItem_Spec(object)!=s->item ||
           s->item->bank!=s->bank || s->item->identity!=s->program_identity || s->item->owner!=s->owner)return false;
    } else if(nativeScriptFighter_Status(object)!=NativeScriptVM_Ok ||
              nativeScriptFighter_Program(((const Fighter*)s->data)->kind)!=s->fighter ||
              s->fighter->bank!=s->bank || s->fighter->identity!=s->program_identity ||
              !nativeScriptBindings_SourceFighterLayout(&c,NULL))return false;
    return nativeScriptActors_Lookup(NULL,object,c.source_class,c.generation,&view) &&
        view.program==&s->bank->declarations && view.program==s->view.program && view.storage==s->view.storage &&
        view.storage && view.storage->live && view.storage->owner==s->owner && view.storage->generation==c.generation &&
        view.storage->layout==&s->bank->storage_layout && view.storage->layout->identity==s->layout &&
        view.storage->parameters==&s->bank->storage_parameters && view.storage->parameters->identity==s->parameters;
}
static bool capture(NativeScriptRuntimeContext* c,Source* s,NativeScriptDiagnostic* d)
{
    if(!c || !c->scene_epoch || (c->source_class!=NativeCallbackSource_Fighter&&c->source_class!=NativeCallbackSource_Item) ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&s->reference) || s->reference.lifetime!=c->scene_epoch ||
       ((HSD_GObj*)c->source)->classifier!=(c->source_class==NativeCallbackSource_Item?HSD_GOBJ_CLASS_ITEM:HSD_GOBJ_CLASS_FIGHTER) ||
       !nativeScriptBindings_SourceAlive(c))return error(d,"World feedback source has no live identity in its scene");
    s->context=*c;s->data=((HSD_GObj*)c->source)->user_data;
    if(c->source_class==NativeCallbackSource_Fighter) {
        s->fighter=nativeScriptFighter_Program(((const Fighter*)s->data)->kind);
        s->bank=s->fighter?s->fighter->bank:NULL;s->program_identity=s->fighter?s->fighter->identity:0;
    } else {
        s->item=nativeScriptItem_Spec(c->source);s->bank=s->item?s->item->bank:NULL;
        s->owner=s->item?s->item->owner:0;s->program_identity=s->item?s->item->identity:0;
    }
    if(!s->bank || !nativeScriptActors_Lookup(NULL,c->source,c->source_class,c->generation,&s->view) ||
       !s->view.storage || !s->view.storage->layout || !s->view.storage->parameters)
        return error(d,"World feedback source has no exact owned program/storage");
    s->layout=s->view.storage->layout->identity;s->parameters=s->view.storage->parameters->identity;
    return current(s)?true:error(d,"World feedback source is faulted, retired or has changed schema");
}
static bool owner_current(const DamageOwner* o,uint64_t epoch)
{
    if(!o->object)return true;
    if(nativeMatchPool_Resolve(o->reference)!=o->object || o->reference.lifetime!=epoch ||
       o->object->classifier!=o->classifier || o->object->user_data!=o->data)return false;
    if(o->classifier!=HSD_GOBJ_CLASS_FIGHTER)return true;
    const Fighter* fp=o->data;
    return nativeCallbackSource_Alive(o->object,o->generation,NativeCallbackSource_Fighter) &&
        fp->gobj==o->object && (uint32_t)fp->kind==o->kind;
}
static bool owner_capture(const Source* s,DamageOwner* o,NativeScriptDiagnostic* d)
{
    o->object=((const Item*)s->data)->owner;if(!o->object)return true;
    if(!nativeMatchPool_Reference(&nativeGObjPool,o->object,&o->reference) || o->reference.lifetime!=s->context.scene_epoch)
        return error(d,"Rumble owner has no live identity in this scene");
    o->data=o->object->user_data;o->classifier=o->object->classifier;
    if(o->classifier==HSD_GOBJ_CLASS_FIGHTER) {
        if(!nativeCallbackSource_Snapshot(o->object,NativeCallbackSource_Fighter,&o->generation))
            return error(d,"Rumble Fighter owner has retired");
        o->kind=(uint32_t)((const Fighter*)o->data)->kind;
    }
    return owner_current(o,s->context.scene_epoch)?true:error(d,"Rumble owner changed during capture");
}
static bool integer(const NativeScriptValue* v,int32_t* out,NativeScriptDiagnostic* d)
{
    NativeScriptValue converted;
    if(!nativeScriptExpression_IsIntegral(v->type))return error(d,"World feedback selector must be integral");
    if(!nativeScriptValue_Convert(v,NativeScriptType_I32,&converted,d))return false;
    *out=(int32_t)converted.as.i64;return true;
}
bool nativeScriptBindingWorldFeedback_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
 const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool quake=semantic==NativeScriptBinding_CameraQuake;
    bool fighter_rumble=semantic==NativeScriptBinding_ControllerRumble;
    if(!out || !args || (!quake&&!fighter_rumble&&semantic!=NativeScriptBinding_ItemRumble) || count!=(quake?4u:2u))
        return error(d,"World feedback signature is invalid");
    int32_t kind,duration=0;
    if(!integer(&args[0],&kind,d) || (!quake&&!integer(&args[1],&duration,d)))return false;
    if(quake&&(kind<=QuakeKind_None||kind>=QuakeKind_Count))return error(d,"Quake kind must be one of the four defined native kinds");
    if(quake&&(args[1].type!=NativeScriptType_F32||args[2].type!=NativeScriptType_F32||args[3].type!=NativeScriptType_F32))
        return error(d,"Quake position requires three marshalled F32 components");
    Source s={0};if(!capture(c,&s,d))return false;
    if(quake) {
        // Native bookkeeping copies these values without coordinate sanitization.
        Vec3 position={args[1].as.f32,args[2].as.f32,args[3].as.f32};
        const Camera* camera=Camera_GetState();NativeMatchPoolReference loop={0};HSD_GObj* retained=NULL;
        if(kind==QuakeKind_Loop && camera->quake_gobj) {
            retained=camera->quake_gobj;
            if(!nativeMatchPool_Reference(&nativeGObjPool,retained,&loop) || loop.lifetime!=c->scene_epoch ||
               retained->classifier!=HSD_GOBJ_CLASS_STAGE)return error(d,"Existing loop quake object has retired");
        }
        Camera_RequestQuake((CmQuakeKind)kind,&position);
        if(retained&&(nativeMatchPool_Resolve(loop)!=retained||retained->classifier!=HSD_GOBJ_CLASS_STAGE||camera->quake_gobj!=retained))
            return error(d,"Captured loop quake object retired during request");
    } else if(fighter_rumble) {
        if(c->source_class!=NativeCallbackSource_Fighter)return error(d,"Controller rumble requires a Fighter source");
        const Fighter* fp=s.data;
        if(!fp->x221F_b4&&(uint32_t)fp->player_id>=Gm_Player_NumMax)return error(d,"Rumble Fighter has an invalid player slot");
        bool eligible=Player_8003544C(fp->player_id,fp->x221F_b4)&&!fp->x221F_b3&&!fp->x2224_b2;
        if(eligible&&(kind<0||(uint32_t)kind>=lbRumblePatternCount||fp->x618_player_id>=PAD_MAX_CONTROLLERS))
            return error(d,"Rumble pattern or controller slot exceeds the native loaded data");
        if(!current(&s))return error(d,"Rumble Fighter retired before native dispatch");
        ftCommon_8007EBAC((Fighter*)s.data,(uint32_t)kind,(uint32_t)duration);
    } else {
        if(c->source_class!=NativeCallbackSource_Item)return error(d,"Item rumble requires an Item source");
        DamageOwner owner={0};if(!owner_capture(&s,&owner,d))return false;
        if(owner.object&&owner.classifier==HSD_GOBJ_CLASS_FIGHTER) {
            const Fighter* fp=owner.data;
            if(!fp->x221F_b4 && (uint32_t)fp->player_id>=Gm_Player_NumMax)return error(d,"Rumble owner has an invalid player slot");
            bool eligible=Player_8003544C(fp->player_id,fp->x221F_b4)&&!fp->x221F_b3&&!fp->x2224_b2;
            if(eligible&&(kind<0||(uint32_t)kind>=lbRumblePatternCount||fp->x618_player_id>=PAD_MAX_CONTROLLERS))
                return error(d,"Rumble pattern or controller slot exceeds the native loaded data");
        }
        if(!current(&s)||!owner_current(&owner,c->scene_epoch)||((const Item*)s.data)->owner!=owner.object)
            return error(d,"Rumble source or owner changed before native dispatch");
        it_80273598(c->source,kind,duration);
        if(!current(&s))return error(d,"Rumble source retired during native dispatch");
        if(!owner_current(&owner,c->scene_epoch)||((const Item*)s.data)->owner!=owner.object)
            return error(d,"Rumble owner changed before output publication");
    }
    if(!current(&s))return error(d,"World feedback source retired before output publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
