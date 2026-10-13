#ifndef MELEE_FT_NATIVE_SCRIPT_STORAGE_H
#define MELEE_FT_NATIVE_SCRIPT_STORAGE_H

#include "native_script_program.h"

typedef struct NativeScriptStorageField {
    NativeScriptSourceSpan source;
    uint32_t owner, offset, count;
    uint8_t type;
    bool parameter, imported, array;
} NativeScriptStorageField;

typedef struct NativeScriptStorageOwner {
    uint32_t owner, state_bytes, parameter_count;
} NativeScriptStorageOwner;

// Caller owns these arrays and pins the indexed program. Build only while
// loading/applying content; the completed layout remains immutable in play.
typedef struct NativeScriptStorageLayout {
    const NativeScriptProgramIndex* program;
    NativeScriptStorageField* fields;
    uint32_t field_count, field_capacity;
    NativeScriptStorageOwner* owners;
    uint32_t owner_count, owner_capacity;
    uint64_t identity;
} NativeScriptStorageLayout;

typedef struct NativeScriptParameterSupply {
    uint32_t semantic, owner;
    NativeScriptValue value;
} NativeScriptParameterSupply;

typedef struct NativeScriptParameterSlot {
    NativeScriptValue value;
    bool resolved;
} NativeScriptParameterSlot;

typedef struct NativeScriptStorageParameters {
    const NativeScriptParameterSlot* slots;
    uint32_t count;
    uint64_t layout_identity;
    uint64_t identity;
} NativeScriptStorageParameters;

typedef struct NativeScriptStorageInstance {
    const NativeScriptStorageLayout* layout;
    const NativeScriptStorageParameters* parameters;
    uint8_t* state;
    uint32_t capacity, owner, identity, generation;
    bool live;
} NativeScriptStorageInstance;

// One unversioned, ephemeral active-program snapshot. The aggregate engine
// owns object lifetime/resurrection; this utility restores only the same live
// instance identity/generation, owner and immutable layout.
typedef struct NativeScriptStorageSnapshot {
    uint8_t* bytes;
    uint32_t capacity, state_bytes, owner, identity, generation;
    uint64_t layout_identity;
    uint64_t parameter_identity;
} NativeScriptStorageSnapshot;

bool nativeScriptStorage_BuildLayout(NativeScriptStorageLayout*,
    const NativeScriptProgramIndex*, NativeScriptDiagnostic*);
const NativeScriptStorageOwner* nativeScriptStorage_Owner(
    const NativeScriptStorageLayout*, uint32_t owner);

// Slots use declaration indices, including unused gaps for state declarations.
// Defaults are typed/resolved; imported attributes stay explicitly unresolved
// until supplied. Validate all inputs before replacing any table/output slots.
bool nativeScriptStorage_PrepareParameters(const NativeScriptStorageLayout*,
    NativeScriptParameterSlot*, uint32_t capacity,
    const NativeScriptParameterSupply*, uint32_t supply_count,
    NativeScriptStorageParameters*, NativeScriptDiagnostic*);
bool nativeScriptStorage_ParametersReady(const NativeScriptStorageLayout*,
    const NativeScriptStorageParameters*, uint32_t owner,
    NativeScriptDiagnostic*);

// Creation clears only the owner's active state footprint. Destroy retires the
// view. Reuse creates a fresh instance and resets its caller-provided bytes.
bool nativeScriptStorage_Create(const NativeScriptStorageLayout*,
    const NativeScriptStorageParameters*, uint32_t owner,
    uint32_t identity, uint32_t generation, uint8_t* bytes, uint32_t capacity,
    NativeScriptStorageInstance*, NativeScriptDiagnostic*);
void nativeScriptStorage_Destroy(NativeScriptStorageInstance*);

// Semantic declaration IDs plus zero scalar selectors or one checked integral
// array index. Exact declaration owner is required for state/local parameters.
// PROGRAM_PARAMETER reads may access root-owned immutable parameters only,
// using the current instance's pinned parameter table; no root actor is needed.
bool nativeScriptStorage_Read(const NativeScriptStorageInstance*,
    uint32_t semantic, const NativeScriptValue* selectors, uint32_t count,
    NativeScriptValue*, NativeScriptDiagnostic*);
bool nativeScriptStorage_Write(NativeScriptStorageInstance*,
    uint32_t semantic, const NativeScriptValue* selectors, uint32_t count,
    const NativeScriptValue*, NativeScriptDiagnostic*);

bool nativeScriptStorage_Save(const NativeScriptStorageInstance*,
    NativeScriptStorageSnapshot*, NativeScriptDiagnostic*);
bool nativeScriptStorage_Restore(NativeScriptStorageInstance*,
    const NativeScriptStorageSnapshot*, NativeScriptDiagnostic*);

#endif
