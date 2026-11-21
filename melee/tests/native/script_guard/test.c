#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_guard.h>
#include <melee/ft/native_script_binding_jump.h>
#include <melee/ft/native_script_binding_common_properties.h>
#include <melee/ft/native_script_binding_effect.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/cm/camera.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/controller.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftCommon/ftCo_Guard.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <sysdolphin/baselib/aobj.h>
#include <melee/ft/kinds/ftYoshi/ftyoshiguard.h>
#include <melee/pl/plbonuslib.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum { N=2048,I=8192,F=64,V=128 };
typedef struct Buffers {
 char source[16384];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
 NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
 NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];
 NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
 NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} Buffers;
NativeSourcePackage guard_packages[3];static Buffers buffers[2];static NativeScriptCompileBank scratch;
#define bank guard_packages[0].compiler.bank
static Fighter fighters[192],alternate;static FighterBone parts[MAX_FT_PARTS];static HSD_JObj joints[MAX_FT_PARTS];
static ftCommonData common;ftCommonData* p_ftCommonData=&common;
static struct ftData_x8 aux;
HSD_JObjInfo hsdJObj;static MotionState motions[342];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static unsigned checks,cases,pairs,vm_calls,allocated,mutation,admission_mutation,admission_calls,guard_completed_tails,stock_reactions,audio_calls,reference_completed_tails;
static unsigned admission_trigger;
static float guard_hitlag_damage,reference_hitlag_damage;static NativeScriptDiagnostic diagnostic;static HSD_GObj* observed,*replacement;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"guard %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void configure(NativeScriptCompileBank* b,Buffers* p)
{
 *b=(NativeScriptCompileBank){.source_storage=p->source,.source_capacity=sizeof(p->source),.tree={.nodes=p->nodes,.capacity=N},.expressions={.nodes=p->ir,.node_capacity=I,.arguments=p->arguments,.argument_capacity=I},.declarations={.functions=p->functions,.function_capacity=F,.parameters=p->parameters,.parameter_capacity=V,.values=p->values,.value_capacity=V},.locals=p->locals,.local_capacity=V,.instructions=p->instructions,.instruction_capacity=I,.function_code=p->codes,.function_code_capacity=F,.execution_functions=p->entries,.execution_function_capacity=F,.graph=p->graph,.graph_capacity=F,.storage_layout={.fields=p->fields,.field_capacity=V,.owners=p->owners,.owner_capacity=V},.parameter_slots=p->slots,.parameter_slot_capacity=V};
}
static const char* source=
 "state float reflected_direction\nfunc ReflectDirection():\n    state.reflected_direction = reflector.hit_direction\nstate bool reflected_keep_owner\nfunc ReflectOwner(bool keep):\n    reflector.keep_owner = keep\n    state.reflected_keep_owner = reflector.keep_owner\n"
 "fighter:\n    behavior independent\nstate int seen\nstate float facing\nstate int tail\nstate int zero\nstate bool active\nstate bool reactive\nstate bool skip\nstate bool front\nstate bool filtered\nstate float melee\nstate float article\n"
 "func Reaction():\n    state.seen = guard.hit_damage\n    state.facing = guard.hit_facing\n    guard.front_only = true\n    guard.melee_hitlag = 3.5\n    position.x = 7.0\n    state.tail = state.tail + 1\n"
 "func Fault():\n    state.seen = 1 / state.zero\n    state.tail = state.tail + 1\n"
 "func Wrong() -> int:\n    return 3\nfunc WithArgument(int n):\n    state.seen = n\n"
 "func Create(int joint, float radius):\n    guard.create(joint, -0.0, 2.25, -3.5, radius, Reaction)\n"
 "func Clear():\n    guard.create(0, 1.0, 2.0, 3.0, 4.0, None)\n"
 "func Refresh():\n    guard.update_position()\n"
 "func Fields():\n    guard.active = false\n    guard.reactive = true\n    guard.skip_contact_details = true\n    guard.front_only = false\n    guard.filtered_attacks_only = true\n    guard.melee_hitlag = -0.0\n    guard.article_hitlag = 8.25\n"
 "func Query():\n    state.active = guard.active\n    state.reactive = guard.reactive\n    state.skip = guard.skip_contact_details\n    state.front = guard.front_only\n    state.filtered = guard.filtered_attacks_only\n    state.melee = guard.melee_hitlag\n    state.article = guard.article_hitlag\n    state.seen = guard.hit_damage\n    state.facing = guard.hit_facing\n"
 "state float shared_elapsed\nstate bool shared_release_requested\nstate float shared_minimum_hold_frames\nstate float shared_reflect_frames\nstate float shared_powershield_frames\nstate int shared_input_lock_frames\nstate int shared_interrupt_mode\nstate int shared_delayed_action_frames\nstate bool shared_volume_enabled\nstate bool shared_new_powershield\nstate bool shared_reflect_window\nstate bool shared_powershield_window\nstate float shared_health\nstate float shared_light\nstate int shared_bone\nstate float shared_size\nstate float common_start_health\nstate float common_minimum_hold_duration\nstate float common_reflect_duration\nstate float common_reflect_radius\nstate float common_reflect_damage_multiplier\nstate float common_reflect_speed_multiplier\nstate float common_powershield_duration\nfunc SharedWrite(float timer, int lock, int interrupt, int delayed, bool flag):\n    guard.elapsed = timer\n    guard.release_requested = flag\n    guard.minimum_hold_frames = timer + 1\n    guard.reflect_frames = timer + 2\n    guard.powershield_frames = timer + 3\n    guard.input_lock_frames = lock\n    guard.interrupt_mode = interrupt\n    guard.delayed_action_frames = delayed\n    guard.volume_enabled = not flag\n    guard.new_powershield = flag\n    guard.reflect_window = not flag\n    guard.powershield_window = flag\n    shield.health = timer\nfunc SharedRead():\n    state.shared_elapsed = guard.elapsed\n    state.shared_release_requested = guard.release_requested\n    state.shared_minimum_hold_frames = guard.minimum_hold_frames\n    state.shared_reflect_frames = guard.reflect_frames\n    state.shared_powershield_frames = guard.powershield_frames\n    state.shared_input_lock_frames = guard.input_lock_frames\n    state.shared_interrupt_mode = guard.interrupt_mode\n    state.shared_delayed_action_frames = guard.delayed_action_frames\n    state.shared_volume_enabled = guard.volume_enabled\n    state.shared_new_powershield = guard.new_powershield\n    state.shared_reflect_window = guard.reflect_window\n    state.shared_powershield_window = guard.powershield_window\n    state.shared_health = shield.health\n    state.shared_light = shield.light_amount\n    state.shared_bone = shield.bone\n    state.shared_size = shield.initial_size\nfunc Tuning():\n    state.common_start_health = shield.start_health\n    state.common_minimum_hold_duration = shield.minimum_hold_duration\n    state.common_reflect_duration = shield.reflect_duration\n    state.common_reflect_radius = shield.reflect_radius\n    state.common_reflect_damage_multiplier = shield.reflect_damage_multiplier\n    state.common_reflect_speed_multiplier = shield.reflect_speed_multiplier\n    state.common_powershield_duration = shield.powershield_duration\n"
 "func Capsule(int slot, int joint, int height, bool grabbable, float scale):\n    hurtboxes.define(slot, joint, height, grabbable, -0.0, 2.25, -3.5, 4.75, -6.125, 8.5, scale)\n"
 "func ShieldCapsule():\n    hurtboxes.set_all(Hurt.intangible)\n    hurtboxes.define(0, shield.bone, Hurt.height_mid, true, 0, 0, 0, 0, 0, 0, 1)\n"
 "func CapsuleHeights():\n    hurtboxes.define(0, 0, Hurt.height_low, false, 0, 0, 0, 0, 0, 0, 1)\n    hurtboxes.define(1, 1, Hurt.height_high, true, 0, 0, 0, 0, 0, 0, 1)\n"
 "func Async(int mode, int joint, float scalar):\n    if mode == 0: effects.spawn_async_position_on_part(1230, joint)\n    else: effects.spawn_async_position_scalar_on_part(1231, joint, scalar)\n"
 "func AsyncPair():\n    effects.spawn_async_position_scalar_on_part(1231, 43, 1.25)\n    effects.spawn_async_position_scalar_on_part(1231, 104, 2.5)\n"
 "func RestoreEffect():\n    effects.spawn_async_position_scalar_on_part(1231, 43, model.effect_scale)\n"
 "state int transition_mode\nstate bool transition_special\nstate int transition_mask\nstate int transition_calls\nstate bool transition_fault\n"
 "state float query_size\nstate bool query_custom\nstate bool query_alpha\nstate bool query_mutate\nstate bool query_fault\nstate bool depleted\nstate bool reflection_active\nfunc ReflectionStatus():\n    state.reflection_active = reflector.active\n"
 "func GuardOperations(int op):\n    if op == 0: guard.scale_joint()\n    elif op == 1: guard.install_shield()\n    elif op == 2: guard.install_reflector()\n    elif op == 3: guard.consume_release_input()\n    elif op == 4: state.depleted = guard.drain()\n    elif op == 5: guard.tick_powershield()\n    elif op == 6: guard.track_sdi()\n    elif op == 7: guard.enter_hold()\n    else: guard.enter_release()\n"
 "state bool input_accepted\nstate int roll_events\nstate int dodge_events\nstate bool entry_fault\n"
 "func GuardFlow(int op):\n    if op == 0: state.input_accepted = controls.try_powershield()\n    elif op == 1: state.input_accepted = controls.try_guard_item_throw()\n    elif op == 2: state.input_accepted = controls.try_spotdodge()\n    elif op == 3: state.input_accepted = controls.try_guard_spotdodge()\n    elif op == 4: state.input_accepted = controls.try_roll()\n    elif op == 5: state.input_accepted = controls.try_guard_platform_drop()\n    elif op == 6: controls.guard_reflect_actions()\n    elif op == 7: physics.guard_motion()\n    elif op == 8: physics.guard_hit_motion()\n    elif op == 9: collision.guard_standard()\n    else: collision.guard_hit()\n"
 "action ShieldPolicy:\n    on shield_transition:\n        if state.transition_fault: state.seen = 1 / state.zero\n        state.transition_mode = event.mode\n        state.transition_special = event.special_hit\n        if event.mode == Shield.start: state.transition_mask = 1\n        elif event.mode == Shield.hold: state.transition_mask = 2\n        elif event.mode == Shield.release: state.transition_mask = 4\n        elif event.mode == Shield.hit: state.transition_mask = 8\n        elif event.mode == Shield.reflect_continue: state.transition_mask = 16\n        elif event.mode == Shield.reflect_start: state.transition_mask = 32\n        position.x = float(event.mode)\n        state.transition_calls = state.transition_calls + 1\n"
 "    on shield_scale:\n        if state.query_fault: state.seen = 1 / state.zero\n        if state.query_mutate: position.x = 3.0\n        if state.query_custom: return state.query_size\n        return shield.initial_size\n"
 "    on shield_alpha:\n        if state.query_fault: state.seen = 1 / state.zero\n        if state.query_mutate: position.x = 3.0\n        return state.query_alpha\n"
 "    on reflect_hit:\n        state.tail = state.tail + 1\n"
 "    on roll_enter:\n        if state.entry_fault: state.seen = 1 / state.zero\n        position.x = 0.0\n        state.roll_events = state.roll_events + 1\n"
 "    on spotdodge_enter:\n        if state.entry_fault: state.seen = 1 / state.zero\n        position.x = 0.0\n        state.dodge_events = state.dodge_events + 1\n"
 "state int fall_selector\nstate bool reverse_pose\n"
 "func StepJump():\n    jump.update_animation()\n    state.fall_selector = motion.fall_animation\n"
 "func ReadFall():\n    state.fall_selector = motion.fall_animation\n"
 "func WriteFall(int value):\n    motion.fall_animation = value\n"
 "func ReversePose(bool enabled):\n    damage.reverse_knockdown_pose = enabled\n    state.reverse_pose = damage.reverse_knockdown_pose\n"
 "action Placeholder:\n    on tick:\n        pass\n";
static void prepare(void)
{
 configure(&bank,&buffers[0]);NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(accepted==&bank);
 NativeSourcePackage* p=&guard_packages[0];CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->ready=p->assets.ready=true;p->target=Ft_Kind_CustomFirst;p->assets.facts.moves=layout;p->assets.facts.joints=MAX_FT_PARTS;
 p->assets.parts.parts_num=MAX_FT_PARTS;p->assets.parts.part_to_joint=p->assets.part_to_joint;p->assets.parts.joint_to_part=p->assets.joint_to_part;p->assets.motions=motions;p->assets.character.data.x8=&aux;aux.x11=43;
 tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 hsdJObj.parent.parent.head.flags=1;hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj);
 for(unsigned n=0;n<MAX_FT_PARTS;++n){parts[n].joint=&joints[n];joints[n].object.parent.class_info=&hsdJObj.parent.parent;}
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);
}
static void remove_data(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* actor(void)
{
 CHECK(allocated<192);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);Fighter* f=&fighters[allocated++];
 *f=(Fighter){.gobj=o,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,.x8_spawnNum=100+allocated,.ft_data=&guard_packages[0].assets.character.data,.parts=parts};
 GObj_InitUserData(o,4,remove_data,f);CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static uint32_t function(const char* name)
{for(unsigned n=0;n<bank.declarations.function_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[n].name,name))return n;CHECK(false);return 0;}
static NativeScriptValue state(HSD_GObj* o,const char* name)
{
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));
 for(unsigned n=0;n<bank.declarations.value_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.values[n].name,name)){NativeScriptValue result;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;}CHECK(false);return(NativeScriptValue){0};
}
static uint8_t* state_address(HSD_GObj* o,const char* name)
{
 NativeScriptReceiverView v;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&v));
 for(unsigned n=0;n<v.storage->layout->field_count;++n){const NativeScriptStorageField* f=&v.storage->layout->fields[n];if(!f->owner&&nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.values[n].name,name))return v.storage->state+f->offset;}CHECK(false);return NULL;
}
static bool helper(HSD_GObj* o,const char* name,const NativeScriptValue* values,uint32_t count)
{
 NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};++vm_calls;NativeScriptVMStatus status=nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,function(name),values,count,&result,&diagnostic);
 if(status==NativeScriptVM_Ok)CHECK(result.type==NativeScriptType_Void);else CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);return status==NativeScriptVM_Ok;
}
void guard_capture_probe(NativeScriptRuntimeContext* c)
{if(c->source==observed&&admission_mutation&&++admission_calls==(admission_mutation==2?(admission_trigger?admission_trigger:2u):1u)){unsigned m=admission_mutation;admission_mutation=0;if(m==1)++((Fighter*)observed->user_data)->x8_spawnNum;if(m==2)++bank.storage_parameters.identity;}}
void guard_write_probe(NativeScriptRuntimeContext* c)
{
 if(c->source!=observed||!mutation)return;unsigned mode=mutation;mutation=0;
 if(mode==1)nativeScriptFighter_Detach(observed);
 if(mode==2)HSD_GObjFree(observed);
 if(mode==3){HSD_GObjFree(observed);replacement=actor();}
 if(mode==4)++((Fighter*)observed->user_data)->x8_spawnNum;
 if(mode==5){alternate=*(Fighter*)observed->user_data;observed->user_data=&alternate;}
 if(mode==6)observed->classifier=HSD_GOBJ_CLASS_ITEM;
 if(mode==7)++bank.storage_layout.identity;
 if(mode==8)++bank.storage_parameters.identity;
 if(mode==9)((Fighter*)observed->user_data)->parts=NULL;
 if(mode==10)parts[aux.x11].joint=NULL;
 if(mode==11)++aux.x11;
 if(mode==12)ftPartsTable=NULL;
 if(mode==13)--guard_packages[0].assets.parts.parts_num;
}
static unsigned drain_breaks,drain_sounds,drain_statistics,alpha_calls;static u8 last_alpha;
void ftCo_80098B20(Fighter_GObj* object){++stock_reactions;++drain_breaks;NativeScriptRuntimeContext c={.source=object};guard_write_probe(&c);}
void ft_PlaySFX(Fighter* f,s32 id,u8 volume,u8 pan){(void)f;CHECK((id==0x82||id==129||id==190115)&&volume==0x7f&&pan==0x40);++audio_calls;if(id==129)++drain_sounds;}
void pl_8003E0E8(int player,int flag){(void)player;(void)flag;++drain_statistics;}
void efLib_SetParamAlpha(HSD_GObj* o,u8 alpha){CHECK(o&&nativeScriptFighter_Status(o)==NativeScriptVM_Ok);++alpha_calls;last_alpha=alpha;}
#include "native_guard.inc"
#include "native_common_guard.inc"
#include "guard_flow_leaves.h"
#include "transition_leaves.h"
#include "native_joint_dirty.inc"
#include "native_shield_operations.inc"
#include "native_shield_operations_reference.inc"
#include "native_guard_transitions.inc"
#pragma clang diagnostic push
// Pinned native bodies retain GameCube stack-padding locals.
#pragma clang diagnostic ignored "-Wunused-variable"
#include "native_guard_flow.inc"
#include "native_guard_flow_reference.inc"
#pragma clang diagnostic push
// Retain signed loop variables in the unchanged native animation oracle.
#pragma clang diagnostic ignored "-Wsign-compare"
#include "native_jump_policy.inc"
#include "native_jump_policy_reference.inc"
#pragma clang diagnostic pop
#pragma clang diagnostic pop
#include "native_hurtbox.inc"
#include "native_hurtbox_reference.inc"
HSD_ObjAllocData efAsync_AllocData;
EF_EffectUsage efLib_Usage;
static EF_QueuedEffect async_packets[16];static bool async_used[16],async_allocation_failure;
static unsigned async_live,async_deliveries,async_samples;
static Vec3 async_position;static float async_scalar;static int async_effect;
static HSD_GObjProc async_process;
void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{
 CHECK(data==&efAsync_AllocData);if(async_allocation_failure)return NULL;
 for(unsigned n=0;n<16;++n)if(!async_used[n]){async_used[n]=true;++async_live;memset(&async_packets[n],0,sizeof(async_packets[n]));return&async_packets[n];}abort();
}
void HSD_ObjFree(HSD_ObjAllocData* data,void* packet)
{
 CHECK(data==&efAsync_AllocData);for(unsigned n=0;n<16;++n)if(packet==&async_packets[n]){CHECK(async_used[n]&&async_live);async_used[n]=false;--async_live;return;}abort();
}
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result)
{CHECK(joint&&!offset&&result);++async_samples;*result=joint->translate;}
void Camera_RequestQuake(CmQuakeKind kind,Vec3* position){(void)kind;(void)position;abort();}
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj* object,...)
{
 CHECK(object&&object->classifier==HSD_GOBJ_CLASS_FIGHTER&&(effect==1230||effect==1231));
 va_list values;va_start(values,object);async_position=*va_arg(values,Vec3*);
 async_scalar=effect==1231?*va_arg(values,float*):0;async_effect=effect;++async_deliveries;va_end(values);return(EF_SpawnResult){0};
}
#include "native_async.inc"
#include "native_guard_reference.inc"
#include "native_guard_schedule.inc"
static void callback_raw(HSD_GObj* o){((Fighter*)o->user_data)->x221B_b3=1;}
static void callback_reference(HSD_GObj* o){Fighter* f=o->user_data;f->x221B_b3=1;f->shield_unk0=3.5f;f->cur_pos.x=7;}
static void construction(HSD_GObj* o)
{
 Fighter* f=o->user_data;static Fighter before,expected;
 const int indices[]={-1,0,43,104,MAX_FT_PARTS-1,MAX_FT_PARTS,INT32_MAX};const float radii[]={-0.0f,-1,4.5f,INFINITY,-INFINITY,NAN};
 for(unsigned b=0;b<7;++b)for(unsigned r=0;r<6;++r){f->x221B_b0=0;f->x221B_b1=f->x221B_b2=f->x221B_b3=f->x221B_b4=1;f->shield_hit.skip_update_pos=true;f->shield_unk0=-3.5f;f->shield_unk1=5.25f;f->x19A4=77;f->specialn_facing_dir=-1;f->shield_hit_cb=NULL;before=*f;bool old_callback;CHECK(nativeScriptFighter_CallbackMatches(o,NativeScriptFighterCallback_ShieldHit,function("Reaction"),&old_callback));
  ShieldDesc shape={.bone=indices[b],.pos={-0.0f,2.25f,-3.5f},.radius=radii[r]};if(indices[b]>=0&&indices[b]<MAX_FT_PARTS&&isfinite(radii[r])&&radii[r]>=0)guard_reference_create(o,&shape,NULL);expected=*f;*f=before;
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=indices[b]},{.type=NativeScriptType_F32,.as.f32=radii[r]}};
  CHECK(helper(o,"Create",args,2));CHECK(!memcmp(f,&expected,sizeof(*f)));bool matching;CHECK(nativeScriptFighter_CallbackMatches(o,NativeScriptFighterCallback_ShieldHit,function("Reaction"),&matching));CHECK(matching==(indices[b]>=0&&indices[b]<MAX_FT_PARTS&&isfinite(radii[r])&&radii[r]>=0?true:old_callback));++pairs;++cases;
 }
 CHECK(helper(o,"Create",(NativeScriptValue[]){{.type=NativeScriptType_I32,.as.i64=104},{.type=NativeScriptType_F32,.as.f32=2}},2));CHECK(nativeScriptFighter_CallbackBound(o,NativeScriptFighterCallback_ShieldHit));
 CHECK(helper(o,"Clear",NULL,0));CHECK(!nativeScriptFighter_CallbackBound(o,NativeScriptFighterCallback_ShieldHit)&&!f->shield_hit_cb);++cases;
 CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_ShieldHit,function("Reaction")));ShieldDesc shape={.bone=0,.radius=1};ftColl_8007B1B8(o,&shape,callback_raw);CHECK(!nativeScriptFighter_CallbackBound(o,NativeScriptFighterCallback_ShieldHit)&&f->shield_hit_cb==callback_raw);++cases;
 before=*f;f->shield_hit.skip_update_pos=true;guard_reference_update(o);expected=*f;*f=before;f->shield_hit.skip_update_pos=true;CHECK(helper(o,"Refresh",NULL,0));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;++cases;
}
static void fields(HSD_GObj* o)
{
 Fighter* f=o->user_data;CHECK(helper(o,"Fields",NULL,0));CHECK(!f->x221B_b0&&f->x221B_b1&&f->x221B_b2&&!f->x221B_b3&&f->x221B_b4);CHECK(signbit(f->shield_unk0)&&f->shield_unk1==8.25f);
 f->x19A4=INT32_MIN;f->specialn_facing_dir=-0.0f;CHECK(helper(o,"Query",NULL,0));CHECK(!state(o,"active").as.u64&&state(o,"reactive").as.u64&&state(o,"skip").as.u64&&!state(o,"front").as.u64&&state(o,"filtered").as.u64);CHECK(state(o,"seen").as.i64==INT32_MIN&&signbit(state(o,"facing").as.f32));CHECK(signbit(state(o,"melee").as.f32)&&state(o,"article").as.f32==8.25f);++cases;
 NativeScriptRuntimeContext c=context(o);NativeScriptValue invalid={.type=NativeScriptType_Bool,.as.u64=2};CHECK(!nativeScriptBindingGuard_Write(&c,NativeScriptBinding_GuardActive,&invalid,&diagnostic)&&!f->x221B_b0);CHECK(!nativeScriptBindingGuard_Write(&c,NativeScriptBinding_GuardHitDamage,&invalid,&diagnostic));++cases;
 const uint32_t float_ids[]={NativeScriptBinding_GuardMeleeHitlag,NativeScriptBinding_GuardArticleHitlag};const uint32_t bits[]={0x80000000,0x7fc12345,0x7f800000,0xff800000};
 for(unsigned id=0;id<2;++id)for(unsigned b=0;b<4;++b){NativeScriptValue value={.type=NativeScriptType_F32},out;memcpy(&value.as.f32,&bits[b],4);CHECK(nativeScriptBindingGuard_Write(&c,float_ids[id],&value,&diagnostic));CHECK(nativeScriptBindingGuard_Read(&c,float_ids[id],&out,&diagnostic));CHECK(!memcmp(&value.as.f32,&out.as.f32,4));++cases;}
}

static void reflector_ownership(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);
 static Fighter before,expected;
 for(unsigned mask=0;mask<8;++mask)for(unsigned keep=0;keep<2;++keep) {
  f->reflecting=mask&1;f->x2218_b4=(mask>>1)&1;f->x2218_b5=(mask>>2)&1;
  before=*f;expected=before;expected.x2218_b4=keep;
  NativeScriptValue argument={.type=NativeScriptType_Bool,.as.u64=keep};
  CHECK(helper(o,"ReflectOwner",&argument,1));
  CHECK(!memcmp(f,&expected,sizeof(*f))&&state(o,"reflected_keep_owner").as.u64==keep);
  ++pairs;++vm_calls;++cases;
 }
 before=*f;
 NativeScriptValue invalid={.type=NativeScriptType_Bool,.as.u64=2};
 CHECK(!nativeScriptBindingGuard_Write(&c,NativeScriptBinding_ReflectorKeepOwner,&invalid,&diagnostic));
 CHECK(!memcmp(f,&before,sizeof(*f)));++cases;
 invalid=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};
 CHECK(!nativeScriptBindingGuard_Write(&c,NativeScriptBinding_ReflectorKeepOwner,&invalid,&diagnostic));
 CHECK(!memcmp(f,&before,sizeof(*f)));++cases;
 for(unsigned fault=0;fault<3;++fault) {
  NativeScriptRuntimeContext bad=c;
  if(fault==0)++bad.generation;
  else if(fault==1)bad.source=NULL;
  else ++bad.scene_epoch;
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  NativeScriptValue flag={.type=NativeScriptType_Bool,.as.u64=!f->x2218_b4};
  CHECK(!nativeScriptBindingGuard_Read(&bad,NativeScriptBinding_ReflectorKeepOwner,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
  CHECK(!nativeScriptBindingGuard_Write(&bad,NativeScriptBinding_ReflectorKeepOwner,&flag,&diagnostic));
  CHECK(!memcmp(f,&before,sizeof(*f)));++cases;
 }
}

static void shared_fields(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);
 const int32_t modes[]={INT32_MIN,-1,0,1,INT32_MAX};
 for(unsigned n=0;n<5;++n){
  NativeScriptValue args[]={{.type=NativeScriptType_F32,.as.f32=n?12.5f:-0.0f},{.type=NativeScriptType_I32,.as.i64=modes[n]},{.type=NativeScriptType_I32,.as.i64=modes[(n+1)%5]},{.type=NativeScriptType_I32,.as.i64=modes[(n+2)%5]},{.type=NativeScriptType_Bool,.as.u64=n&1}};
  CHECK(helper(o,"SharedWrite",args,5));
  // The common native layout and Yoshi's shared view must agree, including the
  // three word-sized input slots that must not drift on a native 64-bit build.
  CHECK(!memcmp(&f->mv.ys.guard.x0,&args[0].as.f32,4)&&f->mv.ys.guard.xC==(bool)(n&1));
  CHECK(f->mv.ys.guard.x1C==modes[n]&&f->mv.ys.guard.x20==modes[(n+1)%5]&&f->mv.ys.guard.x24==modes[(n+2)%5]);
  CHECK(f->mv.ys.guard.x10==args[0].as.f32+1&&f->mv.ys.guard.x14==args[0].as.f32+2&&f->mv.ys.guard.x18==args[0].as.f32+3);
  // A real native guard consumer, not a fixture assignment, requests release.
  f->input.held_buttons[0]=n&1?HSD_PAD_L:0;ftCo_80092BCC(o);
  f->lightshield_amount=0.375f;f->co_attrs.initial_shield_size=1.25f;
  CHECK(helper(o,"SharedRead",NULL,0));
  CHECK(state(o,"shared_release_requested").as.u64==1);
  CHECK(!memcmp(&state_address(o,"shared_elapsed")[0],&args[0].as.f32,4));
  CHECK(state(o,"shared_minimum_hold_frames").as.f32==args[0].as.f32+1&&state(o,"shared_reflect_frames").as.f32==args[0].as.f32+2&&state(o,"shared_powershield_frames").as.f32==args[0].as.f32+3);
  CHECK(state(o,"shared_input_lock_frames").as.i64==modes[n]&&state(o,"shared_interrupt_mode").as.i64==modes[(n+1)%5]&&state(o,"shared_delayed_action_frames").as.i64==modes[(n+2)%5]);
  CHECK(state(o,"shared_volume_enabled").as.u64==!(n&1)&&state(o,"shared_new_powershield").as.u64==(n&1)&&state(o,"shared_reflect_window").as.u64==!(n&1)&&state(o,"shared_powershield_window").as.u64==(n&1));
  CHECK(state(o,"shared_health").as.f32==args[0].as.f32&&state(o,"shared_light").as.f32==0.375f&&state(o,"shared_bone").as.i64==43&&state(o,"shared_size").as.f32==1.25f);++cases;
 }
 NativeScriptValue invalid={.type=NativeScriptType_I32,.as.i64=INT64_C(2147483648)},out={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingGuard_Write(&c,NativeScriptBinding_GuardInputLockFrames,&invalid,&diagnostic));
 invalid.as.i64=INT64_C(-2147483649);CHECK(!nativeScriptBindingGuard_Write(&c,NativeScriptBinding_GuardDelayedActionFrames,&invalid,&diagnostic));
 invalid=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=2};CHECK(!nativeScriptBindingGuard_Write(&c,NativeScriptBinding_GuardReflectWindow,&invalid,&diagnostic));
 f->ft_data->x8=NULL;CHECK(!nativeScriptBindingGuard_Read(&c,NativeScriptBinding_ShieldBone,&out,&diagnostic)&&out.as.u64==99);f->ft_data->x8=&aux;
 for(unsigned n=0;n<3;++n){
  common.x260_startShieldHealth=n?60:-0.0f;common.x268=3;common.x2A4=4;common.x2A8=5;common.x2AC=6;common.x2B0=7;common.x2B4=8;
  CHECK(helper(o,"Tuning",NULL,0));
  CHECK(!memcmp(state_address(o,"common_start_health"),&common.x260_startShieldHealth,4));
  CHECK(state(o,"common_minimum_hold_duration").as.f32==3&&state(o,"common_reflect_duration").as.f32==4&&state(o,"common_reflect_radius").as.f32==5&&state(o,"common_reflect_damage_multiplier").as.f32==6&&state(o,"common_reflect_speed_multiplier").as.f32==7&&state(o,"common_powershield_duration").as.f32==8);++cases;
 }
 p_ftCommonData=NULL;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingCommonProperties_Read(&c,NativeScriptBinding_ShieldStartHealth,&out,&diagnostic)&&out.as.u64==99);p_ftCommonData=&common;
 for(unsigned n=1;n<=2;++n){
  uint32_t generation=f->x8_spawnNum;uint64_t identity=bank.storage_parameters.identity;
  // The fourth liveness admission runs after capture, inside the final layout
  // check. Earlier parameter changes become part of the newly captured state.
  observed=o;admission_mutation=n;admission_trigger=4;admission_calls=0;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  bool accepted=nativeScriptBindingCommonProperties_Read(&c,NativeScriptBinding_ShieldStartHealth,&out,&diagnostic);
  if(accepted||out.type!=NativeScriptType_U64||out.as.u64!=99)fprintf(stderr,"common admission mutation=%u calls=%u accepted=%d type=%u\n",n,admission_calls,accepted,out.type);
  CHECK(!accepted&&out.type==NativeScriptType_U64&&out.as.u64==99);
  f->x8_spawnNum=generation;bank.storage_parameters.identity=identity;observed=NULL;admission_trigger=0;++cases;
 }
}
static void negatives(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);NativeScriptValue output;
 NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=104},{.type=NativeScriptType_F32,.as.f32=0},{.type=NativeScriptType_F32,.as.f32=2},{.type=NativeScriptType_F32,.as.f32=3},{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_Callable,.as.u64=function("Reaction")}};
 const NativeScriptValue invalid[]={{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_Callable,.as.u64=UINT64_C(0x100000000)},{.type=NativeScriptType_Callable,.as.u64=function("Wrong")},{.type=NativeScriptType_Callable,.as.u64=function("WithArgument")}};
 for(unsigned n=0;n<4;++n){static Fighter before;before=*f;args[5]=invalid[n];output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardCreate,args,6,&output,&diagnostic));CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&!memcmp(f,&before,sizeof(*f)));++cases;}
 args[5]=(NativeScriptValue){.type=NativeScriptType_Null};const NativeScriptValue joints_bad[]={{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_Bool,.as.u64=2},{.type=NativeScriptType_I32,.as.i64=INT64_MAX}};
 for(unsigned n=0;n<3;++n){args[0]=joints_bad[n];output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardCreate,args,6,&output,&diagnostic)&&output.as.u64==99);++cases;}
 args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};static Fighter before;before=*f;CHECK(nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardCreate,args,6,&output,&diagnostic)&&!memcmp(f,&before,sizeof(*f)));++cases;
 args[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=104};HSD_JObj* original=parts[104].joint;parts[104].joint=NULL;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardCreate,args,6,&output,&diagnostic)&&output.as.u64==99);parts[104].joint=original;++cases;
 int size=hsdJObj.parent.parent.head.obj_size;hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj)-1;CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardCreate,args,6,&output,&diagnostic)&&output.as.u64==99);hsdJObj.parent.parent.head.obj_size=size;++cases;
 uint32_t bits=0x7fc12345;memcpy(&args[1].as.f32,&bits,4);CHECK(nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardCreate,args,6,&output,&diagnostic)&&!memcmp(&f->shield_hit.offset.x,&args[1].as.f32,4));++cases;
 for(unsigned m=1;m<=2;++m){uint32_t gen=f->x8_spawnNum;uint64_t param=bank.storage_parameters.identity;observed=o;admission_mutation=m;admission_calls=0;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingGuard_Read(&c,NativeScriptBinding_GuardActive,&output,&diagnostic)&&output.as.u64==99&&!admission_mutation);f->x8_spawnNum=gen;bank.storage_parameters.identity=param;observed=NULL;++cases;}
 const char* bad[]={"func Bad():\n    guard.create(0, 0, 0, 0, 1, 123)\n","func Bad():\n    guard.create(0.5, 0, 0, 0, 1, None)\n","func Bad():\n    guard.hit_damage = 1\n"};
 for(unsigned n=0;n<3;++n){char input[1024];int length=snprintf(input,sizeof(input),"fighter:\n    behavior independent\n%s",bad[n]);configure(&scratch,&buffers[1]);NativeScriptCompileRequest request={.source=input,.source_bytes=(uint32_t)length,.motion_layout=&guard_packages[0].assets.facts.moves};const NativeScriptCompileBank* accepted=&bank;CHECK(!nativeScriptCompile_Build(&scratch,&request,&accepted,&diagnostic)&&accepted==&bank);++cases;}
}

static void schedule(HSD_GObj* o)
{
 Fighter* f=o->user_data;CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_ShieldHit,function("Reaction")));f->shield_hit_cb=NULL;f->x19A4=55;f->specialn_facing_dir=-1;
 static Fighter before,expected;before=*f;CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_ShieldHit,UINT32_MAX));f->shield_hit_cb=callback_reference;guard_reference_schedule(o,false);expected=*f;expected.shield_hit_cb=NULL;*f=before;CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_ShieldHit,function("Reaction")));
 unsigned tails=guard_completed_tails;guard_schedule(o,false);CHECK(!memcmp(f,&expected,sizeof(*f))&&reference_completed_tails==1&&guard_hitlag_damage==reference_hitlag_damage);++pairs;++vm_calls;CHECK(guard_completed_tails==tails+1&&guard_hitlag_damage==55&&f->x19A4==0&&f->x221B_b3&&f->shield_unk0==3.5f&&f->cur_pos.x==7);CHECK(state(o,"seen").as.i64==55&&state(o,"facing").as.f32==-1&&state(o,"tail").as.i64==1);++cases;
 f->x19A4=99;guard_schedule(o,true);CHECK(stock_reactions==1&&audio_calls==1&&f->x19A4==0);CHECK(state(o,"tail").as.i64==1);++cases;
 CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_ShieldHit,UINT32_MAX));f->shield_hit_cb=NULL;f->x19A4=66;guard_schedule(o,false);CHECK(f->x19A4==0&&state(o,"tail").as.i64==1);++cases;
}

static void hurtboxes(HSD_GObj* object)
{
 Fighter* fighter=object->user_data;static Fighter before,expected;
 const int slots[]={-1,0,7,14,15,INT32_MAX};
 const int bones[]={-1,0,43,104,MAX_FT_PARTS-1,MAX_FT_PARTS,INT32_MAX};
 const float scales[]={-0.0f,1,3.5f,-1,NAN,INFINITY};
 for(unsigned s=0;s<6;++s)for(unsigned b=0;b<7;++b)for(unsigned h=0;h<3;++h)
 for(unsigned g=0;g<2;++g)for(unsigned z=0;z<6;++z) {
  memset(fighter->hurt_capsules,0x9e,sizeof(fighter->hurt_capsules));fighter->hurt_capsules_len=15;
  fighter->x221A_b6=0;before=*fighter;
  ftHurtboxInit init={.bone_idx=bones[b],.height=(HurtHeight)h,.is_grabbable=g,
   .a_offset={-0.0f,2.25f,-3.5f},.b_offset={4.75f,-6.125f,8.5f},.scale=scales[z]};
  if(slots[s]>=0&&slots[s]<15&&bones[b]>=0&&bones[b]<MAX_FT_PARTS)
   hurtbox_reference_init(fighter,&fighter->hurt_capsules[slots[s]],&init);
  expected=*fighter;*fighter=before;
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=slots[s]},
   {.type=NativeScriptType_I32,.as.i64=bones[b]},{.type=NativeScriptType_I32,.as.i64=h},
   {.type=NativeScriptType_Bool,.as.u64=g},{.type=NativeScriptType_F32,.as.f32=scales[z]}};
  CHECK(helper(object,"Capsule",args,5));CHECK(!memcmp(fighter,&expected,sizeof(*fighter)));++cases;++pairs;
 }
 fighter->hurt_capsules_len=15;memset(fighter->hurt_capsules,0x9e,sizeof(fighter->hurt_capsules));
 before=*fighter;hurtbox_reference_set_all(object,HurtCapsule_Intangible);
 ftHurtboxInit shield={.bone_idx=43,.height=HurtHeight_Mid,.is_grabbable=true,.scale=1};
 hurtbox_reference_init(fighter,&fighter->hurt_capsules[0],&shield);expected=*fighter;*fighter=before;
 CHECK(helper(object,"ShieldCapsule",NULL,0));CHECK(!memcmp(fighter,&expected,sizeof(*fighter)));++cases;++pairs;
 CHECK(helper(object,"CapsuleHeights",NULL,0));
 CHECK(fighter->hurt_capsules[0].height==HurtHeight_Low&&!fighter->hurt_capsules[0].is_grabbable);
 CHECK(fighter->hurt_capsules[1].height==HurtHeight_High&&fighter->hurt_capsules[1].is_grabbable);++cases;
 NativeScriptRuntimeContext c=context(object);
 NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_U32,.as.u64=43},
  {.type=NativeScriptType_I32,.as.i64=HurtHeight_Mid},{.type=NativeScriptType_Bool,.as.u64=1},
  {.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},
  {.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},
  {.type=NativeScriptType_F32,.as.f32=1}};
 const NativeScriptValue wide_bones[]={{.type=NativeScriptType_U64,.as.u64=UINT64_MAX},
  {.type=NativeScriptType_I64,.as.i64=INT64_MIN},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x10000002b)}};
 NativeScriptValue output;
 for(unsigned n=0;n<3;++n){before=*fighter;args[1]=wide_bones[n];output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptBindingCombat_Invoke(&c,NativeScriptBinding_HurtboxesDefine,args,11,&output,&diagnostic));
  CHECK(output.type==NativeScriptType_Void&&!memcmp(fighter,&before,sizeof(*fighter)));++cases;}
 args[1]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=43};
 const unsigned invalid_indices[]={0,1,1,1,2,3,4,10};
 const NativeScriptValue invalid[]={{.type=NativeScriptType_I32,.as.i64=INT64_MAX},
  {.type=NativeScriptType_U32,.as.u64=UINT64_MAX},{.type=NativeScriptType_Bool,.as.u64=2},
  {.type=NativeScriptType_F32,.as.f32=43},{.type=NativeScriptType_I32,.as.i64=3},
  {.type=NativeScriptType_Bool,.as.u64=2},{.type=NativeScriptType_I32,.as.i64=0},
  {.type=NativeScriptType_F64,.as.f64=1}};
 for(unsigned n=0;n<8;++n){unsigned index=invalid_indices[n];NativeScriptValue valid=args[index];
  args[index]=invalid[n];before=*fighter;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingCombat_Invoke(&c,NativeScriptBinding_HurtboxesDefine,args,11,&output,&diagnostic));
  CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&!memcmp(fighter,&before,sizeof(*fighter)));args[index]=valid;++cases;}
 for(unsigned n=0;n<4;++n){FighterBone* old_parts=fighter->parts;HSD_JObj* old_joint=parts[43].joint;
  FighterPartsTable* old_table=tables[fighter->kind];uint8_t old_length=fighter->hurt_capsules_len;
  if(n==0)fighter->parts=NULL;if(n==1)parts[43].joint=NULL;if(n==2)tables[fighter->kind]=NULL;if(n==3)fighter->hurt_capsules_len=16;
  before=*fighter;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingCombat_Invoke(&c,NativeScriptBinding_HurtboxesDefine,args,11,&output,&diagnostic));
  CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&!memcmp(fighter,&before,sizeof(*fighter)));
  fighter->parts=old_parts;parts[43].joint=old_joint;tables[fighter->kind]=old_table;fighter->hurt_capsules_len=old_length;++cases;}
 for(unsigned m=1;m<=2;++m){uint32_t generation=fighter->x8_spawnNum;uint64_t parameters=bank.storage_parameters.identity;
  observed=object;admission_mutation=m;admission_calls=0;before=*fighter;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingCombat_Invoke(&c,NativeScriptBinding_HurtboxesDefine,args,11,&output,&diagnostic));
  fighter->x8_spawnNum=generation;bank.storage_parameters.identity=parameters;
  CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&!memcmp(fighter,&before,sizeof(*fighter))&&!admission_mutation);observed=NULL;++cases;}
 fighter->hurt_capsules_len=0;before=*fighter;
 CHECK(nativeScriptBindingCombat_Invoke(&c,NativeScriptBinding_HurtboxesDefine,args,11,&output,&diagnostic));
 CHECK(output.type==NativeScriptType_Void&&!memcmp(fighter,&before,sizeof(*fighter)));++cases;
}

static void asynchronous_effects(HSD_GObj* object)
{
 Fighter* fighter=object->user_data;const int indices[]={-1,0,43,104,MAX_FT_PARTS-1,MAX_FT_PARTS};
 const float scalars[]={-0.0f,1,3.5f};
 for(unsigned deferred=0;deferred<2;++deferred)for(unsigned mode=0;mode<2;++mode)
 for(unsigned j=0;j<6;++j)for(unsigned v=0;v<3;++v) {
  int index=indices[j];bool valid=index>=0&&index<MAX_FT_PARTS;
  async_process.s_link=deferred?2:9;HSD_GObj_CurrentInvokedProc=&async_process;
  async_deliveries=async_samples=0;async_position=(Vec3){0};async_scalar=0;async_effect=0;
  if(valid)joints[index].translate=(Vec3){(float)index+1.25f,2.5f,-3.75f};
  if(valid){if(mode)efAsync_Spawn(object,&fighter->x60C,EF_SPAWN_POS_PARAM,1231,&joints[index],&scalars[v]);
   else efAsync_Spawn(object,&fighter->x60C,EF_SPAWN_POS,1230,&joints[index]);}
  EF_QueuedEffect expected={0};if(fighter->x60C)expected=*(EF_QueuedEffect*)fighter->x60C;
  Vec3 expected_position=async_position;float expected_scalar=async_scalar;int expected_effect=async_effect;
  unsigned expected_deliveries=async_deliveries,expected_samples=async_samples;
  efAsync_QueueClear(&fighter->x60C);CHECK(!async_live&&!fighter->x60C);
  async_deliveries=async_samples=0;async_position=(Vec3){0};async_scalar=0;async_effect=0;
  NativeScriptValue arguments[]={{.type=NativeScriptType_I32,.as.i64=mode},
   {.type=NativeScriptType_I32,.as.i64=index},{.type=NativeScriptType_F32,.as.f32=scalars[v]}};
  CHECK(helper(object,"Async",arguments,3));
  CHECK(async_deliveries==expected_deliveries&&async_samples==expected_samples);
  CHECK(async_effect==expected_effect&&!memcmp(&async_position,&expected_position,sizeof(Vec3))&&!memcmp(&async_scalar,&expected_scalar,4));
  if(deferred&&valid){CHECK(async_live==1&&fighter->x60C&&!memcmp(fighter->x60C,&expected,sizeof(expected)));
   joints[index].translate=(Vec3){12.5f,-25,50};arguments[2].as.f32=99;
   efAsync_QueueFlush(object,&fighter->x60C);CHECK(async_deliveries==1&&async_samples==1&&async_effect==(mode?1231:1230));
   CHECK(!memcmp(&async_position,&joints[index].translate,sizeof(Vec3)));
   CHECK(!memcmp(&async_scalar,mode?&scalars[v]:&(float){0},4));
  }
  CHECK(!async_live&&!fighter->x60C);++pairs;++cases;
 }
 HSD_GObj_CurrentInvokedProc=&async_process;async_process.s_link=2;async_deliveries=async_samples=0;
 fighter->co_attrs.xBC.size=2.75f;CHECK(helper(object,"RestoreEffect",NULL,0));
 CHECK(async_live==1&&!async_samples&&!async_deliveries);fighter->co_attrs.xBC.size=99;
 efAsync_QueueFlush(object,&fighter->x60C);CHECK(async_scalar==2.75f&&async_effect==1231&&!async_live);++cases;
 async_deliveries=async_samples=0;CHECK(helper(object,"AsyncPair",NULL,0));CHECK(async_live==2&&!async_samples);
 EF_QueuedEffect* latest=fighter->x60C;CHECK(latest->jobj==&joints[104]&&latest->params.x==2.5f&&latest->next->jobj==&joints[43]&&latest->next->params.x==1.25f);
 efAsync_QueueFlush(object,&fighter->x60C);CHECK(async_deliveries==2&&async_scalar==1.25f&&!async_live);++cases;
 async_allocation_failure=true;async_deliveries=async_samples=0;CHECK(helper(object,"RestoreEffect",NULL,0));
 CHECK(!fighter->x60C&&!async_live&&!async_deliveries&&!async_samples);async_allocation_failure=false;++cases;
 NativeScriptRuntimeContext c=context(object);NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=1231},
  {.type=NativeScriptType_I32,.as.i64=43},{.type=NativeScriptType_F32,.as.f32=1}},output;
 const unsigned indices_bad[]={0,0,0,1,2,2,2};
 const NativeScriptValue invalid[]={{.type=NativeScriptType_I32,.as.i64=1230},
  {.type=NativeScriptType_U64,.as.u64=UINT64_C(0x1000004CF)},{.type=NativeScriptType_Bool,.as.u64=2},
  {.type=NativeScriptType_I32,.as.i64=INT64_MAX},{.type=NativeScriptType_F32,.as.f32=NAN},
  {.type=NativeScriptType_F32,.as.f32=INFINITY},{.type=NativeScriptType_I32,.as.i64=1}};
 for(unsigned n=0;n<7;++n){unsigned index=indices_bad[n];NativeScriptValue old=args[index];args[index]=invalid[n];
  output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};unsigned delivered=async_deliveries;
  CHECK(!nativeScriptBindingEffect_Invoke(&c,NativeScriptBinding_EffectsSpawnAsyncPositionScalarOnPart,NULL,0,args,3,&output,&diagnostic));
  CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&!fighter->x60C&&!async_live&&async_deliveries==delivered);args[index]=old;++cases;}
 HSD_GObj_CurrentInvokedProc=NULL;
}
static void transitions(HSD_GObj* o)
{
 Fighter* f=o->user_data;AbsorbDesc absorb={0};
 static const int modes[]={NativeFighterShield_Start,NativeFighterShield_Hold,
  NativeFighterShield_Release,NativeFighterShield_Hit,NativeFighterShield_Hit,
  NativeFighterShield_ReflectContinue,NativeFighterShield_ReflectStart,
  NativeFighterShield_Release,NativeFighterShield_Hold,NativeFighterShield_Hold};
 CHECK(nativeScriptFighter_HasHook(o,NativeScriptFighter_ShieldTransition));
 for(unsigned n=0;n<sizeof(modes)/sizeof(*modes);++n){
  unsigned tails=transition_special_tails,statistics=transition_statistics;
  int32_t calls=(int32_t)state(o,"transition_calls").as.i64;
  f->x19B0=n==4?10:0;f->mv.co.guard.x0=0;f->x2E8=1;
  switch(n){
  case 0:ftCo_800923B4(o);break;
  case 1:ftCo_800928CC(o);break;
  case 2:ftCo_80092BE8(o);break;
  case 3:case 4:ftCo_80092E50(o);break;
  case 5:ftCo_80093850(o);break;
  case 6:ftCo_800939B4(o);break;
  case 7:ftCo_80092BE8_inline(o);break;
  case 8:ftCo_800928CC_inline_arg(o,&absorb);break;
  case 9:ftCo_GuardOn_Anim_inline(o);break;
  }
  ++vm_calls;
  CHECK(nativeScriptFighter_Status(o)==NativeScriptVM_Ok&&nativeScriptRuntime_Idle());
  CHECK(state(o,"transition_calls").as.i64==calls+1);
  CHECK(state(o,"transition_mode").as.i64==modes[n]);
  CHECK(state(o,"transition_mask").as.i64==(1<<modes[n]));
  CHECK(state(o,"transition_special").as.u64==(n==4));
  CHECK(f->cur_pos.x==(float)modes[n]);
  CHECK(transition_special_tails==tails+(n==4));
  CHECK(transition_statistics==statistics+(n==2||n==7));++cases;
 }
 NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=0},
  {.type=NativeScriptType_Bool,.as.u64=0}},result={.type=NativeScriptType_U64,.as.u64=99};
 int32_t calls=(int32_t)state(o,"transition_calls").as.i64;
 CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_ShieldTransition,args,1,NULL,&result)==NativeScriptVM_Error);
 args[0].type=NativeScriptType_I64;
 CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_ShieldTransition,args,2,NULL,&result)==NativeScriptVM_Error);
 CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);
 CHECK(state(o,"transition_calls").as.i64==calls&&nativeScriptRuntime_Idle());cases+=2;
}
static void transition_retirement(void)
{
 for(unsigned m=0;m<=8;++m){
  HSD_GObj* o=actor();Fighter* f=o->user_data;
  observed=o;replacement=NULL;mutation=m;f->x19B0=10;
  uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;
  if(!m)*state_address(o,"transition_fault")=1;
  unsigned tails=transition_special_tails;
  ftCo_80092E50(o);++vm_calls;
  CHECK(transition_special_tails==tails&&nativeScriptRuntime_Idle()&&!mutation);
  // A faulted live source must not fall through into native startup on retry.
  if(!m){CHECK(nativeScriptFighter_Status(o)==NativeScriptVM_Error);ftCo_800923B4(o);CHECK(transition_special_tails==tails);}
  bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;
  if(m==3){CHECK(replacement==o);HSD_GObjFree(replacement);}
  else if(m!=2){if(m==5)o->user_data=f;if(m==6)o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);}
  observed=NULL;++cases;
 }
}
static void guard_operations(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptValue arg={.type=NativeScriptType_I32};
 CHECK(nativeScriptFighter_HasHook(o,NativeScriptFighter_ShieldScale));
 CHECK(nativeScriptFighter_HasHook(o,NativeScriptFighter_ShieldAlpha));
 for(unsigned active=0;active<2;++active){f->reflecting=active!=0;CHECK(helper(o,"ReflectionStatus",NULL,0));CHECK(state(o,"reflection_active").as.u64==active);++cases;}
 common.analog_shoulder_deadzone=0.3f;common.x278=0.25f;
 common.x2EC=1.25f;common.x2F0=0.5f;common.x2F4=90;
 common.x2A8=2.5f;common.x2AC=1.5f;common.x2B0=1.75f;
 const uint32_t scale_bits[]={0x80000000,0,0x3f800000,0xc0200000,0x7fc12345,0x7f800000};
 for(unsigned b=0;b<sizeof(scale_bits)/sizeof(*scale_bits);++b)for(unsigned flags=0;flags<4;++flags){
  float size;memcpy(&size,&scale_bits[b],4);memcpy(state_address(o,"query_size"),&size,4);
  *state_address(o,"query_custom")=1;
  unsigned joint=aux.x11;Vec3 base={-3,4,-5};
  joints[joint].scale=base;joints[joint].flags=flags==0?0:flags==1?JOBJ_MTX_DIRTY:flags==2?JOBJ_MTX_INDEP_SRT:JOBJ_MTX_INDEP_PARENT;
  HSD_JObj before=joints[joint];Vec3 desired={size,size,size};HSD_JObjSetScale(&joints[joint],&desired);
  HSD_JObj expected=joints[joint];joints[joint]=before;arg.as.i64=0;
  CHECK(helper(o,"GuardOperations",&arg,1));CHECK(!memcmp(&joints[joint],&expected,sizeof(expected)));++cases;++pairs;
 }
 *state_address(o,"query_custom")=0;f->co_attrs.initial_shield_size=2.125f;
 arg.as.i64=0;CHECK(helper(o,"GuardOperations",&arg,1));CHECK(joints[aux.x11].scale.x==2.125f);++cases;
 for(unsigned allowed=0;allowed<2;++allowed)for(unsigned light=0;light<3;++light){
  *state_address(o,"query_alpha")=(uint8_t)allowed;f->lightshield_amount=0.5f*(float)light;
  unsigned calls=alpha_calls;update_shield_alpha(o);
  CHECK(alpha_calls==calls+allowed);if(allowed)CHECK(last_alpha==(u8)(90+(int)(f->lightshield_amount*(255-90))));++cases;
 }
 // Native scalar policy is unchanged when evaluated for ordinary and stock
 // egg shields; the authored source instead supplies its own radius.
 Fighter reference=*f;HSD_GObj reference_object={.classifier=HSD_GOBJ_CLASS_FIGHTER,.user_data=&reference};
 reference.gobj=&reference_object;
 for(unsigned kind=0;kind<2;++kind)for(unsigned n=0;n<4;++n){
  reference.kind=kind?Ft_Kind_Yoshi:Ft_Kind_Mario;reference.shield_health=10.0f*(float)n;reference.lightshield_amount=0.25f*(float)n;
  common.x260_startShieldHealth=60;common.x264=0.1f;common.x2D4=0.5f;common.x2D8=1.5f;
  float expected=reference_inlineB0(&reference),actual=-99;CHECK(ftCo_ShieldScale(&reference,&actual));CHECK(!memcmp(&actual,&expected,4));++pairs;++cases;
 }
 for(unsigned op=1;op<=5;++op)for(unsigned n=0;n<12;++n){
  f->x221B_b0=n&1;f->x221A_b7=false;f->shield_health=n==3?-0.25f:20;
  f->lightshield_amount=0.4f;f->input.triggers[0]=n&1?0.75f:0.15f;
  f->input.held_buttons[0]=n&1?HSD_PAD_R:0;f->mv.co.guard.xC=false;f->mv.co.guard.x10=(float)n-0.25f;
  f->x221C_b3=n&1;f->x221C_b1=n&2;f->x221C_b2=n&4;
  f->mv.co.guard.x14=(float)n-2;f->mv.co.guard.x18=(float)n-4;f->reflecting=true;
  Fighter before=*f;bool depleted=false;
  unsigned break_before=drain_breaks,sounds_before=drain_sounds,stats_before=drain_statistics;
  switch(op){case 1:reference_ftCo_80092450_inline(o);break;case 2:reference_ftCo_8009370C(o,NULL);break;
   case 3:ftCo_80092BCC(o);break;case 4:depleted=reference_ftCo_800925A4(o);break;case 5:reference_ftCo_80093BC0(o);break;}
  Fighter expected=*f;unsigned breaks=drain_breaks-break_before,sounds=drain_sounds-sounds_before,stats=drain_statistics-stats_before;
  *f=before;break_before=drain_breaks;sounds_before=drain_sounds;stats_before=drain_statistics;
  arg.as.i64=op;CHECK(helper(o,"GuardOperations",&arg,1));CHECK(!memcmp(f,&expected,sizeof(*f)));
  CHECK(drain_breaks-break_before==breaks&&drain_sounds-sounds_before==sounds&&drain_statistics-stats_before==stats);
  if(op==4)CHECK(state(o,"depleted").as.u64==depleted);
  if(op==2)CHECK(nativeScriptFighter_CallbackFunction(o,NativeScriptFighterCallback_ReflectHit)==function("reflect_hit"));
  ++pairs;++cases;
 }
 CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_PostHitlag,function("Reaction")));
 arg.as.i64=6;CHECK(helper(o,"GuardOperations",&arg,1));
 CHECK(f->hitlag_cb==ftCo_80093240&&f->post_hitlag_cb==ftCo_800932DC);
 CHECK(nativeScriptFighter_CallbackFunction(o,NativeScriptFighterCallback_PostHitlag)==UINT32_MAX);++cases;
 for(unsigned phase=0;phase<2;++phase)for(unsigned n=0;n<16;++n){
  f->ground_or_air=n&1?GA_Air:GA_Ground;f->allow_sdi=n&2;
  f->input.lstick[0].x=(n&4?1.0f:-1.0f)*(n&8?0.75f:0.1f);
  f->x670_timer_lstick_tilt_x=(u8)(n%4);f->cur_pos=(Vec3){4,7,9};f->coll_data.floor.normal=(Vec3){0.6f,0.8f,0};
  common.sdi_min_stick_mag=0.5f;common.sdi_stick_window=3;common.x4C0=0.25f;common.sdi_pos_scale=2.5f;common.x4BC=1.25f;
  Fighter before=*f;if(phase)reference_ftCo_800932DC(o);else reference_ftCo_80093240(o);Fighter expected=*f;
  *f=before;if(phase)f->post_hitlag_cb(o);else f->hitlag_cb(o);CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;++cases;
 }
 for(unsigned op=7;op<=8;++op){arg.as.i64=op;int32_t calls=(int32_t)state(o,"transition_calls").as.i64;CHECK(helper(o,"GuardOperations",&arg,1));CHECK(state(o,"transition_calls").as.i64==calls+1);CHECK(state(o,"transition_mode").as.i64==(op==7?NativeFighterShield_Hold:NativeFighterShield_Release));++cases;}
}
static void guard_operation_negatives(HSD_GObj* o)
{
 NativeScriptRuntimeContext c=context(o);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99},arg={.type=NativeScriptType_I32,.as.i64=0};
 Fighter* f=o->user_data;Fighter before=*f;
 for(unsigned s=NativeScriptBinding_GuardScaleJoint;s<=NativeScriptBinding_GuardEnterRelease;++s){
  CHECK(!nativeScriptBindingGuard_Invoke(&c,s,&arg,1,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(f,&before,sizeof(before)));++cases;
 }
 for(unsigned m=0;m<5;++m){
  FighterBone* saved_parts=f->parts;struct ftData_x8* saved_aux=f->ft_data->x8;
  HSD_JObj* saved_joint=parts[aux.x11].joint;ftCommonData* saved_common=p_ftCommonData;
  unsigned size=hsdJObj.parent.parent.head.obj_size;
  switch(m){case 0:f->parts=NULL;break;case 1:f->ft_data->x8=NULL;break;case 2:parts[aux.x11].joint=NULL;break;
   case 3:p_ftCommonData=NULL;break;case 4:hsdJObj.parent.parent.head.obj_size=sizeof(HSD_JObj)-1;break;}
  HSD_JObj joint_before=joints[aux.x11];
  CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardScaleJoint,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(&joints[aux.x11],&joint_before,sizeof(joint_before)));
  f->parts=saved_parts;f->ft_data->x8=saved_aux;parts[aux.x11].joint=saved_joint;p_ftCommonData=saved_common;
  hsdJObj.parent.parent.head.obj_size=size;++cases;
 }
 NativeScriptFighterProgram* selected=(NativeScriptFighterProgram*)nativeScriptFighter_Program(f->kind);
 uint32_t callback=selected->callbacks[NativeScriptFighterCallback_ReflectHit];
 selected->callbacks[NativeScriptFighterCallback_ReflectHit]=UINT32_MAX;before=*f;
 CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardInstallReflector,NULL,0,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(f,&before,sizeof(before)));
 selected->callbacks[NativeScriptFighterCallback_ReflectHit]=callback;++cases;
}
static void query_retirement(void)
{
 for(unsigned query=0;query<2;++query)for(unsigned m=0;m<=8;++m){
  HSD_GObj* o=actor();Fighter* f=o->user_data;observed=o;replacement=NULL;mutation=m;
  *state_address(o,"query_mutate")=1;*state_address(o,"query_alpha")=1;
  if(!m)*state_address(o,"query_fault")=1;
  uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;
  HSD_JObj before=joints[aux.x11];unsigned calls=alpha_calls;
  if(query)update_shield_alpha(o);
  else{NativeScriptRuntimeContext c=context(o);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
   CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardScaleJoint,NULL,0,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);}
  CHECK(alpha_calls==calls&&!memcmp(&joints[aux.x11],&before,sizeof(before))&&nativeScriptRuntime_Idle()&&!mutation);
  bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;
  if(m==3){CHECK(replacement==o);HSD_GObjFree(replacement);}
  else if(m!=2){if(m==5)o->user_data=f;if(m==6)o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);}
  observed=NULL;++cases;
 }
}
static void drain_retirement(void)
{
 for(unsigned m=1;m<=8;++m){
  HSD_GObj* o=actor();Fighter* f=o->user_data;observed=o;replacement=NULL;mutation=m;
  f->x221B_b0=true;f->shield_health=-1;
  uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;
  unsigned sounds=drain_sounds;NativeScriptRuntimeContext c=context(o);
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardDrain,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&drain_sounds==sounds&&nativeScriptRuntime_Idle()&&!mutation);
  bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;
  if(m==3){CHECK(replacement==o);HSD_GObjFree(replacement);}
  else if(m!=2){if(m==5)o->user_data=f;if(m==6)o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);}
  observed=NULL;++cases;
 }
}
static void query_rig_replacement(void)
{
 for(unsigned m=9;m<=13;++m){
  HSD_GObj* o=actor();Fighter* f=o->user_data;observed=o;mutation=m;*state_address(o,"query_mutate")=1;
  unsigned index=aux.x11,count=guard_packages[0].assets.parts.parts_num;HSD_JObj before=joints[index];
  NativeScriptRuntimeContext c=context(o);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!nativeScriptBindingGuard_Invoke(&c,NativeScriptBinding_GuardScaleJoint,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(&joints[index],&before,sizeof(before))&&nativeScriptRuntime_Idle()&&!mutation);
  f->parts=parts;parts[index].joint=&joints[index];aux.x11=(u8)index;ftPartsTable=tables;
  guard_packages[0].assets.parts.parts_num=(u32)count;HSD_GObjFree(o);observed=NULL;++cases;
 }
}
#include "reflect_direction_tests.h"
#include "guard_flow_tests.h"
#include "jump_policy_tests.h"
int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);HSD_GObj* o=actor();construction(o);fields(o);reflector_ownership(o);reflect_direction_tests(o);shared_fields(o);hurtboxes(o);asynchronous_effects(o);negatives(o);schedule(o);transitions(o);guard_operations(o);guard_operation_negatives(o);guard_flow_tests(o);jump_policy_tests(o);HSD_GObjFree(o);guard_flow_retirement();jump_policy_retirement();transition_retirement();query_retirement();drain_retirement();query_rig_replacement();
 for(unsigned m=0;m<=8;++m){o=actor();Fighter* f=o->user_data;observed=o;mutation=m;replacement=NULL;uint64_t schema=bank.storage_layout.identity,param=bank.storage_parameters.identity;uint8_t* tail=state_address(o,"tail");unsigned tails=guard_completed_tails;
  CHECK(nativeScriptFighter_BindCallback(o,NativeScriptFighterCallback_ShieldHit,function(m?"Reaction":"Fault")));f->x19A4=123;guard_schedule(o,false);++vm_calls;int32_t value;memcpy(&value,tail,4);int32_t expected_tail=(m>=5)?1:0;if(value!=expected_tail||guard_completed_tails!=tails||f->x19A4!=123||!nativeScriptRuntime_Idle())fprintf(stderr,"mutation=%u tail=%d completed=%u/%u damage=%d\n",m,value,guard_completed_tails,tails,f->x19A4);CHECK(value==expected_tail&&guard_completed_tails==tails&&f->x19A4==123&&nativeScriptRuntime_Idle());CHECK(!mutation);++cases;
  bank.storage_layout.identity=schema;bank.storage_parameters.identity=param;if(m==3){CHECK(replacement==o);HSD_GObjFree(replacement);}else if(m!=2)HSD_GObjFree(o);observed=NULL;
 }
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS source guard\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
