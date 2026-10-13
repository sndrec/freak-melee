#include "native_source_bank.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static size_t reserve(size_t* used,size_t bytes)
{*used=(*used+63)&~(size_t)63;size_t offset=*used;*used+=bytes;return offset;}
static size_t layout(NativeSourceBank* output,void* memory)
{
    NativeScriptCompileBank* bank=&output->bank;size_t used=0;
#define BIND(field,capacity,count) do {size_t offset=reserve(&used,sizeof(*bank->field)*(size_t)(count));bank->field=memory?(void*)((uint8_t*)memory+offset):NULL;bank->capacity=(count);}while(0)
    BIND(source_storage,source_capacity,NATIVE_SOURCE_SOURCE_BYTES);
    BIND(tree.nodes,tree.capacity,NATIVE_SOURCE_SYNTAX_NODES);
    BIND(expressions.nodes,expressions.node_capacity,NATIVE_SOURCE_IR_NODES);
    BIND(expressions.arguments,expressions.argument_capacity,NATIVE_SOURCE_ARGUMENTS);
    BIND(declarations.functions,declarations.function_capacity,NATIVE_SOURCE_FUNCTIONS);
    BIND(declarations.parameters,declarations.parameter_capacity,NATIVE_SOURCE_FUNCTION_PARAMETERS);
    BIND(declarations.values,declarations.value_capacity,NATIVE_SOURCE_VALUES);
    BIND(locals,local_capacity,NATIVE_SOURCE_LOCALS);
    BIND(instructions,instruction_capacity,NATIVE_SOURCE_INSTRUCTIONS);
    BIND(function_code,function_code_capacity,NATIVE_SOURCE_FUNCTIONS);
    BIND(execution_functions,execution_function_capacity,NATIVE_SOURCE_FUNCTIONS);
    BIND(graph,graph_capacity,NATIVE_SOURCE_FUNCTIONS);
    BIND(storage_layout.fields,storage_layout.field_capacity,NATIVE_SOURCE_VALUES);
    BIND(storage_layout.owners,storage_layout.owner_capacity,NATIVE_SOURCE_OWNERS);
    BIND(parameter_slots,parameter_slot_capacity,NATIVE_SOURCE_VALUES);
#undef BIND
    return (used+63)&~(size_t)63;
}
bool nativeSourceBank_Allocate(NativeSourceBank* output,NativeScriptDiagnostic* d)
{
    if(!output||output->arena||output->bank.valid)return false;
    memset(&output->bank,0,sizeof(output->bank));size_t bytes=layout(output,NULL);
    void* memory=calloc(1,bytes);if(!memory){if(d)snprintf(d->message,sizeof(d->message),"Could not reserve independent source compile bank");memset(output,0,sizeof(*output));return false;}
    output->arena=memory;output->arena_bytes=bytes;layout(output,memory);return true;
}
void nativeSourceBank_Release(NativeSourceBank* bank)
{if(bank){free(bank->arena);memset(bank,0,sizeof(*bank));}}
bool nativeSourceBank_Stats(const NativeSourceBank* owner,NativeSourceBankStats* output)
{
    if(!owner||!owner->arena||!owner->bank.valid||!output)return false;const NativeScriptCompileBank* b=&owner->bank;
    NativeSourceBankStats s={.reserved_bytes=owner->arena_bytes,.source_bytes=b->code.source_bytes,.syntax_nodes=b->tree.count,
        .ir_nodes=b->expressions.node_count,.arguments=b->expressions.argument_count,.functions=b->declarations.function_count,
        .parameters=b->declarations.parameter_count,.values=b->declarations.value_count,.instructions=b->code.instruction_count,.owners=b->storage_layout.owner_count,
        .maximum_call_registers=b->maximum_call_registers,.maximum_call_depth=b->maximum_call_depth};
    for(unsigned f=0;f<s.functions;++f){const NativeScriptFunctionCode* code=&b->code.functions[f];if(code->local_count>s.maximum_local_count)s.maximum_local_count=code->local_count;if(code->register_count>s.maximum_function_registers)s.maximum_function_registers=code->register_count;}
    s.active_buffer_bytes=(size_t)s.source_bytes+1+(size_t)s.syntax_nodes*sizeof(*b->tree.nodes)+(size_t)s.ir_nodes*sizeof(*b->expressions.nodes)+(size_t)s.arguments*sizeof(*b->expressions.arguments)+
        (size_t)s.functions*(sizeof(*b->declarations.functions)+sizeof(*b->function_code)+sizeof(*b->execution_functions)+sizeof(*b->graph))+
        (size_t)s.parameters*sizeof(*b->declarations.parameters)+(size_t)s.values*(sizeof(*b->declarations.values)+sizeof(*b->storage_layout.fields)+sizeof(*b->parameter_slots))+
        (size_t)s.maximum_local_count*sizeof(*b->locals)+(size_t)s.instructions*sizeof(*b->instructions)+(size_t)s.owners*sizeof(*b->storage_layout.owners);
    *output=s;return true;
}
