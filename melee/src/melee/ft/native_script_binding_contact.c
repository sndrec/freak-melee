#include "native_script_binding_contact.h"
#include "native_fighter_contact.h"
#include "native_fighter_lifetime.h"
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}

bool nativeScriptBindingContact_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic,const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    (void)arguments;
    if(!context || !output || count ||
       semantic!=NativeScriptBinding_CollisionContactReboundAllowed ||
       context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,"Contact rebound requires a live owned Fighter and no arguments");
    NativeFighterLifetime caller;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller) ||
       !caller.source_owned || context->generation!=caller.generation ||
       context->scene_epoch!=caller.reference.lifetime ||
       nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&caller.program->bank->motion_layout)
        return error(diagnostic,"Contact rebound requires its selected source data and program");
    bool allowed;
    NativeScriptVMStatus status=nativeFighterContact_ReboundAllowed(caller.object,&allowed);
    if(status!=NativeScriptVM_Ok)
        return error(diagnostic,status==NativeScriptVM_SourceRetired?
                     "Contact rebound actor retired during the decision":
                     "Contact rebound actor, relation or owned decision failed");
    if(!nativeFighterLifetime_Current(&caller) || !nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,"Contact rebound caller changed before publication");
    *output=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=allowed};return true;
}
