#ifndef MELEE_FT_NATIVE_SOURCE_BANK_H
#define MELEE_FT_NATIVE_SOURCE_BANK_H
#include "native_script_compile.h"

enum {
    NATIVE_SOURCE_SOURCE_BYTES=1024*1024,NATIVE_SOURCE_SYNTAX_NODES=16384,
    NATIVE_SOURCE_IR_NODES=65536,NATIVE_SOURCE_ARGUMENTS=65536,
    NATIVE_SOURCE_FUNCTIONS=512,NATIVE_SOURCE_FUNCTION_PARAMETERS=8192,
    NATIVE_SOURCE_VALUES=2048,NATIVE_SOURCE_LOCALS=512,
    NATIVE_SOURCE_INSTRUCTIONS=131072,NATIVE_SOURCE_OWNERS=129,
};
typedef struct NativeSourceBank {
    NativeScriptCompileBank bank;
    void* arena;
    size_t arena_bytes;
} NativeSourceBank;
typedef struct NativeSourceBankStats {
    size_t reserved_bytes,active_buffer_bytes;
    uint32_t source_bytes,syntax_nodes,ir_nodes,arguments,functions,parameters,values,instructions,owners;
    uint32_t maximum_local_count,maximum_function_registers,maximum_call_registers,maximum_call_depth;
} NativeSourceBankStats;
bool nativeSourceBank_Allocate(NativeSourceBank*,NativeScriptDiagnostic*);
void nativeSourceBank_Release(NativeSourceBank*);
bool nativeSourceBank_Stats(const NativeSourceBank*,NativeSourceBankStats*);
#endif
