#include "native_script_moves.h"
#include "native_script_compile.h"
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan source,const char* message)
{if(diagnostic){diagnostic->source=source;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);}return false;}
bool nativeScriptMoves_Contains(const NativeScriptMoveLayout* layout,uint32_t motion)
{return layout && (motion<layout->common_count || (motion>=layout->first && motion-layout->first<layout->count));}
bool nativeScriptMoves_Validate(const NativeScriptMoveLayout* layout,NativeScriptDiagnostic* diagnostic)
{
    if(!layout || layout->common_count>1024 || layout->first>UINT16_MAX || layout->count>1024 ||
        layout->first<layout->common_count || layout->count>UINT16_MAX+1u-layout->first ||
        (!layout->common_count && !layout->count))return error(diagnostic,(NativeScriptSourceSpan){0},"Invalid owned fighter motion layout");
    for(unsigned i=0;i<NativeScriptMove_Count;++i) {
        uint32_t ids[]={layout->ground[i],layout->air[i]};
        for(unsigned p=0;p<2;++p)if(ids[p] && (ids[p]<layout->first || ids[p]-layout->first>=layout->count))
            return error(diagnostic,(NativeScriptSourceSpan){0},"Named special state is outside the owned motion table");
    }
    return true;
}
static int entry(const NativeScriptSourceTree* tree,NativeScriptSourceSpan name,bool* air)
{
    static const char* const names[]={"special_n","special_s","special_hi","special_lw",
        "special_air_n","special_air_s","special_air_hi","special_air_lw"};
    for(unsigned i=0;i<8;++i)if(nativeScriptSource_SpanEquals(tree,name,names[i])){*air=i>=4;return (int)(i%4);}
    return -1;
}
static int move(const NativeScriptSourceTree* tree,uint32_t node,NativeScriptDiagnostic* diagnostic)
{
    int selected=-1;
    for(uint32_t child=tree->nodes[node].first_child;child!=UINT32_MAX;child=tree->nodes[child].next) {
        bool air;const NativeScriptSourceNode* member=&tree->nodes[child];
        int candidate=member->kind==NativeScriptSource_Callback?entry(tree,member->name,&air):-1;
        if(candidate<0)continue;
        if(selected>=0 && selected!=candidate){error(diagnostic,member->name,"One named move cannot own two different special entries");return -2;}
        selected=candidate;
    }
    return selected;
}
bool nativeScriptMoves_Target(const NativeScriptMoveLayout* layout,const NativeScriptSourceTree* tree,
    const NativeScriptSourceNode* callback,NativeScriptMotionTarget* target,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptEventResolution event;
    if(!layout || !nativeScriptEvents_Resolve(tree,callback,&event,diagnostic))return false;
    const char* name=event.descriptor->name;uint32_t selected_motion=event.motion,selected_phase;
    NativeScriptMotionTarget candidate={.count=1};
    if(event.descriptor->kind==NativeScriptEvent_FighterMotion) {
        if(!strcmp(name,"animation"))selected_phase=NativeScriptMotion_Animation;
        else if(!strcmp(name,"interrupt"))selected_phase=NativeScriptMotion_Interrupt;
        else if(!strcmp(name,"physics"))selected_phase=NativeScriptMotion_Physics;
        else if(!strcmp(name,"collision"))selected_phase=NativeScriptMotion_Collision;
        else if(!strcmp(name,"accessory"))selected_phase=NativeScriptMotion_Accessory;
        else return error(diagnostic,callback->name,"Unsupported fighter motion phase");
    } else if(event.descriptor->kind==NativeScriptEvent_MovePhase) {
        if(callback->parent>=tree->count || tree->nodes[callback->parent].kind!=NativeScriptSource_Move)
            return error(diagnostic,callback->name,"Named phase has no owned move");
        int special=move(tree,callback->parent,diagnostic);
        if(special<0)return special==-2?false:error(diagnostic,callback->name,"Named move phases require a special entry or explicit state_N callback");
        bool air=!strncmp(name,"air_",4);selected_motion=air?layout->air[special]:layout->ground[special];
        const char* suffix=!strncmp(name,"ground_",7)?name+7:air?name+4:name;
        if(!strcmp(suffix,"animation"))selected_phase=NativeScriptMotion_Animation;
        else if(!strcmp(suffix,"interrupt"))selected_phase=NativeScriptMotion_Interrupt;
        else if(!strcmp(suffix,"physics"))selected_phase=NativeScriptMotion_Physics;
        else if(!strcmp(suffix,"collision"))selected_phase=NativeScriptMotion_Collision;
        else if(!strcmp(suffix,"accessory") || !strcmp(suffix,"neutral_special_cue")) {
            selected_phase=NativeScriptMotion_Accessory;
            if(!layout->ground[special] || !layout->air[special])return error(diagnostic,callback->name,"Shared accessory needs both named special states");
            selected_motion=layout->ground[special];candidate.motions[1]=layout->air[special];candidate.count=2;
        } else return error(diagnostic,callback->name,"Move phase scheduler is unsupported");
        if(!selected_motion)return error(diagnostic,callback->name,"Named phase targets an absent special state");
    } else return error(diagnostic,callback->name,"Callback does not target a fighter motion phase");
    if(!nativeScriptMoves_Contains(layout,selected_motion))return error(diagnostic,callback->name,"Scripted motion target exceeds its actual fighter layout");
    candidate.motions[0]=selected_motion;candidate.phase=selected_phase;*target=candidate;return true;
}
bool nativeScriptMoves_Metadata(void* opaque,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptMoveLayout* layout=opaque;const NativeScriptSourceNode* node=&tree->nodes[at];
    if(node->kind==NativeScriptSource_Move)
        return node->parent==0 && node->name.length && !node->value.length?true:error(diagnostic,node->source,"Invalid named move declaration");
    if(node->kind==NativeScriptSource_Attribute)return node->parent==0?true:error(diagnostic,node->source,"Only root fighter parameter imports are installed");
    if(node->kind==NativeScriptSource_Metadata && nativeScriptSource_SpanEquals(tree,node->name,"entry")) {
        if(node->parent>=tree->count || tree->nodes[node->parent].kind!=NativeScriptSource_Move ||
            node->opens || !nativeScriptSource_SpanEquals(tree,node->value,"explicit"))
            return error(diagnostic,node->source,"Entry policy must be 'entry explicit' directly inside a named move");
        bool seen=false,callback_seen=false;
        for(uint32_t child=tree->nodes[node->parent].first_child;child!=UINT32_MAX;child=tree->nodes[child].next) {
            const NativeScriptSourceNode* member=&tree->nodes[child];
            if(member->kind==NativeScriptSource_Callback)callback_seen=true;
            if(member->kind!=NativeScriptSource_Metadata || !nativeScriptSource_SpanEquals(tree,member->name,"entry"))continue;
            if(seen)return error(diagnostic,member->source,"A move can declare its entry policy only once");
            if(callback_seen)return error(diagnostic,member->source,"Declare the entry policy before move callbacks");
            seen=true;
        }
        int special=move(tree,node->parent,diagnostic);
        if(special<0)return special==-2?false:error(diagnostic,node->source,"Explicit entry needs an authored special entry callback");
        // Interpreter entry already runs the complete authored callback. There
        // is no generated command-reset/accessory prologue to suppress.
        return true;
    }
    if(node->kind==NativeScriptSource_Callback) {
        NativeScriptEventResolution event;if(!nativeScriptEvents_Resolve(tree,node,&event,diagnostic))return false;
        if(event.descriptor->kind==NativeScriptEvent_FighterMotion || event.descriptor->kind==NativeScriptEvent_MovePhase) {
            NativeScriptMotionTarget target;return nativeScriptMoves_Target(layout,tree,node,&target,diagnostic);
        }
        if(node->parent<tree->count && tree->nodes[node->parent].kind==NativeScriptSource_Move && event.descriptor->kind==NativeScriptEvent_FighterHook){
            int special=move(tree,node->parent,diagnostic);if(special==-2)return false;
            return true;
        }
    }
    return nativeScriptCompile_MinimalMetadata(NULL,tree,at,diagnostic);
}
int nativeScriptMoves_TryResolve(const NativeScriptMoveLayout* layout,const NativeScriptProgramIndex* index,
    uint32_t function,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,NativeScriptIRResolution* output,NativeScriptDiagnostic* diagnostic)
{
    if(!layout || node->kind!=NativeScriptIR_Name)return 0;
    static const char* const names[]={"NeutralSpecial","SideSpecial","UpSpecial","DownSpecial",
        "AirNeutralSpecial","AirSideSpecial","AirUpSpecial","AirDownSpecial"};
    for(unsigned i=0;i<8;++i) {
        if(node->text_length!=strlen(names[i]) || memcmp(arena->source+node->text_offset,names[i],node->text_length))continue;
        bool air=i>=4;
        if(!air && function<index->function_count && index->functions[function].callback){
            NativeScriptEventResolution event;
            if(!nativeScriptEvents_Resolve(index->tree,&index->tree->nodes[index->functions[function].source_node],&event,diagnostic))return -1;
            if(!strncmp(event.descriptor->name,"air_",4) || !strncmp(event.descriptor->name,"special_air_",12))air=true;
        }
        uint32_t motion=air?layout->air[i%4]:layout->ground[i%4];
        if(!motion){error(diagnostic,node->source,"Named special state is absent from this fighter's owned layout");return -1;}
        *output=(NativeScriptIRResolution){.type=NativeScriptType_I32,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,
            .flags=NativeScriptFlag_Literal,.literal_bits=motion};return 1;
    }
    return 0;
}
