#include "native_script_effect_actor.h"
#include "types.h"
#include <melee/ft/types.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_runtime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <dolphin/os.h>

// Deferred requests store identities and raw part selection, never a VM stack
// pointer or a bank pointer. Reload/retirement drops stale work before any
// source program or joint is dereferenced. Position is sampled at queue flush.
NativeScriptVMStatus nativeScriptEffect_ProcessDeferred(Fighter_GObj* object,
                                       const EF_QueuedEffect* queued)
{
    NativeMatchPoolReference reference;
    NativeScriptReceiverView view;
    if(!queued||!queued->native_script_owner||
       !nativeMatchPool_Reference(&nativeGObjPool,object,&reference)||
       reference.lifetime!=queued->native_script_epoch||
       !nativeScriptFighter_View(object,queued->native_creator_generation,&view)||
       !view.storage||!view.storage->live||!view.storage->layout||!view.storage->parameters||
       view.storage->layout->identity!=queued->native_script_layout_identity||
       view.storage->parameters->identity!=queued->native_script_parameter_identity||
       object->hsd_obj!=queued->native_script_root)return NativeScriptVM_SourceRetired;
    Fighter* fighter=object->user_data;
    const NativeScriptEffectProgram* program=
        nativeScriptEffect_Program(fighter->kind,queued->native_script_owner);
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
    int part=queued->native_sample_part;
    if(!program||program->identity!=queued->native_script_program_identity||
       program->family!=EF_MODEL_POSITION||program->gfx_id!=queued->gfx_id||
       !package||!package->assets.ready||part<0||part>=MAX_FT_PARTS||
       (uint32_t)part>=package->assets.parts.parts_num||!fighter->parts||
       fighter->parts[part].joint!=queued->jobj)return NativeScriptVM_SourceRetired;
    NativeScriptRuntimeContext context={.source=object,
        .source_class=NativeCallbackSource_Fighter,
        .generation=queued->native_creator_generation,
        .scene_epoch=queued->native_script_epoch};
    NativeScriptDiagnostic diagnostic={0};
    NativeScriptVMStatus status=nativeScriptEffect_Spawn(&context,
        queued->native_script_owner,true,nativeFighterRig_Id(&package->assets.rig,(unsigned)part),false,&diagnostic);
    if(status==NativeScriptVM_Error)
        OSReport("[script] Source %u deferred effect %u: %s\n",
            context.generation,queued->native_script_owner,diagnostic.message);
    return status;
}
