#ifndef MELEE_FT_NATIVE_SCRIPT_METADATA_H
#define MELEE_FT_NATIVE_SCRIPT_METADATA_H

#include "native_script_storage.h"
#include <sysdolphin/baselib/dat.h>

typedef struct NativeScriptMetadataDat {
    const void* bytes;
    size_t size;
    const char* fighter_root;
} NativeScriptMetadataDat;

typedef struct NativeScriptMetadataImport {
    uint32_t fighter_imports, unresolved_entity_imports;
    uint32_t attribute_bytes;
} NativeScriptMetadataImport;
// One checked scalar import from a caller-validated owned DAT attribute block.
// Used by fighter and entity imports; runtime values never alias these bytes.
bool nativeScriptMetadata_ReadAttribute(const HSD_Dat*,uint32_t begin,uint32_t bytes,
    const NativeScriptValueDeclaration*,NativeScriptValue*,NativeScriptDiagnostic*);

// Copy indexed root/fighter Attribute declarations directly from the actual
// DAT's owned special-attribute block (root+4). Values are typed immutable
// supplies, not aliases or rewritten source. Bounds/types for every candidate
// are validated before any supplies/count/report publication.
// Entity imports are counted explicitly and remain outside this closure; they
// must never accidentally read an unrelated fighter attribute block.
bool nativeScriptMetadata_ImportFighterParameters(const NativeScriptProgramIndex*,
    const NativeScriptMetadataDat*, NativeScriptParameterSupply*, uint32_t capacity,
    uint32_t* count, NativeScriptMetadataImport*, NativeScriptDiagnostic*);

#endif
