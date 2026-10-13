#ifndef MELEE_FT_NATIVE_SOURCE_PARAMETERS_H
#define MELEE_FT_NATIVE_SOURCE_PARAMETERS_H
#include "native_source_bank.h"
#include "native_script_metadata.h"
#include "native_script_item_program.h"
#include "native_fighter_parameters.h"
typedef struct NativeSourceParameters {
    NativeScriptMetadataDat fighter;
    const NativeScriptItemMetadataContext* items;
    const char* text;size_t text_bytes;
    NativeFighterParameters overrides;
    NativeScriptParameterSupply supplies[NATIVE_SOURCE_VALUES];
    uint32_t count,root_imports,article_imports,override_count;
} NativeSourceParameters;
bool nativeSourceParameters_Parse(NativeSourceParameters*,NativeScriptDiagnostic*);
// Real load-time provider: DAT scalar imports then exact qualified sidecar
// overrides, copied by the compiler into its immutable parameter bank.
bool nativeSourceParameters_Provide(void*,const NativeScriptProgramIndex*,const NativeScriptParameterSupply**,uint32_t*,NativeScriptDiagnostic*);
#endif
