#include "native_script_binding_actions.h"
#include "native_script_bindings.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include "native_fighter_runtime.h"
#include "native_fighter_geometry.h"
#include "ft_081B.h"
#include "ft_0819.h"
#include "ftcliffcommon.h"
#include "ftwalljump.h"
#include "types.h"
#include "kinds/ftCommon/ftCo_FallSpecial.h"
#include "kinds/ftCommon/ftCo_Landing.h"
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <math.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}

static bool copy_caller_current(NativeScriptRuntimeContext* context,
    const NativeFighterLifetime* caller)
{
    return context->source==caller->object &&
        context->source_class==NativeCallbackSource_Fighter &&
        context->generation==caller->generation &&
        context->scene_epoch==caller->reference.lifetime &&
        nativeFighterLifetime_Current(caller) && nativeScriptBindings_SourceAlive(context) &&
        nativeScriptBindings_SourceFighterLayout(context,NULL)==&caller->program->bank->motion_layout;
}

static bool copy_peer(uint32_t id,uint64_t epoch,NativeFighterLifetime* peer,
    NativeScriptDiagnostic* diagnostic)
{
    if(!id || !HSD_GObjPLinkHead)return true;
    uint32_t visited=0;
    for(Fighter_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];object;object=object->next) {
        NativeMatchPoolReference physical;
        // Admit each traversed node before reading it or following its link.
        if(visited++>=nativeGObjPool.capacity ||
           !nativeMatchPool_Reference(&nativeGObjPool,object,&physical) || physical.lifetime!=epoch)
            return error(diagnostic,"Collision copy Fighter registry is stale or cyclic");
        if(object->classifier!=HSD_GOBJ_CLASS_FIGHTER || !object->user_data)continue;
        Fighter* data=object->user_data;
        if((uint32_t)data->x8_spawnNum!=id)continue;
        if(!nativeFighterLifetime_Capture(data,peer) || peer->object!=object ||
           peer->generation!=id || peer->reference.lifetime!=epoch)
            return error(diagnostic,"Collision copy target has changed physical/program/storage identity");
        return true;
    }
    return true;
}

bool nativeScriptBindingCollision_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_CollisionGroundStandardBox) {
        if(!out || !args || count!=6)return error(d,"Custom standard ground collision requires six Float extents");
        for(unsigned i=0;i<6;++i)
            if(args[i].type!=NativeScriptType_F32 || !isfinite(args[i].as.f32))
                return error(d,"Custom standard ground collision extents must be finite Floats");
        if(!context || context->source_class!=NativeCallbackSource_Fighter ||
           !nativeScriptBindings_SourceFighterLayout(context,d))
            return error(d,"Custom standard ground collision requires its live owned Fighter");
        NativeFighterLifetime caller;
        if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller) ||
           !caller.source_owned || !copy_caller_current(context,&caller))
            return error(d,"Custom standard ground collision source identity is unavailable");
        ftCollisionBox box={args[0].as.f32,args[1].as.f32,
            {args[2].as.f32,args[3].as.f32},{args[4].as.f32,args[5].as.f32}};
        // Native facing conversion, ECB history, world-position publication
        // and Fall entry on loss of contact are all part of this operation.
        ft_800847D0(caller.object,&box);
        if(!copy_caller_current(context,&caller))
            return error(d,"Custom standard ground collision source retired before publication");
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_CollisionAirCatchHit) {
        if(!context || !out || count || context->source_class!=NativeCallbackSource_Fighter ||
           !nativeScriptBindings_SourceAlive(context))
            return error(d,"Air catch hit collision requires its live Fighter source and no arguments");
        NativeFighterLifetime caller;
        if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller) ||
           !caller.source_owned || !copy_caller_current(context,&caller))
            return error(d,"Air catch hit collision requires its exact owned Fighter program/storage");
        ftCo_AirCatchHit_Coll(context->source);
        if(!copy_caller_current(context,&caller))
            return error(d,"Air catch hit collision source changed before publication");
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }

    if(semantic==NativeScriptBinding_CollisionProbeLedge || semantic==NativeScriptBinding_CollisionLedgeAvailable) {
        bool probe=semantic==NativeScriptBinding_CollisionProbeLedge;
        if(!out || count!=(probe?2:0) || (probe && (!args || args[0].type!=NativeScriptType_F32 ||
           args[1].type!=NativeScriptType_F32 || !isfinite(args[0].as.f32) || !isfinite(args[1].as.f32))))
            return error(d,"Ledge probe requires two finite Floats; availability takes no arguments");
        NativeMatchPoolReference physical;
        if(!context || context->source_class!=NativeCallbackSource_Fighter || !context->scene_epoch ||
           !nativeMatchPool_Reference(&nativeGObjPool,context->source,&physical) ||
           physical.lifetime!=context->scene_epoch ||
           ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
           !nativeScriptBindings_SourceAlive(context))
            return error(d,"Ledge query requires its current physical Fighter source");
        NativeFighterLifetime caller;
        if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller) ||
           !caller.source_owned || !copy_caller_current(context,&caller))
            return error(d,"Ledge query requires its exact owned Fighter program/storage");
        bool value=probe?nativeFighterGeometry_ProbeLedge(context->source,args[0].as.f32,args[1].as.f32):
            ft_80082E3C(context->source)==NULL;
        if(!copy_caller_current(context,&caller))
            return error(d,"Ledge query source changed before publication");
        *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value};return true;
    }
    if(semantic==NativeScriptBinding_CollisionCopyFrom) {
        if(!args || !out || count!=1 || args->type!=NativeScriptType_I32 ||
           args->as.i64<INT32_MIN || args->as.i64>INT32_MAX)
            return error(d,"Collision copy requires one native signed Int creation ID");
        NativeMatchPoolReference physical;
        if(!context || context->source_class!=NativeCallbackSource_Fighter || !context->scene_epoch ||
           !nativeMatchPool_Reference(&nativeGObjPool,context->source,&physical) ||
           physical.lifetime!=context->scene_epoch ||
           ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
           !nativeScriptBindings_SourceAlive(context))
            return error(d,"Collision copy requires its current physical Fighter source");
        NativeFighterLifetime caller,peer={0};
        if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller) ||
           !caller.source_owned || !copy_caller_current(context,&caller))
            return error(d,"Collision copy requires its exact owned Fighter program/storage");
        uint32_t id=(uint32_t)(int32_t)args->as.i64;
        if(!copy_peer(id,context->scene_epoch,&peer,d))return false;
        if(!copy_caller_current(context,&caller) || (peer.object && !nativeFighterLifetime_Current(&peer)))
            return error(d,"Collision copy caller or target changed before mutation");
        if(peer.object)mpCopyCollData(&peer.data->coll_data,&caller.data->coll_data,2);
        if(!copy_caller_current(context,&caller) || (peer.object && !nativeFighterLifetime_Current(&peer)))
            return error(d,"Collision copy caller or target changed before publication");
        *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=peer.object!=NULL};
        return true;
    }
    if(semantic==NativeScriptBinding_CollisionGroundWithNudge) {
        if(!context || !out || count || context->source_class!=NativeCallbackSource_Fighter ||
           !nativeScriptBindings_SourceFighterLayout(context,d))
            return error(d,"Ground nudge collision requires a live owned Fighter and no arguments");
        NativeFighterLifetime lifetime;
        if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&lifetime))
            return error(d,"Ground nudge collision source is unavailable");
        ft_80084280(context->source);
        if(!nativeFighterLifetime_Current(&lifetime) || !nativeScriptBindings_SourceFighterLayout(context,d))
            return error(d,"Ground nudge collision source retired before publication");
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    (void)count;(void)out;
    if(!context || context->source_class!=NativeCallbackSource_Fighter || !nativeScriptBindings_SourceAlive(context))return error(d,"Standard collision source is unavailable");
    HSD_GObj* object=context->source;Fighter* fp=object->user_data;
    bool owned=nativeFighterRuntime_SourcePackage(fp->kind)!=NULL;
    if(owned && !nativeScriptBindings_SourceFighterLayout(context,d))return false;
    if(semantic==NativeScriptBinding_CollisionGroundStandard)ft_80084104(object);
    else if(semantic==NativeScriptBinding_CollisionAirStandard)ft_80083B68(object);
    else if(semantic==NativeScriptBinding_CollisionAirSpecialLanding) {
        if(!isfinite(args[1].as.f32))return error(d,"Special landing lag is not finite");
        // ft_800831CC's collision and wall/cliff ordering, with its authored
        // callback replaced by the direct typed landing arguments. No callback
        // pointer, generated helper or global parameter scratch is installed.
        CollData* coll=&fp->coll_data;coll->last_pos=coll->cur_pos;coll->cur_pos=fp->cur_pos;
        bool contact;
        if(fp->x2064_ledgeCooldown || fp->x2224_b2)contact=mpColl_80047AC8(coll,ftCo_80096CC8,object);
        else {mpCollSetFacingDir(coll,fp->facing_dir<0?-1:1);contact=mpColl_80047E14(coll,ftCo_80096CC8,object);}
        if(!nativeScriptBindings_SourceAlive(context) || (owned && !nativeScriptBindings_SourceFighterLayout(context,d)))return error(d,"Collision source retired during ground/ledge probe");
        fp->cur_pos=coll->cur_pos;bool interrupted=ft_80081A00(object);
        if(!nativeScriptBindings_SourceAlive(context) || (owned && !nativeScriptBindings_SourceFighterLayout(context,d)))return error(d,"Collision source retired during environmental response");
        if(!interrupted && contact)ftCo_LandingFallSpecial_Enter(object,args[0].as.u64!=0,args[1].as.f32);
        else {
            bool jumped=ftWallJump_8008169C(object);
            if(!nativeScriptBindings_SourceAlive(context) || (owned && !nativeScriptBindings_SourceFighterLayout(context,d)))return error(d,"Collision source retired during walljump response");
            if(!jumped)(void)ftCliffCommon_80081298(object);
        }
    } else return error(d,"Unknown standard collision operation");
    if(!nativeScriptBindings_SourceAlive(context) || (owned && !nativeScriptBindings_SourceFighterLayout(context,d)))
        return error(d,"Standard collision source retired before publication");
    return true;
}
