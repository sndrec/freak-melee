#ifndef MELEE_FT_NATIVE_SOURCE_ITEM_BLUEPRINT_IO_H
#define MELEE_FT_NATIVE_SOURCE_ITEM_BLUEPRINT_IO_H
#include "native_script_ir.h"
#include <stddef.h>

enum { NATIVE_SOURCE_BLUEPRINT_HEADER=16, NATIVE_SOURCE_BLUEPRINT_ARENA=2*1024*1024 };
#ifdef __cplusplus
extern "C" {
#endif
// Write an owned DAT/Article envelope; Validate admits its actual imported
// Article, model and commands in a temporary native arena before publication.
bool nativeSourceItemBlueprint_Write(const void* dat,size_t bytes,uint32_t article,
    void* output,size_t capacity,size_t* written,NativeScriptDiagnostic*);
bool nativeSourceItemBlueprint_Validate(const void*,size_t,NativeScriptDiagnostic*);
#ifdef __cplusplus
}
#endif
#endif
