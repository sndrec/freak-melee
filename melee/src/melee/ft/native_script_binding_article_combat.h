#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_COMBAT_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_COMBAT_H
#include "native_script_bindings.h"

// Item capsule/collision storage only. The four contact properties have no
// selectors; enabled/radius are pure one-index calls through Invoke.
bool nativeScriptBindingArticleCombat_Read(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue* selectors,uint32_t selector_count,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingArticleCombat_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue*,NativeScriptDiagnostic*);
#endif
