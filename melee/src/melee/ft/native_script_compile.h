#ifndef MELEE_FT_NATIVE_SCRIPT_COMPILE_H
#define MELEE_FT_NATIVE_SCRIPT_COMPILE_H

#include "native_script_execution.h"
#include "native_script_moves.h"
#include "native_script_pose_metadata.h"
#include "native_script_hitbox_metadata.h"

typedef struct NativeScriptCompileFunctionScratch {
    uint32_t depth, registers;
    uint8_t color;
} NativeScriptCompileFunctionScratch;

// Each bank owns independent caller-reserved storage. It remains at a stable
// address while referenced; its tree/index/layout/code point into this bank.
// Populate pointer/capacity members before Build. Counts and derived views are
// output, and source bytes are copied/pinned here at the loading transition.
typedef struct NativeScriptCompileBank {
    char* source_storage;
    uint32_t source_capacity;
    NativeScriptSourceTree tree;
    NativeScriptIRArena expressions;
    NativeScriptProgramIndex declarations;
    NativeScriptLocal* locals;
    uint32_t local_capacity;
    NativeScriptInstruction* instructions;
    uint32_t instruction_capacity;
    NativeScriptFunctionCode* function_code;
    uint32_t function_code_capacity;
    NativeScriptExecutionFunction* execution_functions;
    uint32_t execution_function_capacity;
    NativeScriptCompileFunctionScratch* graph;
    uint32_t graph_capacity;
    NativeScriptStorageLayout storage_layout;
    NativeScriptParameterSlot* parameter_slots;
    uint32_t parameter_slot_capacity;
    NativeScriptStorageParameters storage_parameters;
    NativeScriptBytecodeProgram code;
    uint32_t maximum_call_depth, maximum_call_registers;
    NativeScriptMoveLayout motion_layout;
    NativeScriptPoseSymbol pose_symbols[NATIVE_SCRIPT_POSE_SYMBOLS];
    uint32_t pose_symbol_count,pose_graph_count;
    uint16_t pose_graph_mask;
    NativeScriptHitboxDefinition hitbox_definitions[NATIVE_SCRIPT_HITBOX_DEFINITIONS];
    uint32_t hitbox_definition_count;
    bool has_motion_layout,valid;
} NativeScriptCompileBank;

// Called for every non-executable declaration/container, after syntax parsing
// and before code publication. A wider package schema must validate each node
// explicitly; accepting an unknown metadata line is not an implementation.
typedef bool (*NativeScriptCompileMetadataValidator)(void* context,
    const NativeScriptSourceTree*, uint32_t node, NativeScriptDiagnostic*);

// Transition-only typed import provider. IDs refer to this indexed candidate,
// and supplies remain valid through Build. They are copied into owned parameter
// storage; no external DAT address or inherited attribute offset is used at tick.
typedef bool (*NativeScriptCompileParameterProvider)(void* context,
    const NativeScriptProgramIndex*, const NativeScriptParameterSupply**,
    uint32_t* count, NativeScriptDiagnostic*);

// Optional load-time audit output, in independent caller-reserved storage.
// Errors in separate metadata nodes/bodies are collected without admitting an
// invalid program. A structural/package failure can prevent reaching bodies.
typedef struct NativeScriptCompileDiagnostics {
    NativeScriptDiagnostic* entries;
    uint32_t capacity, count, total, functions_checked;
    bool metadata_complete, bodies_complete;
} NativeScriptCompileDiagnostics;

typedef struct NativeScriptCompileRequest {
    const char* source;
    uint32_t source_bytes;
    NativeScriptCompileMetadataValidator metadata;
    void* metadata_context;
    const NativeScriptParameterSupply* parameter_supply;
    uint32_t parameter_supply_count;
    NativeScriptCompileParameterProvider parameter_provider;
    void* parameter_context;
    const NativeScriptMoveLayout* motion_layout;
    const struct NativeSourcePoseGraph* pose_graphs;
    uint32_t pose_graph_count;
    uint16_t pose_graph_mask;
    NativeScriptCompileDiagnostics* diagnostics;
} NativeScriptCompileRequest;

// Minimal schema: independent fighter directive, root helpers/state/parameters,
// named action sections and their source-audited callbacks. All other package,
// entity/move/asset metadata is rejected instead of ignored.
bool nativeScriptCompile_MinimalMetadata(void*, const NativeScriptSourceTree*,
    uint32_t node, NativeScriptDiagnostic*);

// Substantive transactional pipeline: parse/index, schema and context checks,
// complete typed bodies, bytecode, helper graph, execution records and owned
// storage/parameters. Candidate and all its buffers must be independent of the
// currently accepted bank. Accepted pointer is replaced only after success.
bool nativeScriptCompile_Build(NativeScriptCompileBank* candidate,
    const NativeScriptCompileRequest*, const NativeScriptCompileBank** accepted,
    NativeScriptDiagnostic*);

#endif
