#include "engine_fixture.h"
#include "../script_kinetics/engine_fixture.h"
#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/ftdata.h>
#include <melee/mp/forward.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {SourceBytes=32768,Nodes=1024,IRNodes=4096,Functions=64,Parameters=128,Fields=64,Instructions=8192};
typedef struct BankBuffers {
    char source[SourceBytes];NativeScriptSourceNode nodes[Nodes];NativeScriptIRNode ir[IRNodes];uint32_t args[IRNodes];
    NativeScriptFunctionDeclaration declarations[Functions];NativeScriptParameterDeclaration params[Parameters];
    NativeScriptValueDeclaration values[Fields];NativeScriptLocal locals[128];
    NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];
    NativeScriptExecutionFunction entries[Functions];NativeScriptCompileFunctionScratch graph[Functions];
    NativeScriptStorageField fields[Fields];NativeScriptStorageOwner owners[16];NativeScriptParameterSlot slots[Fields];
} BankBuffers;
static BankBuffers buffers[2];static NativeScriptCompileBank banks[2];static const NativeScriptCompileBank* accepted;
static NativeScriptDiagnostic diagnostic;static unsigned checks,runs;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"line %u: %s (%s; %s)\n",__LINE__,#v,diagnostic.message,vm.diagnostic.message);exit(1);}}while(0)
HSD_GObj** HSD_GObjPLinkHead;static HSD_GObj* heads[32];static HSD_GObj object;static Fighter fighter;
static uint8_t state_bytes[64];static NativeScriptStorageInstance storage;
static NativeScriptValue registers[2048];static NativeScriptVMFrame frames[32];
static NativeScriptVM vm={.registers=registers,.register_capacity=2048,.frames=frames,.frame_capacity=32};
static NativeScriptEventPayloadFrame payload_frames[16];
static NativeScriptEventPayloadPool payloads={.frames=payload_frames,.capacity=16};static NativeScriptExecution execution;
static char input[SourceBytes];
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}

static NativeScriptMoveLayout layout(void)
{
    return (NativeScriptMoveLayout){.common_count=ftCo_MS_Count,.first=341,.count=8,
        .ground={341,343,345,347},.air={342,344,346,348},
        .transition_flags={UINT32_C(0x80004000),UINT32_C(0x82000000),0,0}};
}
static void setup_bank(unsigned at)
{
    BankBuffers* b=&buffers[at];
    banks[at]=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=SourceBytes,
        .tree={.nodes=b->nodes,.capacity=Nodes},.expressions={.nodes=b->ir,.node_capacity=IRNodes,.arguments=b->args,.argument_capacity=IRNodes},
        .declarations={.functions=b->declarations,.function_capacity=Functions,.parameters=b->params,.parameter_capacity=Parameters,
            .values=b->values,.value_capacity=Fields},.locals=b->locals,.local_capacity=128,
        .instructions=b->instructions,.instruction_capacity=Instructions,.function_code=b->code,.function_code_capacity=Functions,
        .execution_functions=b->entries,.execution_function_capacity=Functions,.graph=b->graph,.graph_capacity=Functions,
        .storage_layout={.fields=b->fields,.field_capacity=Fields,.owners=b->owners,.owner_capacity=16},
        .parameter_slots=b->slots,.parameter_slot_capacity=Fields};
}
static uint64_t hash(const void* source,size_t count)
{
    const uint8_t* bytes=source;uint64_t value=UINT64_C(1469598103934665603);
    for(size_t i=0;i<count;++i){value^=bytes[i];value*=UINT64_C(1099511628211);}return value;
}
static uint32_t function(const char* parent,const char* name)
{
    const NativeScriptProgramIndex* index=&accepted->declarations;
    for(uint32_t i=0;i<index->function_count;++i) {
        const NativeScriptSourceNode* node=&index->tree->nodes[index->functions[i].source_node];
        if(nativeScriptSource_SpanEquals(index->tree,node->name,name) && node->parent<index->tree->count &&
            nativeScriptSource_SpanEquals(index->tree,index->tree->nodes[node->parent].name,parent))return i;
    }
    CHECK(false);return UINT32_MAX;
}
static int64_t completed(void)
{
    NativeScriptValue value;CHECK(nativeScriptStorage_Read(&storage,NATIVE_SCRIPT_STATE_SEMANTIC|1,NULL,0,&value,&diagnostic));
    CHECK(value.type==NativeScriptType_I32);return value.as.i64;
}
static void setup_actor(void)
{
    scriptKineticsFixture_Reset();scriptStateFixture_Reset();
    memset(&object,0,sizeof(object));memset(&fighter,0,sizeof(fighter));memset(heads,0,sizeof(heads));
    HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;object.classifier=HSD_GOBJ_CLASS_FIGHTER;object.user_data=&fighter;fighter.gobj=&object;
    fighter.kind=Ft_Kind_CustomFirst;fighter.behavior_kind=Ft_Kind_CustomFirst;fighter.x8_spawnNum=17;
    fighter.motion_id=341;fighter.cur_anim_frame=17.25f;fighter.frame_speed_mul=3;fighter.ground_or_air=GA_Ground;
    fighter.gr_vel=5;fighter.fall_fast=true;
    fighter.self_vel.x=3;fighter.co_attrs.ground_max_horizontal_velocity=1.5f;
    fighter.cur_pos.z=9;fighter.x98_atk_shield_kb.z=11;fighter.x74_self_accel.y=7;
    fighter.x1968_jumpsUsed=2;fighter.x1969_walljumpUsed=2;fighter.ecb_lock=3;
    fighter.coll_data.x130_flags|=CollData_X130_Locked;
    NativeScriptCompileBank* selected=scriptKineticsFixture_OwnSource(&fighter,accepted,&storage);
    CHECK(nativeCallbackSource_Alive(&object,17,NativeCallbackSource_Fighter));
    CHECK(nativeScriptStorage_Create(&selected->storage_layout,&selected->storage_parameters,0,17,17,state_bytes,64,&storage,&diagnostic));
    execution=(NativeScriptExecution){.declarations=&selected->declarations,.code=&selected->code,.storage=&storage,
        .vm=&vm,.payloads=&payloads,.functions=accepted->execution_functions,.function_count=accepted->declarations.function_count};
    CHECK(nativeScriptExecution_Bind(&execution,&diagnostic));CHECK(completed()==0);
}
static NativeScriptVMStatus run(const char* parent,const char* event,NativeScriptValue* result)
{
    ++runs;return nativeScriptExecution_Run(&execution,&object,NativeCallbackSource_Fighter,17,123, 0,
        function(parent,event),NULL,0, NULL,result);
}
static void successful(const char* parent,const char* event,uint32_t motion,uint32_t flags,float frame,float rate,
    unsigned startup,int plane)
{
    setup_actor();if(plane==GA_Ground)fighter.ground_or_air=GA_Air;
    NativeScriptValue result;CHECK(run(parent,event,&result)==NativeScriptVM_Ok);CHECK(result.type==NativeScriptType_Void);
    CHECK(script_state_changes==1);ScriptTransitionChange change=script_transition_changes[0];
    CHECK(change.motion==motion && change.flags==flags && change.frame==frame && change.rate==rate);
    CHECK(fighter.motion_id==(int)motion && fighter.cur_anim_frame==frame+(startup?1.25f:0) && fighter.frame_speed_mul==rate);
    CHECK(script_kinetic_stage_count==startup);CHECK(completed()==1);
    if(plane>=0)CHECK(fighter.ground_or_air==plane);
    if(plane==GA_Air) {
        CHECK(fighter.gr_vel==0 && fighter.cur_pos.z==0 && fighter.x98_atk_shield_kb.z==0 && fighter.x74_self_accel.y==0);
        CHECK(fighter.x1968_jumpsUsed==1 && fighter.ecb_lock==10 && (fighter.coll_data.x130_flags&CollData_X130_Locked));
    } else if(plane==GA_Ground) {
        CHECK(fighter.gr_vel==3 && fighter.self_vel.x==3 && fighter.x1968_jumpsUsed==0 && fighter.x1969_walljumpUsed==0);
        CHECK(fighter.ecb_lock==0 && !(fighter.coll_data.x130_flags&CollData_X130_Locked));
    }
    CHECK(vm.depth==0 && vm.register_top==0 && payloads.depth==0);
}
static void retire_landing(HSD_GObj* actor)
{
    heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;actor->user_data=(void*)1;
}
static void retirements(void)
{
    const char* parents[]={"Neutral","Side","Hooks"};const char* events[]={"special_n","ground_collision","tick"};
    for(unsigned i=0;i<3;++i) {
        setup_actor();scriptStateFixture_RetireOnChange(true);
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(run(parents[i],events[i],&result)==NativeScriptVM_SourceRetired);
        CHECK(script_state_changes==1 && script_kinetic_stage_count==0);CHECK(completed()==0);
        CHECK(result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX);
        CHECK(vm.depth==0 && vm.register_top==0 && payloads.depth==0);
        CHECK(vm.diagnostic.message[0] && vm.diagnostic.source.line && vm.error_actor==17 && vm.error_frame==123);
        CHECK(!nativeCallbackSource_Alive(&object,17,NativeCallbackSource_Fighter));
    }
    for(unsigned stage=1;stage<=4;++stage) for(unsigned mode=0;mode<2;++mode) {
        setup_actor();scriptKineticsFixture_RetireAt(stage,mode);
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(run("Neutral","special_n",&result)==NativeScriptVM_SourceRetired);
        CHECK(script_state_changes==1 && script_kinetic_stage_count==stage);CHECK(completed()==0);
        CHECK(result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX);
        CHECK(vm.depth==0 && vm.register_top==0 && payloads.depth==0);
        CHECK(!nativeCallbackSource_Alive(&object,17,NativeCallbackSource_Fighter));
    }
    setup_actor();fighter.kind=Ft_Kind_CustomFirst;fighter.ground_or_air=GA_Air;
    ftData_OnLanding[Ft_Kind_CustomFirst]=retire_landing;
    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(run("Neutral","air_collision",&result)==NativeScriptVM_SourceRetired);
    CHECK(script_state_changes==0 && script_kinetic_stage_count==0 && completed()==0);
    CHECK(result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX);
    CHECK(vm.depth==0 && vm.register_top==0 && payloads.depth==0);
    CHECK(strstr(vm.diagnostic.message,"retired")!=NULL && vm.diagnostic.source.line);
    ftData_OnLanding[Ft_Kind_CustomFirst]=NULL;
}
static void rejected(const char* source,const NativeScriptMoveLayout* motions,const char* message)
{
    uint64_t old=hash(&buffers[0],sizeof(buffers[0])),old_bank=hash(&banks[0],sizeof(banks[0]));
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=motions};
    CHECK(!nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));CHECK(accepted==&banks[0] && accepted->valid);
    CHECK(hash(&buffers[0],sizeof(buffers[0]))==old && hash(&banks[0],sizeof(banks[0]))==old_bank);
    if(!strstr(diagnostic.message,message))fprintf(stderr,"wanted %s; got %s\n",message,diagnostic.message);
    CHECK(strstr(diagnostic.message,message)!=NULL);CHECK(!banks[1].valid);
}
static void failures(void)
{
    NativeScriptMoveLayout motions=layout();
    rejected(input,NULL,"Unsupported");
    NativeScriptMoveLayout missing=motions;missing.ground[0]=0;
    rejected(input,&missing,"absent");
    const char* helper="fighter:\n    behavior independent\nfunc helper():\n    state.to_air()\n";
    rejected(helper,&motions,"helpers can use");
    const char* hook="fighter:\n    behavior independent\naction Hooks:\n    on tick:\n        state.to_ground()\n";
    rejected(hook,&motions,"helpers can use");
    const char* absent="fighter:\n    behavior independent\nmove Missing:\n    on ground_collision:\n        state.to_air()\n";
    rejected(absent,&motions,"require a special entry");
    const char* conflicting="fighter:\n    behavior independent\nmove Conflicting:\n    on special_n:\n        pass\n    on special_s:\n        pass\n    on ground_collision:\n        state.to_air()\n";
    rejected(conflicting,&motions,"two different");
    const char* unknown_hook="fighter:\n    behavior independent\naction Hooks:\n    on no_such_hook:\n        pass\n";
    rejected(unknown_hook,&motions,"Unknown event");
    const char* outside="fighter:\n    behavior independent\nparameter uint flags = 0\nstate int completed\naction Hooks:\n    on tick:\n        state.enter(349)\n";
    // Numeric entry is checked against the selected owned source table at invocation.
    NativeScriptCompileRequest request={.source=outside,.source_bytes=(uint32_t)strlen(outside),.motion_layout=&motions};
    const NativeScriptCompileBank* other=NULL;CHECK(nativeScriptCompile_Build(&banks[1],&request,&other,&diagnostic));
    CHECK(accepted==&banks[0]);
    setup_actor();const NativeScriptCompileBank* old=accepted;accepted=other;setup_actor();
    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(run("Hooks","tick",&result)==NativeScriptVM_Error);CHECK(script_state_changes==0);
    CHECK(result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX);CHECK(strstr(vm.diagnostic.message,"actual table")!=NULL);
    accepted=old;
    // The accepted bank remains executable after all rejected candidates.
    successful("Neutral","ground_collision",342,UINT32_C(0x80004000),17.25f,1,0,GA_Air);
}
int main(int argc,char** argv)
{
    const char* path=argc==2?argv[1]:"tests/native/script_transition/probe.fms";
    FILE* file=fopen(path,"rb");CHECK(file!=NULL);size_t bytes=fread(input,1,sizeof(input)-1,file);CHECK(feof(file));fclose(file);input[bytes]=0;
    setup_bank(0);setup_bank(1);NativeScriptMoveLayout motions=layout();
    NativeScriptCompileRequest request={.source=input,.source_bytes=(uint32_t)bytes,.motion_layout=&motions};
    if(!nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic)){fprintf(stderr,"compile line %u: %s\n",diagnostic.source.line,diagnostic.message);return 1;}
    CHECK(accepted==&banks[0] && accepted->valid && accepted->has_motion_layout);CHECK(accepted->declarations.function_count==14);
    uint64_t immutable=hash(&buffers[0],sizeof(buffers[0]));
    successful("Neutral","special_n",341,0,0,1,4,-1);
    successful("Neutral","special_air_n",342,0,0,1,4,-1);
    successful("Neutral","ground_collision",342,UINT32_C(0x80004000),17.25f,1,0,GA_Air);
    successful("Neutral","air_collision",341,UINT32_C(0x80004000),17.25f,1,0,GA_Ground);
    successful("Side","special_s",343,0,1,1,0,-1);
    successful("Side","ground_physics",345,0,7.125f,1,0,-1);
    successful("Side","air_physics",346,0,8.25f,0.5f,0,-1);
    successful("Side","ground_collision",346,UINT32_C(0x80004000),17.25f,1,0,GA_Air);
    successful("Side","air_collision",345,UINT32_C(0x80004000),17.25f,1,0,GA_Ground);
    successful("Side","ground_animation",344,UINT32_C(0x80004000),0,1,0,-1);
    successful("Side","air_animation",343,UINT32_C(0x80004000),10.5f,1,0,-1);
    successful("Hooks","tick",341,0,0,1,4,-1);
    retirements();failures();CHECK(hash(&buffers[0],sizeof(buffers[0]))==immutable);
    printf("{\"result\":\"pass\",\"checks\":%u,\"execution_runs\":%u,\"functions\":14,\"engine_motion_boundary_controlled\":true}\n",checks,runs);return 0;
}
