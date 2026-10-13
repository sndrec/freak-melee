#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_LIFECYCLE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_LIFECYCLE_H

#include "native_script_bindings.h"

// Relations belong to the source Item actor. Clearing attachment ownership
// does not discard original-creator provenance or run physical item release.
bool nativeScriptBindingArticleLifecycle_Invoke(NativeScriptRuntimeContext*,
    uint32_t semantic,const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue* result,NativeScriptDiagnostic*);

#endif
