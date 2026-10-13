#include "native_script_fighter_launch.h"
#include "native_script_fighter.h"
#include "native_fighter_runtime.h"
#include "kinds/ftCommon/forward.h"
#include <dolphin/os.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SourceBytes=65536,Nodes=2048,Expressions=4096,Functions=128,
    Parameters=256,Fields=256,Instructions=8192 };
static struct {
    char source[SourceBytes];NativeScriptSourceNode nodes[Nodes];
    NativeScriptIRNode expressions[Expressions];uint32_t arguments[Expressions];
    NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[Parameters];
    NativeScriptValueDeclaration values[Fields];NativeScriptLocal locals[128];
    NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];
    NativeScriptExecutionFunction executions[Functions];NativeScriptCompileFunctionScratch graph[Functions];
    NativeScriptStorageField fields[Fields];NativeScriptStorageOwner owners[32];NativeScriptParameterSlot slots[Fields];
} buffers;
static NativeScriptCompileBank bank;
static bool selected;

bool nativeScriptFighterLaunch_Load(const char* kind,const char* path)
{
    char* end;
    unsigned long value=kind?strtoul(kind,&end,0):Ft_Kind_RuntimeCapacity;
    if(!kind || end==kind || *end || value>=Ft_Kind_RuntimeCapacity || value==Ft_Kind_None || !path || selected)return false;
    FILE* input=fopen(path,"rb");
    if(!input)return false;
    bool ok=fseek(input,0,SEEK_END)==0;long size=ok?ftell(input):-1;
    ok=ok && size>0 && size<SourceBytes && fseek(input,0,SEEK_SET)==0;
    char* source=ok?malloc((size_t)size):NULL;
    ok=source && fread(source,1,(size_t)size,input)==(size_t)size;
    fclose(input);
    if(!ok){free(source);return false;}
    bank=(NativeScriptCompileBank){.source_storage=buffers.source,.source_capacity=SourceBytes,
        .tree={.nodes=buffers.nodes,.capacity=Nodes},
        .expressions={.nodes=buffers.expressions,.node_capacity=Expressions,.arguments=buffers.arguments,.argument_capacity=Expressions},
        .declarations={.functions=buffers.functions,.function_capacity=Functions,.parameters=buffers.parameters,.parameter_capacity=Parameters,
            .values=buffers.values,.value_capacity=Fields},.locals=buffers.locals,.local_capacity=128,
        .instructions=buffers.instructions,.instruction_capacity=Instructions,.function_code=buffers.code,.function_code_capacity=Functions,
        .execution_functions=buffers.executions,.execution_function_capacity=Functions,.graph=buffers.graph,.graph_capacity=Functions,
        .storage_layout={.fields=buffers.fields,.field_capacity=Fields,.owners=buffers.owners,.owner_capacity=32},
        .parameter_slots=buffers.slots,.parameter_slot_capacity=Fields};
    const NativeScriptCompileBank* accepted=NULL;
    // Offline diagnostics may override verified common-state phases. Extended
    // states require an installed owned definition through the package loader.
    NativeScriptMoveLayout layout={.common_count=ftCo_MS_Count,.first=ftCo_MS_Count};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)size,.motion_layout=&layout};
    NativeScriptDiagnostic diagnostic={0};
    NativeScriptFighterProgram prepared;
    ok=nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic) &&
        nativeScriptFighter_PrepareProgram((FighterKind)value,accepted,&layout,NULL,&prepared,&diagnostic) &&
        nativeScriptFighter_CanPublish((FighterKind)value);
    if(ok)nativeScriptFighter_PublishProgram((FighterKind)value,&prepared);
    if(!ok)OSReport("[script] Source %u:%u: %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);
    else OSReport("[script] Selected interpreted program for fighter kind %lu: %u functions, %u state bytes.\n",value,
        bank.declarations.function_count,nativeScriptStorage_Owner(&bank.storage_layout,0)->state_bytes);
    free(source);selected=ok;return ok;
}
