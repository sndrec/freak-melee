#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_MOTION_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_MOTION_H
#include "native_script_bindings.h"

bool nativeScriptBindingArticleMotion_Read(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue* selectors,uint32_t selector_count,
    NativeScriptValue*,NativeScriptDiagnostic*);
// release_attachment(Float XYZ[, Bool preserve_held]) uses the actual native
// release kernel and retains its Item/owner/held identities through drop hooks.
// Both stage_collision() and stage_collision(Bool) execute native collision
// kernels here. The zero-argument form selects the original true path.
bool nativeScriptBindingArticleMotion_Invoke(NativeScriptRuntimeContext*,uint32_t,
    const NativeScriptValue* arguments,uint32_t count,
    NativeScriptValue*,NativeScriptDiagnostic*);
#endif
