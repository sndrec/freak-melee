#include "engine_fixture.h"
#include "../script_setup/engine_fixture.h"
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_program.h>
#include <melee/ft/native_fighter_damage.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/ftcoll.h>
#include <melee/ef/types.h>
#include <melee/ef/eflib.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;static HSD_GObj* heads[32];static HSD_GObj object,other;
NativeMatchPool nativeGObjPool;static NativeMatchPoolSlot pooled_slot[1];static uint32_t pooled_order[1];static uint64_t pooled_epoch;
static Fighter actor;static NativeScriptRuntimeContext context;static NativeScriptDiagnostic diagnostic;
static HSD_GObj effect_objects[4];static EF_Effect effects[4];static unsigned checks,rng_pairs,body_pairs;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"Combat check %d: %s (%s)\n",__LINE__,#v,diagnostic.message);exit(1);}}while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
// The shared state fixture owns the attachment predicate; no owned collision
// actor is enabled in this engine-only historical oracle.
NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj* o,uint32_t h,const NativeScriptValue* a,uint32_t n,NativeScriptEventRecordIO* io,NativeScriptValue* v)
{(void)o;(void)h;(void)a;(void)n;(void)io;(void)v;return NativeScriptVM_Error;}
bool nativeScriptBindingTransition_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,NativeScriptValue* o,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)a;(void)n;(void)o;(void)d;return false;}
static NativeScriptValue integer(int64_t n){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=n};}
static void setup(void)
{
 memset(heads,0,sizeof(heads));memset(&object,0,sizeof(object));memset(&actor,0,sizeof(actor));
 HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;object.user_data=&actor;actor.gobj=&object;actor.x8_spawnNum=17;actor.kind=Ft_Kind_None;
 object.classifier=HSD_GOBJ_CLASS_FIGHTER;CHECK(nativeMatchPool_Bind(&nativeGObjPool,&object,sizeof(object),pooled_slot,pooled_order,1,sizeof(object),_Alignof(HSD_GObj),++pooled_epoch));NativeMatchPoolReference allocated;CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&allocated)==&object);object.user_data=&actor;object.classifier=HSD_GOBJ_CLASS_FIGHTER;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
 object.obj_kind=HSD_GObj_JObjKind;actor.dmg.x1830_percent=31.25f;
 scriptSetupFixture_Reset();scriptCombatFixture_Reset();CHECK(nativeScriptBindings_Context(&object,NativeCallbackSource_Fighter,17,&context,&diagnostic));context.scene_epoch=nativeGObjPool.lifetime;
}
static NativeScriptValue invoke(uint32_t semantic,NativeScriptValue* values,unsigned count)
{NativeScriptValue result;CHECK(nativeScriptBindings_Invoke(&context,semantic,NULL,0,values,count,&result,&diagnostic));CHECK(result.type==nativeScriptBindings_Descriptor(semantic)->result_type);return result;}
static void rng(void)
{
 setup();
 for(unsigned bound=0;bound<=12;++bound)for(int first=-2;first<=(int)bound+1;++first)for(int second=-2;second<=(int)bound+1;++second)for(unsigned sample=0;sample<8;++sample){
  u32 initial=sample*UINT32_C(0x9876543)+bound*71+3;*seed_ptr=initial;s32 expected=scriptCombatFixture_random_int_excluding((s32)bound,first,second);u32 wanted=*seed_ptr;
  *seed_ptr=initial;NativeScriptValue values[]={integer(bound),integer(first),integer(second)};NativeScriptValue actual=invoke(NativeScriptBinding_RandomIntExcluding,values,3);
  CHECK(actual.as.i64==expected&&*seed_ptr==wanted);++rng_pairs;
 }
 const int bounds[]={-100,0,1,9,32768};
 for(unsigned i=0;i<sizeof(bounds)/sizeof(bounds[0]);++i)for(unsigned sample=0;sample<32;++sample){
  u32 initial=sample*65539+17;*seed_ptr=initial;int expected=scriptCombatFixture_random_int(bounds[i]);u32 wanted=*seed_ptr;*seed_ptr=initial;
  NativeScriptValue value=integer(bounds[i]);NativeScriptValue actual=invoke(NativeScriptBinding_RandomInt,&value,1);CHECK(actual.as.i64==expected&&*seed_ptr==wanted);++rng_pairs;
  *seed_ptr=initial;float expected_float=HSD_Randf();wanted=*seed_ptr;*seed_ptr=initial;actual=invoke(NativeScriptBinding_RandomFloat,NULL,0);CHECK(actual.as.f32==expected_float&&*seed_ptr==wanted);++rng_pairs;
 }
 for(unsigned sample=0;sample<64;++sample){u32 initial=sample*65537+9;*seed_ptr=initial;s32 expected=scriptCombatFixture_random_int_excluding(32768,32766,32767);u32 wanted=*seed_ptr;*seed_ptr=initial;NativeScriptValue values[]={integer(32768),integer(32766),integer(32767)};CHECK(invoke(NativeScriptBinding_RandomIntExcluding,values,3).as.i64==expected&&*seed_ptr==wanted);++rng_pairs;}
 *seed_ptr=123;NativeScriptValue oversized=integer(32769),result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_RandomInt,NULL,0,&oversized,1,&result,&diagnostic)&&*seed_ptr==123&&result.type==NativeScriptType_U64);
 // The same engine RNG operates in article context; its entity generation is
 // still checked before seed mutation. No article-specific solver is involved.
 HSD_GObj item_object={0};Item item={0};item_object.user_data=&item;item.entity=&item_object;item.x1C=34;heads[HSD_GOBJ_PLINK_ITEM]=&item_object;
 NativeScriptRuntimeContext saved=context;CHECK(nativeScriptBindings_Context(&item_object,NativeCallbackSource_Item,34,&context,&diagnostic));invoke(NativeScriptBinding_RandomFloat,NULL,0);context=saved;
}
static void damage(void)
{
 const double amounts[]={-1,0,0.1000000000000001,33.333333333333,1000};
 for(unsigned healing=0;healing<2;++healing)for(unsigned blocked=0;blocked<2;++blocked)for(unsigned i=0;i<sizeof(amounts)/sizeof(amounts[0]);++i){
  setup();actor.x2226_b4=blocked;actor.metal_timer=1;actor.metal_health=66;actor.x2034=1;actor.x2038=44;Fighter original=actor;
  if(healing)nativeFighterDamage_Heal(&object,(float)amounts[i]);else nativeFighterDamage_Self(&object,(float)amounts[i]);
  Fighter expected=actor;int hp=script_combat_hp,heal=script_combat_heal_credit;float credit=script_combat_damage_credit,percent=script_combat_damage_percent;
  actor=original;scriptCombatFixture_Reset();NativeScriptValue input={.type=NativeScriptType_F64,.as.f64=amounts[i]};invoke(healing?NativeScriptBinding_DamageHeal:NativeScriptBinding_FighterSelfDamage,&input,1);
  CHECK(!memcmp(&expected,&actor,sizeof(actor)));CHECK(hp==script_combat_hp&&heal==script_combat_heal_credit&&credit==script_combat_damage_credit&&percent==script_combat_damage_percent);++body_pairs;
 }
 setup();NativeScriptValue value={.type=NativeScriptType_F64,.as.f64=0.1000000000000001},result;
 CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_DamageArmor,NULL,0,&value,&diagnostic));CHECK(actor.dmg.armor0==(float)value.as.f64);
 CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_DamageJumpArmor,NULL,0,&value,&diagnostic));CHECK(actor.dmg.armor1==(float)value.as.f64);
 value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_DamageSongImmune,NULL,0,&value,&diagnostic));CHECK(actor.x2222_b2);
 CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_DamageSongImmune,NULL,0,&result,&diagnostic)&&result.type==NativeScriptType_Bool&&result.as.u64==1);
 actor.x221C_b6=true;CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_DamageInHitstun,NULL,0,&result,&diagnostic)&&result.as.u64==1);CHECK(!nativeScriptBindings_Write(&context,NativeScriptBinding_DamageInHitstun,NULL,0,&value,&diagnostic));
 actor.kind=Ft_Kind_MasterH;value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};float before=actor.dmg.x1830_percent;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_FighterSelfDamage,NULL,0,&value,1,&result,&diagnostic)&&actor.dmg.x1830_percent==before);
}
static void hurt(void)
{
 for(unsigned all=0;all<2;++all)for(unsigned state=0;state<3;++state){
  setup();actor.hurt_capsules_len=15;for(unsigned i=0;i<15;++i){actor.hurt_capsules[i].capsule.state=HurtCapsule_Disabled;actor.hurt_capsules[i].capsule.skip_update_pos=true;}Fighter original=actor;
  if(all)ftColl_8007B0C0(&object,(HurtCapsuleState)state);else ftColl_8007B62C(&object,state);Fighter expected=actor;unsigned color=script_combat_color;
  actor=original;scriptCombatFixture_Reset();NativeScriptValue value=integer(state);invoke(all?NativeScriptBinding_HurtboxesSetAll:NativeScriptBinding_HurtboxesSetBodyStatus,&value,1);CHECK(!memcmp(&expected,&actor,sizeof(actor))&&color==script_combat_color);++body_pairs;
 }
 setup();actor.x1988=1;actor.x198C=2;actor.x221D_b6=true;NativeScriptValue result;CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_HurtboxesStatus,NULL,0,&result,&diagnostic)&&result.as.i64==ftColl_8007B868(&object));
 NativeScriptValue invalid=integer(3);CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_HurtboxesSetBodyStatus,NULL,0,&invalid,1,&result,&diagnostic)&&actor.x1988==1);
 actor.hurt_capsules_len=16;invalid=integer(0);CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_HurtboxesSetAll,NULL,0,&invalid,1,&result,&diagnostic));
}
static void effect_lists(bool native)
{
 memset(effects,0,sizeof(effects));memset(effect_objects,0,sizeof(effect_objects));
 for(unsigned i=0;i<4;++i){effect_objects[i].obj_kind=HSD_GOBJ_OBJ_NONE;effect_objects[i].user_data=&effects[i];effects[i].gobj=&effect_objects[i];effects[i].parent_gobj=i%2==0?&object:&other;effects[i].state_flags=(u8)(i|EF_STATE_ASYNC);if(native&&i%2==0)effects[i].native_policy=(void*)1;}
 effect_objects[0].next=&effect_objects[1];effect_objects[2].next=&effect_objects[3];heads[HSD_GOBJ_PLINK_EFFECT_UNK0]=&effect_objects[0];heads[HSD_GOBJ_PLINK_EFFECT_UNK1]=&effect_objects[2];efLib_ParamTable[0].gobj=&object;efLib_ParamTable[1].gobj=&other;
}
static void effect(void)
{
 setup();effect_lists(false);invoke(NativeScriptBinding_EffectsPause,NULL,0);for(unsigned i=0;i<4;++i)CHECK(effects[i].state_flags==(i%2==0?(EF_STATE_ASYNC|EF_STATE_ENTERING_PAUSE):(i|EF_STATE_ASYNC)));
 invoke(NativeScriptBinding_EffectsResume,NULL,0);for(unsigned i=0;i<4;++i)CHECK(effects[i].state_flags==(i%2==0?EF_STATE_ASYNC:(i|EF_STATE_ASYNC)));
 NativeScriptValue denied={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_EffectsPauseDuringHitlag,NULL,0,NULL,0,&denied,&diagnostic));CHECK(denied.type==NativeScriptType_U64&&!actor.pre_hitlag_cb&&!actor.post_hitlag_cb);
 for(unsigned native=0;native<2;++native)for(unsigned clear=0;clear<2;++clear){setup();effect_lists(native!=0);actor.x2219_b0=true;invoke(clear?NativeScriptBinding_EffectsClearAll:NativeScriptBinding_EffectsDestroyAll,NULL,0);CHECK(script_combat_free_count==2&&script_combat_walk_count==1);CHECK(heads[HSD_GOBJ_PLINK_EFFECT_UNK0]==&effect_objects[1]&&heads[HSD_GOBJ_PLINK_EFFECT_UNK1]==&effect_objects[3]);CHECK(efLib_ParamTable[0].gobj==NULL&&efLib_ParamTable[1].gobj==&other);CHECK(actor.x2219_b0==(clear==0));++body_pairs;}
 // Reach the other effect list without deleting anything in the first one.
 // The actual body must guard its second-loop Free/Cancel paths as well.
 for(unsigned native=0;native<2;++native)for(unsigned replace=0;replace<2;++replace){setup();effect_lists(native!=0);effects[0].parent_gobj=&other;scriptCombatFixture_Retire(&object,1,replace!=0);efLib_DestroyAll(&object);CHECK(script_combat_free_count==1&&script_combat_walk_count==0);CHECK(heads[HSD_GOBJ_PLINK_EFFECT_UNK0]==&effect_objects[0]&&heads[HSD_GOBJ_PLINK_EFFECT_UNK1]==&effect_objects[3]);CHECK(!nativeScriptBindings_SourceAlive(&context));}
}
static void frontend(void)
{
 const char* sources[]={"func Apply() -> int:\n    let float draw = random.float()\n    damage.armor = draw + 0.1\n    damage.song_immune = true\n    hurtboxes.set_all(Hurt.intangible)\n    effects.pause()\n    return random.int_excluding(9, 2, 5)\n",
  "func DestroyAndContinue():\n    effects.destroy_all()\n    command(0) = 123\n",
  "func Inspect() -> float:\n    return random.float()\n"};
 for(unsigned i=0;i<3;++i){
  const char* text=sources[i];NativeScriptSourceNode syntax[64];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,64};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptIRNode nodes[256];uint32_t args[128];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,256,args,0,128};
  NativeScriptFunctionDeclaration declarations[4];NativeScriptParameterDeclaration parameters[4];NativeScriptValueDeclaration fields[4];NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=declarations,.function_capacity=4,.parameters=parameters,.parameter_capacity=4,.values=fields,.value_capacity=4};CHECK(nativeScriptProgram_Index(&index,&diagnostic));if(i==2)declarations[0].body_policy=NativeScriptBodyPolicy_ReadOnly;
  NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};NativeScriptLocal locals[16];NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&binding,.locals=locals,.local_capacity=16};NativeScriptStatementCompiler body;
  if(i==2){CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));CHECK(strstr(diagnostic.message,"pure")!=NULL);continue;}
  CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));NativeScriptInstruction code[256];NativeScriptFunctionCode fn[1];NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=256,.functions=fn,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};CHECK(nativeScriptBytecode_Compile(&builder,&body,declarations[0].source_node,0,0,&diagnostic));NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,fn,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[128];NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=registers,.register_capacity=128,.frames=frames,.frame_capacity=4};
  if(i==0){setup();actor.hurt_capsules_len=15;*seed_ptr=55;float draw=HSD_Randf();s32 expected=scriptCombatFixture_random_int_excluding(9,2,5);u32 wanted=*seed_ptr;*seed_ptr=55;NativeScriptValue result;CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_Ok);CHECK(result.as.i64==expected&&*seed_ptr==wanted&&actor.dmg.armor0==(float)((double)draw+0.1));CHECK(actor.x2222_b2&&actor.hurt_capsules[14].capsule.state==HurtCapsule_Intangible&&!actor.pre_hitlag_cb);}
  else for(unsigned native=0;native<2;++native)for(unsigned replace=0;replace<2;++replace){setup();effect_lists(native!=0);actor.cmd_vars[0]=99;scriptCombatFixture_Retire(&object,1,replace!=0);NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_SourceRetired);CHECK(script_combat_free_count==1&&script_combat_walk_count==0&&actor.cmd_vars[0]==99);CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX&&vm.depth==0&&vm.register_top==0);}
 }
}
static void semantic_rejections(void)
{
 const char* unsupported[]={"damage.in_hitstun(1)","hurtboxes.set_all()","effects.spawn_root(1)","Hurt.disabled","damage.armor()","random.int(1,2)"};
 NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};
 for(unsigned i=0;i<sizeof(unsupported)/sizeof(unsupported[0]);++i){NativeScriptIRNode nodes[64];uint32_t arguments[32],result;const char* text=unsupported[i];NativeScriptIRArena arena={text,(uint32_t)strlen(text),nodes,0,64,arguments,0,32};CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,1,1},nativeScriptBindings_Resolve,&binding,&result,&diagnostic));}
 const char* text="damage.song_immune";NativeScriptIRNode nodes[16];uint32_t arguments[8],result;NativeScriptIRArena arena={text,(uint32_t)strlen(text),nodes,0,16,arguments,0,8};binding.context_mask=NativeScriptBindings_Article;CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,1,1},nativeScriptBindings_Resolve,&binding,&result,&diagnostic));
}
int main(void){rng();damage();hurt();effect();frontend();semantic_rejections();printf("{\"checks\":%u,\"rng_pairs\":%u,\"native_body_pairs\":%u,\"status\":\"PASS combat typed native contracts\"}\n",checks,rng_pairs,body_pairs);return 0;}
