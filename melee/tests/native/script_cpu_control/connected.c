#include "fixture.h"
#include <melee/ft/native_script_binding_cpu_control.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/db/db.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftDonkey/forward.h>
#include <melee/ft/kinds/ftKoopa/forward.h>
#include <melee/ft/kinds/ftKirby/forward.h>
#include <melee/ft/native_cpu_history.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/mp/mplib.h>
#include <melee/it/inlines.h>
#include <placeholder.h>
#include <melee/ft/ft_3C61.h>
#include <melee/gr/ground.h>
#include <melee/gr/types.h>
#include <math.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum {N=1024,I=4096,F=32,V=64};
typedef struct CompileStorage {
 char source_bytes[8192];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
 NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
 NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
 NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} CompileStorage;
static CompileStorage compile_storage[2];
NativeSourcePackage cpu_packages[3];
#define primary_bank cpu_packages[0].compiler.bank
static Fighter fighters[32],substitute;static unsigned allocated,checks,cases,pairs,vm_calls,receiver_calls;
static Article article;static ItemAttr article_attributes;static ItemModelDesc article_model;static Item items[4];static unsigned item_count;
static MotionState motions[342];static FighterBone parts[MAX_FT_PARTS];static HSD_JObj joints[MAX_FT_PARTS];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"CPU actor %u case %u: %s (%s)\n",__LINE__,cases,#x,diagnostic.message);exit(1);}}while(0)
static void configure(unsigned index)
{
 CompileStorage* s=&compile_storage[index];
 cpu_packages[index].compiler.bank=(NativeScriptCompileBank){.source_storage=s->source_bytes,.source_capacity=sizeof(s->source_bytes),.tree={.nodes=s->nodes,.capacity=N},.expressions={.nodes=s->ir,.node_capacity=I,.arguments=s->arguments,.argument_capacity=I},.declarations={.functions=s->functions,.function_capacity=F,.parameters=s->parameters,.parameter_capacity=V,.values=s->values,.value_capacity=V},.locals=s->locals,.local_capacity=V,.instructions=s->instructions,.instruction_capacity=I,.function_code=s->codes,.function_code_capacity=F,.execution_functions=s->entries,.execution_function_capacity=F,.graph=s->graph,.graph_capacity=F,.storage_layout={.fields=s->fields,.field_capacity=V,.owners=s->owners,.owner_capacity=V},.parameter_slots=s->slots,.parameter_slot_capacity=V};
}
static const char* source=
 "fighter:\n    behavior independent\n"
 "state bool prepared\nstate float x\nstate float y\nstate float z\nstate float direction\nstate float distance\nstate float spacing\nstate int mode\nstate int level\nstate bool combat\nstate bool ranged\nstate int zero\nstate int input_id\nstate bool recorded\nstate bool applied\nstate bool posed\nstate bool attack\nstate bool grabbing\nstate bool grab_fault\nstate int left_x\nstate int left_y\nstate int left_trigger\nstate int right_trigger\nstate int buttons\n"
 "state int travel\nstate int selected\nstate int elapsed\nstate bool floor\nstate float width\n"
 "func ObservePlannerFields():\n    state.travel = cpu.travel_frames\n    state.selected = cpu.selected_command\n    state.elapsed = cpu.elapsed_frames\n    state.floor = cpu.floor_in_bounds\n    state.width = cpu.body_half_width\n"
 "func ApplyPlannerFields():\n    cpu.travel_frames = state.travel\n    cpu.selected_command = state.selected\n"
 "state int target_id\nfunc Target():\n    state.target_id = int(cpu.target_id)\n"
 "func Record():\n    state.recorded = cpu.record_input(state.input_id)\n"
 "func Pose():\n    state.posed = cpu.read_history_pose(state.x, state.y, state.z, state.direction)\n"
 "func ApplyHistory():\n    state.applied = cpu.apply_recorded_input()\n"
 "func Combat():\n    cpu.prepare_combat_plan()\n"
 "func Recovery():\n    cpu.update_recovery_destination()\n"
 "func Attack():\n    state.attack = cpu.plan_attack()\n"
 "func NoTarget():\n    cpu.plan_without_target()\n"
 "func ApplyOutputs():\n    cpu.output.left_x = state.left_x\n    cpu.output.left_y = state.left_y\n    cpu.output.left_trigger = state.left_trigger\n    cpu.output.right_trigger = state.right_trigger\n    cpu.output.buttons = state.buttons\n"
 "func ObserveOutputs():\n    state.left_x = cpu.output.left_x\n    state.left_y = cpu.output.left_y\n    state.left_trigger = cpu.output.left_trigger\n    state.right_trigger = cpu.output.right_trigger\n    state.buttons = cpu.output.buttons\n"
 "func Prepare():\n    state.prepared = cpu.prepare_destination_update()\n"
 "func Destination():\n    cpu.set_destination(state.x, state.y, state.distance)\n"
 "func Spacing():\n    cpu.set_approach_spacing(state.spacing)\n"
 "func Apply():\n    cpu.mode = state.mode\n    cpu.level = state.level\n    cpu.follow_distance = state.distance\n    cpu.combat_candidates_enabled = state.combat\n    cpu.ranged_gate_enabled = state.ranged\n"
 "func Observe():\n    state.x = cpu.destination.x\n    state.y = cpu.destination.y\n    state.spacing = cpu.approach_spacing\n    state.distance = cpu.follow_distance\n    state.mode = cpu.mode\n    state.level = cpu.level\n    state.combat = cpu.combat_candidates_enabled\n    state.ranged = cpu.ranged_gate_enabled\n"
 "func IntegerArguments():\n    cpu.set_destination(-3, 8, 5)\n    cpu.set_approach_spacing(-9)\n    cpu.follow_distance = 11\n"
 "func Fault():\n    state.level = 1 / state.zero\n"
 "action Placeholder:\n    on tick:\n        pass\n    on cpu_grab_motion:\n        if state.grab_fault:\n            state.level = 1 / state.zero\n        return state.grabbing\n"
 "article Probe states 1:\n    state int level\n    state float width\n    on signal ObserveWidth:\n        item.state.width = attachment_owner.cpu.body_half_width\n    on signal ObserveOwner:\n        item.state.level = attachment_owner.cpu.level\n    on signal WriteOwner:\n        attachment_owner.cpu.level = item.state.level\n    state 0 anim -1:\n";
static void prepare(void)
{
 configure(0);article=(Article){.x0_common_attr=&article_attributes,.x10_modelDesc=&article_model};
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&primary_bank,&request,&accepted,&diagnostic));CHECK(accepted==&primary_bank);
 NativeSourcePackage* p=&cpu_packages[0];CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&primary_bank,&layout,&metadata,&p->program,&diagnostic));
 p->address=p;p->ready=p->assets.ready=true;p->target=Ft_Kind_CustomFirst;p->assets.facts.moves=layout;p->assets.facts.joints=MAX_FT_PARTS;p->assets.parts.parts_num=MAX_FT_PARTS;p->assets.parts.part_to_joint=p->assets.part_to_joint;p->assets.parts.joint_to_part=p->assets.joint_to_part;p->assets.motions=motions;
 tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 for(unsigned n=0;n<MAX_FT_PARTS;++n)parts[n].joint=&joints[n];
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);
 configure(1);p=&cpu_packages[1];const char* foreign="fighter:\n    behavior independent\nstate float unrelated\naction Foreign:\n    on tick:\n        pass\n";
 request=(NativeScriptCompileRequest){.source=foreign,.source_bytes=(uint32_t)strlen(foreign),.motion_layout=&layout};
 CHECK(nativeScriptCompile_Build(&p->compiler.bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst+1,&p->compiler.bank,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->ready=p->assets.ready=true;p->target=Ft_Kind_CustomFirst+1;p->assets.facts.moves=layout;p->assets.facts.joints=MAX_FT_PARTS;p->assets.parts.parts_num=MAX_FT_PARTS;p->assets.motions=motions;
 tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);
}
static void remove_data(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static HSD_GObj* actor(int index)
{
 CHECK(allocated<32);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);Fighter* f=&fighters[allocated++];FighterKind kind=index>=0?Ft_Kind_CustomFirst+index:Ft_Kind_Mario;
 *f=(Fighter){.gobj=o,.kind=kind,.behavior_kind=kind,.x8_spawnNum=100+allocated,.ft_data=&cpu_packages[index>=0?index:0].assets.character.data,.parts=parts};
 GObj_InitUserData(o,4,remove_data,f);if(index>=0)CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool fighter=o->classifier==HSD_GOBJ_CLASS_FIGHTER;return(NativeScriptRuntimeContext){.source=o,.source_class=fighter?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=fighter?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};}
static uint32_t function(const char* name)
{for(unsigned n=0;n<primary_bank.declarations.function_count;++n)if(nativeScriptSource_SpanEquals(&primary_bank.tree,primary_bank.declarations.functions[n].name,name))return n;CHECK(false);return 0;}
static NativeScriptValue field(HSD_GObj* o,const char* name)
{
 NativeScriptRuntimeContext c=context(o);NativeScriptReceiverView view;CHECK(nativeScriptActors_Lookup(NULL,o,c.source_class,c.generation,&view));
 for(unsigned n=0;n<primary_bank.declarations.value_count;++n)if(primary_bank.declarations.values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&primary_bank.tree,primary_bank.declarations.values[n].name,name)){
  NativeScriptValue r;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&r,&diagnostic));return r;}
 CHECK(false);return(NativeScriptValue){0};
}
static bool helper(HSD_GObj* o,const char* name)
{
 NativeScriptValue r={.type=NativeScriptType_U64,.as.u64=99};++vm_calls;NativeScriptVMStatus s=nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,function(name),NULL,0,&r,&diagnostic);
 CHECK(s==NativeScriptVM_Ok?r.type==NativeScriptType_Void:r.type==NativeScriptType_U64&&r.as.u64==99);return s==NativeScriptVM_Ok;
}

static void set_field(HSD_GObj* o,const char* name,NativeScriptValue value)
{
 NativeScriptRuntimeContext c=context(o);NativeScriptReceiverView view;CHECK(nativeScriptActors_Lookup(NULL,o,c.source_class,c.generation,&view));
 for(unsigned n=0;n<primary_bank.declarations.value_count;++n)if(primary_bank.declarations.values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&primary_bank.tree,primary_bank.declarations.values[n].name,name)){
  CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&value,&diagnostic));return;}
 CHECK(false);
}
static NativeScriptValue floating(uint32_t bits){NativeScriptValue v={.type=NativeScriptType_F32};memcpy(&v.as.f32,&bits,4);return v;}
static NativeScriptValue integer(int32_t value){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=value};}
static NativeScriptValue boolean(bool value){return(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value};}
// The extracted setter remains byte-for-byte native. Its stage-entry leaf is
// controlled and observable; no floor, island or scene acceptance is implied.
StageInfo stage_info;ftCo_803C6594_t* ftCo_803C6594[Gr_Kind_Count];
static ftCo_803C6594_t stage_entry;static unsigned stage_mode,stage_calls,retire;
static void ftCo_800A1CC4(Fighter* f,ftCo_803C6594_t* entry)
{
 CHECK(entry==&stage_entry);++stage_calls;
 if(stage_mode){f->cpu.x64=f->cpu.x54;f->cpu.x60=300;f->cpu.x54=(Vec2){13,-27};f->cpu.x38=5;}
 if(retire){unsigned action=retire;retire=0;NativeScriptReceiverView v;
  if(action==1){CHECK(nativeScriptFighter_View(f->gobj,f->x8_spawnNum,&v));nativeScriptStorage_Destroy(v.storage);}
  if(action==2)++f->x8_spawnNum;
  if(action==3){substitute=*f;f->gobj->user_data=&substitute;}
  if(action==4)cpu_packages[0].program.identity++;
 }
}
float Stage_GetBlastZoneLeftOffset(void){return -100;}float Stage_GetBlastZoneRightOffset(void){return 100;}
float Stage_GetBlastZoneBottomOffset(void){return -100;}float Stage_GetBlastZoneTopOffset(void){return 100;}
bool ftCo_IsAlly_dontinline(Fighter* a,Fighter* b){(void)a;(void)b;return false;}
bool ftCo_800A1C44(Fighter* f){(void)f;return false;}
static bool ftCo_800A0F00(Fighter_GObj* g){(void)g;return false;}
bool ftLib_8008732C(Fighter_GObj* g){(void)g;return false;}
#include "native.inc"
#include "destination.inc"
#include "history_lookup.inc"
#include "grabbing.inc"
#include "planner_leaves.h"
static const uint32_t scalar_bits[]={0,0x80000000,0x3f800000,0xc0200000,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,1,0x80000001,0x7f7fffff};
static void scalar_cases(HSD_GObj* o,bool vm)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);
 const int32_t ints[]={0,1,9,-1,INT32_MIN,INT32_MAX};
 for(unsigned b=0;b<sizeof(scalar_bits)/sizeof(*scalar_bits);++b)for(unsigned flags=0;flags<4;++flags){
  NativeScriptValue value=floating(scalar_bits[b]);static Fighter before,expected;
  f->cpu.xF9_b0=f->cpu.xF9_b1=f->cpu.xF9_b3=f->cpu.xF9_b5=f->cpu.xF9_b6=f->cpu.xF9_b7=flags&1;
  f->cpu.xF9_b2=flags&1;f->cpu.xF9_b4=(flags>>1)&1;memcpy(&f->cpu.x56C,&value.as.f32,4);memcpy(&f->cpu.x3C,&value.as.f32,4);before=*f;
  const uint32_t ids[]={NativeScriptBinding_CPUApproachSpacing,NativeScriptBinding_CPUFollowDistance,NativeScriptBinding_CPUCombatCandidatesEnabled,NativeScriptBinding_CPURangedGateEnabled};
  for(unsigned i=0;i<4;++i){NativeScriptValue r;CHECK(nativeScriptBindings_Read(&c,ids[i],NULL,0,&r,&diagnostic));
   if(i<2)CHECK(r.type==NativeScriptType_F32&&!memcmp(&r.as.f32,&scalar_bits[b],4));else CHECK(r.type==NativeScriptType_Bool&&r.as.u64==(i==2?(flags&1):((flags>>1)&1)));++pairs;}
  CHECK(!memcmp(f,&before,sizeof(*f)));
  if(vm){CHECK(helper(o,"Observe"));NativeScriptValue observed=field(o,"spacing");CHECK(!memcmp(&observed.as.f32,&value.as.f32,4));}
  f->cpu.x3C=17;expected=*f;if(isfinite(value.as.f32)&&value.as.f32>=0)memcpy(&expected.cpu.x3C,&value.as.f32,4);
  CHECK(nativeScriptBindings_Write(&c,NativeScriptBinding_CPUFollowDistance,NULL,0,&value,&diagnostic));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
  if(vm){f->cpu.x3C=17;set_field(o,"distance",value);set_field(o,"mode",integer(f->cpu.xC));set_field(o,"level",integer(f->cpu.level));set_field(o,"combat",boolean(f->cpu.xF9_b2));set_field(o,"ranged",boolean(f->cpu.xF9_b4));CHECK(helper(o,"Apply"));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;}
  f->cpu.x56C=17;expected=*f;memcpy(&expected.cpu.x56C,&value.as.f32,4);NativeScriptValue result;
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPUSetApproachSpacing,NULL,0,&value,1,&result,&diagnostic));CHECK(result.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f)));++pairs;
  if(vm){f->cpu.x56C=17;set_field(o,"spacing",value);CHECK(helper(o,"Spacing"));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;}
  for(unsigned gate=0;gate<2;++gate){NativeScriptValue opposite=boolean(gate);expected=*f;expected.cpu.xF9_b2=gate;
   CHECK(nativeScriptBindings_Write(&c,NativeScriptBinding_CPUCombatCandidatesEnabled,NULL,0,&opposite,&diagnostic));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
   expected=*f;expected.cpu.xF9_b4=gate;CHECK(nativeScriptBindings_Write(&c,NativeScriptBinding_CPURangedGateEnabled,NULL,0,&opposite,&diagnostic));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;}
  ++cases;
 }
 for(unsigned n=0;n<sizeof(ints)/sizeof(*ints);++n){NativeScriptValue v=integer(ints[n]),r;static Fighter expected;
  expected=*f;expected.cpu.xC=ints[n];CHECK(nativeScriptBindings_Write(&c,NativeScriptBinding_CPUMode,NULL,0,&v,&diagnostic));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
  expected=*f;expected.cpu.level=ints[n];CHECK(nativeScriptBindings_Write(&c,NativeScriptBinding_CPULevel,NULL,0,&v,&diagnostic));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
  CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_CPUMode,NULL,0,&r,&diagnostic)&&r.type==NativeScriptType_I32&&r.as.i64==ints[n]);
  CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_CPULevel,NULL,0,&r,&diagnostic)&&r.type==NativeScriptType_I32&&r.as.i64==ints[n]);
  if(vm){CHECK(helper(o,"Observe"));CHECK(field(o,"mode").as.i64==ints[n]&&field(o,"level").as.i64==ints[n]);}
  ++cases;
 }
 if(vm){f->cpu.x60=0;stage_mode=0;static Fighter expected;expected=*f;ftCo_CpuSetDestination(&expected,-3,8,5);expected.cpu.x56C=-9;expected.cpu.x3C=11;CHECK(helper(o,"IntegerArguments"));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;}
}
static void destination_cases(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);const int behavior[]={-1,0,4,6};
 for(unsigned n=0;n<4;++n)for(unsigned match=0;match<4;++match)for(unsigned latch=0;latch<2;++latch){
  f->cpu.x18=behavior[n];f->cpu.x1C=(match&1)?behavior[n]:99;f->cpu.x20=(match&2)?behavior[n]:77;f->cpu.x60=15;f->cpu.xFA_b2=latch;
  static Fighter before,expected;before=*f;expected=before;bool result=ftCo_CpuPrepareDestinationUpdate(&expected);NativeScriptValue out;
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPUPrepareDestinationUpdate,NULL,0,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_Bool&&out.as.u64==result&&!memcmp(f,&expected,sizeof(*f)));++pairs;
  *f=before;CHECK(helper(o,"Prepare"));CHECK(field(o,"prepared").as.u64==result&&!memcmp(f,&expected,sizeof(*f)));++pairs;++cases;
 }
 for(unsigned b=0;b<sizeof(scalar_bits)/sizeof(*scalar_bits);++b)for(unsigned argument=0;argument<3;++argument)for(unsigned blocked=0;blocked<2;++blocked)for(stage_mode=0;stage_mode<2;++stage_mode){
  NativeScriptValue args[]={floating(0x3f800000),floating(0xc0200000),floating(0x40a00000)};args[argument]=floating(scalar_bits[b]);f->cpu.x60=blocked?3:0;f->cpu.x54=(Vec2){21,22};f->cpu.x38=23;
  static Fighter before,expected;before=*f;expected=before;stage_calls=0;ftCo_CpuSetDestination(&expected,args[0].as.f32,args[1].as.f32,args[2].as.f32);unsigned calls=stage_calls;stage_calls=0;NativeScriptValue out;
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPUSetDestination,NULL,0,args,3,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void&&stage_calls==calls&&!memcmp(f,&expected,sizeof(*f)));++pairs;
  *f=before;set_field(o,"x",args[0]);set_field(o,"y",args[1]);set_field(o,"distance",args[2]);stage_calls=0;CHECK(helper(o,"Destination"));CHECK(stage_calls==calls&&!memcmp(f,&expected,sizeof(*f)));++pairs;++cases;
 }
}
static void mirror(Fighter* target,const Fighter* source)
{
 HSD_GObj* object=target->gobj;int spawn=target->x8_spawnNum;*target=*source;target->gobj=object;target->x8_spawnNum=spawn;
 uintptr_t begin=(uintptr_t)source->cpu.xFC,end=begin+sizeof(source->cpu.xFC);
 uintptr_t read=(uintptr_t)source->cpu.x448,write=(uintptr_t)source->cpu.x444;
 if(read>=begin&&read<end)target->cpu.x448=(void*)((uintptr_t)target->cpu.xFC+read-begin);
 if(write>=begin&&write<end)target->cpu.x444=(void*)((uintptr_t)target->cpu.xFC+write-begin);
 uintptr_t bytes=(uintptr_t)source->cpu.buffer,pos=(uintptr_t)source->cpu.write_pos;
 if(pos>=bytes&&pos<=bytes+sizeof(source->cpu.buffer))target->cpu.write_pos=(void*)((uintptr_t)target->cpu.buffer+pos-bytes);
}
static void same_fighter(Fighter* actual,const Fighter* reference)
{
 static Fighter normalized;normalized=*reference;normalized.gobj=actual->gobj;normalized.x8_spawnNum=actual->x8_spawnNum;
 uintptr_t begin=(uintptr_t)reference->cpu.xFC,end=begin+sizeof(reference->cpu.xFC),read=(uintptr_t)reference->cpu.x448,write=(uintptr_t)reference->cpu.x444;
 if(read>=begin&&read<end)normalized.cpu.x448=(void*)((uintptr_t)actual->cpu.xFC+read-begin);
 if(write>=begin&&write<end)normalized.cpu.x444=(void*)((uintptr_t)actual->cpu.xFC+write-begin);
 uintptr_t bytes=(uintptr_t)reference->cpu.buffer,pos=(uintptr_t)reference->cpu.write_pos;
 if(pos>=bytes&&pos<=bytes+sizeof(reference->cpu.buffer))normalized.cpu.write_pos=(void*)((uintptr_t)actual->cpu.buffer+pos-bytes);
 CHECK(!memcmp(actual,&normalized,sizeof(*actual)));++pairs;
}
static void output_cases(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);const int values[]={INT32_MIN,-257,-129,-128,-127,-1,0,1,127,128,255,256,257,INT32_MAX};
 const uint32_t ids[]={NativeScriptBinding_CPUOutputLeftX,NativeScriptBinding_CPUOutputLeftY,NativeScriptBinding_CPUOutputLeftTrigger,NativeScriptBinding_CPUOutputRightTrigger,NativeScriptBinding_CPUOutputButtons};
 const char* names[]={"left_x","left_y","left_trigger","right_trigger","buttons"};
 for(unsigned n=0;n<sizeof(values)/sizeof(*values);++n){NativeScriptValue value=integer(values[n]);static Fighter before,expected;f->cpu.lstick=(S8Vec2){-31,47};f->cpu.cstick=(S8Vec2){-19,73};f->cpu.ltrigger=53;f->cpu.rtrigger=97;f->cpu.buttons=0x12345678;
  for(unsigned i=0;i<5;++i){before=*f;expected=before;
   if(i==0&&values[n]>=-128&&values[n]<=127)expected.cpu.lstick.x=values[n];
   if(i==1&&values[n]>=-128&&values[n]<=127)expected.cpu.lstick.y=values[n];
   if(i==2&&values[n]>=0&&values[n]<=255)expected.cpu.ltrigger=values[n];
   if(i==3&&values[n]>=0&&values[n]<=255)expected.cpu.rtrigger=values[n];
   if(i==4)expected.cpu.buttons=(uint32_t)values[n];
   CHECK(nativeScriptBindings_Write(&c,ids[i],NULL,0,&value,&diagnostic));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;++cases;}
  CHECK(helper(o,"ObserveOutputs"));const int outputs[]={f->cpu.lstick.x,f->cpu.lstick.y,f->cpu.ltrigger,f->cpu.rtrigger,(int32_t)f->cpu.buttons};
  for(unsigned i=0;i<5;++i){NativeScriptValue result;CHECK(nativeScriptBindings_Read(&c,ids[i],NULL,0,&result,&diagnostic));CHECK(result.type==NativeScriptType_I32&&result.as.i64==outputs[i]&&field(o,names[i]).as.i64==outputs[i]);set_field(o,names[i],value);}
  expected=*f;if(values[n]>=-128&&values[n]<=127)expected.cpu.lstick=(S8Vec2){values[n],values[n]};if(values[n]>=0&&values[n]<=255)expected.cpu.ltrigger=expected.cpu.rtrigger=values[n];expected.cpu.buttons=(uint32_t)values[n];
  CHECK(helper(o,"ApplyOutputs"));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
 }
 NativeScriptValue bad={.type=NativeScriptType_F32,.as.f32=1};static Fighter before;before=*f;
 for(unsigned i=0;i<5;++i){CHECK(!nativeScriptBindingCPUControl_Write(&c,ids[i],&bad,&diagnostic));bad=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=(int64_t)INT32_MAX+1};CHECK(!nativeScriptBindingCPUControl_Write(&c,ids[i],&bad,&diagnostic));bad=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};CHECK(!memcmp(f,&before,sizeof(*f)));++cases;}
}
static void seed_history(Fighter* f,unsigned write,unsigned read)
{
 for(unsigned n=0;n<30;++n)f->cpu.xFC[n]=(struct Fighter_x1A88_xFC_t){.x0=0x80000000u+n,.x4=n,.x5=255-n,.lstick={-128+n,127-n},.cstick={n,-(int)n},.xA=0xA0+n,.xB=0xD0+n,.cur_pos={n,-(float)n,n+0.5f},.facing_dir=n&1?-1:1};
 f->cpu.x444=&f->cpu.xFC[write];f->cpu.x448=&f->cpu.xFC[read];
}
static void history_cases(HSD_GObj* o,HSD_GObj* reference,HSD_GObj* input)
{
 Fighter* f=o->user_data,*expected=reference->user_data,*from=input->user_data;NativeScriptRuntimeContext c=context(o);
 const float axes[]={-1,-0.999f,-0.5f,-0.0f,0,0.5f,1,1.01f,NAN,INFINITY};const float triggers[]={-1,-0.9f,0,0.99f,1,255.9f,256,NAN};
 for(unsigned axis=0;axis<sizeof(axes)/sizeof(*axes);++axis)for(unsigned trigger=0;trigger<sizeof(triggers)/sizeof(*triggers);++trigger)for(unsigned wrap=0;wrap<2;++wrap){
  seed_history(f,wrap?29:3,wrap?29:1);from->input.lstick[0]=(Vec2){axes[axis],-0.5f};from->input.cstick[0]=(Vec2){0.5f,1};from->input.triggers[0]=triggers[trigger];from->input.held_buttons[0]=0xA5000081;from->cur_pos=(Vec3){-0.0f,17,-5};from->facing_dir=-1;
  static Fighter before;before=*f;mirror(expected,f);bool success=nativeCpuHistory_Record(reference,(uint32_t)from->x8_spawnNum);NativeScriptValue id=integer(from->x8_spawnNum),out;
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPURecordInput,NULL,0,&id,1,&out,&diagnostic));CHECK(out.type==NativeScriptType_Bool&&out.as.u64==success);same_fighter(f,expected);
  *f=before;set_field(o,"input_id",id);CHECK(helper(o,"Record"));CHECK(field(o,"recorded").as.u64==success);same_fighter(f,expected);++cases;
 }
 const uint32_t bad_pose[]={0x7f800000,0xff800000,0x7fc12345,0x7f812345};
 for(unsigned index=0;index<30;++index)for(unsigned bad=0;bad<6;++bad){
  seed_history(f,9,index);if(bad<4){float value;memcpy(&value,&bad_pose[bad],4);if(bad==0)f->cpu.xFC[index].cur_pos.x=value;if(bad==1)f->cpu.xFC[index].cur_pos.y=value;if(bad==2)f->cpu.xFC[index].cur_pos.z=value;if(bad==3)f->cpu.xFC[index].facing_dir=value;}
  if(bad==5)f->cpu.x448=(void*)((uintptr_t)f->cpu.xFC+1);mirror(expected,f);
  Vec3 position={31,32,33};float direction=34;bool success=nativeCpuHistory_ReadPose(reference,&position,&direction);NativeScriptValue args[]={floating(0x41f80000),floating(0x42000000),floating(0x42040000),floating(0x42080000)},initial[4];memcpy(initial,args,sizeof(args));NativeScriptValue out;
  NativeScriptValue family_args[4];memcpy(family_args,initial,sizeof(family_args));
  CHECK(nativeScriptBindingCPUControl_Invoke(&c,NativeScriptBinding_CPUReadHistoryPose,family_args,4,&out,&diagnostic));CHECK(out.type==NativeScriptType_Bool&&out.as.u64==success);same_fighter(f,expected);
  if(success)CHECK(family_args[0].as.f32==position.x&&family_args[1].as.f32==position.y&&family_args[2].as.f32==position.z&&family_args[3].as.f32==direction);else CHECK(!memcmp(family_args,initial,sizeof(family_args)));
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPUReadHistoryPose,NULL,0,args,4,&out,&diagnostic));CHECK(out.type==NativeScriptType_Bool&&out.as.u64==success);same_fighter(f,expected);
  if(success)CHECK(args[0].as.f32==position.x&&args[1].as.f32==position.y&&args[2].as.f32==position.z&&args[3].as.f32==direction);else for(unsigned i=0;i<4;++i)CHECK(args[i].type==NativeScriptType_F32&&args[i].as.f32==0.0f);
  const char* names[]={"x","y","z","direction"};for(unsigned i=0;i<4;++i)set_field(o,names[i],initial[i]);CHECK(helper(o,"Pose"));CHECK(field(o,"posed").as.u64==success);
  for(unsigned i=0;i<4;++i){NativeScriptValue observed=field(o,names[i]);CHECK(!memcmp(&observed.as.f32,success?&args[i].as.f32:&initial[i].as.f32,4));}
  bool applied=nativeCpuHistory_Apply(reference);CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPUApplyRecordedInput,NULL,0,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_Bool&&out.as.u64==applied);same_fighter(f,expected);
  CHECK(helper(o,"ApplyHistory"));CHECK(field(o,"applied").as.u64==applied);same_fighter(f,expected);++cases;
 }
 seed_history(f,1,2);mirror(expected,f);static Fighter before;before=*f;NativeScriptValue missing=integer(0),out;
 CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPURecordInput,NULL,0,&missing,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&!out.as.u64);CHECK(!memcmp(f,&before,sizeof(*f)));
 missing=integer(-1);CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPURecordInput,NULL,0,&missing,1,&out,&diagnostic)&&!out.as.u64);CHECK(!memcmp(f,&before,sizeof(*f)));
 const uintptr_t invalid[]={0,(uintptr_t)f->cpu.xFC-1,(uintptr_t)f->cpu.xFC+sizeof(f->cpu.xFC),(uintptr_t)f->cpu.xFC+1};
 for(unsigned i=0;i<4;++i){seed_history(f,1,2);f->cpu.x444=(void*)invalid[i];before=*f;NativeScriptValue id=integer(from->x8_spawnNum);CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPURecordInput,NULL,0,&id,1,&out,&diagnostic)&&!out.as.u64);CHECK(!memcmp(f,&before,sizeof(*f)));++cases;}
 NativeScriptValue malformed[]={boolean(true),floating(0x3f800000),{.type=NativeScriptType_I64,.as.i64=from->x8_spawnNum},{.type=NativeScriptType_I32,.as.i64=(int64_t)INT32_MAX+1}};
 for(unsigned i=0;i<4;++i){out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_CPURecordInput,NULL,0,&malformed[i],1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);CHECK(!memcmp(f,&before,sizeof(*f)));++cases;}
 seed_history(f,1,2);f->cpu.x44=NULL;f->cpu.x4C=NULL;f->cpu.special_item_target=NULL;++cases;
}
static void planner_seed(Fighter* f,unsigned n)
{
 memset(&f->cpu,0,sizeof(f->cpu));f->cpu.write_pos=f->cpu.buffer;f->cpu.x18=0;f->cpu.x1C=2;f->cpu.x20=3;f->cpu.level=5;f->cpu.x7C=1;f->cpu.x54=(Vec2){n&1?200:0,0};f->cpu.x88=2;f->cpu.x8C=1;f->cpu.x94=7;
 f->motion_id=0;f->ground_or_air=n&1?GA_Air:GA_Ground;f->cur_pos=(Vec3){2,3,0};f->facing_dir=n&1?-1:1;f->x221A_b3=false;f->x221B_b5=(n&2)!=0;f->item_gobj=NULL;
 f->cpu.x48=(void*)(uintptr_t)0x1234; // equality-only cache may be stale.
}
static bool native_plan(Fighter* f,unsigned which)
{
 if(which==0)ftCo_CpuPrepareCombatPlan(f);if(which==1)ftCo_CpuUpdateRecoveryDestination(f);if(which==2)return ftCo_CpuPlanAttack(f);if(which==3)ftCo_CpuPlanWithoutTarget(f);return false;
}
static void planner_cases(HSD_GObj* o,HSD_GObj* reference,HSD_GObj* target,HSD_GObj* item)
{
 Fighter* f=o->user_data,*expected=reference->user_data;NativeScriptRuntimeContext c=context(o);
 const uint32_t ids[]={NativeScriptBinding_CPUPrepareCombatPlan,NativeScriptBinding_CPUUpdateRecoveryDestination,NativeScriptBinding_CPUPlanAttack,NativeScriptBinding_CPUPlanWithoutTarget};const char* names[]={"Combat","Recovery","Attack","NoTarget"};
 const int motions[]={0,ftCo_MS_BarrelWait,0x131,0xBF,0xFC,0x26,0xE0};const int behaviors[]={0,0x12,0x11,0x13,5,6,9,4,0xF,0x10,7,3,2,8,0xD,0xE,0xA};
 for(unsigned which=0;which<4;++which)for(unsigned n=0;n<40;++n){planner_seed(f,n);policy=(PlannerPolicy){.target=n&1?target->user_data:NULL,.common_item=n&2?item->user_data:NULL,.special_item=n&4?item->user_data:NULL,.floor=(n&8)!=0,.excluded=(n&16)!=0,.game_count=n&1?2:1,.right=(n&1)!=0,.left=(n&2)!=0,.command=n%3==0?2:0,.random=n&1?0.75f:0.25f};
  if(which==2){f->cpu.x18=n<17?behaviors[n]:0;f->motion_id=n<17?0:motions[n%7];f->cpu.x44=target->user_data;f->item_gobj=n&1?item:NULL;policy.candidate=n==17;policy.defend=n==18;policy.projectile=n==19?3:0;policy.recovery=n==20;policy.travel=n==21;policy.grabbed=n==22;policy.item_check=n==23;f->cpu.xC=n==24?14:0;f->cpu.x7C=n==24?0:1;}
  if(which==1){f->cpu.xFA_b2=n&4;f->facing_dir=n%5==0?NAN:n&1?-1:1;}
  static Fighter before;before=*f;mirror(expected,f);set_field(o,"grabbing",boolean(n==25));set_field(reference,"grabbing",boolean(n==25));leaf_count=0;bool result=native_plan(expected,which);unsigned trace_count=leaf_count,trace_values[128];memcpy(trace_values,leaf_trace,sizeof(trace_values));leaf_count=0;NativeScriptValue out;
  CHECK(nativeScriptBindings_Invoke(&c,ids[which],NULL,0,NULL,0,&out,&diagnostic));CHECK(out.type==(which==2?NativeScriptType_Bool:NativeScriptType_Void));if(which==2)CHECK(out.as.u64==result);CHECK(leaf_count==trace_count&&!memcmp(leaf_trace,trace_values,trace_count*sizeof(unsigned)));same_fighter(f,expected);
  *f=before;leaf_count=0;CHECK(helper(o,names[which]));if(which==2)CHECK(field(o,"attack").as.u64==result);CHECK(leaf_count==trace_count&&!memcmp(leaf_trace,trace_values,trace_count*sizeof(unsigned)));same_fighter(f,expected);++cases;
 }
 policy=(PlannerPolicy){0};planner_seed(f,0);f->cpu.x44=&substitute;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};leaf_count=0;CHECK(!nativeScriptBindingCPUControl_Invoke(&c,NativeScriptBinding_CPUPlanAttack,NULL,0,&out,&diagnostic));CHECK(leaf_count==0&&out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 for(unsigned mode=1;mode<=2;++mode){planner_seed(f,0);f->cpu.x44=target->user_data;f->cpu.x4C=item->user_data;f->item_gobj=item;policy=(PlannerPolicy){.target=target->user_data,.common_item=item->user_data,.retire_target=mode};uint32_t generation=mode==1?((Fighter*)target->user_data)->x8_spawnNum:((Item*)item->user_data)->x1C;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingCPUControl_Invoke(&c,NativeScriptBinding_CPUPlanAttack,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);if(mode==1)((Fighter*)target->user_data)->x8_spawnNum=generation;else((Item*)item->user_data)->x1C=generation;++cases;
 }
 policy=(PlannerPolicy){0};planner_seed(f,0);f->cpu.x44=NULL;f->cpu.x4C=NULL;f->cpu.special_item_target=NULL;f->item_gobj=NULL;set_field(o,"grabbing",boolean(false));set_field(reference,"grabbing",boolean(false));
 HSD_GObj* fault=actor(0);Fighter* failed=fault->user_data;planner_seed(failed,0);set_field(fault,"grab_fault",boolean(true));NativeScriptRuntimeContext bad=context(fault);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};leaf_count=0;
 CHECK(!nativeScriptBindingCPUControl_Invoke(&bad,NativeScriptBinding_CPUPlanAttack,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&failed->cpu.x18==0);CHECK(nativeScriptFighter_Status(fault)!=NativeScriptVM_Ok);HSD_GObjFree(fault);++cases;
}
static uint32_t name_key(const char* name){uint32_t result=2166136261u;while(*name){result^=(unsigned char)*name++;result*=16777619u;}return result;}
static void item_remove(void* data){nativeScriptItem_Detach(((Item*)data)->entity);}
static HSD_GObj* item_actor(void)
{
 CHECK(item_count<4);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
 HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* f=&items[item_count++];
 *f=(Item){.entity=o,.kind=p->kind,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.xC4_article_data=&article,.x1C=200+item_count,.msid=-1};
 GObj_InitUserData(o,6,item_remove,f);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static void receivers(HSD_GObj* item,HSD_GObj* owned,HSD_GObj* stock)
{
 NativeScriptRuntimeContext c=context(item);NativeScriptReceiverScope scope={.pool=nativeScriptRuntime_Receivers()};NativeScriptValue token;
 CHECK(nativeScriptReceiver_Push(scope.pool,&c,c.scene_epoch,&primary_bank.declarations,Ft_Kind_CustomFirst,nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope.frame,&diagnostic));
 HSD_GObj* targets[]={owned,stock};
 for(unsigned n=0;n<2;++n){Fighter* f=targets[n]->user_data;f->cpu.level=-31-(int)n;static Fighter before;before=*f;
  CHECK(nativeScriptItem_SetAttachmentOwner(item,targets[n]));CHECK(nativeScriptReceiver_Capture(&scope,&c,NativeScriptReceiver_AttachmentOwner,true,false,&token,&diagnostic));NativeScriptValue r;
  CHECK(nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_CPULevel,&token,1,&r,&diagnostic)&&r.type==NativeScriptType_I32&&r.as.i64==f->cpu.level);++pairs;
  r=integer(INT32_MIN);bool wrote=nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_CPULevel,&token,1,&r,&diagnostic);
  CHECK(wrote==(n==0));if(wrote)before.cpu.level=INT32_MIN;CHECK(!memcmp(f,&before,sizeof(*f)));++pairs;
  CHECK(nativeScriptItem_RunSignal(item,name_key("ObserveOwner"),0,&r)==NativeScriptVM_Ok);++vm_calls;++receiver_calls;CHECK(field(item,"level").as.i64==f->cpu.level);
  if(n==0){set_field(item,"level",integer(INT32_MAX));before.cpu.level=INT32_MAX;CHECK(nativeScriptItem_RunSignal(item,name_key("WriteOwner"),0,&r)==NativeScriptVM_Ok);++vm_calls;++receiver_calls;CHECK(!memcmp(f,&before,sizeof(*f)));++pairs;}
 }
 CHECK(nativeScriptItem_SetAttachmentOwner(item,stock));CHECK(nativeScriptReceiver_Capture(&scope,&c,NativeScriptReceiver_AttachmentOwner,true,false,&token,&diagnostic));
 uint32_t generation=((Fighter*)stock->user_data)->x8_spawnNum;((Fighter*)stock->user_data)->x8_spawnNum++;
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},r=sentinel;CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_CPULevel,&token,1,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));((Fighter*)stock->user_data)->x8_spawnNum=generation;++cases;
 CHECK(nativeScriptReceiver_Pop(scope.pool,scope.frame,&diagnostic));
}
static void rejected(NativeScriptRuntimeContext* c)
{
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},r=sentinel,v=floating(0x3f800000),i=integer(9),b=boolean(true);
 const uint32_t reads[]={NativeScriptBinding_CPUApproachSpacing,NativeScriptBinding_CPUMode,NativeScriptBinding_CPUFollowDistance,NativeScriptBinding_CPUCombatCandidatesEnabled,NativeScriptBinding_CPURangedGateEnabled,NativeScriptBinding_CPUTargetID,NativeScriptBinding_CPUTravelFrames,NativeScriptBinding_CPUSelectedCommand,NativeScriptBinding_CPUElapsedFrames,NativeScriptBinding_CPUFloorInBounds};
 for(unsigned n=0;n<sizeof(reads)/sizeof(*reads);++n){CHECK(!nativeScriptBindingCPUControl_Read(c,reads[n],&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));}
 const uint32_t writes[]={NativeScriptBinding_CPUMode,NativeScriptBinding_CPULevel,NativeScriptBinding_CPUFollowDistance,NativeScriptBinding_CPUCombatCandidatesEnabled,NativeScriptBinding_CPURangedGateEnabled};
 for(unsigned n=0;n<5;++n)CHECK(!nativeScriptBindingCPUControl_Write(c,writes[n],n<2?&i:n==2?&v:&b,&diagnostic));
 CHECK(!nativeScriptBindingCPUControl_Invoke(c,NativeScriptBinding_CPUPrepareDestinationUpdate,NULL,0,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingCPUControl_Invoke(c,NativeScriptBinding_CPUSetApproachSpacing,&v,1,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));++cases;
}
static void negatives(HSD_GObj* o,HSD_GObj* stock)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);static Fighter before;before=*f;
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},r=sentinel,values[]={integer(3),boolean(true),floating(0x3f800000)};
 const uint32_t calls[]={NativeScriptBinding_CPUPrepareDestinationUpdate,NativeScriptBinding_CPUSetDestination,NativeScriptBinding_CPUSetApproachSpacing};
 for(unsigned n=0;n<3;++n){CHECK(!nativeScriptBindingCPUControl_Invoke(&c,calls[n],NULL,1,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));++cases;}
 CHECK(!nativeScriptBindingCPUControl_Invoke(&c,NativeScriptBinding_CPUSetDestination,values,3,&r,&diagnostic));
 CHECK(!nativeScriptBindingCPUControl_Invoke(&c,NativeScriptBinding_CPUSetApproachSpacing,values,1,&r,&diagnostic));
 CHECK(!nativeScriptBindingCPUControl_Write(&c,NativeScriptBinding_CPULevel,&values[2],&diagnostic));
 CHECK(!nativeScriptBindingCPUControl_Write(&c,NativeScriptBinding_CPUMode,&values[2],&diagnostic));
 CHECK(!nativeScriptBindingCPUControl_Write(&c,NativeScriptBinding_CPUFollowDistance,&values[1],&diagnostic));
 CHECK(!nativeScriptBindingCPUControl_Write(&c,NativeScriptBinding_CPUCombatCandidatesEnabled,&values[0],&diagnostic));
 CHECK(!nativeScriptBindingCPUControl_Write(&c,NativeScriptBinding_CPURangedGateEnabled,&values[0],&diagnostic));
 CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_CPUApproachSpacing,NULL,0,&values[2],&diagnostic));CHECK(!memcmp(f,&before,sizeof(*f)));cases+=8;
 rejected(NULL);NativeScriptRuntimeContext raw=context(stock);rejected(&raw);
 c.source_class=NativeCallbackSource_Item;rejected(&c);c=context(o);++c.generation;rejected(&c);c=context(o);++c.scene_epoch;rejected(&c);c=context(o);
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,c.generation,&view));const NativeScriptStorageLayout* layout=view.storage->layout;const NativeScriptStorageParameters* params=view.storage->parameters;
 NativeScriptStorageLayout other=*layout;NativeScriptStorageParameters other_params=*params;view.storage->layout=&other;rejected(&c);view.storage->layout=layout;view.storage->parameters=&other_params;rejected(&c);view.storage->parameters=params;
 f->gobj=NULL;rejected(&c);f->gobj=o;f->ft_data=NULL;rejected(&c);f->ft_data=before.ft_data;
 substitute=*f;substitute.gobj=NULL;o->user_data=&substitute;rejected(&c);o->user_data=f;CHECK(!memcmp(f,&before,sizeof(*f)));
}
static void continuation_cases(void)
{
 for(unsigned mode=1;mode<=4;++mode){HSD_GObj* o=actor(0);Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);f->cpu.x60=0;stage_mode=0;retire=mode;
  uint64_t identity=cpu_packages[0].program.identity;uint32_t generation=f->x8_spawnNum;
  NativeScriptValue args[]={floating(0x3f800000),floating(0x40000000),floating(0x40400000)},r={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingCPUControl_Invoke(&c,NativeScriptBinding_CPUSetDestination,args,3,&r,&diagnostic));CHECK(r.type==NativeScriptType_U64&&r.as.u64==99);
  CHECK(f->cpu.x54.x==1&&f->cpu.x54.y==2&&f->cpu.x38==3);++cases;
  cpu_packages[0].program.identity=identity;f->x8_spawnNum=generation;o->user_data=f;HSD_GObjFree(o);
 }
 HSD_GObj* o=actor(0);NativeScriptRuntimeContext c=context(o);CHECK(!helper(o,"Fault"));rejected(&c);HSD_GObjFree(o);
}
static void compile_negatives(void)
{
 const char* invalid[]={"func Invalid():\n    cpu.elapsed_frames = 1\n","func Invalid():\n    cpu.floor_in_bounds = true\n","func Invalid():\n    cpu.body_half_width = 1\n","func Invalid():\n    cpu.travel_frames = 1.5\n","func Invalid():\n    cpu.selected_command = true\n","article Probe states 1:\n    on created:\n        let int value = attachment_owner.cpu.travel_frames\n    state 0 anim -1:\n","func Invalid():\n    cpu.target_id = 1\n","article Probe states 1:\n    on created:\n        let uint target = cpu.target_id\n    state 0 anim -1:\n","func Invalid():\n    cpu.record_input(true)\n","func Invalid():\n    cpu.record_input(1.0)\n","func Invalid():\n    cpu.apply_recorded_input(1)\n","func Invalid():\n    cpu.prepare_combat_plan(1)\n","func Invalid():\n    cpu.update_recovery_destination(1)\n","func Invalid():\n    cpu.plan_attack(1)\n","func Invalid():\n    cpu.plan_without_target(1)\n","func Invalid():\n    let float x = 0\n    let float y = 0\n    let float z = 0\n    cpu.read_history_pose(x, y, z, x)\n","func Invalid():\n    let float x = 0\n    let float y = 0\n    let float z = 0\n    let int direction = 0\n    cpu.read_history_pose(x, y, z, direction)\n","func Invalid():\n    cpu.output.left_x = 1.0\n","func Invalid():\n    cpu.output.buttons = true\n","article Probe states 1:\n    on created:\n        attachment_owner.cpu.output.buttons = 1\n    state 0 anim -1:\n","func Invalid():\n    cpu.approach_spacing = 1.0\n","func Invalid():\n    cpu.mode = 1.0\n","func Invalid():\n    cpu.level = 1.0\n","func Invalid():\n    cpu.follow_distance = true\n","func Invalid():\n    cpu.combat_candidates_enabled = 1\n","func Invalid():\n    cpu.ranged_gate_enabled = 1\n","func Invalid():\n    cpu.prepare_destination_update(1)\n","func Invalid():\n    cpu.set_destination(1, 2)\n","func Invalid():\n    cpu.set_destination(1, true, 3)\n","func Invalid():\n    cpu.set_approach_spacing(true)\n","article Probe states 1:\n    on created:\n        attachment_owner.cpu.mode = 3\n    state 0 anim -1:\n","article Probe states 1:\n    state float n\n    on created:\n        item.state.n = attachment_owner.cpu.approach_spacing\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        attachment_owner.cpu.follow_distance = 3\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        attachment_owner.cpu.combat_candidates_enabled = true\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        attachment_owner.cpu.ranged_gate_enabled = false\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        attachment_owner.cpu.prepare_destination_update()\n    state 0 anim -1:\n"};
 NativeScriptCompileBank scratch;static NativeScriptCompileBank accepted_bank;const NativeScriptCompileBank* accepted=&accepted_bank;
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 for(unsigned n=0;n<sizeof(invalid)/sizeof(*invalid);++n){configure(0);scratch=primary_bank;NativeScriptCompileRequest request={.source=invalid[n],.source_bytes=(uint32_t)strlen(invalid[n]),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};CHECK(!nativeScriptCompile_Build(&scratch,&request,&accepted,&diagnostic));CHECK(accepted==&accepted_bank);++cases;}
}
 #include "target_cases.h"
 #include "planner_field_cases.h"
int main(void)
{
 prepare();stage_info.grkind=0;ftCo_803C6594[0]=&stage_entry;HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 HSD_GObj* owned=actor(0),*foreign=actor(1),*stock=actor(-1),*item=item_actor();
 target_cases(owned,foreign,stock);planner_field_cases(owned,foreign,stock,item);scalar_cases(owned,true);scalar_cases(foreign,false);destination_cases(owned);receivers(item,foreign,stock);negatives(owned,stock);
 HSD_GObj* reference=actor(0);output_cases(owned);history_cases(owned,reference,foreign);uint32_t stock_generation=((Fighter*)stock->user_data)->x8_spawnNum;((Fighter*)stock->user_data)->x8_spawnNum=INT32_MIN;history_cases(owned,reference,stock);((Fighter*)stock->user_data)->x8_spawnNum=stock_generation;planner_cases(owned,reference,foreign,item);HSD_GObjFree(reference);
 NativeScriptRuntimeContext raw=context(stock);NativeScriptValue level;CHECK(nativeScriptBindings_Read(&raw,NativeScriptBinding_CPULevel,NULL,0,&level,&diagnostic));
 HSD_GObjFree(item);HSD_GObjFree(foreign);HSD_GObjFree(owned);NativeScriptRuntimeContext stale=context(stock);HSD_GObjFree(stock);HSD_GObj* recycled=actor(-1);CHECK(recycled==stock);rejected(&stale);HSD_GObjFree(recycled);
 continuation_cases();CHECK(nativeGObjPool_RetireScene());compile_negatives();
 printf("{\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u,\"receiver_vm_calls\":%u}\n",checks,cases,pairs,vm_calls,receiver_calls);return 0;
}
