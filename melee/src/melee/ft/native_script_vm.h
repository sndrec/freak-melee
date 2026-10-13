#ifndef MELEE_FT_NATIVE_SCRIPT_VM_H
#define MELEE_FT_NATIVE_SCRIPT_VM_H

#include "native_script_bytecode.h"

typedef struct NativeScriptObjectValue {
    uint32_t identity, generation, kind;
} NativeScriptObjectValue;
typedef struct NativeScriptTextValue { uint32_t offset, length; } NativeScriptTextValue;

typedef struct NativeScriptValue {
    union {
        uint64_t u64;
        int64_t i64;
        float f32;
        double f64;
        NativeScriptObjectValue object;
        NativeScriptTextValue text;
    } as;
    uint8_t type;
} NativeScriptValue;

// Native execution context is the only opaque pointer boundary. VM values,
// immutable code and source artifacts never contain object or machine pointers.
typedef struct NativeScriptRuntimeContext {
    void* source;
    void* user;
    uint32_t source_class, generation, frame, actor_id;
    uint64_t scene_epoch;
} NativeScriptRuntimeContext;

typedef struct NativeScriptVMHost {
    bool (*source_alive)(NativeScriptRuntimeContext*);
    bool (*read)(NativeScriptRuntimeContext*, uint32_t semantic,
        const NativeScriptValue* selectors, uint32_t selector_count,
        NativeScriptValue* result, NativeScriptDiagnostic*);
    bool (*write)(NativeScriptRuntimeContext*, uint32_t semantic,
        const NativeScriptValue* selectors, uint32_t selector_count,
        const NativeScriptValue* value, NativeScriptDiagnostic*);
    bool (*invoke)(NativeScriptRuntimeContext*, uint32_t semantic,
        const NativeScriptValue* selectors, uint32_t selector_count,
        NativeScriptValue* arguments, uint32_t argument_count,
        NativeScriptValue* result, NativeScriptDiagnostic*);
    // Validate a captured assignment destination after all selector expressions
    // but before evaluating its RHS. This performs no observable getter/write.
    bool (*validate)(NativeScriptRuntimeContext*,uint32_t semantic,
        const NativeScriptValue* selectors,uint32_t selector_count,NativeScriptDiagnostic*);
} NativeScriptVMHost;

typedef struct NativeScriptVMFrame {
    uint32_t function, instruction, register_base;
    uint16_t return_destination;
} NativeScriptVMFrame;

typedef enum NativeScriptVMStatus {
    NativeScriptVM_Ok, NativeScriptVM_Error, NativeScriptVM_SourceRetired,
} NativeScriptVMStatus;

typedef struct NativeScriptVM {
    NativeScriptValue* registers;
    uint32_t register_capacity, register_top;
    NativeScriptVMFrame* frames;
    uint32_t frame_capacity, depth;
    NativeScriptDiagnostic diagnostic;
    uint32_t error_function, error_frame, error_actor;
} NativeScriptVM;

// Synchronous direct helper calls share caller-owned register/frame pools.
// An entire invocation finishes before returning; retirement stops the whole
// frame chain. Source diagnostics include function ID, actor and frame metadata.
NativeScriptVMStatus nativeScriptVM_Run(NativeScriptVM*,
    const NativeScriptBytecodeProgram*, const NativeScriptVMHost*,
    NativeScriptRuntimeContext*, uint32_t function,
    const NativeScriptValue* arguments, uint32_t argument_count,
    NativeScriptValue* result);

// Checked numeric execution; overflow wraps at explicit integer widths, invalid
// division/conversion/shift reports errors rather than C undefined behavior.
bool nativeScriptValue_Convert(const NativeScriptValue*, NativeScriptIRType,
                              NativeScriptValue*, NativeScriptDiagnostic*);
bool nativeScriptValue_Unary(NativeScriptIROperation, const NativeScriptValue*,
                            NativeScriptValue*, NativeScriptDiagnostic*);
bool nativeScriptValue_Binary(NativeScriptIROperation, const NativeScriptValue*,
    const NativeScriptValue*, NativeScriptValue*, NativeScriptDiagnostic*);
bool nativeScriptValue_Truth(const NativeScriptValue*, bool*);

#endif
