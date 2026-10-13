#ifndef MELEE_FT_NATIVE_SCRIPT_EVENT_PAYLOAD_H
#define MELEE_FT_NATIVE_SCRIPT_EVENT_PAYLOAD_H

#include "native_script_events.h"

#define NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC UINT32_C(0x30000000)
#define NATIVE_SCRIPT_EVENT_RECORD_SEMANTIC UINT32_C(0x31000000)
#define NATIVE_SCRIPT_EVENT_SEMANTIC_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_EVENT_NAMESPACE UINT32_C(0x3e000001)
#define NATIVE_SCRIPT_SIGNAL_NAMESPACE UINT32_C(0x3e000002)
#define NATIVE_SCRIPT_EVENT_RECORD_NAMESPACE UINT32_C(0x3e000003)
enum { NATIVE_SCRIPT_EVENT_PAYLOAD_ARGUMENTS=4 };

typedef struct NativeScriptEventPayloadResolverContext {
    const NativeScriptSourceTree* tree;
    const NativeScriptEventDescriptor* descriptor;
    uint32_t callback, event_id, first_byte, last_byte;
} NativeScriptEventPayloadResolverContext;

// Compile context is tied to a real source callback and its audited descriptor.
// Helpers cannot read event payload implicitly; pass typed values explicitly.
bool nativeScriptEventPayload_ResolverContext(const NativeScriptSourceTree*,
    uint32_t callback, NativeScriptEventPayloadResolverContext*, NativeScriptDiagnostic*);
// 0: unrelated semantic path; 1: resolved; -1: invalid/unavailable event path.
int nativeScriptEventPayload_TryResolve(void*, const NativeScriptIRArena*,
    const NativeScriptIRNode*, NativeScriptIRResolution*, NativeScriptDiagnostic*);
bool nativeScriptEventPayload_Resolve(void*, const NativeScriptIRArena*,
    const NativeScriptIRNode*, NativeScriptIRResolution*, NativeScriptDiagnostic*);

typedef struct NativeScriptEventPayloadHandle { uint64_t serial; uint32_t slot; } NativeScriptEventPayloadHandle;

typedef struct NativeScriptEventPayloadFrame {
    NativeScriptValue arguments[NATIVE_SCRIPT_EVENT_PAYLOAD_ARGUMENTS];
    NativeScriptValue fields[NATIVE_SCRIPT_EVENT_RECORD_FIELDS];
    uint8_t record_seed[NATIVE_SCRIPT_EVENT_RECORD_BYTES];
    void* source;
    uint64_t serial, scene_epoch;
    uint32_t event_id, source_class, generation, frame, actor_id;
} NativeScriptEventPayloadFrame;

// Caller initializes frames/capacity at loading; depth/serial start at zero.
// Only the top active synchronous frame is readable. Suspended frames become
// available again after balanced nested dispatch; old popped handles stay dead.
typedef struct NativeScriptEventPayloadPool {
    NativeScriptEventPayloadFrame* frames;
    uint32_t capacity, depth;
    uint64_t serial;
} NativeScriptEventPayloadPool;

typedef bool (*NativeScriptEventPayloadSourceAlive)(NativeScriptRuntimeContext*);

// Call-stack-only native record input/output. No pointer enters script values.
typedef struct NativeScriptEventRecordIO { void* data; uint32_t bytes; } NativeScriptEventRecordIO;

bool nativeScriptEventPayload_Push(NativeScriptEventPayloadPool*, uint32_t event_id,
    NativeScriptRuntimeContext*, NativeScriptEventPayloadSourceAlive,
    const NativeScriptValue*, uint32_t argument_count,
    const NativeScriptEventRecordIO*,
    NativeScriptEventPayloadHandle*, NativeScriptDiagnostic*);
bool nativeScriptEventPayload_Pop(NativeScriptEventPayloadPool*,
    NativeScriptEventPayloadHandle, NativeScriptDiagnostic*);
bool nativeScriptEventPayload_Read(const NativeScriptEventPayloadPool*,
    NativeScriptEventPayloadHandle, NativeScriptRuntimeContext*,
    NativeScriptEventPayloadSourceAlive, uint32_t semantic,
    NativeScriptValue*, NativeScriptDiagnostic*);
bool nativeScriptEventPayload_Write(NativeScriptEventPayloadPool*,
    NativeScriptEventPayloadHandle, NativeScriptRuntimeContext*,
    NativeScriptEventPayloadSourceAlive, uint32_t semantic,
    const NativeScriptValue*, NativeScriptDiagnostic*);
bool nativeScriptEventPayload_ExportRecord(const NativeScriptEventPayloadPool*,
    NativeScriptEventPayloadHandle, NativeScriptRuntimeContext*,
    NativeScriptEventPayloadSourceAlive, void* output, uint32_t bytes,
    NativeScriptDiagnostic*);
// Native controller can seed anonymous VM parameters from the same validated
// frame. These native pointers never become script values or artifact metadata.
const NativeScriptValue* nativeScriptEventPayload_Arguments(const NativeScriptEventPayloadPool*,
    NativeScriptEventPayloadHandle, NativeScriptRuntimeContext*,
    NativeScriptEventPayloadSourceAlive, uint32_t* count, NativeScriptDiagnostic*);

#endif
