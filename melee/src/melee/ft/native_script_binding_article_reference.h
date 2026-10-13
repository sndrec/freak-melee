#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_REFERENCE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_REFERENCE_H
#include "native_script_receiver.h"
bool nativeScriptBindingArticleReference_Read(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
 uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingArticleReference_Invoke(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
 uint32_t semantic,const NativeScriptValue* selectors,uint32_t selector_count,NativeScriptValue* arguments,
 uint32_t count,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
