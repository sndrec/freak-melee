#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_REFERENCE_RESOLVE_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_ARTICLE_REFERENCE_RESOLVE_H
#include "native_script_binding_spawn.h"
#include "native_script_receiver_semantics.h"
static inline bool nativeScriptBindingArticleReference_Handles(uint32_t semantic)
{
    uint32_t category=semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK;
    return (category>=NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT&&category<=NATIVE_SCRIPT_RECEIVER_ARTICLE_EXISTS)||
        category==NATIVE_SCRIPT_RECEIVER_ARTICLE_PHYSICAL||category==NATIVE_SCRIPT_RECEIVER_ARTICLE_BONE_POSITION;
}
static inline bool nativeScriptBindingArticleReference_Namespace(uint32_t semantic)
{
    uint32_t category=semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK;
    return category==NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE_NAMESPACE||
        category==NATIVE_SCRIPT_RECEIVER_ARTICLE_POSITION_NAMESPACE||
        category==NATIVE_SCRIPT_RECEIVER_ARTICLE_NORMAL_NAMESPACE;
}
static inline uint32_t nativeScriptBindingArticleReference_Base(uint32_t semantic)
{
    uint32_t category=semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK;
    if(category==NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT)return NativeScriptBinding_ArticlesRef;
    if(category==NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION)return NativeScriptBinding_ArticlesReadPrimaryTranslation;
    if(category==NATIVE_SCRIPT_RECEIVER_ARTICLE_BONE_POSITION)return NativeScriptBinding_ArticlesReadBonePosition;
    return semantic;
}
// Exact indexed program/owner context shared with source declaration literals.
// 1 resolved, 0 unrelated, -1 unsupported/malformed Article reference projection.
int nativeScriptBindingArticleReference_TryResolve(void*,const NativeScriptIRArena*,
    const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
#endif
