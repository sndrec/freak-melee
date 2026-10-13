#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage cpu_package;
#define bank cpu_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Fighter fighters[64],alternate;static unsigned count,checks;static NativeScriptDiagnostic diagnostic;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned mutation_mode,write_count,mode_tails,output_tails,queue_count,tick_order;HSD_GObj* mutation_actor,*replacement;HSD_GObj*(*reuse_actor)(void);float random_sample;bool item_attack;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"CPU scheduler %u %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
void ftCo_800A0CB0(Fighter*);bool ftCo_IsGrabbing(Fighter*);bool ftCo_800B630C(Fighter*);void ftCo_800AA320(Fighter*,int*,int*);void ftCo_800B3900(Fighter_GObj*);void cpu_initialize_tail(Fighter*);void cpu_recovery_dispatch(Fighter*);
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate bool handled\nstate int step_value\nstate int limit_value\nstate int trace\naction CPU:\n    on cpu_initialize:\n        state.trace = state.trace + 1\n        position.x = 1\n    on cpu_spacing_update:\n        state.trace = state.trace + 10\n        position.x = event.value\n    on cpu_stick_step:\n        position.x = 2\n        return state.step_value\n    on cpu_stick_limit:\n        position.x = 3\n        return state.limit_value\n    on cpu_grab_motion:\n        position.x = 4\n        return state.handled\n    on cpu_attack_motion:\n        position.x = 5\n        return state.handled\n    on cpu_mode_update:\n        state.trace = state.trace + 100\n        position.x = 6\n        return state.handled\n    on cpu_output:\n        state.trace = state.trace + 1000\n        position.x = 7\n        return state.handled\n    on cpu_recovery:\n        state.trace = state.trace + 10000\n        position.x = 8\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,NULL,&cpu_package.program,&diagnostic));cpu_package.address=&cpu_package;cpu_package.target=Ft_Kind_CustomFirst;cpu_package.ready=cpu_package.assets.ready=true;cpu_package.assets.facts.moves=layout;cpu_package.assets.facts.joints=4;cpu_package.assets.parts.parts_num=4;tables[Ft_Kind_CustomFirst]=&cpu_package.assets.parts;gFtDataList[Ft_Kind_CustomFirst]=&cpu_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=cpu_package.assets.motions;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&cpu_package.program);
}
static void removed(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static HSD_GObj* create(void){CHECK(count<64);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[count++];*f=(Fighter){.gobj=o,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,.x8_spawnNum=10+count,.ft_data=&cpu_package.assets.character.data};f->cpu.level=5;GObj_InitUserData(o,4,removed,f);CHECK(nativeScriptFighter_Attach(o));return o;}
static HSD_GObj* reuse(void){replacement=create();return replacement;}
static NativeScriptValue state(HSD_GObj* o,const char* name,const NativeScriptValue* input){NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));for(unsigned i=0;i<bank.declarations.value_count;++i)if(!values[i].owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name)){NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&result,&diagnostic));return result;}CHECK(false);return(NativeScriptValue){0};}
static void set(HSD_GObj* o,const char* name,int value){NativeScriptValue v={.type=!strcmp(name,"handled")?NativeScriptType_Bool:NativeScriptType_I32,.as.i64=value};state(o,name,&v);}
void cpu_mutate(HSD_GObj* o){if(!mutation_mode||((Fighter*)o->user_data)->cur_pos.x!=(mutation_mode==6?6:3))return;unsigned mode=mutation_mode;mutation_mode=0;if(mode==1)HSD_GObjFree(o);if(mode==2){HSD_GObjFree(o);if(reuse_actor)reuse_actor();}if(mode==3){alternate=*(Fighter*)o->user_data;o->user_data=&alternate;}if(mode==4)((Fighter*)o->user_data)->x8_spawnNum++;}
int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);HSD_GObj* o=create();Fighter* f=o->user_data;random_sample=0.5f;f->cpu.x7C=600;
 cpu_initialize_tail(f);CHECK(state(o,"trace",NULL).as.i64==1);ftCo_800A0CB0(f);CHECK(f->cur_pos.x==0.875f&&state(o,"trace",NULL).as.i64==11);cpu_recovery_dispatch(f);CHECK(state(o,"trace",NULL).as.i64==10011&&queue_count==0);
 set(o,"step_value",64);set(o,"limit_value",127);int step=-3,limit=-4;ftCo_800AA320(f,&step,&limit);CHECK(step==64&&limit==127);
 set(o,"step_value",999);set(o,"limit_value",-1);ftCo_800AA320(f,&step,&limit);CHECK(step==2&&limit==0x5C);
 f->motion_id=400;set(o,"handled",1);CHECK(ftCo_IsGrabbing(f)&&ftCo_800B630C(f));set(o,"handled",0);CHECK(!ftCo_IsGrabbing(f)&&!ftCo_800B630C(f));f->motion_id=ftCo_MS_CatchWait;CHECK(ftCo_IsGrabbing(f));f->motion_id=ftCo_MS_Attack11;CHECK(ftCo_800B630C(f));
 HSD_GObjFree(o);
 for(unsigned mode=1;mode<=5;++mode){o=create();f=o->user_data;set(o,"step_value",64);set(o,"limit_value",127);step=-3;limit=-4;mutation_mode=mode;reuse_actor=mode==2?reuse:NULL;NativeFighterLifetime frame;CHECK(nativeFighterLifetime_Capture(f,&frame));ftCo_800AA320(f,&step,&limit);CHECK(step==-3&&limit==-4&&!nativeFighterLifetime_Current(&frame));if(mode==2){CHECK(replacement==o);HSD_GObjFree(replacement);}else if(mode!=1)HSD_GObjFree(o);reuse_actor=NULL;}
 o=create();f=o->user_data;set(o,"handled",0);mutation_mode=6;tick_order=0;unsigned prior_output=output_tails;ftCo_800B3900(o);CHECK(tick_order==1&&output_tails==prior_output&&f->cpu.x7C==0&&nativeScriptFighter_Status(o)==NativeScriptVM_Error);HSD_GObjFree(o);
 o=create();f=o->user_data;set(o,"handled",0);tick_order=0;ftCo_800B3900(o);CHECK(tick_order==134&&mode_tails==1&&output_tails==1&&f->cpu.x7C==1);HSD_GObjFree(o);
 cpu_package.program.hooks[NativeScriptFighter_CpuInitialize]=UINT32_MAX;cpu_package.program.hooks[NativeScriptFighter_CpuSpacingUpdate]=UINT32_MAX;cpu_package.program.hooks[NativeScriptFighter_CpuRecovery]=UINT32_MAX;cpu_package.program.hooks[NativeScriptFighter_CpuAttackMotion]=UINT32_MAX;cpu_package.program.hooks[NativeScriptFighter_CpuGrabMotion]=UINT32_MAX;CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&cpu_package.program);
 o=create();f=o->user_data;cpu_initialize_tail(f);CHECK(f->cpu.x56C==3.5f);f->cpu.x7C=600;ftCo_800A0CB0(f);CHECK(f->cpu.x56C==3.5f);cpu_recovery_dispatch(f);CHECK(queue_count>0);f->motion_id=400;CHECK(ftCo_800B630C(f)&&!ftCo_IsGrabbing(f));HSD_GObjFree(o);
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS CPU scheduler\",\"checks\":%u}\n",checks);return 0;
}
