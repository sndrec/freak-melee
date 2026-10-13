#ifndef MELEE_FT_NATIVE_SCRIPT_RECEIVER_H
#define MELEE_FT_NATIVE_SCRIPT_RECEIVER_H

#include "native_script_storage.h"
#include "native_callback_lifetime.h"
#include <melee/gm/native_match_pool.h>

#define NATIVE_SCRIPT_RECEIVER_ENGINE_KIND UINT32_C(0x40000000)
#define NATIVE_SCRIPT_RECEIVER_RETAINED_KIND UINT32_C(0x80000000)
#define NATIVE_SCRIPT_RECEIVER_KIND_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_RECEIVER_SOURCE_MASK UINT32_C(0x000000ff)

typedef enum NativeScriptReceiverRole {
    NativeScriptReceiver_Creator=1, NativeScriptReceiver_Owner,
    NativeScriptReceiver_CurrentOwner, NativeScriptReceiver_Holder,
    NativeScriptReceiver_AttachmentOwner,
    NativeScriptReceiver_ContactTarget,
} NativeScriptReceiverRole;

typedef struct NativeScriptReceiverView {
    const NativeScriptProgramIndex* program;
    NativeScriptStorageInstance* storage;
} NativeScriptReceiverView;

// Pure lookup of an already bound actor. False means no interpreted view.
// It performs no allocation/callback and never invents a compatible schema.
typedef bool (*NativeScriptReceiverLookup)(void*,void* actor,uint32_t source_class,
    uint32_t generation,NativeScriptReceiverView*);
// Direct code-ID execution in the selected receiver's actual root view.
typedef bool (*NativeScriptReceiverHelper)(void*,void* actor,uint32_t generation,
    const NativeScriptProgramIndex* expected_program,uint32_t function,
    const NativeScriptValue*,uint32_t count,NativeScriptValue*,NativeScriptDiagnostic*);

typedef struct NativeScriptReceiverFrameHandle {uint32_t slot,serial;} NativeScriptReceiverFrameHandle;
typedef struct NativeScriptReceiverRecord {
    void* actor;
    const void* native_data;
    const void* definition;
    NativeScriptReceiverView view;
    NativeMatchPoolReference reference;
    uint64_t layout_identity,parameter_identity;
    uint32_t generation,kind,source_class,serial,frame_serial,storage_owner;
} NativeScriptReceiverRecord;
typedef struct NativeScriptReceiverFrame {
    NativeScriptRuntimeContext caller;
    NativeMatchPoolReference caller_reference;
    const void* caller_data;
    uint64_t scene_epoch;
    NativeScriptReceiverView caller_view;
    uint64_t caller_layout_identity,caller_parameter_identity;
    const NativeScriptProgramIndex* expected_program;
    NativeScriptReceiverLookup lookup;
    NativeScriptReceiverHelper helper;
    void* user;
    uint32_t first,serial,expected_fighter_kind,caller_storage_owner;
} NativeScriptReceiverFrame;

// Invocation scratch, caller-reserved at load. Nested invocations reserve
// disjoint ranges; popped/reused handles never become valid again. Keep serial
// monotonic when clearing idle scratch; exhaustion is an explicit error.
typedef struct NativeScriptReceiverPool {
    NativeScriptReceiverRecord* records;
    uint32_t capacity,top;
    NativeScriptReceiverFrame* frames;
    uint32_t frame_capacity,depth,serial;
} NativeScriptReceiverPool;

typedef struct NativeScriptReceiverScope {
    NativeScriptReceiverPool* pool;
    NativeScriptReceiverFrameHandle frame;
} NativeScriptReceiverScope;

bool nativeScriptReceiver_Push(NativeScriptReceiverPool*,const NativeScriptRuntimeContext*,uint64_t scene_epoch,
    const NativeScriptProgramIndex* expected_program,uint32_t expected_fighter_kind,
    NativeScriptReceiverLookup,NativeScriptReceiverHelper,void* user,
    NativeScriptReceiverFrameHandle*,NativeScriptDiagnostic*);
bool nativeScriptReceiver_Pop(NativeScriptReceiverPool*,NativeScriptReceiverFrameHandle,
    NativeScriptDiagnostic*);
bool nativeScriptReceiver_Capture(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    NativeScriptReceiverRole,bool require_present,bool require_owned,
    NativeScriptValue*,NativeScriptDiagnostic*);
// Select one already-bound source Article from this exact root program. Missing
// handles/kinds/banks produce an Item-class null token; caller retirement faults.
// A successful token retains its physical instance, never its mutable owner.
bool nativeScriptReceiver_CaptureArticle(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    uint32_t handle,uint32_t owner_node,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptReceiver_ValidateArticle(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    const NativeScriptValue*,uint32_t owner_node,const NativeScriptReceiverRecord**,
    NativeScriptDiagnostic*);
// Scheduler input, valid only for immediate synchronous import in this pinned
// scene. The value carries pool slot+generation/class; the trusted scheduler
// keeps/passes the full scene epoch separately, without truncating it.
bool nativeScriptReceiver_CaptureExternal(void* actor,uint32_t source_class,
    uint64_t scene_epoch,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptReceiver_Import(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    const NativeScriptValue* external,uint64_t source_scene_epoch,
    NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptReceiver_Export(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    const NativeScriptValue* retained,NativeScriptValue* external,NativeScriptDiagnostic*);
// Program-agnostic engine input resolution. Null is permitted; stale pool slots,
// class mismatches and the wrong full scene epoch fail without output changes.
bool nativeScriptReceiver_ResolveExternal(const NativeScriptRuntimeContext* caller,
    const NativeScriptValue* external,uint64_t scene_epoch,uint32_t expected_source_class,
    void** actor,uint32_t* source_generation,NativeScriptDiagnostic*);

// Validates the retained token against its original GObj, class/kind,
// Definition and optional program/storage/schema. Never selects owner again.
bool nativeScriptReceiver_Validate(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    const NativeScriptValue*,bool require_owned,const NativeScriptReceiverRecord**,
    NativeScriptDiagnostic*);
bool nativeScriptReceiver_Exists(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    const NativeScriptValue*,bool*,NativeScriptDiagnostic*);
// Calls the frame's direct helper runner, then validates caller/receiver before
// publication. Helpers must belong to the exact expected root program/kind.
bool nativeScriptReceiver_RunHelper(const NativeScriptReceiverScope*,NativeScriptRuntimeContext*,
    const NativeScriptValue*,uint32_t function,const NativeScriptValue*,uint32_t count,
    NativeScriptValue*,NativeScriptDiagnostic*);

#endif
