#include <melee/ft/native_script_bindings.h>
#include <melee/it/native_script_item_actor.h>

// This compiler fixture never schedules engine actors. These deliberately absent
// native execution boundaries satisfy the compiled catalog's runtime symbols;
// actual registry/property/kinetic execution has separate native unit evidence.
bool nativeScriptBindings_Context(void* actor,uint32_t source_class,uint32_t generation,
    NativeScriptRuntimeContext* context,NativeScriptDiagnostic* diagnostic)
{(void)actor;(void)source_class;(void)generation;(void)context;(void)diagnostic;return false;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* context)
{(void)context;return false;}
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)selectors;(void)count;(void)result;(void)diagnostic;return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)selectors;(void)count;(void)value;(void)diagnostic;return false;}
bool nativeScriptBindingKinetics_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)result;(void)diagnostic;return false;}
bool nativeScriptBindingKinetics_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)value;(void)diagnostic;return false;}
bool nativeScriptBindingKinetics_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)values;(void)count;(void)result;(void)diagnostic;return false;}
bool nativeScriptBindingState_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)values;(void)count;(void)result;(void)diagnostic;return false;}

bool nativeScriptBindingSetup_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)values;(void)count;(void)result;(void)diagnostic;return false;}

bool nativeScriptBindingCombat_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)values;(void)count;(void)result;(void)diagnostic;return false;}

bool nativeScriptBindingTransition_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
