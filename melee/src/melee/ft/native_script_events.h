#ifndef MELEE_FT_NATIVE_SCRIPT_EVENTS_H
#define MELEE_FT_NATIVE_SCRIPT_EVENTS_H

#include "native_script_program.h"

typedef enum NativeScriptEventContext {
    NativeScriptEvent_Fighter, NativeScriptEvent_Article,
    NativeScriptEvent_WorkshopItem, NativeScriptEvent_ModelEffect,
} NativeScriptEventContext;

typedef enum NativeScriptEventKind {
    NativeScriptEvent_FighterHook, NativeScriptEvent_FighterMotion,
    NativeScriptEvent_MovePhase, NativeScriptEvent_ArticleLogic,
    NativeScriptEvent_ArticleState, NativeScriptEvent_ArticleSignal,
    NativeScriptEvent_ItemHook, NativeScriptEvent_EffectUpdate,
} NativeScriptEventKind;

typedef enum NativeScriptEventFallthrough {
    NativeScriptEvent_ReturnVoid, NativeScriptEvent_ReturnFalse,
    NativeScriptEvent_ReturnZero, NativeScriptEvent_ReturnMinusOne,
    NativeScriptEvent_ReturnArgument,
} NativeScriptEventFallthrough;

typedef enum NativeScriptEventBodyPolicy {
    NativeScriptEvent_GeneralBody=NativeScriptBodyPolicy_None,
    NativeScriptEvent_ReadOnlyBody=NativeScriptBodyPolicy_ReadOnly,
    NativeScriptEvent_StrictReturnBody=NativeScriptBodyPolicy_StrictReturn,
    NativeScriptEvent_TruthReturnBody=NativeScriptBodyPolicy_TruthReturn,
} NativeScriptEventBodyPolicy;

typedef struct NativeScriptEventArgument {
    const char* property;
    const char* native_type;
    uint8_t type;
} NativeScriptEventArgument;

enum { NATIVE_SCRIPT_EVENT_RECORD_FIELDS=16, NATIVE_SCRIPT_EVENT_RECORD_BYTES=128 };
typedef struct NativeScriptEventRecordField {
    const char* name;
    uint16_t offset;
    uint8_t type, width;
} NativeScriptEventRecordField;
// Immutable native scalar schema. Offsets never become authored values.
typedef struct NativeScriptEventRecordDescriptor {
    const char* namespace_name;
    uint16_t native_bytes;
    uint8_t field_count;
    NativeScriptEventRecordField fields[NATIVE_SCRIPT_EVENT_RECORD_FIELDS];
} NativeScriptEventRecordDescriptor;

typedef struct NativeScriptEventDescriptor {
    const char* name;
    const char* native_slot;
    const char* native_signature;
    const char* source_evidence;
    NativeScriptEventArgument arguments[4];
    uint8_t context, kind, result_type, argument_count;
    uint8_t fallthrough, fallthrough_argument;
    uint8_t body_policy;
    bool signature_supported;
    const NativeScriptEventRecordDescriptor* record;
} NativeScriptEventDescriptor;

typedef struct NativeScriptEventResolution {
    uint32_t event_id, owner_node, motion;
    NativeScriptSourceSpan signal_name;
    const NativeScriptEventDescriptor* descriptor;
} NativeScriptEventResolution;

// Resolve actual ownership/ancestry plus validated callback syntax. Event IDs
// identify immutable semantic entries; motion IDs and signal names remain
// separate authored metadata. This performs no scheduler/engine migration.
bool nativeScriptEvents_Resolve(const NativeScriptSourceTree*,
    const NativeScriptSourceNode*, NativeScriptEventResolution*, NativeScriptDiagnostic*);
const NativeScriptEventDescriptor* nativeScriptEvents_Descriptor(uint32_t);
uint32_t nativeScriptEvents_Count(void);

// ProgramIndex EventSignature callback. Native event payloads occupy anonymous
// typed parameter slots; their authored names are descriptor event properties,
// not invented source-local names. The actor is the opaque runtime context.
bool nativeScriptEvents_Signature(void*, const NativeScriptSourceTree*,
    const NativeScriptSourceNode*, uint32_t owner, NativeScriptIRType* result,
    NativeScriptDefaultReturn* default_return,
    uint8_t* body_policy,
    NativeScriptParameterDeclaration* arguments, uint32_t capacity,
    uint32_t* count, NativeScriptDiagnostic*);

#endif
