#ifndef MELEE_FT_NATIVE_SCRIPT_EXECUTION_H
#define MELEE_FT_NATIVE_SCRIPT_EXECUTION_H

#include "native_script_bindings.h"
#include "native_script_event_payload.h"
#include "native_script_storage.h"
#include "native_script_receiver.h"

typedef struct NativeScriptExecutionFunction {
    NativeScriptSourceSpan source;
    uint32_t owner, source_class, event_id;
    bool callback;
} NativeScriptExecutionFunction;

// Immutable declarations/code and live actor storage must refer to the same
// loaded program. VM and payload pools are caller-reserved transient scratch.
typedef struct NativeScriptExecution {
    const NativeScriptProgramIndex* declarations;
    const NativeScriptBytecodeProgram* code;
    NativeScriptStorageInstance* storage;
    NativeScriptVM* vm;
    NativeScriptEventPayloadPool* payloads;
    const NativeScriptExecutionFunction* functions;
    uint32_t function_count, owner, identity, generation;
    uint64_t layout_identity, parameter_identity;
    bool bound;
    // Actual actor controllers supply pure recipient views and direct helper
    // execution. Isolated programs with no receivers need neither callback.
    NativeScriptReceiverLookup receiver_lookup;
    NativeScriptReceiverHelper receiver_helper;
    void* receiver_user;
    uint32_t fighter_kind;
} NativeScriptExecution;

// Resolve/validate function ABI, ownership and callback identities once at load.
// No source-name parsing or parameter-table scan occurs during Run.
bool nativeScriptExecution_BuildFunctions(const NativeScriptProgramIndex*,
    const NativeScriptBytecodeProgram*, NativeScriptExecutionFunction*,
    uint32_t capacity, NativeScriptDiagnostic*);
// Bind a newly created live storage instance to immutable code/configuration.
bool nativeScriptExecution_Bind(NativeScriptExecution*, NativeScriptDiagnostic*);

// Entry validates the actual actor registry, source class, storage lifetime,
// declaration ownership and event ABI before running. Result remains untouched
// on failure/retirement. Nested dispatch may share VM/payload pools, but must use
// its own invocation context and a balanced lifetime.
NativeScriptVMStatus nativeScriptExecution_Run(NativeScriptExecution*,
    void* actor, uint32_t source_class, uint32_t generation, uint32_t frame,uint64_t scene_epoch,
    uint32_t function, const NativeScriptValue* arguments, uint32_t count, NativeScriptEventRecordIO* record,
    NativeScriptValue* result);

#endif
