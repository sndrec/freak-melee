#include "engine_fixture.h"
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_program.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/inlines.h>
#include <melee/it/it_2725.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/mp/forward.h>
#include <melee/ef/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

HSD_GObj** HSD_GObjPLinkHead;static HSD_GObj* heads[32];static HSD_GObj object;
static Item actor;static ItemAttr attributes;static NativeScriptRuntimeContext context;static NativeScriptDiagnostic diagnostic;
static unsigned checks,physics_pairs,sound_pairs,vm_ticks;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"Item check %d: %s (%s; source %u:%u)\n",__LINE__,#v,diagnostic.message,diagnostic.source.line,diagnostic.source.column);exit(1);}}while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
static NativeScriptValue integer(int64_t v){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=v};}
static void setup(unsigned sample)
{
 memset(heads,0,sizeof(heads));memset(&object,0,sizeof(object));memset(&actor,0,sizeof(actor));memset(&attributes,0,sizeof(attributes));HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_ITEM]=&object;object.user_data=&actor;actor.entity=&object;actor.x1C=17;actor.kind=It_Kind_Mario_Fire;actor.xCC_item_attr=&attributes;
 attributes.x10_fall_speed=0.0625f;attributes.x14_fall_speed_max=2.75f;attributes.x58=0.8125f;attributes.x60_scale=1.25f;actor.x40_vel=(Vec3){(float)((int)sample-32)*0.0625f,(float)((int)(sample%23)-11)*0.3125f,0.25f};actor.facing_dir=sample&1?-1:1;actor.spin_spd=sample&2?0:3.333333f;actor.xDC8_word.flags.x19=(sample>>2)&1;actor.xC70=1.125f;actor.xD44_lifeTimer=(float)sample*0.5f-3;actor.xD48_halfLifeTimer=13.5f;actor.xC58=(Vec3){0.6f,0.8f,0};actor.pos=(Vec3){2.25f,-3.5f,1.75f};actor.sfx_unk1=-1;scriptItemFixture_Reset();CHECK(nativeScriptBindings_Context(&object,NativeCallbackSource_Item,17,&context,&diagnostic));
}
static NativeScriptValue invoke(uint32_t id,NativeScriptValue* args,unsigned count)
{NativeScriptValue result;CHECK(nativeScriptBindings_Invoke(&context,id,NULL,0,args,count,&result,&diagnostic));CHECK(result.type==nativeScriptBindings_Descriptor(id)->result_type);return result;}
static void physics(void)
{
 for(unsigned sample=0;sample<128;++sample)for(unsigned operation=0;operation<5;++operation){setup(sample);Item initial=actor;bool expected_result=false;
  if(operation==0)Item_ApplyFallingPhysics(&object);else if(operation==1)it_80272860(&object,attributes.x10_fall_speed,attributes.x14_fall_speed_max);else if(operation==2)expected_result=it_802751D8(&object);else if(operation==3)expected_result=it_80273030(&object);else expected_result=itColl_BounceOffShield(&object);Item expected=actor;actor=initial;
  NativeScriptValue spin={.type=NativeScriptType_Bool,.as.u64=0};uint32_t id=operation==0?NativeScriptBinding_ItemGravity:operation==1?NativeScriptBinding_ItemGravityWithSpin:operation==2?NativeScriptBinding_ItemExpireTick:operation==3?NativeScriptBinding_ItemReflect:NativeScriptBinding_ItemShieldBounce;NativeScriptValue result=invoke(id,operation==1?&spin:NULL,operation==1?1:0);CHECK(!memcmp(&actor,&expected,sizeof(actor)));if(operation>=2)CHECK(result.as.u64==expected_result);++physics_pairs;
 }
 const unsigned masks[]={Collide_LeftWallMask,Collide_RightWallMask,Collide_CeilingMask,Collide_FloorMask};
 for(unsigned sample=0;sample<64;++sample)for(unsigned flags=0;flags<16;++flags){setup(sample);for(unsigned i=0;i<4;++i)if(flags&(1u<<i))actor.x378_itemColl.env_flags|=masks[i];actor.x378_itemColl.left_facing_wall.normal=(Vec3){1,0,0};actor.x378_itemColl.right_facing_wall.normal=(Vec3){-1,0,0};actor.x378_itemColl.ceiling.normal=(Vec3){0,-1,0};actor.x378_itemColl.floor.normal=(Vec3){0,1,0};Item initial=actor;bool wanted=it_8027781C(&object);Item expected=actor;actor=initial;NativeScriptValue result=invoke(NativeScriptBinding_ItemBounceStage,NULL,0);CHECK(result.as.u64==wanted&&!memcmp(&actor,&expected,sizeof(actor)));++physics_pairs;
 }
 setup(0);NativeScriptValue lifetime={.type=NativeScriptType_F64,.as.f64=31.1000000000001};invoke(NativeScriptBinding_ItemSetLifetime,&lifetime,1);CHECK(actor.xD44_lifeTimer==(float)lifetime.as.f64&&actor.xD48_halfLifeTimer==(float)lifetime.as.f64*0.375f);
 invoke(NativeScriptBinding_ItemPause,NULL,0);CHECK(actor.xDC8_word.flags.x3);actor.xDC8_word.flags.x7=1;invoke(NativeScriptBinding_ItemResume,NULL,0);CHECK(!actor.xDC8_word.flags.x3&&actor.xDC8_word.flags.x5);actor.xDC8_word.flags.x15=1;invoke(NativeScriptBinding_ItemUseManualLifetime,NULL,0);CHECK(!actor.xDC8_word.flags.x15);
}
static void properties(void)
{
 setup(0);const uint32_t high[]={0,16777217,UINT32_C(0x80000001),UINT32_MAX};
 for(unsigned i=0;i<4;++i){NativeScriptValue value={.type=NativeScriptType_U32,.as.u64=high[i]},result;CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_ItemCommand0,NULL,0,&value,&diagnostic));CHECK(actor.xDAC_itcmd_var0==high[i]);CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_ItemCommand0,NULL,0,&result,&diagnostic)&&result.type==NativeScriptType_U32&&result.as.u64==high[i]);CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_ItemCollisionGroup,NULL,0,&value,&diagnostic));CHECK(actor.xAC4_ignoreItemID==high[i]);actor.x1C=high[i];context.generation=high[i];CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_ItemCreationID,NULL,0,&result,&diagnostic)&&result.type==NativeScriptType_U32&&result.as.u64==high[i]);}
 setup(0);NativeScriptValue negative=integer(-1),result;CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_ItemCommand3,NULL,0,&negative,&diagnostic)&&actor.xDB8_itcmd_var3==UINT32_MAX);CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_ItemExitCategory,NULL,0,&negative,&diagnostic)&&actor.xDCC_flag.b4567==15);
 const uint32_t flags[]={NativeScriptBinding_ItemPhysicsDisabled,NativeScriptBinding_ItemCustomFloorMotion,NativeScriptBinding_ItemRemoveAtBounds,NativeScriptBinding_ItemSuppressDestructionEffects,NativeScriptBinding_ItemWorldInteraction,NativeScriptBinding_ItemOwnerBound,NativeScriptBinding_ItemReverseSpin,NativeScriptBinding_ItemThrowSpeedDamage,NativeScriptBinding_ItemDamageHitlag,NativeScriptBinding_ItemContactHitlag,NativeScriptBinding_ItemHarmOwner,NativeScriptBinding_ItemHarmTeammates,NativeScriptBinding_ItemHarmSiblingItems,NativeScriptBinding_ItemCanBeHitByOwner,NativeScriptBinding_ItemCanBeHitByTeammates,NativeScriptBinding_ItemCanBeHitBySiblingItems,NativeScriptBinding_ItemSubactionSoundsMuted,NativeScriptBinding_ItemStopOnSurface,NativeScriptBinding_ItemDynamicECB};
 setup(0);Item original=actor;for(unsigned i=0;i<sizeof(flags)/sizeof(flags[0]);++i){NativeScriptValue flag={.type=NativeScriptType_Bool,.as.u64=1};CHECK(nativeScriptBindings_Write(&context,flags[i],NULL,0,&flag,&diagnostic));CHECK(nativeScriptBindings_Read(&context,flags[i],NULL,0,&result,&diagnostic)&&result.type==NativeScriptType_Bool&&result.as.u64==1);flag.as.u64=0;CHECK(nativeScriptBindings_Write(&context,flags[i],NULL,0,&flag,&diagnostic));}actor.xDC8_word.flags.x19=original.xDC8_word.flags.x19;CHECK(!memcmp(&actor,&original,sizeof(actor)));
 setup(0);CHECK(!nativeScriptBindings_Read(&context,NativeScriptBinding_ItemAge,NULL,0,&result,&diagnostic));CHECK(!nativeScriptBindings_Read(&context,NativeScriptBinding_ItemGroundContact,NULL,0,&result,&diagnostic));actor.kind=It_Kind_NativeHoldable;actor.xDD4_itemVar.native_holdable.age=UINT16_MAX;actor.xDD4_itemVar.native_holdable.collision_flags=8;CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_ItemAge,NULL,0,&result,&diagnostic)&&result.type==NativeScriptType_I32&&result.as.i64==65535);CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_ItemGroundContact,NULL,0,&result,&diagnostic)&&result.as.u64==1);
 for(int state=0;state<5;++state){actor.msid=state;CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_ItemGrounded,NULL,0,&result,&diagnostic)&&result.as.u64==(state==1));CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_ItemAirborne,NULL,0,&result,&diagnostic)&&result.as.u64==(state==2||state==3));}
 actor.kind=It_Kind_Mario_Fire;actor.ground_or_air=GA_Ground;CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_ItemGrounded,NULL,0,&result,&diagnostic)&&result.as.u64==1);CHECK(!nativeScriptBindings_Write(&context,NativeScriptBinding_ItemAge,NULL,0,&negative,&diagnostic));
 actor.x1C=18;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemGravity,NULL,0,NULL,0,&result,&diagnostic));
}
static void sounds(void)
{
 const int ids[]={180025,540000,540001};for(unsigned looping=0;looping<2;++looping)for(unsigned i=0;i<3;++i)for(unsigned sample=0;sample<16;++sample){setup(sample);actor.sfx_unk1=sample&1?12:-1;Item initial=actor;if(looping)Item_8026AF0C(&actor,ids[i],(u8)(sample*13),(u8)(255-sample*11));else Item_8026AE84(&actor,ids[i],(u8)(sample*13),(u8)(255-sample*11));Item expected=actor;int log[16][4];unsigned count=script_item_sound_count;memcpy(log,script_item_sound,sizeof(log));actor=initial;scriptItemFixture_Reset();NativeScriptValue args[]={integer(ids[i]),integer(sample*13),integer(255-sample*11)};invoke(looping?NativeScriptBinding_ItemLoopSound:NativeScriptBinding_ItemPlaySound,args,3);CHECK(!memcmp(&actor,&expected,sizeof(actor))&&count==script_item_sound_count&&!memcmp(log,script_item_sound,count*sizeof(log[0])));++sound_pairs;}
 setup(0);NativeScriptValue invalid[]={integer(1),integer(256),integer(64)},result;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemPlaySound,NULL,0,invalid,3,&result,&diagnostic)&&script_item_sound_count==0);
}
static void collision_extents(void)
{
 const uint32_t ids[]={NativeScriptBinding_ECBUp,NativeScriptBinding_ECBDown,NativeScriptBinding_ECBFront,NativeScriptBinding_ECBBack};
 const float values[]={3.0f,-4.25f,0.0f,-0.0f,INFINITY,NAN};
 for(unsigned edge=0;edge<4;++edge)for(unsigned sample=0;sample<sizeof(values)/sizeof(*values);++sample) {
  setup(0);Item expected=actor;
  switch(edge){case 0:expected.x378_itemColl.ecb_source.up=values[sample];break;case 1:expected.x378_itemColl.ecb_source.down=values[sample];break;case 2:expected.x378_itemColl.ecb_source.front=values[sample];break;case 3:expected.x378_itemColl.ecb_source.back=values[sample];break;}
  NativeScriptValue input={.type=NativeScriptType_F32,.as.f32=values[sample]},output;
  CHECK(nativeScriptBindings_Write(&context,ids[edge],NULL,0,&input,&diagnostic));
  CHECK(!memcmp(&actor,&expected,sizeof(actor)));
  CHECK(nativeScriptBindings_Read(&context,ids[edge],NULL,0,&output,&diagnostic));
  CHECK(output.type==NativeScriptType_F32&&!memcmp(&output.as.f32,&input.as.f32,sizeof(float)));
  Item before=actor;NativeScriptRuntimeContext wrong=context;wrong.source_class=NativeCallbackSource_Fighter;
  CHECK(!nativeScriptBindings_Write(&wrong,ids[edge],NULL,0,&input,&diagnostic));
  CHECK(!nativeScriptBindings_Read(&wrong,ids[edge],NULL,0,&output,&diagnostic));
  ++actor.x1C;before.x1C=actor.x1C;
  output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
  CHECK(!nativeScriptBindings_Write(&context,ids[edge],NULL,0,&input,&diagnostic));
  CHECK(!nativeScriptBindings_Read(&context,ids[edge],NULL,0,&output,&diagnostic));
  CHECK(!memcmp(&actor,&before,sizeof(actor))&&output.type==NativeScriptType_U64&&output.as.u64==UINT64_MAX);
 }
}
static void frontend(void)
{
 const char* sources[]={"article It_Kind_Mario_Fire states 1:\n    func Tick() -> bool:\n        item.life = item.life - 1\n        item.gravity()\n        return item.life <= 0\n",
  "item Snack damage 3 angle 45 lifetime 180:\n    func Tick():\n        if item.age > 12:\n            velocity.y = velocity.y + 0.18\n        velocity.x = velocity.x * 0.99\n"};
 for(unsigned which=0;which<2;++which){const char* text=sources[which];NativeScriptSourceNode syntax[64];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,64};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptIRNode nodes[256];uint32_t args[128];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,256,args,0,128};NativeScriptFunctionDeclaration functions[4];NativeScriptParameterDeclaration params[4];NativeScriptValueDeclaration fields[4];NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=4,.parameters=params,.parameter_capacity=4,.values=fields,.value_capacity=4};CHECK(nativeScriptProgram_Index(&index,&diagnostic));NativeScriptBindingResolverContext binding={NativeScriptBindings_Article};NativeScriptLocal locals[16];NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&binding,.locals=locals,.local_capacity=16};NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));NativeScriptInstruction code[256];NativeScriptFunctionCode fn[1];NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=256,.functions=fn,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};CHECK(nativeScriptBytecode_Compile(&builder,&body,functions[0].source_node,0,0,&diagnostic));NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,fn,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[128];NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=registers,.register_capacity=128,.frames=frames,.frame_capacity=4};
  setup(29);if(which)actor.kind=It_Kind_NativeHoldable;Item reference=actor;HSD_GObj reference_object={.user_data=&reference};reference.entity=&reference_object;
  for(unsigned tick=0;tick<64;++tick){if(which){actor.xDD4_itemVar.native_holdable.age=reference.xDD4_itemVar.native_holdable.age=(u16)tick;if(tick>12)reference.x40_vel.y+=0.18;reference.x40_vel.x*=0.99;}else{reference.xD44_lifeTimer-=1;Item_ApplyFallingPhysics(&reference_object);}NativeScriptValue result;CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_Ok);CHECK(actor.x40_vel.x==reference.x40_vel.x&&actor.x40_vel.y==reference.x40_vel.y&&actor.xD44_lifeTimer==reference.xD44_lifeTimer&&actor.xD3C_spinSpeed==reference.xD3C_spinSpeed);if(!which)CHECK(result.type==NativeScriptType_Bool&&result.as.u64==(reference.xD44_lifeTimer<=0));++vm_ticks;}
 }
}
static void source_rejections(void)
{
 const char* sources[]={"article It_Kind_Mario_Fire states 1:\n    func Pending():\n        item.change_state(0)\n",
  "article It_Kind_Mario_Cape states 2:\n    func Pending() -> bool:\n        return item.attachment_active\n",
  "item Snack damage 3 angle 45 lifetime 180:\n    func Pending():\n        item.spawn_food()\n",
  "article It_Kind_Mario_Fire states 1:\n    func ReadOnly() -> bool:\n        return item.expire_tick()\n",
  "article It_Kind_Mario_Fire states 1:\n    func Identity() -> uint:\n        return item.creation_id\n"};
 for(unsigned i=0;i<5;++i){const char* text=sources[i];NativeScriptSourceNode syntax[32];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptIRNode nodes[128];uint32_t args[64];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,128,args,0,64};NativeScriptFunctionDeclaration functions[2];NativeScriptParameterDeclaration parameters[2];NativeScriptValueDeclaration fields[2];NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=2,.parameters=parameters,.parameter_capacity=2,.values=fields,.value_capacity=2};CHECK(nativeScriptProgram_Index(&index,&diagnostic));if(i>=3)functions[0].body_policy=NativeScriptBodyPolicy_ReadOnly;NativeScriptBindingResolverContext binding={NativeScriptBindings_Article};NativeScriptLocal locals[4];NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&binding,.locals=locals,.local_capacity=4};NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic)==(i!=3));}
 setup(0);NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};NativeScriptRuntimeContext wrong=context;wrong.source_class=NativeCallbackSource_Fighter;CHECK(!nativeScriptBindings_Invoke(&wrong,NativeScriptBinding_ItemExpireTick,NULL,0,NULL,0,&result,&diagnostic));CHECK(!nativeScriptBindings_Read(&wrong,NativeScriptBinding_ItemLife,NULL,0,&result,&diagnostic));
 // Signatures are supported; this Item-self executable has no source factories
 // or entity program, so the runtime boundary must still reject without output.
 NativeScriptValue state_arg=integer(0);int32_t old_state=actor.msid;
 CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemChangeState,NULL,0,&state_arg,1,&result,&diagnostic));
 CHECK(actor.msid==old_state&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
 // Collision dispatch now requires a source-owned actor. Its actual default/
 // Bool modes and map kernels are covered by script_article_motion, not this
 // engine-free Item-self fixture.
 Item old_item=actor;
 CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemStageCollision,NULL,0,NULL,0,&result,&diagnostic));
 CHECK(!memcmp(&old_item,&actor,sizeof(actor))&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
 NativeScriptValue stage_mode={.type=NativeScriptType_Bool,.as.u64=0};
 CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemStageCollisionMode,NULL,0,&stage_mode,1,&result,&diagnostic));
 CHECK(!memcmp(&old_item,&actor,sizeof(actor))&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
 CHECK(!nativeScriptBindings_Read(&context,NativeScriptBinding_ItemAttachmentActive,NULL,0,&result,&diagnostic));
 CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
 CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemSpawnFood,NULL,0,NULL,0,&result,&diagnostic));
 CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
 ++actor.x1C;float before=actor.xD44_lifeTimer;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_ItemExpireTick,NULL,0,NULL,0,&result,&diagnostic));CHECK(actor.xD44_lifeTimer==before&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
}
int main(void){physics();properties();sounds();collision_extents();frontend();source_rejections();printf("{\"checks\":%u,\"physics_pairs\":%u,\"sound_pairs\":%u,\"vm_ticks\":%u,\"status\":\"PASS Item self typed native contracts\"}\n",checks,physics_pairs,sound_pairs,vm_ticks);return 0;}
