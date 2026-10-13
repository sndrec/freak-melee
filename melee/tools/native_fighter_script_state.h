#ifndef NATIVE_FIGHTER_SCRIPT_STATE_H
#define NATIVE_FIGHTER_SCRIPT_STATE_H

#include "native_fighter_script.h"

enum { SCRIPT_STATE_ARRAY_LIMIT = 128, SCRIPT_ARTICLE_STATE_BYTES = 512 };

typedef struct NativeScriptStateReference {
    const ScriptStateField* field;
    const char* after;
    char index[192];
} NativeScriptStateReference;

bool nativeFighterScript_AddStateField(ScriptStateTable*, const char*);
const ScriptStateField* nativeFighterScript_StateField(const ScriptStateTable*,
                                                      const char*);
// Returns 0 for a different prefix, 1 for a valid reference, -1 for invalid.
int nativeScriptState_Reference(const ScriptStateTable*, const char*,
                                const char* prefix, NativeScriptStateReference*);
bool nativeScriptState_ArrayRead(const ScriptStateField*, const char* bytes,
                                 const char* translated_index, char*, size_t);
bool nativeScriptState_ArrayWrite(const ScriptStateField*, const char* bytes,
                                  const char* translated_index,
                                  const char* translated_value, char*, size_t);
unsigned nativeScriptState_ArrayTypes(const ScriptStateTable*);
bool nativeScriptState_EmitArrayAccess(FILE*, unsigned type_mask);
bool nativeScriptState_EmitMetadata(FILE*, const ScriptStateTable*,
                                    const char* symbol, bool constant);
bool nativeScriptState_EmitCombinedMetadata(FILE*, const ScriptStateTable*);


#endif
