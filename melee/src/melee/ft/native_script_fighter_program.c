#include "native_script_fighter.h"
#include "native_fighter_runtime.h"
#include "native_script_runtime.h"
#include "native_script_pose_metadata.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/ef/native_script_effect_actor.h>
#include "kinds/ftCommon/forward.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Immutable program selection is host-owned. Actual actor state lives in the
// separate owner unit; synchronous execution scratch belongs to runtime.c.
static NativeScriptFighterProgram programs[Ft_Kind_RuntimeCapacity];

const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{
    return (uint32_t)kind<Ft_Kind_RuntimeCapacity && programs[kind].bank?&programs[kind]:NULL;
}

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{
    if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    return false;
}

static bool motionTarget(NativeScriptFighterProgram* program,uint32_t motion,uint32_t phase,uint32_t function,bool cue,NativeScriptDiagnostic* diagnostic)
{
    uint32_t at=0;while(at<program->motion_count && program->motions[at].motion<motion)++at;
    if(at==program->motion_count || program->motions[at].motion!=motion) {
        if(program->motion_count==NATIVE_SCRIPT_MOTION_PATCHES)return error(diagnostic,"Too many patched fighter motions");
        memmove(program->motions+at+1,program->motions+at,(program->motion_count-at)*sizeof(*program->motions));
        program->motions[at].motion=motion;
        program->motions[at].accessory_cue=false;
        for(unsigned p=0;p<NativeScriptMotion_Count;++p)program->motions[at].functions[p]=UINT32_MAX;
        ++program->motion_count;
    }
    uint32_t* slot=&program->motions[at].functions[phase];
    if(*slot!=UINT32_MAX && *slot!=function)return error(diagnostic,"Duplicate interpreted motion phase");
    *slot=function;
    if(phase==NativeScriptMotion_Accessory)program->motions[at].accessory_cue=cue;
    return true;
}

bool nativeScriptFighter_PrepareProgram(FighterKind kind,const NativeScriptCompileBank* bank,
    const NativeScriptMoveLayout* owned_layout,const NativeScriptItemMetadataContext* item_metadata,
    NativeScriptFighterProgram* output,NativeScriptDiagnostic* diagnostic)
{
    if((uint32_t)kind>=Ft_Kind_RuntimeCapacity || kind==Ft_Kind_None)
        return error(diagnostic,"Invalid fighter kind for interpreted program");
    if(!output)return error(diagnostic,"Missing prepared program storage");
    // Package preparation is an explicit loading transition. Keep its large
    // transactional candidate off the thread stack; no tick or callback allocates.
    NativeScriptFighterProgram* candidate=calloc(1,sizeof(*candidate));
    if(!candidate)return error(diagnostic,"Unable to allocate fighter preparation scratch");
    bool accepted=false;candidate->bank=bank;
    for(unsigned i=0;i<NativeScriptFighter_HookCount;++i)candidate->hooks[i]=UINT32_MAX;
    for(unsigned i=0;i<NativeScriptFighterCallback_Count;++i)candidate->callbacks[i]=UINT32_MAX;
    if(bank) {
        const NativeScriptStorageOwner* owner=nativeScriptStorage_Owner(&bank->storage_layout,0);
        if(!bank->valid || !owner || owner->state_bytes>NATIVE_SCRIPT_FIGHTER_STATE_BYTES ||
            bank->maximum_call_depth>64 || bank->maximum_call_registers>65536)
            {error(diagnostic,"Program exceeds reserved fighter/interpreter storage");goto cleanup;}
        if(bank->has_motion_layout) {
            const NativeScriptMoveLayout* layout=&bank->motion_layout;
            if(layout->common_count>ftCo_MS_Count){error(diagnostic,"Common motion layout exceeds the actual engine table");goto cleanup;}
            if(!owned_layout || !nativeScriptMoves_Validate(owned_layout,diagnostic) ||
                owned_layout->common_count!=layout->common_count || owned_layout->first!=layout->first ||
                owned_layout->count!=layout->count){error(diagnostic,"Scripted motion layout differs from owned numeric assets");goto cleanup;}
            {
                for(unsigned special=0;special<NativeScriptMove_Count;++special)
                    if(owned_layout->ground[special]!=layout->ground[special] ||
                        owned_layout->air[special]!=layout->air[special] ||
                        owned_layout->transition_flags[special]!=layout->transition_flags[special])
                        {error(diagnostic,"Named special layout differs from owned numeric assets");goto cleanup;}
            }
        }
        for(uint32_t i=0;i<bank->declarations.function_count;++i) {
            const NativeScriptFunctionDeclaration* function=&bank->declarations.functions[i];
            if(function->owner) {
                unsigned source=bank->tree.nodes[function->owner].kind;
                if(source!=NativeScriptSource_Article && source!=NativeScriptSource_Item && source!=NativeScriptSource_Effect)
                    {error(diagnostic,"This entity source class has no installed owner/scheduler");goto cleanup;}
                continue; // Complete Item ABI/state/signal validation follows.
            }
            if(!function->callback)continue;
            const NativeScriptEventDescriptor* event=nativeScriptEvents_Descriptor(bank->execution_functions[i].event_id);
            if(!event){error(diagnostic,"Unknown interpreted event contract");goto cleanup;}
            if(event->kind==NativeScriptEvent_FighterMotion || event->kind==NativeScriptEvent_MovePhase) {
                if(event->argument_count || event->record || event->result_type!=NativeScriptType_Void)
                    {error(diagnostic,"Motion scheduler requires a zero-argument Void callback");goto cleanup;}
                NativeScriptMotionTarget target;
                if(!bank->has_motion_layout || !nativeScriptMoves_Target(&bank->motion_layout,&bank->tree,
                        &bank->tree.nodes[function->source_node],&target,diagnostic)){error(diagnostic,"Motion callback needs an installed owned layout");goto cleanup;}
                bool cue=!strcmp(event->name,"neutral_special_cue");
                for(unsigned t=0;t<target.count;++t)if(!motionTarget(candidate,target.motions[t],target.phase,i,cue,diagnostic))goto cleanup;
                continue;
            }
            if(event->kind!=NativeScriptEvent_FighterHook){error(diagnostic,"Callback scheduler has not migrated this event class");goto cleanup;}
            unsigned hook=0;
            while(hook<NativeScriptFighter_HookCount &&
                  (!nativeScriptFighterHookContracts[hook].name ||
                   strcmp(event->name,nativeScriptFighterHookContracts[hook].name)))++hook;
            if(hook==NativeScriptFighter_HookCount) {
                if(event->argument_count || event->result_type!=NativeScriptType_Void)
                    {
                        if(diagnostic) {
                            diagnostic->source=function->name;
                            snprintf(diagnostic->message,sizeof(diagnostic->message),
                                "Fighter hook '%s' has no migrated scheduler for its argument/return contract",event->name);
                        }
                        goto cleanup;
                    }
                static const char* const names[NativeScriptFighterCallback_Count]={
                    "damage","death1","death2","death2","pre_hitlag","post_hitlag","deal_damage","state_change",
                    "reflect_hit","reflect","hurtbox_detect","shield_hit",NULL,NULL,"secondary_damage"};
                bool registered=false;
                for(unsigned slot=0;slot<NativeScriptFighterCallback_Count;++slot) {
                    if(!names[slot] || strcmp(event->name,names[slot]))continue;
                    if(candidate->callbacks[slot]!=UINT32_MAX){error(diagnostic,"Duplicate interpreted instance event");goto cleanup;}
                    candidate->callbacks[slot]=i;registered=true;
                }
                if(!registered){
                    if(diagnostic){diagnostic->source=function->name;
                        snprintf(diagnostic->message,sizeof(diagnostic->message),
                            "Fighter hook '%s' has no migrated callback scheduler",event->name);}
                    goto cleanup;
                }
                continue; // Named event definitions require explicit registration.
            }
            const NativeScriptFighterHookContract* contract=&nativeScriptFighterHookContracts[hook];
            if(event->argument_count!=contract->argument_count || event->result_type!=contract->result_type ||
               (event->record?event->record->native_bytes:0)!=contract->record_bytes ||
               function->parameter_count!=contract->argument_count || function->result_type!=contract->result_type) {
                error(diagnostic,"Interpreted fighter event differs from its installed scheduler ABI");goto cleanup;
            }
            for(unsigned argument=0;argument<contract->argument_count;++argument)
                if(event->arguments[argument].type!=contract->arguments[argument] ||
                   bank->declarations.parameters[function->parameter_first+argument].type!=contract->arguments[argument]) {
                    error(diagnostic,"Interpreted fighter event argument differs from its installed scheduler ABI");goto cleanup;
                }
            if(candidate->hooks[hook]!=UINT32_MAX){error(diagnostic,"Duplicate interpreted fighter hook");goto cleanup;}
            candidate->hooks[hook]=i;
        }
    }
    if(bank) {
        uint64_t identity=UINT64_C(1469598103934665603);
        for(uint32_t i=0;i<bank->code.source_bytes;++i){identity^=(uint8_t)bank->code.source[i];identity*=UINT64_C(1099511628211);}
        const uint8_t* code=(const uint8_t*)bank->code.instructions;
        for(size_t i=0;i<(size_t)bank->code.instruction_count*sizeof(*bank->code.instructions);++i){identity^=code[i];identity*=UINT64_C(1099511628211);}
        identity^=bank->storage_layout.identity;identity*=UINT64_C(1099511628211);
        identity^=bank->storage_parameters.identity;candidate->identity=identity?identity:1;
        NativeScriptItemMetadataContext metadata=item_metadata?*item_metadata:(NativeScriptItemMetadataContext){0};
        metadata.motions=bank->has_motion_layout?&bank->motion_layout:NULL;
        for(uint32_t node=0;node<bank->tree.count;++node) {
            unsigned type=bank->tree.nodes[node].kind;
            if(type>=NativeScriptSource_If && type<=NativeScriptSource_Pass)continue;
            if(nativeScriptEffectMetadata_Owns(&bank->tree,node)) {
                if(!nativeScriptEffectMetadata_Validate(&bank->tree,node,diagnostic))goto cleanup;
            } else if(nativeScriptPoseMetadata_Owns(&bank->tree,node)) {
                if(!nativeScriptPoseMetadata_Validate(bank,&bank->tree,node,diagnostic))goto cleanup;
            } else if(nativeScriptHitboxMetadata_Owns(&bank->tree,node)) {
                if(!nativeScriptHitboxMetadata_Validate(bank,&bank->tree,node,diagnostic))goto cleanup;
            } else if(!nativeScriptItemMetadata_Validate(&metadata,&bank->tree,node,diagnostic))goto cleanup;
        }
        if(!nativeScriptItemPrograms_Prepare(bank,kind,candidate->identity,&metadata,&candidate->items,diagnostic) ||
           !nativeScriptEffectPrograms_Prepare(bank,kind,candidate->identity,&candidate->effects,diagnostic))goto cleanup;
    }
    *output=*candidate;nativeScriptItemPrograms_Rebind(&output->items,&candidate->items);accepted=true;
cleanup:
    free(candidate);return accepted;
}

bool nativeScriptFighter_CanPublish(FighterKind kind)
{
    return (uint32_t)kind<Ft_Kind_RuntimeCapacity && kind!=Ft_Kind_None && nativeScriptRuntime_Idle() &&
        !nativeScriptFighter_HasActors(kind) && !nativeScriptItem_HasActors(kind) && !nativeScriptEffect_HasActors(kind);
}
void nativeScriptFighter_PublishProgram(FighterKind kind,const NativeScriptFighterProgram* prepared)
{
    programs[kind]=*prepared;nativeScriptItemPrograms_Rebind(&programs[kind].items,&prepared->items);
}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program(author);
    if(!program || !owner)return NULL;
    for(unsigned i=0;i<program->items.count;++i)if(program->items.programs[i].owner==owner)return &program->items.programs[i];
    return NULL;
}
const NativeScriptEffectProgram* nativeScriptEffect_Program(FighterKind author,uint32_t owner)
{
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program(author);
    if(!program || !owner)return NULL;
    for(unsigned i=0;i<program->effects.count;++i)if(program->effects.programs[i].owner==owner)return &program->effects.programs[i];
    return NULL;
}
