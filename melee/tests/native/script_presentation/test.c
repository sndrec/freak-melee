#include "engine_fixture.h"
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_presentation.h>
#include <melee/ft/native_script_program.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/lb/lb_00B0.h>
#include <melee/ef/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;static HSD_GObj* heads[32];static HSD_GObj object;
static Fighter actor;static FighterBone bones[4];static HSD_JObj joints[8];
static FighterPartsTable table,*tables[Ft_Kind_RuntimeCapacity];static u8 mapping[FT_COMMON_PART_COUNT];
static NativeFighterDefinition definition;static NativeScriptRuntimeContext context;static NativeScriptDiagnostic diagnostic;
static unsigned checks,sound_pairs,transform_pairs;
// Real physical pool identity with a single fixture-owned native GObj payload.
NativeMatchPool nativeGObjPool;
static NativeMatchPoolSlot object_slot[1];static uint32_t object_free[1];
static uint64_t physical_epoch;

#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"Presentation check %d: %s (%s; source %u:%u)\n",__LINE__,#v,diagnostic.message,diagnostic.source.line,diagnostic.source.column);exit(1);}}while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
static NativeScriptValue integer(int64_t v){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=v};}
static void setup(unsigned sample)
{
 memset(&object,0,sizeof(object));memset(&actor,0,sizeof(actor));memset(heads,0,sizeof(heads));memset(joints,0,sizeof(joints));memset(bones,0,sizeof(bones));memset(&definition,0,sizeof(definition));memset(mapping,FTPART_INVALID,sizeof(mapping));
 CHECK(nativeMatchPool_Bind(&nativeGObjPool,&object,sizeof(object),object_slot,object_free,1,sizeof(object),_Alignof(HSD_GObj),++physical_epoch));
 NativeMatchPoolReference physical;CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&physical)==&object);
 HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;object.classifier=HSD_GOBJ_CLASS_FIGHTER;object.user_data=&actor;actor.gobj=&object;actor.x8_spawnNum=17;actor.kind=Ft_Kind_Mario;actor.parts=bones;actor.player_id=2;actor.x221F_b4=sample&1;
 definition.source_kind=Ft_Kind_None;definition.joint_count=4;scriptPresentationFixture_Reset(&definition);
 for(unsigned i=0;i<4;++i){HSD_CLASS_METHOD(&joints[i])=HSD_CLASS_INFO(&hsdJObj);HSD_CLASS_METHOD(&joints[i+4])=HSD_CLASS_INFO(&hsdJObj);bones[i].joint=&joints[i];bones[i].x4_jobj2=&joints[i+4];joints[i].flags=sample&1?JOBJ_USE_QUATERNION:0;joints[i].scale=(Vec3){1,1,1};joints[i].translate=(Vec3){(float)i+0.123f,-(float)i-0.75f,5.25f};joints[i].rotate=(Quaternion){.x=0.125f,.y=-0.5f,.z=1.75f};joints[i+4].rotate=(Quaternion){.x=-3,.y=2,.z=0.625f};
  joints[i].mtx[0][0]=1.1234f;joints[i].mtx[0][1]=-0.125f;joints[i].mtx[1][1]=0.8123f;joints[i].mtx[2][2]=-2.25f;joints[i].mtx[0][3]=7.123f;joints[i].mtx[1][3]=-9.765f;joints[i].mtx[2][3]=3.333f;
 }
 mapping[FtPart_TopN]=3;mapping[FtPart_LThumbNb]=2;mapping[FtPart_TransN]=1;table=(FighterPartsTable){NULL,mapping,4};for(unsigned i=0;i<Ft_Kind_RuntimeCapacity;++i)tables[i]=&table;ftPartsTable=tables;
 CHECK(nativeScriptBindings_Context(&object,NativeCallbackSource_Fighter,17,&context,&diagnostic));context.scene_epoch=nativeGObjPool.lifetime;
}
static NativeScriptValue invoke(uint32_t id,NativeScriptValue* args,unsigned count)
{NativeScriptValue result;CHECK(nativeScriptBindings_Invoke(&context,id,NULL,0,args,count,&result,&diagnostic));CHECK(result.type==nativeScriptBindings_Descriptor(id)->result_type);return result;}
static void native_sound(unsigned channel,int sound,u8 volume,u8 pan)
{
 switch(channel){case 0:ft_PlaySFX(&actor,sound,volume,pan);break;case 1:ft_80088478(&actor,sound,volume,pan);break;case 2:ft_800881D8(&actor,sound,volume,pan);break;case 3:ft_80088510(&actor,sound,volume,pan);break;case 4:ft_800885A8(&actor,sound,volume,pan);break;case 5:ft_80088640(&actor,sound,volume,pan);break;case 6:ft_80088328(&actor,sound,volume,pan);break;}
}
static void sound(void)
{
 const int sounds[]={332,359,407,130021,0x83D60,0x83D61};
 const NativeFighterSoundIdRule rule={.costume_mask=1,.bank=0,.first=332,.last=370,.offset=100};
 const NativeFighterSoundPolicy policy={.rules=&rule,.rule_count=1};
 for(unsigned channel=0;channel<7;++channel)for(unsigned id=0;id<6;++id)for(unsigned muted=0;muted<2;++muted)for(unsigned sample=0;sample<8;++sample){
  setup(sample);actor.x2225_b6=true;definition.sound_mute_channels=muted?127:0;actor.x2220_b5=sample&2;actor.x2220_b6=sample&4;actor.x619_costume_id=(sample>>1)&1;if(sample&4)definition.sound_policy=&policy;Fighter original=actor;*seed_ptr=123+sample;
  native_sound(channel,sounds[id],(u8)(127+sample*77),(u8)(64-sample*33));Fighter expected=actor;int log[32][5];unsigned count=script_presentation_log_count;memcpy(log,script_presentation_log,sizeof(log));u32 wanted=*seed_ptr;
  actor=original;scriptPresentationFixture_Reset(&definition);*seed_ptr=123+sample;NativeScriptValue args[]={integer(channel),integer(sounds[id]),integer(127+sample*77),integer(64-(int)sample*33)};invoke(NativeScriptBinding_SoundChannel,args,4);
  CHECK(!memcmp(&actor,&expected,sizeof(actor))&&*seed_ptr==wanted);CHECK(count==script_presentation_log_count&&!memcmp(log,script_presentation_log,count*sizeof(log[0])));++sound_pairs;
 }
 setup(0);NativeScriptValue args[]={integer(100),integer(127),integer(64)};invoke(NativeScriptBinding_SoundPlay,args,3);CHECK(script_presentation_log[0][0]==1);scriptPresentationFixture_Reset(&definition);invoke(NativeScriptBinding_SoundLoop,args,3);CHECK(script_presentation_log[0][4]==0x42+4);
 NativeScriptValue invalid[]={integer(7),integer(1),integer(127),integer(64)},result;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_SoundChannel,NULL,0,invalid,4,&result,&diagnostic));
}
static void transforms(void)
{
 const uint32_t methods[]={NativeScriptBinding_BonesRootRotateXWithJoint,NativeScriptBinding_BonesRootRotateYWithJoint,NativeScriptBinding_BonesRootRotateZWithJoint};
 for(unsigned quaternion=0;quaternion<2;++quaternion)for(unsigned axis=0;axis<3;++axis)for(unsigned index=0;index<4;++index)for(unsigned sample=0;sample<32;++sample){
  setup(quaternion);double angle=(double)(int)sample*0.010000000000001-0.125;HSD_JObj original[8];memcpy(original,joints,sizeof(joints));if(axis==0)ftPartSetRotX(&actor,index,(float)angle);else if(axis==1)ftPartSetRotY(&actor,index,(float)angle);else ftPartSetRotZ(&actor,index,(float)angle);HSD_JObj expected[8];memcpy(expected,joints,sizeof(joints));memcpy(joints,original,sizeof(joints));NativeScriptValue args[]={integer(index),{.type=NativeScriptType_F64,.as.f64=angle}};invoke(methods[axis],args,2);CHECK(!memcmp(joints,expected,sizeof(joints)));++transform_pairs;
 }
 setup(0);NativeScriptValue part=integer(FtPart_LThumbNb);CHECK(invoke(NativeScriptBinding_BonesIndex,&part,1).as.i64==2);NativeScriptValue args[]={integer(FtPart_TopN),{.type=NativeScriptType_F32,.as.f32=9.5f}};invoke(NativeScriptBinding_BonesRotateX,args,2);CHECK(joints[3].rotate.x==9.5f&&joints[0].rotate.x==0.125f);
 NativeScriptValue angle={.type=NativeScriptType_F32,.as.f32=2.25f};invoke(NativeScriptBinding_BonesRootRotateY,&angle,1);CHECK(joints[0].rotate.y==2.25f);invoke(NativeScriptBinding_BonesRootResetZ,NULL,0);CHECK(joints[0].rotate.z==0);
 NativeScriptValue result;CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_BonesCount,NULL,0,&result,&diagnostic)&&result.as.i64==4);CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_BonesRootRotationX,NULL,0,&result,&diagnostic)&&result.as.f32==ftPartGetRotX(&actor,0));
 NativeScriptValue scale[]={integer(FtPart_LThumbNb),{.type=NativeScriptType_F64,.as.f64=1.123456789},{.type=NativeScriptType_F32,.as.f32=-2},{.type=NativeScriptType_I32,.as.i64=3}};invoke(NativeScriptBinding_BonesSetScale,scale,4);CHECK(joints[2].scale.x==(float)1.123456789&&joints[2].scale.y==-2&&joints[2].scale.z==3);
 NativeScriptValue copy[]={integer(2),{.type=NativeScriptType_Bool,.as.u64=1}};invoke(NativeScriptBinding_BonesSetDirectAnimationCopy,copy,2);CHECK(bones[2].flags_b4);
 for(unsigned sample=0;sample<32;++sample){setup(0);joints[2].parent=&joints[4];Vec3 offset={(float)sample*0.123f,-0.25f,3.456f},expected;lb_8000B1CC(&joints[2],&offset,&expected);NativeScriptValue out[]={integer(2),{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32,.as.f32=offset.x},{.type=NativeScriptType_F32,.as.f32=offset.y},{.type=NativeScriptType_F32,.as.f32=offset.z}};invoke(NativeScriptBinding_BonesWorldPositionWithOffset,out,7);CHECK(out[1].as.f32==expected.x&&out[2].as.f32==expected.y&&out[3].as.f32==expected.z);++transform_pairs;}
 setup(0);part=integer(FtPart_HeadN);CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_BonesIndex,NULL,0,&part,1,&result,&diagnostic));joints[0].flags|=JOBJ_USE_QUATERNION;bones[0].x4_jobj2=NULL;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_BonesRootRotateY,NULL,0,&angle,1,&result,&diagnostic));
}
static void frontend(void)
{
 const char* sources[]={"func Transform() -> float:\n    let float x = 0\n    let float y = 0\n    let float z = 0\n    bones.world_position(bones.index(LeftThumb), x, y, z, 0.125, -0.25, 3.5)\n    bones.root.rotate_y(pi_half * facing.direction)\n    sound.channel(2, 332, 127, 64)\n    return x + y + z\n",
  "func PositionThenWrite():\n    let float x = 99\n    let float y = 88\n    let float z = 77\n    bones.world_position(2, x, y, z)\n    position.x = x\n"};
 for(unsigned which=0;which<2;++which){
  const char* text=sources[which];NativeScriptSourceNode syntax[64];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,64};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptIRNode nodes[256];uint32_t arguments[128];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,256,arguments,0,128};NativeScriptFunctionDeclaration functions[4];NativeScriptParameterDeclaration params[4];NativeScriptValueDeclaration fields[4];NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=4,.parameters=params,.parameter_capacity=4,.values=fields,.value_capacity=4};CHECK(nativeScriptProgram_Index(&index,&diagnostic));NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};NativeScriptLocal locals[16];NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&binding,.locals=locals,.local_capacity=16};NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));NativeScriptInstruction code[256];NativeScriptFunctionCode fn[1];NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=256,.functions=fn,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};CHECK(nativeScriptBytecode_Compile(&builder,&body,functions[0].source_node,0,0,&diagnostic));NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,fn,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[128];NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=registers,.register_capacity=128,.frames=frames,.frame_capacity=4};
  if(which==0){setup(0);actor.facing_dir=-1;joints[2].parent=&joints[4];Vec3 offset={0.125f,-0.25f,3.5f},expected;lb_8000B1CC(&joints[2],&offset,&expected);NativeScriptValue result;CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_Ok);CHECK(result.as.f32==(expected.x+expected.y)+expected.z&&joints[0].rotate.y==(float)(-3.14159265358979323846/2));}
  else for(unsigned replace=0;replace<2;++replace){setup(0);actor.cur_pos.x=17;joints[2].parent=&joints[4];joints[2].flags|=JOBJ_MTX_DIRTY;scriptPresentationFixture_RetireOnMatrix(&object,replace!=0);NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_SourceRetired);CHECK(actor.cur_pos.x==17&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);CHECK(vm.depth==0&&vm.register_top==0);}
 }
}
static void source_validation(void)
{
 const char* sources[]={"func SameOutputs():\n    let float x = 0\n    let float z = 0\n    bones.world_position(2, x, x, z)\n",
  "func WrongOutput():\n    let int x = 0\n    let float y = 0\n    let float z = 0\n    bones.world_position(2, x, y, z)\n",
  "func PureIndex() -> int:\n    return bones.index(LeftThumb)\n",
  "func Audio():\n    sound.play(332, 127, 64)\n"};
 for(unsigned i=0;i<4;++i){const char* text=sources[i];NativeScriptSourceNode syntax[32];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptIRNode nodes[128];uint32_t arguments[64];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,128,arguments,0,64};NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};NativeScriptLocal locals[8];NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&binding,.locals=locals,.local_capacity=8,.return_type=i==2?NativeScriptType_I32:NativeScriptType_Void,.body_policy=i>=2?NativeScriptBodyPolicy_ReadOnly:0};bool valid=nativeScriptStatement_Compile(&body,1,&diagnostic);CHECK(valid==(i==2));if(i==0)CHECK(strstr(diagnostic.message,"distinct")||strstr(diagnostic.message,"duplicate"));}
}
static void source_rig_ownership(void)
{
 static NativeSourcePackage source;
 for(unsigned variant=0;variant<6;++variant){
  setup(0);memset(&source,0,sizeof(source));source.target=actor.kind=Ft_Kind_CustomFirst;
  source.assets.ready=true;source.assets.facts.joints=4;source.assets.parts=table;
  actor.ft_data=&source.assets.character.data;tables[actor.kind]=&source.assets.parts;
  scriptPresentationFixture_Source(&source);
  NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
  // Unprepared source metadata has no admitted actor/program/view. It must reject.
  CHECK(!nativeScriptBindings_Read(&context,NativeScriptBinding_BonesCount,NULL,0,&result,&diagnostic));
  switch(variant){
  case 0:source.assets.ready=false;break;case 1:actor.ft_data=NULL;break;
  case 2:tables[actor.kind]=&table;break;case 3:++source.assets.facts.joints;break;
  case 4:source.assets.parts.parts_num=0;break;case 5:source.assets.parts.parts_num=MAX_FT_PARTS+1;break;
  }
  result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
  CHECK(!nativeScriptBindings_Read(&context,NativeScriptBinding_BonesCount,NULL,0,&result,&diagnostic));
  CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
 }
}
int main(void){sound();transforms();frontend();source_validation();source_rig_ownership();printf("{\"checks\":%u,\"sound_pairs\":%u,\"transform_pairs\":%u,\"status\":\"PASS presentation typed native contracts\"}\n",checks,sound_pairs,transform_pairs);return 0;}
