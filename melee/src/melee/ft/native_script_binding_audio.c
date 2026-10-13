#include "native_script_binding_audio.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include <melee/it/native_item_lifetime.h>
#include <melee/lb/lbaudio_ax.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic, const char* message)
{
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    return false;
}

bool nativeScriptBindingAudio_Invoke(NativeScriptRuntimeContext* context,
    uint32_t semantic, const NativeScriptValue* arguments, uint32_t count,
    NativeScriptValue* output, NativeScriptDiagnostic* diagnostic)
{
    if(semantic!=NativeScriptBinding_AudioPlayGlobal || !arguments || !output || count!=3)
        return error(diagnostic,"Global audio requires three Int arguments and an output");
    int values[3];
    for(unsigned i=0;i<3;++i) {
        if(arguments[i].type!=NativeScriptType_I32 || arguments[i].as.i64<INT32_MIN ||
           arguments[i].as.i64>INT32_MAX)
            return error(diagnostic,"Global audio arguments require native signed 32-bit Int values");
        values[i]=(int32_t)arguments[i].as.i64;
    }
    // The ordinary lbAudioAx path doubles signed int volume/pan before clamp.
    // Its high-ID fallback discards both arguments, so it cannot overflow.
    if(values[0]<0x83D61 &&
       (values[1]<INT32_MIN/2 || values[1]>INT32_MAX/2 ||
        values[2]<INT32_MIN/2 || values[2]>INT32_MAX/2))
        return error(diagnostic,"Global audio volume/pan would overflow the native doubling operation");

    NativeMatchPoolReference reference;
    if(!context || !context->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference) ||
       reference.lifetime!=context->scene_epoch)
        return error(diagnostic,"Global audio requires its current physical source scene");
    HSD_GObj* object=context->source;
    NativeFighterLifetime fighter;
    NativeItemLifetime item;
    bool from_fighter=context->source_class==NativeCallbackSource_Fighter;
    if(from_fighter) {
        if(object->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
           !nativeScriptBindings_SourceAlive(context) ||
           !nativeFighterLifetime_Capture(object->user_data,&fighter) ||
           !fighter.source_owned || fighter.generation!=context->generation ||
           fighter.reference.lifetime!=context->scene_epoch ||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&fighter.program->bank->motion_layout)
            return error(diagnostic,"Global audio requires its exact owned Fighter program/storage");
    } else {
        if(context->source_class!=NativeCallbackSource_Item ||
           !nativeItemLifetime_Capture(object,&item) || !item.source_owned ||
           item.generation!=context->generation || item.reference.lifetime!=context->scene_epoch ||
           !nativeScriptBindings_SourceAlive(context) ||
           item.program->article!=item.article || item.program->kind!=item.kind ||
           item.program->owner!=item.data->native_script_owner ||
           item.program->author_kind!=item.data->native_article_author_kind)
            return error(diagnostic,"Global audio requires its exact owned Article program/storage");
    }

    int handle=lbAudioAx_800237A8(values[0],values[1],values[2]);
    if(context->source!=object || context->scene_epoch!=reference.lifetime ||
       context->source_class!=(from_fighter?NativeCallbackSource_Fighter:NativeCallbackSource_Item) ||
       context->generation!=(from_fighter?fighter.generation:item.generation) ||
       !(from_fighter?nativeFighterLifetime_Current(&fighter):nativeItemLifetime_Current(&item)) ||
       !nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,"Global audio source retired before handle publication");
    if(from_fighter && nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&fighter.program->bank->motion_layout)
        return false;
    if(!from_fighter && (item.program->article!=item.article || item.program->kind!=item.kind ||
       item.program->owner!=item.data->native_script_owner ||
       item.program->author_kind!=item.data->native_article_author_kind))
        return error(diagnostic,"Global audio Article ownership changed before handle publication");
    *output=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=handle};
    return true;
}
