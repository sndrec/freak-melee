#ifndef MELEE_FT_NATIVE_SCRIPT_PROGRAM_H
#define MELEE_FT_NATIVE_SCRIPT_PROGRAM_H

#include "native_script_vm.h"

#define NATIVE_SCRIPT_STATE_SEMANTIC UINT32_C(0x10000000)
#define NATIVE_SCRIPT_PARAMETER_SEMANTIC UINT32_C(0x20000000)
// Read-only root or entity parameters from the executing package, independent of the
// current entity owner. Keep the helper/local marker bits clear.
#define NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC UINT32_C(0x28000000)
#define NATIVE_SCRIPT_STORAGE_DOMAIN_MASK UINT32_C(0xf8000000)
#define NATIVE_SCRIPT_STORAGE_INDEX_MASK UINT32_C(0x07ffffff)
#define NATIVE_SCRIPT_STATE_NAMESPACE UINT32_C(0x3f000001)
#define NATIVE_SCRIPT_ATTRIBUTE_NAMESPACE UINT32_C(0x3f000002)
#define NATIVE_SCRIPT_ITEM_NAMESPACE UINT32_C(0x3f000003)
#define NATIVE_SCRIPT_PROGRAM_NAMESPACE UINT32_C(0x3f000004)
#define NATIVE_SCRIPT_PROGRAM_ATTRIBUTE_NAMESPACE UINT32_C(0x3f000005)
#define NATIVE_SCRIPT_TABLE_NAMESPACE UINT32_C(0x3f000006)
#define NATIVE_SCRIPT_PROGRAM_TABLE_NAMESPACE UINT32_C(0x3f000007)
#define NATIVE_SCRIPT_PROGRAM_ENTITY_PARAMETERS_NAMESPACE UINT32_C(0x3f000008)
// Compile-time namespaces only; the payload is a source-tree entity owner ID.
#define NATIVE_SCRIPT_ENTITY_PARAMETERS_NAMESPACE UINT32_C(0x3c000000)
#define NATIVE_SCRIPT_ENTITY_PARAMETERS_DOMAIN_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_ENTITY_PARAMETERS_OWNER_MASK UINT32_C(0x00ffffff)
#define NATIVE_SCRIPT_TABLE_SEMANTIC UINT32_C(0x09000000)
#define NATIVE_SCRIPT_TABLE_DOMAIN_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_TABLE_INDEX_MASK UINT32_C(0x00ffffff)
enum { NATIVE_SCRIPT_TABLES=64, NATIVE_SCRIPT_TABLE_ELEMENTS=2048 };

// Recipient calls retain a prepared literal name in the caller's immutable
// index. Runtime lookup compares hash buckets AND pinned bytes, never hashes.
// The semantic also captures the total call arity (handle + name + parameters).
#define NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN UINT32_C(0x07000000)
#define NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN_MASK UINT32_C(0xff000000)
#define NATIVE_SCRIPT_TARGET_EXPORT_OPERATION_MASK UINT32_C(0x00f00000)
#define NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK UINT32_C(0x000fc000)
#define NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT 14
#define NATIVE_SCRIPT_TARGET_EXPORT_INDEX_MASK UINT32_C(0x00003fff)
enum { NATIVE_SCRIPT_EXPORTS=128,NATIVE_SCRIPT_EXPORT_NAMES=128 };
_Static_assert(NATIVE_SCRIPT_EXPORT_NAMES<=NATIVE_SCRIPT_TARGET_EXPORT_INDEX_MASK+1,
               "Prepared recipient name indices exceed their semantic field");
_Static_assert(NATIVE_SCRIPT_EXPRESSION_ARGUMENTS<=
               (NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_MASK>>NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT),
               "Prepared recipient call arity exceeds its semantic field");
typedef struct NativeScriptExport {
    uint32_t key,function;
    NativeScriptSourceSpan name;
    NativeScriptIRType result_type;
} NativeScriptExport;
typedef struct NativeScriptExportName {
    uint32_t key;
    NativeScriptSourceSpan name;
} NativeScriptExportName;

typedef struct NativeScriptParameterDeclaration {
    NativeScriptSourceSpan name;
    NativeScriptIRType type;
} NativeScriptParameterDeclaration;

typedef struct NativeScriptFunctionDeclaration {
    NativeScriptSourceSpan name;
    uint32_t source_node, owner, parameter_first, parameter_count;
    NativeScriptIRType result_type;
    bool callback, exported, signature_resolved;
    NativeScriptDefaultReturn default_return;
    uint8_t body_policy;
} NativeScriptFunctionDeclaration;

typedef struct NativeScriptValueDeclaration {
    NativeScriptSourceSpan name;
    uint32_t source_node, owner, count, byte_offset;
    uint32_t import_offset;
    NativeScriptValue default_value;
    NativeScriptIRType type;
    bool parameter, imported, array;
} NativeScriptValueDeclaration;

// Immutable, load-time evaluated data. Shared by all actors running this index;
// it consumes neither actor state bytes nor rollback snapshot storage.
typedef struct NativeScriptTableDeclaration {
    NativeScriptSourceSpan name;
    uint32_t source_node, owner, first, count;
    NativeScriptIRType type;
} NativeScriptTableDeclaration;

typedef bool (*NativeScriptEventSignature)(void* context,
    const NativeScriptSourceTree*, const NativeScriptSourceNode*, uint32_t owner,
    NativeScriptIRType* result, NativeScriptDefaultReturn*, uint8_t* body_policy,
    NativeScriptParameterDeclaration* arguments,
    uint32_t capacity, uint32_t* count, NativeScriptDiagnostic*);

typedef struct NativeScriptProgramIndex {
    NativeScriptSourceTree* tree;
    NativeScriptIRArena* expressions;
    NativeScriptFunctionDeclaration* functions;
    uint32_t function_count, function_capacity;
    NativeScriptParameterDeclaration* parameters;
    uint32_t parameter_count, parameter_capacity;
    NativeScriptValueDeclaration* values;
    uint32_t value_count, value_capacity;
    NativeScriptEventSignature event_signature;
    void* event_context;
    // Load-time defaults only. Provider may publish numeric/Bool Literal results
    // or namespaces used to reach them. No calls, indices, runtime values or
    // references to owned parameters/state/helpers are accepted in defaults.
    NativeScriptIRResolver constant_resolver;
    void* constant_context;
    NativeScriptExport exports[NATIVE_SCRIPT_EXPORTS];
    uint32_t export_count;
    NativeScriptExportName export_names[NATIVE_SCRIPT_EXPORT_NAMES];
    uint32_t export_name_count;
    NativeScriptTableDeclaration tables[NATIVE_SCRIPT_TABLES];
    uint32_t table_bits[NATIVE_SCRIPT_TABLE_ELEMENTS];
    uint32_t table_count, table_element_count;
} NativeScriptProgramIndex;

typedef struct NativeScriptProgramCompiler {
    NativeScriptProgramIndex* index;
    NativeScriptIRResolver native_bindings;
    void* native_binding_context;
    NativeScriptLocal* locals;
    uint32_t local_capacity;
    uint32_t active_function;
} NativeScriptProgramCompiler;

// Index declarations and helper/event signatures before resolving bodies. The
// index alone is not acceptance of metadata, bindings or an executable package.
// An absent event contract leaves callback signatures explicitly unresolved.
bool nativeScriptProgram_Index(NativeScriptProgramIndex*, NativeScriptDiagnostic*);
// Load-only scalar constant language, shared by parameters, tables and typed
// immutable engine definitions. No variable state, pointers or runtime calls.
bool nativeScriptProgram_EvaluateConstant(const NativeScriptProgramIndex*,NativeScriptSourceSpan,
    NativeScriptIRType,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptProgram_ReadTable(const NativeScriptProgramIndex*, uint32_t owner,
    uint32_t semantic, const NativeScriptValue* selectors, uint32_t count,
    NativeScriptValue*, NativeScriptDiagnostic*);
// Load-time name admission/deduplication. Returned IDs belong only to this
// pinned index; names need not be exported by the caller's own package.
bool nativeScriptProgram_ExportName(NativeScriptProgramIndex*,NativeScriptSourceSpan,
    uint32_t* index,NativeScriptDiagnostic*);
bool nativeScriptProgram_CompileBody(NativeScriptProgramCompiler*, uint32_t function,
    NativeScriptStatementCompiler* body, NativeScriptDiagnostic*);
bool nativeScriptProgram_Resolve(void*, const NativeScriptIRArena*,
    const NativeScriptIRNode*, NativeScriptIRResolution*, NativeScriptDiagnostic*);
bool nativeScriptProgram_DescribeHelper(void*, uint32_t semantic,
    NativeScriptCallLayout*, NativeScriptDiagnostic*);

#endif
