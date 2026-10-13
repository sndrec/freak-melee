#include "native_fighter_contact.h"
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include "native_script_bindings.h"
#include "fighter.h"

static NativeScriptVMStatus current(const NativeFighterLifetime* saved)
{
    if(nativeFighterLifetime_Current(saved))return NativeScriptVM_Ok;
    if(nativeMatchPool_Resolve(saved->reference)!=saved->object)
        return NativeScriptVM_SourceRetired;
    // Never inspect substituted userdata/class as the cached Fighter.
    if(saved->object->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       saved->object->user_data!=saved->data)return NativeScriptVM_Error;
    if(!nativeCallbackSource_Alive(saved->object,saved->generation,
                                  NativeCallbackSource_Fighter))
        return NativeScriptVM_SourceRetired;
    if(saved->source_owned) {
        NativeScriptVMStatus status=nativeScriptFighter_Status(saved->object);
        if(status!=NativeScriptVM_Ok)return status;
    }
    return NativeScriptVM_Error;
}

static NativeScriptVMStatus capture(Fighter_GObj* object,
                                   NativeFighterLifetime* saved)
{
    NativeMatchPoolReference reference;
    if(!object || !nativeMatchPool_Reference(&nativeGObjPool,object,&reference) ||
       object->classifier!=HSD_GOBJ_CLASS_FIGHTER || !object->user_data)
        return NativeScriptVM_Error;
    if(!nativeFighterLifetime_Capture(object->user_data,saved)) {
        if(nativeScriptFighter_IsAttached(object)) {
            NativeScriptVMStatus status=nativeScriptFighter_Status(object);
            if(status!=NativeScriptVM_Ok)return status;
        }
        return NativeScriptVM_Error;
    }
    // A selected source without its actor is not an ordinary stock recipient.
    if(!saved->source_owned && nativeFighterRuntime_SourcePackage(saved->kind))
        return NativeScriptVM_Error;
    if(saved->source_owned) {
        NativeScriptRuntimeContext context={.source=object,
            .source_class=NativeCallbackSource_Fighter,.generation=saved->generation,
            .scene_epoch=saved->reference.lifetime};
        if(nativeScriptBindings_SourceFighterLayout(&context,NULL)!=
           &saved->program->bank->motion_layout)return NativeScriptVM_Error;
    }
    return NativeScriptVM_Ok;
}

NativeScriptVMStatus nativeFighterContact_ReboundAllowed(Fighter_GObj* object,
                                                       bool* output)
{
    if(!output)return NativeScriptVM_Error;
    NativeFighterLifetime source,recipient;
    NativeScriptVMStatus status=capture(object,&source);
    if(status!=NativeScriptVM_Ok)return status;
    if(!source.data->x221C_b5 || !source.data->unk_gobj) {
        *output=true;return NativeScriptVM_Ok;
    }
    Fighter_GObj* target=source.data->unk_gobj;
    status=capture(target,&recipient);
    if(status!=NativeScriptVM_Ok)return status;
    bool allowed=false;
    if(recipient.source_owned) {
        NativeScriptValue value;
        status=nativeScriptFighter_Run(target,NativeScriptFighter_BodyRebound,
                                      NULL,0, NULL,&value);
        if(status==NativeScriptVM_Ok) {
            if(value.type!=NativeScriptType_Bool)status=NativeScriptVM_Error;
            else allowed=value.as.u64!=0;
        }
    } else if(recipient.native_definition &&
              recipient.native_definition->on_body_rebound) {
        if(!nativeCallbackDispatch_Enter())return NativeScriptVM_Error;
        allowed=recipient.native_definition->on_body_rebound(target);
        nativeCallbackDispatch_Leave();
    } else {
        switch(recipient.kind) {
        case Ft_Kind_Mars:
        case Ft_Kind_Emblem:
            allowed=recipient.data->motion_id==369 || recipient.data->motion_id==371;
            break;
        case Ft_Kind_Peach:
            allowed=recipient.data->motion_id==365 || recipient.data->motion_id==367;
            break;
        default:break;
        }
    }
    // False is a gameplay result, never permission to skip continuation guards.
    NativeScriptVMStatus source_status=current(&source);
    NativeScriptVMStatus recipient_status=current(&recipient);
    if(source_status!=NativeScriptVM_Ok)return source_status;
    if(recipient_status!=NativeScriptVM_Ok)return recipient_status;
    if(status!=NativeScriptVM_Ok)return status;
    if(!source.data->x221C_b5 || source.data->unk_gobj!=target)
        return NativeScriptVM_Error;
    *output=allowed;return NativeScriptVM_Ok;
}
