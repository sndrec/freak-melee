#include "native_script_move_lowering.h"
#include "native_script_bindings.h"
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan source,const char* message)
{if(diagnostic){diagnostic->source=source;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);}return false;}
static bool spelling(const NativeScriptIRArena* arena,const NativeScriptIRNode* node,const char* name)
{return node->text_offset<=arena->source_bytes && node->text_length<=arena->source_bytes-node->text_offset &&
    node->text_length==strlen(name) && !memcmp(arena->source+node->text_offset,name,node->text_length);}
static int special(const NativeScriptProgramIndex* index,uint32_t function,NativeScriptDiagnostic* diagnostic)
{
    if(function>=index->function_count || !index->functions[function].callback)return -1;
    const NativeScriptSourceTree* tree=index->tree;
    const NativeScriptSourceNode* callback=&tree->nodes[index->functions[function].source_node];
    if(callback->parent>=tree->count || tree->nodes[callback->parent].kind!=NativeScriptSource_Move)return -1;
    static const char* const names[]={"special_n","special_s","special_hi","special_lw",
        "special_air_n","special_air_s","special_air_hi","special_air_lw"};
    int selected=-1;
    for(uint32_t node=tree->nodes[callback->parent].first_child;node!=UINT32_MAX;node=tree->nodes[node].next) {
        if(tree->nodes[node].kind!=NativeScriptSource_Callback)continue;
        for(unsigned i=0;i<8;++i)if(nativeScriptSource_SpanEquals(tree,tree->nodes[node].name,names[i])){
            if(selected>=0 && selected!=(int)(i%4)){error(diagnostic,callback->source,"Contextual transition has conflicting special owners");return -2;}
            selected=(int)(i%4);
        }
    }
    return selected;
}
bool nativeScriptMoveLowering_Apply(const NativeScriptMoveLayout* layout,const NativeScriptProgramIndex* index,
    uint32_t function,NativeScriptIRArena* arena,uint32_t first,NativeScriptDiagnostic* diagnostic)
{
    uint32_t end=arena->node_count;
    for(uint32_t i=first;i<end;++i) {
        NativeScriptIRNode* call=&arena->nodes[i];if(call->kind!=NativeScriptIR_Call)continue;
        if(call->semantic==NativeScriptBinding_StateEnter && call->argument_count==1) {
            const NativeScriptIRNode* argument=&arena->nodes[arena->arguments[call->argument_first]];
            static const char* const names[]={"NeutralSpecial","SideSpecial","UpSpecial","DownSpecial",
                "AirNeutralSpecial","AirSideSpecial","AirUpSpecial","AirDownSpecial"};
            for(unsigned n=0;n<8;++n)if(argument->kind==NativeScriptIR_Integer && spelling(arena,argument,names[n])){
                // Original named special entry evaluates frame0 then performs
                // its shared startup animation step; numeric enter starts at1.
                call->semantic=NativeScriptBinding_StateBegin;break;
            }
        }
        if(call->semantic!=NativeScriptBinding_StateToAir && call->semantic!=NativeScriptBinding_StateToGround)continue;
        int selected=special(index,function,diagnostic);
        if(!layout || selected<0)return selected==-2?false:error(diagnostic,call->source,
            "Contextual plane transition needs an owned move; helpers can use switch_to_air/ground(motion, flags)");
        uint32_t motion=call->semantic==NativeScriptBinding_StateToAir?layout->air[selected]:layout->ground[selected];
        if(!motion || !nativeScriptMoves_Contains(layout,motion))return error(diagnostic,call->source,"Contextual transition targets an absent owned state");
        if(arena->node_count>arena->node_capacity || arena->node_capacity-arena->node_count<2 ||
            arena->argument_count>arena->argument_capacity || arena->argument_capacity-arena->argument_count<2)
            return error(diagnostic,call->source,"Contextual transition lowering storage is exhausted");
        uint32_t node=arena->node_count,args=arena->argument_count;
        arena->nodes[node]=(NativeScriptIRNode){.source=call->source,.a=UINT32_MAX,.b=UINT32_MAX,
            .semantic=UINT32_MAX,.bits=motion,.flags=NativeScriptFlag_Readable|NativeScriptFlag_Pure,
            .kind=NativeScriptIR_Integer,.type=NativeScriptType_I32};
        arena->nodes[node+1]=(NativeScriptIRNode){.source=call->source,.a=UINT32_MAX,.b=UINT32_MAX,
            .semantic=UINT32_MAX,.bits=layout->transition_flags[selected],.flags=NativeScriptFlag_Readable|NativeScriptFlag_Pure,
            .kind=NativeScriptIR_Integer,.type=NativeScriptType_U32};
        arena->arguments[args]=node;arena->arguments[args+1]=node+1;
        arena->node_count+=2;arena->argument_count+=2;
        call->argument_first=args;call->argument_count=2;
        call->semantic=call->semantic==NativeScriptBinding_StateToAir?NativeScriptBinding_StateSwitchToAir:NativeScriptBinding_StateSwitchToGround;
    }
    return true;
}
