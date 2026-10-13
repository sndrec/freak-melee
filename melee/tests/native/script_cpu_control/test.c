#include <melee/ft/native_script_binding_cpu_control.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/fighter.h>
#include <melee/ft/types.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/ft_3C61.h>
#include <melee/gr/ground.h>
#include <melee/gr/types.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/debug.h>
#include <placeholder.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,queue_pairs,selector_pairs;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"CPU %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
NativeMatchPool nativeGObjPool;static NativeMatchPoolSlot slots[8];static uint32_t order[8];static _Alignas(32) uint8_t payload[8*256];
static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;static HSD_GObj* objects[4];static NativeMatchPoolReference refs[4];static Fighter fighters[4];
static NativeSourcePackage package;static MotionState motions[1];static NativeScriptStorageInstance storage;static NativeScriptRuntimeContext context;static bool view_valid=true, view_faulted;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static bool ally[4],unavailable[4],disabled[4];static unsigned retire;
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind==Ft_Kind_CustomFirst?&package:NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){return kind==Ft_Kind_CustomFirst?&package.program:NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* g){return g==objects[0]&&view_valid?(view_faulted?NativeScriptVM_Error:NativeScriptVM_Ok):NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* g,uint32_t gen,NativeScriptReceiverView* out){if(!view_valid||g!=objects[0]||gen!=17)return false;*out=(NativeScriptReceiverView){&package.compiler.bank.declarations,&storage};return true;}
bool nativeScriptFighter_IsAttached(Fighter_GObj* g){return g==objects[0];}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return NULL;}
static unsigned index_of(Fighter* f){for(unsigned i=0;i<4;++i)if(f==&fighters[i])return i;abort();}
float Stage_GetBlastZoneLeftOffset(void){return -100;}float Stage_GetBlastZoneRightOffset(void){return 100;}float Stage_GetBlastZoneBottomOffset(void){return -100;}float Stage_GetBlastZoneTopOffset(void){return 100;}
bool ftCo_IsAlly_dontinline(Fighter* a,Fighter* b)
{
    switch (retire) {
    case 0: break;
    case 1: storage.live=false; break;
    case 2: ++b->x8_spawnNum; break;
    case 3: a->kind=Ft_Kind_Mario; break;
    case 4: a->gobj=NULL; break;
    case 5: a->ft_data=NULL; break;
    case 6: storage.parameters=NULL; break;
    case 7: ++storage.generation; break;
    case 8: view_valid=false; break;
    case 9: CHECK(nativeMatchPool_Free(&nativeGObjPool,refs[0])); break;
    case 10: view_faulted=true; break;
    case 11: ++package.program.identity; break;
    case 12: storage.layout=NULL; break;
    case 13: objects[0]->user_data=&fighters[1]; break;
    default: abort();
    }
    retire=0;
    return ally[index_of(b)];
}
bool ftCo_800A1C44(Fighter* f){return unavailable[index_of(f)];}
bool ftCo_800A0F00(Fighter_GObj* g){return disabled[index_of(g->user_data)];}
bool ftLib_8008732C(Fighter_GObj* g){(void)g;return false;}
void OSReport(char* format,...){(void)format;abort();}ATTRIBUTE_NORETURN void __assert(char* file,u32 line,char* msg){fprintf(stderr,"native assertion %s:%lu %s",file,line,msg);abort();}
#include "native.inc"
StageInfo stage_info;ftCo_803C6594_t* ftCo_803C6594[Gr_Kind_Count];
static void ftCo_800A1CC4(Fighter* f,ftCo_803C6594_t* entry){(void)f;(void)entry;}
#include "destination.inc"
#include "history_lookup.inc"
// Only the registry-admission cases permit this controlled leaf. It records
// entry/retirement; it supplies no CPU decision or planner-parity evidence.
static bool registry_probe;
static unsigned registry_kernel_calls;
static Item* registry_probe_item;
static bool registry_retire_payload;
void ftCo_CpuPrepareCombatPlan(Fighter* f){(void)f;if(!registry_probe)abort();++registry_kernel_calls;if(registry_retire_payload)registry_probe_item->entity=NULL;}
void ftCo_CpuUpdateRecoveryDestination(Fighter* f){(void)f;abort();}
bool ftCo_CpuPlanAttack(Fighter* f){(void)f;abort();}
void ftCo_CpuPlanWithoutTarget(Fighter* f){(void)f;abort();}
static void setup(void)
{
    memset(&nativeGObjPool,0,sizeof(nativeGObjPool));memset(heads,0,sizeof(heads));memset(fighters,0,sizeof(fighters));memset(&package,0,sizeof(package));memset(ally,0,sizeof(ally));memset(unavailable,0,sizeof(unavailable));memset(disabled,0,sizeof(disabled));retire=0;view_valid=true;view_faulted=false;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,payload,sizeof(payload),slots,order,8,sizeof(HSD_GObj),32,UINT64_C(0x100000002)));
    for(unsigned i=0;i<4;++i){objects[i]=nativeMatchPool_Allocate(&nativeGObjPool,&refs[i]);CHECK(objects[i]);memset(objects[i],0,sizeof(HSD_GObj));objects[i]->classifier=HSD_GOBJ_CLASS_FIGHTER;objects[i]->user_data=&fighters[i];objects[i]->next=NULL;fighters[i].gobj=objects[i];fighters[i].x8_spawnNum=17+i;fighters[i].kind=i?Ft_Kind_Mario:Ft_Kind_CustomFirst;fighters[i].behavior_kind=fighters[i].kind;fighters[i].cpu.write_pos=fighters[i].cpu.buffer;if(i)objects[i-1]->next=objects[i];}
    heads[HSD_GOBJ_PLINK_FIGHTER]=objects[0];package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.motions=motions;fighters[0].ft_data=&package.assets.character.data;gFtDataList[Ft_Kind_CustomFirst]=fighters[0].ft_data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
    package.program.bank=&package.compiler.bank;package.program.identity=88;package.compiler.bank.valid=package.compiler.bank.has_motion_layout=true;package.compiler.bank.motion_layout=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};package.assets.facts.moves=package.compiler.bank.motion_layout;package.compiler.bank.storage_layout.identity=55;package.compiler.bank.storage_layout.program=&package.compiler.bank.declarations;package.compiler.bank.storage_parameters.identity=66;storage=(NativeScriptStorageInstance){.layout=&package.compiler.bank.storage_layout,.parameters=&package.compiler.bank.storage_parameters,.generation=17,.identity=17,.owner=0,.live=true};context=(NativeScriptRuntimeContext){.source=objects[0],.source_class=NativeCallbackSource_Fighter,.generation=17,.scene_epoch=refs[0].lifetime};
}
static void queues(void)
{
    const double values[]={-257,-128,-127,-1,-.9,0,.9,1,127,255,256,513};unsigned recipes=0;
    for(unsigned k=0;k<nativeScriptConstants_Count();++k){NativeScriptConstant command;CHECK(nativeScriptConstants_At(k,&command));if(command.family!=NativeScriptConstant_CPUCommand)continue;++recipes;int operands=nativeScriptCPUControl_CommandOperands((uint32_t)command.bits);CHECK(operands>=0);
        for(unsigned v=0;v<sizeof(values)/sizeof(*values);++v){setup();NativeScriptValue args[3]={{.type=NativeScriptType_I32,.as.i64=(int64_t)command.bits},{.type=NativeScriptType_F64,.as.f64=values[v]},{.type=NativeScriptType_I64,.as.i64=-1}},out;Fighter reference={0};reference.cpu.write_pos=reference.cpu.buffer;
            uint8_t a=(uint8_t)(int32_t)values[v];if(operands==0)ftCo_800B463C(&reference,(u8)command.bits);else if(operands==1)ftCo_800B46B8(&reference,(u8)command.bits,a);else ftCo_800B4778(&reference,(u8)command.bits,a,255);
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_CPUQueue+(unsigned)operands,NULL,0,args,(unsigned)operands+1,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void);CHECK(!memcmp(fighters[0].cpu.buffer,reference.cpu.buffer,sizeof(reference.cpu.buffer)));CHECK(fighters[0].cpu.write_pos-fighters[0].cpu.buffer==operands+1);++queue_pairs;}}
    CHECK(recipes==51);setup();NativeScriptValue args[3]={{.type=NativeScriptType_I32,.as.i64=CpuCmd_LstickForwardClamped},{.type=NativeScriptType_I32,.as.i64=-127},{.type=NativeScriptType_I32,.as.i64=127}},out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};fighters[0].cpu.write_pos=fighters[0].cpu.buffer+254;s8 before[256];memcpy(before,fighters[0].cpu.buffer,256);CHECK(!nativeScriptBindingCPUControl_Invoke(&context,NativeScriptBinding_CPUQueuePair,args,3,&out,&diagnostic));CHECK(!memcmp(before,fighters[0].cpu.buffer,256)&&fighters[0].cpu.write_pos==fighters[0].cpu.buffer+254&&out.as.u64==UINT64_MAX);
    fighters[0].cpu.write_pos=NULL;CHECK(!nativeScriptBindingCPUControl_Invoke(&context,NativeScriptBinding_CPUQueuePair,args,3,&out,&diagnostic));setup();args[1]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=NAN};CHECK(!nativeScriptBindingCPUControl_Invoke(&context,NativeScriptBinding_CPUQueuePair,args,3,&out,&diagnostic));CHECK(fighters[0].cpu.write_pos==fighters[0].cpu.buffer);args[1].as.f64=1e30;CHECK(!nativeScriptBindingCPUControl_Invoke(&context,NativeScriptBinding_CPUQueuePair,args,3,&out,&diagnostic));
}
static void selectors(void)
{
    const float points[]={0,1,-1,NAN,INFINITY};for(unsigned mask=0;mask<16;++mask)for(unsigned p=0;p<5;++p){setup();fighters[1].cur_pos=(Vec3){1,0,0};fighters[2].cur_pos=(Vec3){-1,0,0};fighters[3].cur_pos=(Vec3){200,0,0};ally[1]=(mask&1)!=0;unavailable[2]=(mask&2)!=0;disabled[1]=(mask&4)!=0;fighters[2].x221F_b3=(mask&8)!=0;Vec3 point={points[p],0,0};uint32_t handle=0;bool found=ftCo_CpuNearestEligibleTarget(objects[0],&point,&handle);NativeScriptValue args[3]={{.type=NativeScriptType_F32,.as.f32=point.x},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32}},out;CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_CPUNearestEnemyAt,NULL,0,args,3,&out,&diagnostic));CHECK(out.type==NativeScriptType_U32&&out.as.u64==(found?handle:0));++selector_pairs;}
    setup();fighters[1].cur_pos.x=1;fighters[2].cur_pos.x=-1;fighters[3].cur_pos.x=200;NativeScriptValue args[3]={{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32}},out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(nativeScriptBindingCPUControl_Invoke(&context,NativeScriptBinding_CPUNearestEnemyAt,args,3,&out,&diagnostic));CHECK(out.as.u64==18);setup();objects[3]->next=objects[1];CHECK(!nativeScriptBindingCPUControl_Invoke(&context,NativeScriptBinding_CPUNearestEnemyAt,args,3,&out,&diagnostic));setup();objects[1]->classifier=0;CHECK(!nativeScriptBindingCPUControl_Invoke(&context,NativeScriptBinding_CPUNearestEnemyAt,args,3,&out,&diagnostic));setup();retire=1;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!nativeScriptBindingCPUControl_Invoke(&context,NativeScriptBinding_CPUNearestEnemyAt,args,3,&out,&diagnostic));CHECK(out.as.u64==UINT64_MAX&&out.type==NativeScriptType_U64);
}
static void vm_case(const char* text,bool accepted)
{
    char source[512];snprintf(source,sizeof(source),"func Sample():\n    %s\n",text);NativeScriptSourceNode syntax[32];NativeScriptIRNode nodes[128];uint32_t args[128];NativeScriptLocal locals[4];NativeScriptSourceTree tree={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=syntax,.capacity=32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptIRArena arena={.source=source,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};NativeScriptBindingResolverContext bindings={NativeScriptBindings_Fighter};NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&bindings,.locals=locals,.local_capacity=4,.return_type=NativeScriptType_Void};CHECK(nativeScriptStatement_Compile(&body,1,&diagnostic)==accepted);if(!accepted)return;
    NativeScriptInstruction ops[128];NativeScriptFunctionCode functions[1];NativeScriptBytecodeBuilder builder={.instructions=ops,.instruction_capacity=128,.functions=functions,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&bindings};CHECK(nativeScriptBytecode_Compile(&builder,&body,1,0,0,&diagnostic));NativeScriptBytecodeProgram program={source,tree.source_bytes,ops,builder.instruction_count,functions,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[64],out;NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=4};setup();CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&out)==NativeScriptVM_Ok);
}
#include "registry_cases.h"
#include "scalar_admission_cases.h"
int main(void)
{
    queues();selectors();setup();NativeScriptValue out;fighters[0].cpu.x18=-77;fighters[0].cpu.x1C=999;fighters[0].cpu.x54=(Vec2){NAN,-0.0f};CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_CPUBehavior,NULL,0,&out,&diagnostic)&&out.as.i64==-77);CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_CPUDestinationX,NULL,0,&out,&diagnostic)&&isnan(out.as.f32));NativeScriptValue yes={.type=NativeScriptType_Bool,.as.u64=1};CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_CPUPendingAction,NULL,0,&yes,&diagnostic)&&fighters[0].cpu.xF8_b5);CHECK(!nativeScriptBindings_Write(&context,NativeScriptBinding_CPUPreviousBehavior,NULL,0,&yes,&diagnostic));view_valid=false;CHECK(!nativeScriptBindingCPUControl_Read(&context,NativeScriptBinding_CPUBehavior,&out,&diagnostic));
    vm_case("cpu.queue(Cpu.stick_y,-127)\n    cpu.queue(Cpu.wait,1)\n    cpu.queue(Cpu.done)",true);CHECK((uint8_t)fighters[0].cpu.buffer[1]==129);vm_case("cpu.pending_action = false\n    cpu.behavior = cpu.previous_behavior\n    cpu.clear_target()",true);vm_case("let uint target = cpu.nearest_enemy_at(0,0,0)",true);vm_case("cpu.queue(Cpu.wait)",false);vm_case("cpu.queue(126)",false);vm_case("let int command = Cpu.done\n    cpu.queue(command)",false);vm_case("cpu.queue(127.0)",false);
    registry_cases();
    scalar_admission_cases();
    cpu_continuation_cases();
    printf("{\"status\":\"PASS native CPU queue/selector/properties/VM\",\"checks\":%u,\"queue_pairs\":%u,\"selector_pairs\":%u,\"game_claim\":false}\n",checks,queue_pairs,selector_pairs);return 0;
}
