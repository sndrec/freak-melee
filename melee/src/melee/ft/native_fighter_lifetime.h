#ifndef MELEE_FT_NATIVE_FIGHTER_LIFETIME_H
#define MELEE_FT_NATIVE_FIGHTER_LIFETIME_H

#include "native_script_fighter.h"
#include "native_fighter_runtime.h"
#include "native_callback_lifetime.h"
#include "types.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

// Native helpers may invoke authored decisions inside nested engine algorithms.
// This stack snapshot validates the caller before a native continuation reads
// its cached Fighter/CpuFighter again. It executes no hook or gameplay policy.
typedef struct NativeFighterLifetime {
    Fighter_GObj* object;
    Fighter* data;
    NativeMatchPoolReference reference;
    const ftData* fighter_data;
    const NativeScriptFighterProgram* program;
    const NativeFighterDefinition* native_definition;
    NativeScriptReceiverView view;
    uint64_t program_identity,layout_identity,parameter_identity;
    uint32_t generation;
    FighterKind kind;
    bool source_owned;
} NativeFighterLifetime;

static inline bool nativeFighterLifetime_Current(const NativeFighterLifetime* saved)
{
    Fighter_GObj* object=saved->object;
    if(nativeMatchPool_Resolve(saved->reference)!=object || saved->reference.lifetime!=nativeGObjPool.lifetime ||
       object->classifier!=HSD_GOBJ_CLASS_FIGHTER || object->user_data!=saved->data ||
       !nativeCallbackSource_Alive(object,saved->generation,NativeCallbackSource_Fighter) ||
       saved->data->gobj!=object || saved->data->kind!=saved->kind || saved->data->ft_data!=saved->fighter_data ||
       nativeScriptFighter_IsAttached(object)!=saved->source_owned)return false;
    if(!saved->source_owned)return nativeFighterRuntime_Definition(saved->kind)==saved->native_definition;
    NativeScriptReceiverView view;
    const NativeScriptFighterProgram* p=nativeScriptFighter_Program(saved->kind);
    return p==saved->program && p && p->identity==saved->program_identity && p->bank && p->bank->valid &&
        nativeScriptFighter_Status(object)==NativeScriptVM_Ok && nativeScriptFighter_View(object,saved->generation,&view) &&
        view.program==saved->view.program && view.program==&p->bank->declarations && view.storage==saved->view.storage &&
        view.storage && view.storage->live && view.storage->owner==0 && view.storage->generation==saved->generation &&
        view.storage->layout==&p->bank->storage_layout && view.storage->layout->identity==saved->layout_identity &&
        view.storage->parameters==&p->bank->storage_parameters && view.storage->parameters->identity==saved->parameter_identity;
}

static inline bool nativeFighterLifetime_Capture(Fighter* data,NativeFighterLifetime* saved)
{
    if(!data || !saved)return false;
    NativeFighterLifetime value={.object=data->gobj,.data=data};
    if(!nativeMatchPool_Reference(&nativeGObjPool,value.object,&value.reference) ||
       value.object->classifier!=HSD_GOBJ_CLASS_FIGHTER || value.object->user_data!=data ||
       !nativeCallbackSource_Snapshot(value.object,NativeCallbackSource_Fighter,&value.generation))return false;
    value.kind=data->kind;value.fighter_data=data->ft_data;
    value.source_owned=nativeScriptFighter_IsAttached(value.object);
    if(value.source_owned) {
        value.program=nativeScriptFighter_Program(value.kind);
        if(!value.program || !nativeScriptFighter_View(value.object,value.generation,&value.view) ||
           !value.view.storage || !value.view.storage->layout || !value.view.storage->parameters)return false;
        value.program_identity=value.program->identity;
        value.layout_identity=value.view.storage->layout->identity;
        value.parameter_identity=value.view.storage->parameters->identity;
    } else value.native_definition=nativeFighterRuntime_Definition(value.kind);
    if(!nativeFighterLifetime_Current(&value))return false;*saved=value;return true;
}
#endif
