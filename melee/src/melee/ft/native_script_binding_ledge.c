#include "native_script_binding_ledge.h"
#include "native_callback_lifetime.h"
#include "native_fighter_runtime.h"
#include "native_fighter_lifetime.h"
#include "fighter.h"
#include "ft_081B.h"
#include "ftcliffcommon.h"
#include "kinds/ftCommon/ftCo_JumpAerial.h"
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false;}

bool nativeScriptBindingLedge_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||
       !nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Ledge/jump source is unavailable or retired");
    bool catch_ledge=semantic==NativeScriptBinding_StateCatchLedge;
    bool ground=semantic==NativeScriptBinding_CollisionCheckGroundLedge;
    if((ground&&(count!=1||!arguments||arguments[0].type!=NativeScriptType_I32))||
       (!ground&&count)||(!catch_ledge&&(semantic<NativeScriptBinding_CollisionCheckGroundLedge||
       semantic>NativeScriptBinding_ControlsTryAerialJump)))
        return error(diagnostic,"Invalid ledge/jump operation or typed arguments");
    HSD_GObj* object=context->source;Fighter* fighter=object->user_data;
    bool owned=nativeFighterRuntime_SourcePackage(fighter->kind)!=NULL;
    if(owned&&!nativeScriptBindings_SourceFighterLayout(context,diagnostic))return false;
    if(catch_ledge) {
        NativeFighterLifetime saved;
        if(!nativeScriptBindings_SourceFighterLayout(context,diagnostic)||
           !nativeFighterLifetime_Capture(fighter,&saved))
            return error(diagnostic,"Ledge entry requires a live owned fighter");
        ftCliffCommon_80081370(object);
        if(!nativeFighterLifetime_Current(&saved)||!nativeScriptBindings_SourceAlive(context)||
           !nativeScriptBindings_SourceFighterLayout(context,diagnostic))
            return error(diagnostic,"Ledge entry source retired or faulted before completion");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    NativeFighterLifetime saved;
    if(owned && (!nativeFighterLifetime_Capture(fighter,&saved) || !saved.source_owned ||
       saved.object!=object || saved.generation!=context->generation ||
       saved.reference.lifetime!=context->scene_epoch ||
       nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout))
        return error(diagnostic,"Ledge/jump requires its retained owned Fighter, program and storage");
    bool value;
    if(ground)value=ft_CheckGroundAndLedge(object,(int)arguments[0].as.i64);
    else if(semantic==NativeScriptBinding_CollisionCliffCatch)value=ftCliffCommon_80081298(object);
    else value=ftCo_800CB870(object);
    if(!nativeScriptBindings_SourceAlive(context)||
       (owned && (context->source!=saved.object || context->generation!=saved.generation ||
        context->scene_epoch!=saved.reference.lifetime || !nativeFighterLifetime_Current(&saved) ||
        nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout)))
        return error(diagnostic,"Ledge/jump source retired or faulted before result publication");
    *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value};return true;
}
