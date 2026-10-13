#include "native_script_effect_program.h"
#include "native_script_binding_effect.h"
#include <melee/ef/native_script_effect_actor.h>
#include <melee/ef/efsync.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,NativeScriptSourceSpan source,const char* message)
{if(d){d->source=source;snprintf(d->message,sizeof(d->message),"%s",message);}return false;}
static uint32_t owner(const NativeScriptSourceTree* tree,uint32_t at)
{
    for(uint32_t bound=0;at<tree->count&&bound<tree->count;++bound) {
        if(tree->nodes[at].kind==NativeScriptSource_Effect)return at;
        if(!at)break;at=tree->nodes[at].parent;
    }
    return 0;
}
bool nativeScriptEffectMetadata_Owns(const NativeScriptSourceTree* tree,uint32_t at)
{return tree&&at<tree->count&&owner(tree,at)!=0;}
static const EF_ModelRecipe* recipe(const NativeScriptSourceTree* tree,uint32_t at,int32_t* id)
{
    NativeScriptSourceSpan value=tree->nodes[at].value;if(!value.length||value.length>=32)return NULL;
    char text[32];memcpy(text,tree->source+value.offset,value.length);text[value.length]=0;
    char* end;long long number=strtoll(text,&end,0);if(*end||number<0||number>INT32_MAX)return NULL;
    const EF_ModelRecipe* found=efSync_ModelRecipe((int32_t)number);
    if(!found||!found->model_count||found->model_count>EF_MODEL_GROUP_MAX)return NULL;
    *id=(int32_t)number;return found;
}
bool nativeScriptEffectMetadata_Validate(const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
    if(!tree||at>=tree->count)return false;uint32_t enclosing=owner(tree,at);int32_t id;
    if(!enclosing||!recipe(tree,enclosing,&id))return error(d,tree->nodes[at].source,"Effect needs a supported numeric native model recipe");
    const NativeScriptSourceNode* node=&tree->nodes[at];
    if(at==enclosing)return node->parent==0;
    if(node->kind==NativeScriptSource_Metadata&&node->parent==enclosing&&!node->opens&&
       nativeScriptSource_SpanEquals(tree,node->name,"pass")&&!node->value.length)return true;
    if(node->kind==NativeScriptSource_State||node->kind==NativeScriptSource_Parameter||node->kind==NativeScriptSource_Table||node->kind==NativeScriptSource_Function)
        return node->parent==enclosing;
    if(node->kind==NativeScriptSource_Callback) {
        NativeScriptEventResolution event;
        return nativeScriptEvents_Resolve(tree,node,&event,d)&&event.owner_node==enclosing&&
            event.descriptor->kind==NativeScriptEvent_EffectUpdate&&event.descriptor->result_type==NativeScriptType_Void&&
            event.descriptor->argument_count==0;
    }
    return error(d,node->source,"Unsupported interpreted effect metadata");
}
bool nativeScriptEffectPrograms_Prepare(const NativeScriptCompileBank* bank,FighterKind author,uint64_t identity,
    NativeScriptEffectPrograms* output,NativeScriptDiagnostic* d)
{
    if(!bank||!bank->valid||!output||!identity)return error(d,(NativeScriptSourceSpan){0},"Missing immutable effect program inputs");
    NativeScriptEffectPrograms candidate={0};const NativeScriptSourceTree* tree=&bank->tree;
    for(uint32_t at=0;at<tree->count;++at) {
        if(tree->nodes[at].kind!=NativeScriptSource_Effect)continue;
        if(candidate.count==NATIVE_SCRIPT_EFFECT_PROGRAMS)return error(d,tree->nodes[at].source,"Too many source model effects");
        int32_t id;const EF_ModelRecipe* native=recipe(tree,at,&id);
        const NativeScriptStorageOwner* storage=nativeScriptStorage_Owner(&bank->storage_layout,at);
        if(!native||!storage||storage->state_bytes>NATIVE_SCRIPT_EFFECT_STATE_BYTES)
            return error(d,tree->nodes[at].source,"Effect recipe/state exceeds reserved native owner");
        for(unsigned prior=0;prior<candidate.count;++prior) {
            NativeScriptSourceSpan a=tree->nodes[candidate.programs[prior].owner].name,b=tree->nodes[at].name;
            if(a.length==b.length&&!memcmp(tree->source+a.offset,tree->source+b.offset,a.length))
                return error(d,b,"Duplicate source effect name");
        }
        for(uint32_t n=0;n<tree->count;++n)if(owner(tree,n)==at) {
            unsigned kind=tree->nodes[n].kind;if(kind>=NativeScriptSource_If&&kind<=NativeScriptSource_Pass)continue;
            if(!nativeScriptEffectMetadata_Validate(tree,n,d))return false;
        }
        NativeScriptEffectProgram* p=&candidate.programs[candidate.count++];
        *p=(NativeScriptEffectProgram){.bank=bank,.identity=identity,.author_kind=author,.owner=at,.update=UINT32_MAX,
            .gfx_id=id,.family=native->family,.member_count=native->model_count};
        for(uint32_t f=0;f<bank->declarations.function_count;++f) {
            const NativeScriptFunctionDeclaration* fn=&bank->declarations.functions[f];if(fn->owner!=at||!fn->callback)continue;
            const NativeScriptExecutionFunction* code=&bank->execution_functions[f];
            const NativeScriptEventDescriptor* event=nativeScriptEvents_Descriptor(code->event_id);
            if(p->update!=UINT32_MAX||!event||event->kind!=NativeScriptEvent_EffectUpdate||event->result_type!=NativeScriptType_Void||
                event->argument_count||fn->result_type!=NativeScriptType_Void||fn->parameter_count||code->owner!=at||
                code->source_class!=NativeCallbackSource_ModelEffect)
                return error(d,fn->name,"Invalid/duplicate interpreted effect update ABI");
            p->update=f;
        }
        if(!nativeScriptBindingEffect_PrepareSelectors(bank,p,d))return false;
    }
    *output=candidate;return true;
}
