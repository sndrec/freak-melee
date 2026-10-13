#ifndef MELEE_FT_NATIVE_SCRIPT_ACTOR_ACCESS_H
#define MELEE_FT_NATIVE_SCRIPT_ACTOR_ACCESS_H
#include "native_script_receiver.h"
bool nativeScriptActors_Lookup(void*,void* actor,uint32_t source_class,uint32_t generation,NativeScriptReceiverView*);
bool nativeScriptActors_Helper(void*,void* actor,uint32_t generation,const NativeScriptProgramIndex*,uint32_t function,
    const NativeScriptValue*,uint32_t count,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
