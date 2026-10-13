#include "native_script_compile.h"
#include "native_callback_lifetime.h"
#include "native_script_constants.h"
#include "native_script_move_lowering.h"
#include "native_script_binding_receiver.h"
#include "native_script_binding_spawn.h"
#include "native_script_binding_effect.h"
#include "native_script_binding_signal_resolve.h"
#include "native_script_binding_article_reference_resolve.h"
#include "native_script_binding_item_model_resolve.h"
#include "native_script_binding_item_resources_resolve.h"

#include <stdio.h>
#include <string.h>

typedef struct CompileContext {
    NativeScriptBindingResolverContext native;
    NativeScriptEventPayloadResolverContext event;
    NativeScriptBindingReceiverResolver receiver;
    NativeScriptBindingSpawnResolverContext spawn;
    NativeScriptPoseResolverContext pose;
    NativeScriptCompileBank* bank;uint32_t function;
    bool has_event;
} CompileContext;

static bool error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan span,const char* text)
{
    diagnostic->source=span;
    snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",text);return false;
}

static bool span_empty(NativeScriptSourceSpan span) {return span.length==0;}

bool nativeScriptCompile_MinimalMetadata(void* unused,const NativeScriptSourceTree* tree,
    uint32_t at,NativeScriptDiagnostic* diagnostic)
{
    (void)unused;
    if (!tree || at>=tree->count) return false;
    const NativeScriptSourceNode* node=&tree->nodes[at];
    unsigned parent=node->parent<tree->count ? tree->nodes[node->parent].kind : NativeScriptSource_Root;
    switch (node->kind) {
    case NativeScriptSource_Root: return true;
    case NativeScriptSource_Fighter:
        if (parent==NativeScriptSource_Root && span_empty(node->name) && span_empty(node->value)) return true;
        break;
    case NativeScriptSource_Metadata:
        if (parent==NativeScriptSource_Fighter && nativeScriptSource_SpanEquals(tree,node->name,"behavior") &&
            nativeScriptSource_SpanEquals(tree,node->value,"independent") && !node->opens) return true;
        break;
    case NativeScriptSource_Function:
    case NativeScriptSource_State:
    case NativeScriptSource_Parameter:
    case NativeScriptSource_Table:
        if (parent==NativeScriptSource_Root) return true;
        break;
    case NativeScriptSource_Action:
        if (parent==NativeScriptSource_Root && node->name.length && span_empty(node->value)) return true;
        break;
    case NativeScriptSource_Callback:
        if (parent==NativeScriptSource_Action) {
            NativeScriptEventResolution event;
            if (!nativeScriptEvents_Resolve(tree,node,&event,diagnostic)) return false;
            if (event.descriptor->kind==NativeScriptEvent_FighterHook) return true;
            return error(diagnostic,node->source,"Motion/entry event needs a broader validated package metadata contract");
        }
        break;
    default: break;
    }
    return error(diagnostic,node->source,"Unsupported declaration/metadata in the minimal direct-program schema");
}

typedef struct BufferRange {uintptr_t first,last;} BufferRange;
enum { BankRanges=17 };

static bool range(BufferRange* output,const void* pointer,uint32_t count,size_t width)
{
    uintptr_t first=(uintptr_t)pointer;
    if ((count && !pointer) || (count && width>SIZE_MAX/count)) return false;
    size_t bytes=count*width;
    if (first>UINTPTR_MAX-bytes) return false;
    *output=(BufferRange){first,first+bytes};return true;
}

static bool ranges(const NativeScriptCompileBank* bank,BufferRange list[BankRanges])
{
    unsigned i=0;
#define RANGE(pointer,count) do {if(!range(&list[i++],pointer,count,sizeof(*(pointer))))return false;}while(0)
    RANGE(bank,1);RANGE(bank->source_storage,bank->source_capacity);
    RANGE(bank->tree.nodes,bank->tree.capacity);RANGE(bank->expressions.nodes,bank->expressions.node_capacity);
    RANGE(bank->expressions.arguments,bank->expressions.argument_capacity);
    RANGE(bank->declarations.functions,bank->declarations.function_capacity);
    RANGE(bank->declarations.parameters,bank->declarations.parameter_capacity);
    RANGE(bank->declarations.values,bank->declarations.value_capacity);
    RANGE(bank->locals,bank->local_capacity);RANGE(bank->instructions,bank->instruction_capacity);
    RANGE(bank->function_code,bank->function_code_capacity);RANGE(bank->execution_functions,bank->execution_function_capacity);
    RANGE(bank->graph,bank->graph_capacity);RANGE(bank->storage_layout.fields,bank->storage_layout.field_capacity);
    RANGE(bank->storage_layout.owners,bank->storage_layout.owner_capacity);RANGE(bank->parameter_slots,bank->parameter_slot_capacity);
#undef RANGE
    // Final empty range keeps a fixed size without an allocation or sentinel.
    list[i++]=(BufferRange){0,0};return i==BankRanges;
}

static bool overlap(BufferRange a,BufferRange b)
{
    return a.first<a.last && b.first<b.last && a.first<b.last && b.first<a.last;
}

static bool independent(NativeScriptCompileBank* candidate,const NativeScriptCompileBank* accepted,
    NativeScriptDiagnostic* diagnostic)
{
    BufferRange fresh[BankRanges],old[BankRanges];
    if (!ranges(candidate,fresh)) return error(diagnostic,(NativeScriptSourceSpan){0},"Candidate buffer pointers/capacities are invalid");
    for (unsigned i=0;i<BankRanges;++i) for (unsigned j=0;j<i;++j)
        if (overlap(fresh[i],fresh[j])) return error(diagnostic,(NativeScriptSourceSpan){0},"Candidate bank buffers overlap");
    if (!accepted) return true;
    if (candidate==accepted || !accepted->valid || !ranges(accepted,old))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Compile candidate must be independent of the accepted program");
    for (unsigned i=0;i<BankRanges;++i) for (unsigned j=0;j<BankRanges;++j)
        if (overlap(fresh[i],old[j])) return error(diagnostic,(NativeScriptSourceSpan){0},"Compile candidate aliases an accepted program buffer");
    return true;
}

static bool executable(unsigned kind)
{
    return kind>=NativeScriptSource_If && kind<=NativeScriptSource_Pass;
}

static void record_diagnostic(NativeScriptCompileDiagnostics* report,const NativeScriptDiagnostic* diagnostic)
{
    if(report->count<report->capacity)report->entries[report->count++]=*diagnostic;
    ++report->total;
}

static bool validate_metadata(NativeScriptCompileBank* bank,NativeScriptCompileMetadataValidator validate,
    void* context,NativeScriptCompileDiagnostics* report,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptSourceTree* tree=&bank->tree;
    unsigned fighters=0,independent_directives=0;
    for (uint32_t i=0;i<tree->count;++i) {
        const NativeScriptSourceNode* node=&tree->nodes[i];
        if (executable(node->kind)) continue;
        if(report)memset(diagnostic,0,sizeof(*diagnostic));
        bool valid=nativeScriptPoseMetadata_Owns(tree,i) ?
            nativeScriptPoseMetadata_Validate(bank,tree,i,diagnostic) :
            nativeScriptHitboxMetadata_Owns(tree,i) ? nativeScriptHitboxMetadata_Validate(bank,tree,i,diagnostic) :
            validate(context,tree,i,diagnostic);
        if (!valid) {
            if (!diagnostic->message[0]) error(diagnostic,node->source,"Metadata validator rejected this source node");
            if(!report)return false;
            record_diagnostic(report,diagnostic);
            continue;
        }
        if (node->kind==NativeScriptSource_Fighter) ++fighters;
        if (node->kind==NativeScriptSource_Metadata && node->parent<tree->count &&
            tree->nodes[node->parent].kind==NativeScriptSource_Fighter &&
            nativeScriptSource_SpanEquals(tree,node->name,"behavior") &&
            nativeScriptSource_SpanEquals(tree,node->value,"independent")) ++independent_directives;
    }
    if(report) {
        report->metadata_complete=true;
        if(report->total){*diagnostic=report->entries[0];return false;}
    }
    if ((validate==nativeScriptCompile_MinimalMetadata || validate==nativeScriptMoves_Metadata) && (fighters!=1 || independent_directives!=1))
        return error(diagnostic,tree->nodes[0].source,"Minimal direct fighter program needs one fighter: behavior independent declaration");
    return true;
}

static bool resolve(void* opaque,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    CompileContext* context=opaque;
    if(node->kind==NativeScriptIR_Call&&node->a<arena->node_count){
        uint32_t binding=arena->nodes[node->a].semantic;
        if(binding>=NativeScriptBinding_TargetsCall&&binding<=NativeScriptBinding_TargetsCallFloat){
            if(!nativeScriptBindings_ResolveCall(&context->native,arena,node,binding,result,diagnostic))return false;
            const NativeScriptIRNode* name=&arena->nodes[arena->arguments[node->argument_first+1]];
            NativeScriptSourceSpan span=name->source;span.offset=name->text_offset;span.length=name->text_length;
            uint32_t token;
            if(!nativeScriptProgram_ExportName(&context->bank->declarations,span,&token,diagnostic))return false;
            result->semantic=NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN|((binding-NativeScriptBinding_TargetsCall+1)<<20)|
                (node->argument_count<<NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT)|token;
            return true;
        }
    }
    int event=nativeScriptEventPayload_TryResolve(context->has_event ? &context->event : NULL,
        arena,node,result,diagnostic);
    if (event) return event>0;
    int resources=nativeScriptBindingItemResources_TryResolve(&context->spawn,arena,node,result,diagnostic);
    if(resources)return resources>0;
    int model=nativeScriptBindingItemModel_TryResolve(&context->spawn,arena,node,result,diagnostic);
    if(model)return model>0;
    int article=nativeScriptBindingArticleReference_TryResolve(&context->spawn,arena,node,result,diagnostic);
    if(article)return article>0;
    int receiver=nativeScriptBindingReceiver_TryResolve(&context->receiver,arena,node,result,diagnostic);
    if(receiver)return receiver>0;
    if(context->bank->has_motion_layout) {
        int motion=nativeScriptMoves_TryResolve(&context->bank->motion_layout,&context->bank->declarations,
            context->function,arena,node,result,diagnostic);
        if(motion)return motion>0;
    }
    int pose=nativeScriptBindingPose_TryResolve(&context->pose,arena,node,result,diagnostic);
    if(pose)return pose>0;
    int hitbox=nativeScriptHitboxMetadata_TryResolve(context->bank,context->native.context_mask,
        context->function<context->bank->declarations.function_count?context->bank->declarations.functions[context->function].owner:0,
        arena,node,result,diagnostic);
    if(hitbox)return hitbox>0;
    int constant=nativeScriptConstants_TryResolve(NULL,arena,node,result,diagnostic);
    if(constant)return constant>0;
    int effect=nativeScriptBindingEffect_TryResolve(&context->spawn,arena,node,result,diagnostic);
    if(effect)return effect>0;
    int spawn=nativeScriptBindingSpawn_TryResolve(&context->spawn,arena,node,result,diagnostic);
    if(spawn)return spawn>0;
    int signal=nativeScriptBindingSignal_TryResolve(&context->spawn,arena,node,result,diagnostic);
    if(signal)return signal>0;
    return nativeScriptBindings_Resolve(&context->native,arena,node,result,diagnostic);
}

static bool function_context(NativeScriptCompileBank* bank,uint32_t function,
    CompileContext* context,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptFunctionDeclaration* declaration=&bank->declarations.functions[function];
    *context=(CompileContext){.native={NativeScriptBindings_Fighter},.bank=bank,.function=function};
    if (declaration->owner) {
        if (declaration->owner>=bank->tree.count) return error(diagnostic,declaration->name,"Function owner is out of source range");
        unsigned kind=bank->tree.nodes[declaration->owner].kind;
        if (kind==NativeScriptSource_Article || kind==NativeScriptSource_Item) context->native.context_mask=NativeScriptBindings_Article;
        else if (kind==NativeScriptSource_Effect) context->native.context_mask=NativeScriptBindings_Effect;
        else return error(diagnostic,declaration->name,"Function has unsupported native source ownership");
    }
    if (declaration->callback) {
        if (!nativeScriptEventPayload_ResolverContext(&bank->tree,declaration->source_node,&context->event,diagnostic)) return false;
        context->has_event=true;
    }
    context->pose=(NativeScriptPoseResolverContext){.bank=bank,.owner=declaration->owner,.context_mask=context->native.context_mask};
    context->receiver=(NativeScriptBindingReceiverResolver){.program=&bank->declarations,.owner=declaration->owner,
        .source_class=context->native.context_mask==NativeScriptBindings_Fighter?NativeCallbackSource_Fighter:
            context->native.context_mask==NativeScriptBindings_Article?NativeCallbackSource_Item:NativeCallbackSource_ModelEffect};
    context->spawn=(NativeScriptBindingSpawnResolverContext){.program=&bank->declarations,
        .owner=declaration->owner,.bindings=context->native};
    return true;
}
static bool describe(void* opaque,uint32_t semantic,NativeScriptCallLayout* layout,NativeScriptDiagnostic* diagnostic)
{
    CompileContext* context=opaque;
    if((semantic&NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN)
        return nativeScriptBindingItemResources_DescribeCall(&context->spawn,semantic,layout,diagnostic);
    if((semantic&NATIVE_SCRIPT_RECEIVER_DOMAIN_MASK)==NATIVE_SCRIPT_RECEIVER_DOMAIN)
        return nativeScriptBindingReceiver_DescribeCall(&context->receiver,semantic,layout,diagnostic);
    if((semantic&NATIVE_SCRIPT_EFFECT_DOMAIN_MASK)==NATIVE_SCRIPT_EFFECT_DOMAIN)
        return nativeScriptBindingEffect_DescribeCall(&context->spawn,semantic,layout,diagnostic);
    uint32_t model_domain=semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK;
    if((model_domain>=NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD&&model_domain<=NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE)||
       model_domain==NATIVE_SCRIPT_ITEM_MODEL_ASYNC_EFFECT||model_domain==NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT)
        return nativeScriptBindingItemModel_DescribeCall(&context->spawn,semantic,layout,diagnostic);
    return nativeScriptBindings_DescribeCall(&context->native,semantic,layout,diagnostic);
}

static bool graph_function(NativeScriptCompileBank* bank,uint32_t function,unsigned depth,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptCompileFunctionScratch* scratch=&bank->graph[function];
    if (scratch->color==2) return true;
    if (scratch->color==1) return error(diagnostic,bank->declarations.functions[function].name,"Authored helper recursion is unsupported");
    if (depth>NATIVE_SCRIPT_EXPRESSION_DEPTH) return error(diagnostic,bank->declarations.functions[function].name,"Helper call depth exceeds compiler capacity");
    scratch->color=1;scratch->depth=1;scratch->registers=bank->function_code[function].register_count;
    const NativeScriptFunctionCode* code=&bank->function_code[function];
    for (uint32_t i=code->first_instruction;i<code->first_instruction+code->instruction_count;++i) {
        const NativeScriptInstruction* instruction=&bank->instructions[i];
        bool recipient=instruction->opcode==NativeScriptBC_Invoke &&
            (instruction->semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK)==NATIVE_SCRIPT_RECEIVER_HELPER;
        bool signal=instruction->opcode==NativeScriptBC_Invoke &&
            (instruction->semantic&NATIVE_SCRIPT_SIGNAL_DOMAIN_MASK)==NATIVE_SCRIPT_SIGNAL_DOMAIN;
        if (instruction->opcode!=NativeScriptBC_Call && !recipient && !signal) continue;
        uint32_t called=signal?instruction->semantic&NATIVE_SCRIPT_SIGNAL_FUNCTION_MASK:
            recipient?instruction->semantic&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK:instruction->semantic;
        if (called>=bank->declarations.function_count || bank->declarations.functions[called].callback!=signal)
            return error(diagnostic,instruction->source,"Authored call does not identify a helper function");
        if (!recipient && !signal && bank->declarations.functions[called].owner!=bank->declarations.functions[function].owner)
            return error(diagnostic,instruction->source,"Cross-owner helper calls need an accepted receiver/context contract");
        if (!graph_function(bank,called,depth+1,diagnostic)) return false;
        NativeScriptCompileFunctionScratch child=bank->graph[called];
        if (child.registers>UINT32_MAX-code->register_count)
            return error(diagnostic,instruction->source,"Helper register requirement exceeds capacity");
        if (scratch->depth<child.depth+1) scratch->depth=child.depth+1;
        if (scratch->registers<child.registers+code->register_count) scratch->registers=child.registers+code->register_count;
    }
    if (scratch->depth>NATIVE_SCRIPT_EXPRESSION_DEPTH)
        return error(diagnostic,bank->declarations.functions[function].name,"Helper call depth exceeds compiler capacity");
    scratch->color=2;return true;
}

static bool event_collisions(const NativeScriptCompileBank* bank,NativeScriptDiagnostic* diagnostic)
{
    for (uint32_t i=0;i<bank->declarations.function_count;++i) {
        const NativeScriptFunctionDeclaration* function=&bank->declarations.functions[i];
        if (!function->callback) continue;
        NativeScriptEventResolution current;
        if (!nativeScriptEvents_Resolve(&bank->tree,&bank->tree.nodes[function->source_node],&current,diagnostic)) return false;
        for (uint32_t j=0;j<i;++j) {
            const NativeScriptFunctionDeclaration* earlier=&bank->declarations.functions[j];
            if (!earlier->callback || earlier->owner!=function->owner) continue;
            NativeScriptEventResolution previous;
            if (!nativeScriptEvents_Resolve(&bank->tree,&bank->tree.nodes[earlier->source_node],&previous,diagnostic)) return false;
            bool current_motion=current.descriptor->kind==NativeScriptEvent_FighterMotion || current.descriptor->kind==NativeScriptEvent_MovePhase;
            bool previous_motion=previous.descriptor->kind==NativeScriptEvent_FighterMotion || previous.descriptor->kind==NativeScriptEvent_MovePhase;
            if(bank->has_motion_layout && current_motion && previous_motion) {
                NativeScriptMotionTarget a,b;
                if(!nativeScriptMoves_Target(&bank->motion_layout,&bank->tree,&bank->tree.nodes[function->source_node],&a,diagnostic) ||
                    !nativeScriptMoves_Target(&bank->motion_layout,&bank->tree,&bank->tree.nodes[earlier->source_node],&b,diagnostic))return false;
                if(a.phase==b.phase)for(unsigned ai=0;ai<a.count;++ai)for(unsigned bi=0;bi<b.count;++bi)
                    if(a.motions[ai]==b.motions[bi])return error(diagnostic,function->name,"Duplicate callback target in owned motion table");
                continue;
            }
            if (current.event_id!=previous.event_id || current.motion!=previous.motion) continue;
            if (current.descriptor->kind==NativeScriptEvent_MovePhase &&
                bank->tree.nodes[function->source_node].parent!=bank->tree.nodes[earlier->source_node].parent) continue;
            if (current.signal_name.length!=previous.signal_name.length ||
                (current.signal_name.length && memcmp(bank->tree.source+current.signal_name.offset,
                    bank->tree.source+previous.signal_name.offset,current.signal_name.length))) continue;
            return error(diagnostic,function->name,"Duplicate callback identity for this source owner");
        }
    }
    return true;
}

bool nativeScriptCompile_Build(NativeScriptCompileBank* bank,const NativeScriptCompileRequest* request,
    const NativeScriptCompileBank** accepted,NativeScriptDiagnostic* diagnostic)
{
    if (!bank || !request || !accepted || !diagnostic) return false;
    memset(diagnostic,0,sizeof(*diagnostic));
    NativeScriptCompileDiagnostics* report=request->diagnostics;
    if(report) {
        if(!report->entries||!report->capacity)
            return error(diagnostic,(NativeScriptSourceSpan){0},"Audit needs independent diagnostic storage with nonzero capacity");
        report->count=report->total=report->functions_checked=0;
        report->metadata_complete=report->bodies_complete=false;
    }
    if (!independent(bank,*accepted,diagnostic)) return false;
    bank->valid=false;bank->has_motion_layout=false;
    bank->pose_symbol_count=bank->pose_graph_count=bank->pose_graph_mask=0;
    bank->hitbox_definition_count=0;
    memset(bank->pose_symbols,0,sizeof(bank->pose_symbols));
    if(request->motion_layout) {
        if(!nativeScriptMoves_Validate(request->motion_layout,diagnostic))return false;
        bank->motion_layout=*request->motion_layout;bank->has_motion_layout=true;
    }
    if(request->parameter_provider && (request->parameter_supply || request->parameter_supply_count))
        return error(diagnostic,(NativeScriptSourceSpan){0},"Parameter provider and explicit supplies are mutually exclusive");
    if (!request->source || !request->source_bytes || request->source_bytes>=bank->source_capacity ||
        !bank->locals || !bank->instructions || !bank->function_code || !bank->execution_functions || !bank->graph ||
        !bank->tree.nodes || !bank->expressions.nodes || !bank->expressions.arguments ||
        !bank->declarations.functions || !bank->declarations.parameters || !bank->declarations.values)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Missing fixed compile buffers or source exceeds capacity");
    memmove(bank->source_storage,request->source,request->source_bytes);bank->source_storage[request->source_bytes]=0;
    bank->tree.source=bank->source_storage;bank->tree.source_bytes=request->source_bytes;bank->tree.count=0;
    bank->expressions.source=bank->source_storage;bank->expressions.source_bytes=request->source_bytes;
    bank->expressions.node_count=bank->expressions.argument_count=0;
    bank->declarations.tree=&bank->tree;bank->declarations.expressions=&bank->expressions;
    bank->declarations.event_signature=nativeScriptEvents_Signature;bank->declarations.event_context=NULL;
    bank->declarations.function_count=bank->declarations.parameter_count=bank->declarations.value_count=0;
    NativeScriptCompileMetadataValidator metadata=request->metadata ? request->metadata :
        bank->has_motion_layout?nativeScriptMoves_Metadata:nativeScriptCompile_MinimalMetadata;
    void* metadata_context=request->metadata?request->metadata_context:
        bank->has_motion_layout?&bank->motion_layout:NULL;
    CompileContext defaults={.native={NativeScriptBindings_Fighter},.bank=bank,.function=UINT32_MAX,
        .pose={.bank=bank,.owner=0,.context_mask=NativeScriptBindings_Fighter}};
    bank->declarations.constant_resolver=resolve;bank->declarations.constant_context=&defaults;
    if (!nativeScriptSource_Parse(&bank->tree,diagnostic) ||
        !nativeScriptPoseMetadata_Prepare(bank,request->pose_graphs,request->pose_graph_count,request->pose_graph_mask,diagnostic) ||
        !nativeScriptHitboxMetadata_Prepare(bank,diagnostic) ||
        !validate_metadata(bank,metadata,metadata_context,report,diagnostic) ||
        !nativeScriptProgram_Index(&bank->declarations,diagnostic)) {
        bank->declarations.constant_resolver=NULL;bank->declarations.constant_context=NULL;return false;
    }
    bank->declarations.constant_resolver=NULL;bank->declarations.constant_context=NULL;
    if (!nativeScriptPoseMetadata_CheckSymbols(bank,diagnostic) ||
        !nativeScriptHitboxMetadata_CheckSymbols(bank,diagnostic) || !event_collisions(bank,diagnostic)) return false;
    if (bank->declarations.function_count>bank->function_code_capacity ||
        bank->declarations.function_count>bank->execution_function_capacity || bank->declarations.function_count>bank->graph_capacity)
        return error(diagnostic,bank->tree.nodes[0].source,"Function code/execution/graph capacity exceeded");
    NativeScriptBytecodeBuilder builder={.instructions=bank->instructions,.instruction_capacity=bank->instruction_capacity,
        .functions=bank->function_code,.function_count=bank->declarations.function_count,
        .describe_call=describe};
    NativeScriptProgramCompiler compiler={.index=&bank->declarations,.native_bindings=resolve,
        .locals=bank->locals,.local_capacity=bank->local_capacity};
    for (uint32_t i=0;i<bank->declarations.function_count;++i) {
        CompileContext context;NativeScriptStatementCompiler body;
        if(report) {
            ++report->functions_checked;
            memset(diagnostic,0,sizeof(*diagnostic));
        }
        if (!function_context(bank,i,&context,diagnostic)) {
            if(!report)return false;
            record_diagnostic(report,diagnostic);continue;
        }
        compiler.native_binding_context=&context;
        builder.binding_context=&context;
        uint32_t first_expression=bank->expressions.node_count;
        uint32_t first_argument=bank->expressions.argument_count;
        uint32_t first_instruction=builder.instruction_count;
        uint32_t first_export_name=bank->declarations.export_name_count;
        if (!nativeScriptProgram_CompileBody(&compiler,i,&body,diagnostic) ||
            !nativeScriptMoveLowering_Apply(bank->has_motion_layout?&bank->motion_layout:NULL,&bank->declarations,i,
                &bank->expressions,first_expression,diagnostic) ||
            !nativeScriptBytecode_Compile(&builder,&body,bank->declarations.functions[i].source_node,i,
                (uint16_t)bank->declarations.functions[i].parameter_count,diagnostic)) {
            if(!report)return false;
            record_diagnostic(report,diagnostic);
            bank->expressions.node_count=first_expression;
            bank->expressions.argument_count=first_argument;
            bank->declarations.export_name_count=first_export_name;
            builder.instruction_count=first_instruction;
            memset(&bank->function_code[i],0,sizeof(bank->function_code[i]));
        }
    }
    if(report) {
        report->bodies_complete=true;
        if(report->total){*diagnostic=report->entries[0];return false;}
    }
    bank->code=(NativeScriptBytecodeProgram){bank->source_storage,request->source_bytes,bank->instructions,
        builder.instruction_count,bank->function_code,bank->declarations.function_count,false};
    if(!nativeScriptBytecode_Prepare(&bank->code,diagnostic))return false;
    memset(bank->graph,0,bank->declarations.function_count*sizeof(*bank->graph));
    bank->maximum_call_depth=bank->maximum_call_registers=0;
    for (uint32_t i=0;i<bank->declarations.function_count;++i) {
        if (!graph_function(bank,i,0,diagnostic)) return false;
        if (bank->maximum_call_depth<bank->graph[i].depth) bank->maximum_call_depth=bank->graph[i].depth;
        if (bank->maximum_call_registers<bank->graph[i].registers) bank->maximum_call_registers=bank->graph[i].registers;
    }
    if (!nativeScriptExecution_BuildFunctions(&bank->declarations,&bank->code,bank->execution_functions,
            bank->execution_function_capacity,diagnostic) ||
        !nativeScriptStorage_BuildLayout(&bank->storage_layout,&bank->declarations,diagnostic)) return false;
    const NativeScriptParameterSupply* supplies=request->parameter_supply;
    uint32_t supply_count=request->parameter_supply_count;
    if(request->parameter_provider && !request->parameter_provider(request->parameter_context,&bank->declarations,
            &supplies,&supply_count,diagnostic))return false;
    if(!nativeScriptStorage_PrepareParameters(&bank->storage_layout,bank->parameter_slots,bank->parameter_slot_capacity,
            supplies,supply_count,&bank->storage_parameters,diagnostic)) return false;
    for (uint32_t i=0;i<bank->storage_layout.owner_count;++i)
        if (!nativeScriptStorage_ParametersReady(&bank->storage_layout,&bank->storage_parameters,
                bank->storage_layout.owners[i].owner,diagnostic)) return false;
    bank->valid=true;*accepted=bank;return true;
}
