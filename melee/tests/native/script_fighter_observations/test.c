#include "fixture.h"
#include <melee/ft/native_script_binding_fighter_observations.h>
#include <melee/ft/native_script_binding_hitlag.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_script_binding_common_properties.h>
#include <melee/ft/native_script_binding_fighter_items.h>
#include <melee/it/it_26B1.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ft_0892.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum {N=1024,I=4096,F=64,V=96};
typedef struct CompileStorage {
 char source_bytes[8192];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
 NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
 NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
 NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} CompileStorage;
static CompileStorage compile_storage[2];
NativeSourcePackage observation_packages[3];
#define primary_bank observation_packages[0].compiler.bank
static Fighter fighters[32],substitute;static unsigned allocated,checks,cases,pairs,vm_calls,receiver_calls;
static Article article;static ItemAttr article_attributes;static ItemModelDesc article_model;static Item items[4];static unsigned item_count;
static MotionState motions[342];static FighterBone parts[MAX_FT_PARTS];static HSD_JObj joints[MAX_FT_PARTS];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static NativeScriptDiagnostic diagnostic;
static ftCommonData shield_common;
ftCommonData* p_ftCommonData=&shield_common;
static unsigned heavy_pairs,shield_pairs,shield_vm_cases,observation_rejections;
// Attack accounting is exercised by the scheduler cohort, not this reader batch.
void ft_80089824(Fighter_GObj* o){(void)o;abort();}
void ft_800892A0(Fighter_GObj* o){(void)o;abort();}
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"observations %u case %u: %s (%s)\n",__LINE__,cases,#x,diagnostic.message);exit(1);}}while(0)
static void configure(unsigned index)
{
 CompileStorage* s=&compile_storage[index];
 observation_packages[index].compiler.bank=(NativeScriptCompileBank){.source_storage=s->source_bytes,.source_capacity=sizeof(s->source_bytes),.tree={.nodes=s->nodes,.capacity=N},.expressions={.nodes=s->ir,.node_capacity=I,.arguments=s->arguments,.argument_capacity=I},.declarations={.functions=s->functions,.function_capacity=F,.parameters=s->parameters,.parameter_capacity=V,.values=s->values,.value_capacity=V},.locals=s->locals,.local_capacity=V,.instructions=s->instructions,.instruction_capacity=I,.function_code=s->codes,.function_code_capacity=F,.execution_functions=s->entries,.execution_function_capacity=F,.graph=s->graph,.graph_capacity=F,.storage_layout={.fields=s->fields,.field_capacity=V,.owners=s->owners,.owner_capacity=V},.parameter_slots=s->slots,.parameter_slot_capacity=V};
}
static const char* source="fighter:\n    behavior independent\nstate float lag\nstate int debug\nstate int marks\nstate int zero\nstate int category\nstate bool incapacitated\nstate bool sync\nstate float offset\nstate float mid_walk\nstate bool reject_healing\nstate bool heavy\nstate float push_damage\nstate float push_base\nstate float push_velocity\nstate float push_min\nstate float push_max\nstate int hit_damage\nstate float light\nstate float push_result\nfunc ObserveWeight():\n    state.heavy = items.heavy\nfunc ObservePushback():\n    state.push_damage = shield.pushback_damage_scale\n    state.push_base = shield.pushback_base\n    state.push_velocity = shield.pushback_velocity_scale\n    state.push_min = shield.pushback_light_min\n    state.push_max = shield.pushback_light_max\nfunc Pushback():\n    let pushback = shield.pushback_damage_scale * (int(state.hit_damage) * (1 - (state.light * (shield.pushback_light_max - shield.pushback_light_min) + shield.pushback_light_min))) + shield.pushback_base\n    state.push_result = pushback * shield.pushback_velocity_scale\nstate bool hitlag\nfunc ObserveHitlag():\n    state.hitlag = hitlag.active\nstate float collision_x\nstate float collision_y\nstate float collision_z\nfunc ObserveCollision():\n    state.collision_x = collision.position.x\n    state.collision_y = collision.position.y\n    state.collision_z = collision.position.z\nfunc ObserveHealing():\n    state.reject_healing = items.reject_healing_pickup\nfunc EnableHealing():\n    items.reject_healing_pickup = true\nfunc DisableHealing():\n    items.reject_healing_pickup = false\nstate float friction\nfunc ObserveFriction():\n    state.friction = floor.friction\nfunc ObserveScalar():\n    state.sync = motion.allows_controller_sync\n    state.offset = spawn.horizontal_offset\n    state.mid_walk = ground.mid_walk_point\nfunc ApplyOffset():\n    spawn.horizontal_offset = state.offset\nfunc AssignOffset():\n    spawn.horizontal_offset = -3\nfunc Observe():\n    state.lag = movement.special_landing_lag\n    state.debug = engine.debug_level\nfunc ObserveMetadata():\n    state.category = action.category\n    state.incapacitated = fighter.incapacitated\nfunc Mark():\n    stats.count_as_thrown_item()\n    state.marks = state.marks + 1\nfunc Fault():\n    state.marks = 1 / state.zero\naction Placeholder:\n    on tick:\n        pass\narticle Probe states 1:\n    state int category\n    state bool incapacitated\n    state bool sync\n    state float offset\n    state float mid_walk\n    state bool reject_healing\n    state bool hitlag\n    on signal ObserveOwnerHitlag:\n        item.state.hitlag = attachment_owner.hitlag.active\n    state float normal_x\n    state float normal_y\n    state float normal_z\n    state float collision_x\n    state float collision_y\n    state float collision_z\n    state float left_x\n    state float left_y\n    state float right_x\n    state float right_y\n    state float ceiling_x\n    state float ceiling_y\n    on signal ObserveNormals:\n        item.state.left_x = wall.left.normal.x\n        item.state.left_y = wall.left.normal.y\n        item.state.right_x = wall.right.normal.x\n        item.state.right_y = wall.right.normal.y\n        item.state.ceiling_x = ceiling.normal.x\n        item.state.ceiling_y = ceiling.normal.y\n    on signal ObserveCollision:\n        item.state.collision_x = collision.position.x\n        item.state.collision_y = collision.position.y\n        item.state.collision_z = collision.position.z\n    on signal ObserveOwnerCollision:\n        item.state.collision_x = attachment_owner.collision.position.x\n        item.state.collision_y = attachment_owner.collision.position.y\n        item.state.collision_z = attachment_owner.collision.position.z\n    state float friction\n    on signal ObserveFriction:\n        item.state.friction = floor.friction\n    on signal ObserveFloor:\n        item.state.normal_x = floor.normal.x\n        item.state.normal_y = floor.normal.y\n        item.state.normal_z = floor.normal.z\n    on signal ObserveHealing:\n        item.state.reject_healing = attachment_owner.items.reject_healing_pickup\n    on signal EnableHealing:\n        attachment_owner.items.reject_healing_pickup = true\n    on signal DisableHealing:\n        attachment_owner.items.reject_healing_pickup = false\n    on signal WriteOwner:\n        attachment_owner.spawn.horizontal_offset = item.state.offset\n    on signal ObserveOwner:\n        item.state.category = attachment_owner.action.category\n        item.state.incapacitated = attachment_owner.fighter.incapacitated\n        item.state.sync = attachment_owner.motion.allows_controller_sync\n        item.state.offset = attachment_owner.spawn.horizontal_offset\n        item.state.mid_walk = attachment_owner.ground.mid_walk_point\n    state float slide_x\n    state float slide_y\n    state float slide_z\n    on signal ReadSlide:\n        item.state.slide_x = item.slide_velocity.x\n        item.state.slide_y = item.slide_velocity.y\n        item.state.slide_z = item.slide_velocity.z\n    on signal WriteSlide:\n        item.slide_velocity.x = item.state.slide_x + 1\n        item.slide_velocity.y = item.state.slide_y - 2\n        item.slide_velocity.z = item.state.slide_z * float(0.5)\n    state 0 anim -1:\n"
 "state float walk_acceleration\nstate float walk_maximum\nstate float jump_momentum\nstate float jump_maximum\nfunc ReadLocomotion():\n    state.walk_acceleration = ground.walk_acceleration_multiplier\n    state.walk_maximum = ground.walk_max_speed\n    state.jump_momentum = air.ground_jump_momentum_multiplier\n    state.jump_maximum = air.jump_horizontal_max\n"
 "state int current_metadata\nstate int current_move\nfunc ReadCurrentMetadata():\n    state.current_metadata = action.current_metadata\n    state.current_move = action.move_id(action.current_metadata)\n"
 "state int cleanup_order\nfunc ResetCleanup():\n    state.cleanup_order = 0\nfunc CleanupDamage():\n    state.cleanup_order = state.cleanup_order * 10 + 3\nfunc CleanupDeath():\n    state.cleanup_order = state.cleanup_order * 10 + 4\nfunc RunCleanup():\n    damage.interrupt_cleanup()\nfunc CleanupFault():\n    let int invalid = 1 / state.zero\n";
static void prepare(void)
{
 configure(0);article=(Article){.x0_common_attr=&article_attributes,.x10_modelDesc=&article_model};
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 static char combined[16384];
 const char* policies="\nstate bool policy_arm\nstate bool policy_hip\nstate bool policy_ko\n"
  "func PolicyEnable():\n    capture.armed = true\n    animation.hip_root_compensation = true\n    damage.force_top_blast_ko = true\n    ReadPolicies()\n"
  "func PolicyDisable():\n    capture.armed = false\n    animation.hip_root_compensation = false\n    damage.force_top_blast_ko = false\n    ReadPolicies()\n"
  "func ReadPolicies():\n    state.policy_arm = capture.armed\n    state.policy_hip = animation.hip_root_compensation\n    state.policy_ko = damage.force_top_blast_ko\n"
  "\nstate int stat_count\nstate int stat_slot\nstate int release_age\nfunc ObserveStatistics():\n    state.stat_slot = fighter.player_slot\n    state.release_age = input.down_press_age\n    state.stat_count = stats.action_count(fighter.player_slot, 0x72)\n    stats.notify_taunt()\n";
 CHECK(strlen(source)+strlen(policies)<sizeof(combined));snprintf(combined,sizeof(combined),"%s%s",source,policies);
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=combined,.source_bytes=(uint32_t)strlen(combined),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&primary_bank,&request,&accepted,&diagnostic));CHECK(accepted==&primary_bank);
 NativeSourcePackage* p=&observation_packages[0];CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&primary_bank,&layout,&metadata,&p->program,&diagnostic));
 p->address=p;p->ready=p->assets.ready=true;p->target=Ft_Kind_CustomFirst;p->assets.facts.moves=layout;p->assets.facts.joints=MAX_FT_PARTS;p->assets.parts.parts_num=MAX_FT_PARTS;p->assets.parts.part_to_joint=p->assets.part_to_joint;p->assets.parts.joint_to_part=p->assets.joint_to_part;p->assets.motions=motions;
 tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 for(unsigned n=0;n<MAX_FT_PARTS;++n)parts[n].joint=&joints[n];
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);
 configure(1);p=&observation_packages[1];const char* foreign="fighter:\n    behavior independent\nstate float unrelated\naction Foreign:\n    on tick:\n        pass\n";
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
 *f=(Fighter){.gobj=o,.kind=kind,.behavior_kind=kind,.x8_spawnNum=100+allocated,.ft_data=&observation_packages[index>=0?index:0].assets.character.data,.parts=parts};
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
static uint32_t name_key(const char* name)
{uint32_t result=2166136261u;while(*name){result^=(unsigned char)*name++;result*=16777619u;}return result;}
static void positives(HSD_GObj* o,bool owned)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);
 const uint32_t bits[]={0,0x80000000,0x3f800000,0xc0200000,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,1,0x80000001,0x7f7fffff};
 const int32_t levels[]={0,1,2,3,4,-1,INT32_MIN,INT32_MAX};
 for(unsigned b=0;b<sizeof(bits)/sizeof(bits[0]);++b)for(unsigned l=0;l<sizeof(levels)/sizeof(levels[0]);++l){memcpy(&f->x2EC,&bits[b],4);memcpy(&DbLevel,&levels[l],4);static Fighter before;before=*f;NativeScriptValue lag,debug;
  CHECK(nativeScriptBindingFighterObservations_Read(&c,NativeScriptBinding_MovementSpecialLandingLag,&lag,&diagnostic));CHECK(lag.type==NativeScriptType_F32&&!memcmp(&lag.as.f32,&f->x2EC,4));
  CHECK(nativeScriptBindingFighterObservations_Read(&c,NativeScriptBinding_EngineDebugLevel,&debug,&diagnostic));CHECK(debug.type==NativeScriptType_I32&&debug.as.i64==levels[l]);CHECK(!memcmp(&before,f,sizeof(*f)));
  if(owned){CHECK(helper(o,"Observe"));lag=field(o,"lag");debug=field(o,"debug");CHECK(!memcmp(&lag.as.f32,&f->x2EC,4)&&debug.as.i64==levels[l]);CHECK(!memcmp(&before,f,sizeof(*f)));}++cases;pairs+=2;
 }
 for(unsigned initial=0;initial<2;++initial){f->x2070.count_thrown_items=initial;static Fighter expected;expected=*f;expected.x2070.count_thrown_items=true;NativeScriptValue r;
  CHECK(nativeScriptBindingFighterObservations_Invoke(&c,NativeScriptBinding_StatsCountAsThrownItem,NULL,0,&r,&diagnostic));CHECK(r.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f)));
  if(owned){CHECK(helper(o,"Mark"));CHECK(helper(o,"Mark"));CHECK(!memcmp(f,&expected,sizeof(*f)));}++cases;++pairs;
 }
 if(owned)CHECK(field(o,"marks").as.i64==4);
}
static void metadata_positives(HSD_GObj* target,HSD_GObj* caller,bool self)
{
 Fighter* f=target->user_data;NativeScriptRuntimeContext c=context(target);
 for(unsigned other=0;other<2;++other)for(unsigned category=0;category<16;++category)for(unsigned inactive=0;inactive<2;++inactive){
  f->x2070.x2070_int=other?-1:0;f->x2070.x2071_b0_3=category;f->x221F_b3=inactive;static Fighter before;before=*f;
  NativeScriptValue a,b;CHECK(nativeScriptBindingFighterObservations_Read(&c,NativeScriptBinding_ActionCategory,&a,&diagnostic));
  CHECK(a.type==NativeScriptType_I32&&a.as.i64==f->x2070.x2071_b0_3);
  CHECK(nativeScriptBindingFighterObservations_Read(&c,NativeScriptBinding_FighterIncapacitated,&b,&diagnostic));
  CHECK(b.type==NativeScriptType_Bool&&b.as.u64==(f->x221F_b3!=0));CHECK(!memcmp(f,&before,sizeof(*f)));pairs+=2;
  if(self){CHECK(helper(target,"ObserveMetadata"));CHECK(field(target,"category").as.i64==category&&field(target,"incapacitated").as.u64==inactive);}
  CHECK(nativeScriptItem_SetAttachmentOwner(caller,target));NativeScriptValue result;
  CHECK(nativeScriptItem_RunSignal(caller,name_key("ObserveOwner"),0,&result)==NativeScriptVM_Ok);++vm_calls;++receiver_calls;
  a=field(caller,"category");b=field(caller,"incapacitated");CHECK(a.type==NativeScriptType_I32&&a.as.i64==category&&b.type==NativeScriptType_Bool&&b.as.u64==inactive);
  CHECK(!memcmp(f,&before,sizeof(*f)));++cases;
 }
}
static void item_remove(void* data){nativeScriptItem_Detach(((Item*)data)->entity);}
static void scalar_positives(HSD_GObj* target,HSD_GObj* caller,bool self)
{
 Fighter* f=target->user_data;NativeScriptRuntimeContext c=context(target);
 const uint32_t bits[]={0,0x80000000,0x3f800000,0xc0200000,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,1,0x80000001,0x7f7fffff};
 for(unsigned other=0;other<2;++other)for(unsigned sync=0;sync<2;++sync)for(unsigned b=0;b<sizeof(bits)/sizeof(bits[0]);++b){
  f->x2225_b0=f->x2225_b1=f->x2225_b2=f->x2225_b4=f->x2225_b5=f->x2225_b6=f->x2225_b7=other;
  f->x2225_b3=sync;memcpy(&f->x40,&bits[b],4);memcpy(&f->co_attrs.mid_walk_point,&bits[b],4);static Fighter before,expected;before=*f;
  NativeScriptValue flag,value,mid_walk;
  CHECK(nativeScriptBindingFighterObservations_Read(&c,NativeScriptBinding_MotionAllowsControllerSync,&flag,&diagnostic));
  CHECK(flag.type==NativeScriptType_Bool&&flag.as.u64==sync);
  CHECK(nativeScriptBindingFighterObservations_Read(&c,NativeScriptBinding_SpawnHorizontalOffset,&value,&diagnostic));
  CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&f->x40,4)&&!memcmp(f,&before,sizeof(*f)));pairs+=2;
  CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_GroundMidWalkPoint,NULL,0,&mid_walk,&diagnostic));CHECK(mid_walk.type==NativeScriptType_F32&&!memcmp(&mid_walk.as.f32,&bits[b],4));++pairs;
  f->x40=10.0f;expected=*f;memcpy(&expected.x40,&bits[b],4);
  CHECK(nativeScriptBindingFighterObservations_Write(&c,NativeScriptBinding_SpawnHorizontalOffset,&value,&diagnostic));
  CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
  f->x40=10.0f;
  CHECK(nativeScriptBindings_Write(&c,NativeScriptBinding_SpawnHorizontalOffset,NULL,0,&value,&diagnostic));
  CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
  if(self){
   CHECK(helper(target,"ObserveScalar"));mid_walk=field(target,"mid_walk");CHECK(mid_walk.type==NativeScriptType_F32&&!memcmp(&mid_walk.as.f32,&bits[b],4));flag=field(target,"sync");NativeScriptValue observed=field(target,"offset");
   CHECK(flag.type==NativeScriptType_Bool&&flag.as.u64==sync&&observed.type==NativeScriptType_F32&&!memcmp(&observed.as.f32,&bits[b],4));
   CHECK(!memcmp(f,&expected,sizeof(*f)));f->x40=10.0f;CHECK(helper(target,"ApplyOffset"));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
  }
  CHECK(nativeScriptItem_SetAttachmentOwner(caller,target));NativeScriptValue result;
  CHECK(nativeScriptItem_RunSignal(caller,name_key("ObserveOwner"),0,&result)==NativeScriptVM_Ok);++vm_calls;++receiver_calls;
  mid_walk=field(caller,"mid_walk");CHECK(mid_walk.type==NativeScriptType_F32&&!memcmp(&mid_walk.as.f32,&bits[b],4));
  flag=field(caller,"sync");NativeScriptValue observed=field(caller,"offset");
  CHECK(flag.type==NativeScriptType_Bool&&flag.as.u64==sync&&observed.type==NativeScriptType_F32&&!memcmp(&observed.as.f32,&bits[b],4));
  CHECK(!memcmp(f,&expected,sizeof(*f)));f->x40=10.0f;
  CHECK(nativeScriptItem_RunSignal(caller,name_key("WriteOwner"),0,&result)==NativeScriptVM_Ok);++vm_calls;++receiver_calls;
  CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;++cases;
 }
 NativeScriptValue integer={.type=NativeScriptType_I32,.as.i64=-3};static Fighter expected;expected=*f;expected.x40=-3.0f;
 CHECK(nativeScriptBindings_Write(&c,NativeScriptBinding_SpawnHorizontalOffset,NULL,0,&integer,&diagnostic));
 CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;++cases;
 if(self){f->x40=10.0f;CHECK(helper(target,"AssignOffset"));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;}
}
static HSD_GObj* item_actor(void)
{
 CHECK(item_count<4);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
 HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* data=&items[item_count++];
 *data=(Item){.entity=object,.kind=p->kind,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.xC4_article_data=&article,.x1C=200+item_count,.msid=-1};
 GObj_InitUserData(object,6,item_remove,data);CHECK(nativeScriptItem_Attach(object,p));return object;
}
#include "surface_pickup_cases.h"
#include "collision_position_cases.h"
#include "shield_weight_cases.h"
#include "hitlag_cases.h"
#include "engine_policy_cases.h"
#include "locomotion_attribute_cases.h"
#include "action_slide_cases.h"
#include "interruption_cases.h"
#include "floor_material_cases.h"
static void receiver_boundaries(HSD_GObj* caller,HSD_GObj* target,HSD_GObj* replacement)
{
 NativeScriptRuntimeContext c=context(caller);NativeScriptReceiverScope scope={.pool=nativeScriptRuntime_Receivers()};NativeScriptValue token;
 CHECK(nativeScriptItem_SetAttachmentOwner(caller,target));
 CHECK(nativeScriptReceiver_Push(scope.pool,&c,c.scene_epoch,&primary_bank.declarations,Ft_Kind_CustomFirst,nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope.frame,&diagnostic));
 CHECK(nativeScriptReceiver_Capture(&scope,&c,NativeScriptReceiver_AttachmentOwner,true,false,&token,&diagnostic));
 Fighter* f=target->user_data;f->x2070.x2071_b0_3=7;((Fighter*)replacement->user_data)->x2070.x2071_b0_3=12;
 CHECK(nativeScriptItem_SetAttachmentOwner(caller,replacement));NativeScriptValue result;
 CHECK(nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_ActionCategory,&token,1,&result,&diagnostic));
 CHECK(result.as.i64==7);++cases;
 NativeScriptValue offset={.type=NativeScriptType_F32,.as.f32=-2.5f};static Fighter expected;expected=*f;expected.x40=offset.as.f32;
 NativeScriptValue healing={.type=NativeScriptType_Bool,.as.u64=0};expected.x2222_b4=0;
 static Fighter replacement_before;replacement_before=*(Fighter*)replacement->user_data;
 CHECK(nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_SpawnHorizontalOffset,&token,1,&offset,&diagnostic));
 CHECK(nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_ItemsRejectHealingPickup,&token,1,&healing,&diagnostic));
 CHECK(!memcmp(f,&expected,sizeof(*f))&&!memcmp(replacement->user_data,&replacement_before,sizeof(replacement_before)));++cases;++pairs;
 NativeScriptValue missing;CHECK(nativeScriptItem_SetAttachmentOwner(caller,NULL));
 CHECK(nativeScriptReceiver_Capture(&scope,&c,NativeScriptReceiver_AttachmentOwner,false,false,&missing,&diagnostic));
 result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_MotionAllowsControllerSync,&missing,1,&result,&diagnostic));
 CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);
 CHECK(!nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_SpawnHorizontalOffset,&missing,1,&offset,&diagnostic));
 CHECK(!nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_ItemsRejectHealingPickup,&missing,1,&healing,&diagnostic));
 CHECK(!memcmp(f,&expected,sizeof(*f))&&!memcmp(replacement->user_data,&replacement_before,sizeof(replacement_before)));++cases;
 CHECK(nativeScriptItem_SetAttachmentOwner(caller,replacement));
 uint32_t generation=f->x8_spawnNum;FighterKind kind=f->kind;void* data=target->user_data;u16 classifier=target->classifier;
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(target,generation,&view));
 const NativeScriptStorageLayout* saved_layout=view.storage->layout;const NativeScriptStorageParameters* saved_parameters=view.storage->parameters;
 NativeScriptStorageLayout other_layout=*saved_layout;NativeScriptStorageParameters other_parameters=*saved_parameters;
 NativeScriptCompileBank* target_bank=&observation_packages[1].compiler.bank;bool valid=target_bank->valid;
 for(unsigned mode=0;mode<7;++mode){
  if(mode==0)f->x8_spawnNum++;
  if(mode==1)f->kind=Ft_Kind_Zelda;
  if(mode==2){substitute=*f;target->user_data=&substitute;}
  if(mode==3)target->classifier=HSD_GOBJ_CLASS_ITEM;
  if(mode==4)view.storage->layout=&other_layout;
  if(mode==5)view.storage->parameters=&other_parameters;
  if(mode==6)target_bank->valid=false;
  const uint32_t semantics[]={NativeScriptBinding_ActionCategory,NativeScriptBinding_FighterIncapacitated,NativeScriptBinding_MotionAllowsControllerSync,NativeScriptBinding_SpawnHorizontalOffset,NativeScriptBinding_ItemsRejectHealingPickup,NativeScriptBinding_HitlagActive};
  for(unsigned s=0;s<sizeof(semantics)/sizeof(semantics[0]);++s){result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
   CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|semantics[s],&token,1,&result,&diagnostic));
   CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);++cases;}
  expected=*f;
  CHECK(!nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_SpawnHorizontalOffset,&token,1,&offset,&diagnostic));
 CHECK(!nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_ItemsRejectHealingPickup,&token,1,&healing,&diagnostic));
  CHECK(!memcmp(f,&expected,sizeof(*f)));++cases;
  f->x8_spawnNum=generation;f->kind=kind;target->user_data=data;target->classifier=classifier;
  view.storage->layout=saved_layout;view.storage->parameters=saved_parameters;target_bank->valid=valid;
 }
 HSD_GObjFree(target);HSD_GObj* recycled=actor(1);CHECK(recycled==target);
 result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_ActionCategory,&token,1,&result,&diagnostic));
 CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);
 CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_HitlagActive,&token,1,&result,&diagnostic));
 CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);++cases;
 expected=*(Fighter*)recycled->user_data;
 CHECK(!nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_SpawnHorizontalOffset,&token,1,&offset,&diagnostic));
 CHECK(!nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_ItemsRejectHealingPickup,&token,1,&healing,&diagnostic));
 CHECK(!memcmp(recycled->user_data,&expected,sizeof(expected)));++cases;
 CHECK(nativeScriptReceiver_Capture(&scope,&c,NativeScriptReceiver_AttachmentOwner,true,false,&token,&diagnostic));
 HSD_GObjFree(caller);HSD_GObj* recycled_caller=item_actor();CHECK(recycled_caller==caller);
 result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_FighterIncapacitated,&token,1,&result,&diagnostic));
 CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);
 CHECK(!nativeScriptBindingReceiver_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_HitlagActive,&token,1,&result,&diagnostic));
 CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);++cases;
 expected=*(Fighter*)replacement->user_data;
 CHECK(!nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_SpawnHorizontalOffset,&token,1,&offset,&diagnostic));
 CHECK(!nativeScriptBindingReceiver_Write(&scope,&c,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_ItemsRejectHealingPickup,&token,1,&healing,&diagnostic));
 CHECK(!memcmp(replacement->user_data,&expected,sizeof(expected)));++cases;
 CHECK(nativeScriptReceiver_Pop(scope.pool,scope.frame,&diagnostic));
}
static void rejected(NativeScriptRuntimeContext* c)
{
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=0xabcdef},r=sentinel;
 CHECK(!nativeScriptBindingFighterObservations_Read(c,NativeScriptBinding_MovementSpecialLandingLag,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingFighterObservations_Read(c,NativeScriptBinding_EngineDebugLevel,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingFighterObservations_Read(c,NativeScriptBinding_ActionCategory,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingFighterObservations_Read(c,NativeScriptBinding_FighterIncapacitated,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingFighterObservations_Read(c,NativeScriptBinding_MotionAllowsControllerSync,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingFighterObservations_Read(c,NativeScriptBinding_SpawnHorizontalOffset,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingFighterObservations_Read(c,NativeScriptBinding_GroundMidWalkPoint,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingFighterObservations_Read(c,NativeScriptBinding_FloorFriction,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingHitlag_Read(c,NativeScriptBinding_HitlagActive,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 CHECK(!nativeScriptBindingFighterItems_Read(c,NativeScriptBinding_ItemsRejectHealingPickup,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));
 NativeScriptValue healing={.type=NativeScriptType_Bool};CHECK(!nativeScriptBindingFighterItems_Write(c,NativeScriptBinding_ItemsRejectHealingPickup,&healing,&diagnostic));
 NativeScriptValue value={.type=NativeScriptType_F32,.as.f32=2.5f};
 CHECK(!nativeScriptBindingFighterObservations_Write(c,NativeScriptBinding_SpawnHorizontalOffset,&value,&diagnostic));
 CHECK(!nativeScriptBindingFighterObservations_Invoke(c,NativeScriptBinding_StatsCountAsThrownItem,NULL,0,&r,&diagnostic));CHECK(!memcmp(&r,&sentinel,sizeof(r)));++cases;
}
static void negatives(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},r=sentinel,arg={.type=NativeScriptType_I32};static Fighter before;before=*f;
 CHECK(!nativeScriptBindingFighterObservations_Invoke(&c,NativeScriptBinding_StatsCountAsThrownItem,&arg,1,&r,&diagnostic)&&r.as.u64==99&&!memcmp(f,&before,sizeof(*f)));
 CHECK(!nativeScriptBindingFighterObservations_Invoke(&c,NativeScriptBinding_MovementSpecialLandingLag,NULL,0,&r,&diagnostic)&&r.as.u64==99);
 CHECK(!nativeScriptBindingFighterObservations_Read(&c,NativeScriptBinding_StatsCountAsThrownItem,&r,&diagnostic)&&r.as.u64==99);CHECK(!nativeScriptBindingFighterObservations_Read(&c,NativeScriptBinding_EngineDebugLevel,NULL,&diagnostic));cases+=4;
 const unsigned invalid_types[]={NativeScriptType_Void,NativeScriptType_Bool,NativeScriptType_I32,NativeScriptType_I64,NativeScriptType_U32,NativeScriptType_U64,NativeScriptType_Object};
 for(unsigned i=0;i<sizeof(invalid_types)/sizeof(invalid_types[0]);++i){NativeScriptValue invalid={.type=invalid_types[i]};
  CHECK(!nativeScriptBindingFighterObservations_Write(&c,NativeScriptBinding_SpawnHorizontalOffset,&invalid,&diagnostic));CHECK(!memcmp(f,&before,sizeof(*f)));++cases;}
 CHECK(!nativeScriptBindingFighterObservations_Write(&c,NativeScriptBinding_SpawnHorizontalOffset,NULL,&diagnostic));
 NativeScriptValue offset={.type=NativeScriptType_F32,.as.f32=2.5f};
 CHECK(!nativeScriptBindingFighterObservations_Write(&c,NativeScriptBinding_MotionAllowsControllerSync,&offset,&diagnostic));
 CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_MotionAllowsControllerSync,NULL,0,&offset,&diagnostic));
 CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_SpawnHorizontalOffset,&arg,1,&offset,&diagnostic));
 CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_SpawnHorizontalOffset,NULL,0,NULL,&diagnostic));
 CHECK(!memcmp(f,&before,sizeof(*f)));cases+=5;
 rejected(NULL);c.source_class=NativeCallbackSource_Item;rejected(&c);c=context(o);++c.generation;rejected(&c);c=context(o);++c.scene_epoch;rejected(&c);c=context(o);
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));
 NativeScriptStorageLayout other_layout=primary_bank.storage_layout;NativeScriptStorageParameters other_parameters=primary_bank.storage_parameters;
 view.storage->layout=&other_layout;rejected(&c);view.storage->layout=&primary_bank.storage_layout;
 view.storage->parameters=&other_parameters;rejected(&c);view.storage->parameters=&primary_bank.storage_parameters;
 NativeFighterLifetime retained;CHECK(nativeFighterLifetime_Capture(f,&retained));
 substitute=*f;o->user_data=&substitute;CHECK(!nativeFighterLifetime_Current(&retained));substitute.gobj=NULL;rejected(&c);o->user_data=f;o->classifier=HSD_GOBJ_CLASS_ITEM;rejected(&c);o->classifier=HSD_GOBJ_CLASS_FIGHTER;f->gobj=NULL;rejected(&c);f->gobj=o;
 CHECK(!helper(o,"Fault"));rejected(&c);CHECK(!memcmp(f,&before,sizeof(*f)));
}
static void compile_negatives(void)
{
 const char* invalid[]={"article Probe states 1:\n    on created:\n        attachment_owner.hitlag.active = true\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        let bool flag = attachment_owner.hitlag.active()\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        let bool flag = hitlag.active\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        let int linked = attachment_owner.hitlag.linked_id\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        attachment_owner.hitlag.unlink()\n    state 0 anim -1:\n","func Invalid():\n    collision.position.x = 1.0\n","func Invalid():\n    let float p = collision.position.x()\n","article Probe states 1:\n    on created:\n        collision.position.z = 1.0\n    state 0 anim -1:\n","func Invalid():\n    items.reject_healing_pickup(true)\n","func Invalid():\n    items.reject_healing_pickup = 1\n","article Probe states 1:\n    on created:\n        floor.normal.x = 1.0\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        let float n = floor.normal.x()\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        items.reject_healing_pickup = true\n    state 0 anim -1:\n","func Invalid():\n    ground.mid_walk_point = 1.0\n","func Invalid():\n    let float v = ground.mid_walk_point()\n","func Invalid():\n    motion.allows_controller_sync = true\n","func Invalid():\n    spawn.horizontal_offset = true\n","func Invalid():\n    let float offset = spawn.horizontal_offset()\n","article Probe states 1:\n    on created:\n        spawn.horizontal_offset = 1.0\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        attachment_owner.motion.allows_controller_sync = true\n    state 0 anim -1:\n","func Invalid():\n    movement.special_landing_lag = 1.0\n","func Invalid():\n    engine.debug_level = 3\n","func Invalid():\n    action.category = 2\n","func Invalid():\n    fighter.incapacitated = true\n","func Invalid():\n    state.n = action.category()\nstate int n\n","func Invalid():\n    stats.count_as_thrown_item(1)\n","article Probe states 1:\n    on created:\n        state.n = engine.debug_level\n    state int n\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        item.state.n = action.category\n    state int n\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        attachment_owner.action.category = 3\n    state 0 anim -1:\n","article Probe states 1:\n    on created:\n        attachment_owner.fighter.incapacitated = true\n    state 0 anim -1:\n"};
 NativeScriptCompileBank scratch;static NativeScriptCompileBank accepted_bank;const NativeScriptCompileBank* accepted=&accepted_bank;
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 for(unsigned n=0;n<sizeof(invalid)/sizeof(invalid[0]);++n){configure(0);scratch=primary_bank;NativeScriptCompileRequest request={.source=invalid[n],.source_bytes=(uint32_t)strlen(invalid[n]),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};CHECK(!nativeScriptCompile_Build(&scratch,&request,&accepted,&diagnostic));CHECK(accepted==&accepted_bank);++cases;}
}
#include "statistics_cases.h"

int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 HSD_GObj* owned=actor(0),*foreign=actor(1),*raw=actor(-1),*caller=item_actor();
 interruption_cases(owned,raw);
 statistics_cases(owned);
 engine_policy_cases(owned,raw);
 locomotion_attribute_cases(owned,raw);
 action_metadata_cases(owned,raw);
 slide_velocity_cases(caller,owned);
 floor_material_cases(owned,caller);
 shield_weight_cases(owned,foreign,raw,caller);
 positives(owned,true);positives(raw,false);metadata_positives(owned,caller,true);metadata_positives(foreign,caller,false);metadata_positives(raw,caller,false);
 hitlag_cases(owned,foreign,raw,caller);
 collision_position_cases(owned,foreign,raw,caller);
 surface_pickup_cases(owned,foreign,raw,caller);
 scalar_positives(owned,caller,true);scalar_positives(foreign,caller,false);scalar_positives(raw,caller,false);
 receiver_boundaries(caller,foreign,raw);HSD_GObjFree(caller);HSD_GObjFree(foreign);negatives(owned);HSD_GObjFree(owned);
 NativeScriptRuntimeContext stale=context(raw);HSD_GObjFree(raw);HSD_GObj* replacement=actor(-1);CHECK(replacement==raw);rejected(&stale);HSD_GObjFree(replacement);
 HSD_GObj* detached=actor(0);NativeScriptRuntimeContext retired=context(detached);NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(detached,retired.generation,&view));nativeScriptStorage_Destroy(view.storage);rejected(&retired);HSD_GObjFree(detached);
 CHECK(nativeGObjPool_RetireScene());compile_negatives();compile_observation_negatives();printf("{\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u,\"receiver_vm_calls\":%u,\"heavy_pairs\":%u,\"shield_pairs\":%u,\"shield_vm_cases\":%u,\"observation_rejections\":%u}\n",checks,cases,pairs,vm_calls,receiver_calls,heavy_pairs,shield_pairs,shield_vm_cases,observation_rejections);return 0;
}
