#include "native_script_actor_access.h"
#include "native_script_fighter.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/ef/native_script_effect_actor.h>
bool nativeScriptActors_Lookup(void* unused,void* actor,uint32_t source_class,uint32_t generation,NativeScriptReceiverView* result)
{
    (void)unused;
    if(source_class==NativeCallbackSource_Fighter)return nativeScriptFighter_View(actor,generation,result);
    if(source_class==NativeCallbackSource_Item)return nativeScriptItem_View(actor,generation,result);
    if(source_class==NativeCallbackSource_ModelEffect)return nativeScriptEffect_View(actor,generation,result);
    return false;
}
bool nativeScriptActors_Helper(void* unused,void* actor,uint32_t generation,const NativeScriptProgramIndex* expected,
    uint32_t function,const NativeScriptValue* args,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)unused;
    NativeScriptReceiverView view;
    if(!nativeScriptActors_Lookup(NULL,actor,NativeCallbackSource_Fighter,generation,&view) || view.program!=expected ||
        function>=expected->function_count || expected->functions[function].owner || expected->functions[function].callback)return false;
    return nativeScriptFighter_Helper(actor,generation,function,args,count,result,diagnostic)==NativeScriptVM_Ok;
}
