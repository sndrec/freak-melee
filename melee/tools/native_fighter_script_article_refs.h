#ifndef NATIVE_FIGHTER_SCRIPT_ARTICLE_REFS_H
#define NATIVE_FIGHTER_SCRIPT_ARTICLE_REFS_H

#include "native_fighter_script_state.h"

enum { SCRIPT_ARTICLE_DECLARATIONS = 16, SCRIPT_ARTICLE_REF_EXPRESSION = 192 };

typedef struct NativeScriptArticleDeclaration {
    char kind[64];
    unsigned motion_states;
    ScriptStateTable state;
} NativeScriptArticleDeclaration;

typedef struct NativeScriptArticleCatalog {
    NativeScriptArticleDeclaration declarations[SCRIPT_ARTICLE_DECLARATIONS];
    unsigned count;
} NativeScriptArticleCatalog;

typedef struct NativeScriptArticleReference {
    const NativeScriptArticleDeclaration* declaration;
    const ScriptStateField* field; // NULL for exists/current_owner.
    bool current_owner;
    unsigned field_index;
    ScriptStateType type;
    char handle[SCRIPT_ARTICLE_REF_EXPRESSION];
    char index[SCRIPT_ARTICLE_REF_EXPRESSION];
    const char* after;
} NativeScriptArticleReference;

// Build before translating any fighter, helper or article callback. The owning
// item program retains this fixed catalog until all generated code is emitted.
bool nativeScriptArticleRefs_CatalogParse(NativeScriptArticleCatalog*,
                                         const char*, size_t,
                                         NativeFighterScriptError*);
const NativeScriptArticleDeclaration* nativeScriptArticleRefs_Declaration(
    const NativeScriptArticleCatalog*, const char* kind);

// 0: another parser owns the prefix; 1: valid read; -1: malformed/undeclared.
// Owning expression parsers must type-check and translate handle/index as ints.
int nativeScriptArticleRefs_Reference(const NativeScriptArticleCatalog*,
                                      const char*, NativeScriptArticleReference*);
bool nativeScriptArticleRefs_Emit(const NativeScriptArticleReference*,
                                 const char* translated_handle,
                                 const char* translated_index,
                                 char*, size_t);

#endif
