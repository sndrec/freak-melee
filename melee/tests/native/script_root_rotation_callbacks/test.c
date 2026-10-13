#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_binding_presentation.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_pool.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=512,F=32,V=32};
static char text[8192];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N];static uint32_t args[N];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[2048];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
NativeScriptCompileBank presentation_bank;unsigned presentation_checks;static NativeScriptFighterProgram program;static NativeSourcePackage source;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];
static HSD_JObj copies[2];static u8 mapping[FT_COMMON_PART_COUNT];static unsigned comparisons;
void presentation_check(bool v,const char* message,unsigned line){++presentation_checks;if(!v){fprintf(stderr,"rotation callback %u: %s\n",line,message);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return source.assets.ready&&source.target==kind?&source:NULL;}
static void prepare(void){presentation_bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=args,.argument_capacity=N},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=2048,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* script="fighter:\n    behavior independent\nstate int resets\nstate float root_y\nfunc ReadRootY():\n    state.root_y = bones.root.rotation_y\nfunc TurnRootY():\n    bones.root.rotate_y(bones.root.rotation_y - 0.25)\nfunc ResetRootRotation():\n    bones.root.rotate_x(0)\n    state.resets = state.resets + 1\nfunc Arm():\n    callbacks.damage(ResetRootRotation)\n    callbacks.death2(ResetRootRotation)\nfunc Clear():\n    callbacks.clear_damage_death()\nfunc Alternate():\n    bones.root.rotate_y(1.5)\nfunc ArmAlternate():\n    callbacks.damage(Alternate)\nfunc BadParameter(int x):\n    pass\nfunc BadResult() -> bool:\n    return false\n";
 const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic d;NativeScriptCompileRequest request={.source=script,.source_bytes=(uint32_t)strlen(script)};
 if(!nativeScriptCompile_Build(&presentation_bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,accepted,NULL,NULL,&program,&d)){fprintf(stderr,"prepare %s\n",d.message);exit(1);}CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&program);}
static uint32_t function(const char* name){for(unsigned i=0;i<presentation_bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&presentation_bank.tree,declarations[i].name,name))return i;CHECK(false);return 0;}
static bool helper(HSD_GObj* object,const char* name){NativeScriptValue out={0};NativeScriptDiagnostic d;return nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function(name),NULL,0,&out,&d)==NativeScriptVM_Ok;}
static void rig(HSD_GObj* object,bool quaternion){Fighter* fp=object->user_data;NativeFighterPoolSlot* slot=nativeFighterPool_Slot(fp);CHECK(slot);fp->parts=slot->parts;fp->parts[0].joint=object->hsd_obj;fp->parts[0].x4_jobj2=&copies[0];fp->parts[1].joint=&copies[1];
 HSD_JObj* root=object->hsd_obj;*root=(HSD_JObj){.rotate={.x=-3.25f,.y=2.5f,.z=1.75f},.flags=quaternion?JOBJ_USE_QUATERNION:0};copies[0]=(HSD_JObj){.rotate={.x=4.5f,.y=-1.5f,.z=7.25f}};copies[1]=(HSD_JObj){.rotate={.x=9,.y=8,.z=7}};
 source=(NativeSourcePackage){.target=Ft_Kind_Mario};source.assets.ready=true;source.assets.facts.joints=2;source.assets.parts=(FighterPartsTable){.part_to_joint=mapping,.parts_num=2};source.assets.character.data=*fp->ft_data;fp->ft_data=&source.assets.character.data;tables[fp->kind]=&source.assets.parts;ftPartsTable=tables;memset(mapping,1,sizeof(mapping));}
static void native_reset(Fighter* fp){ftPartSetRotX(fp,0,0);}
static void native_takeover(HSD_GObj* object){(void)object;}
#include "root_y_reference.inc"
#include "root_y_cases.h"
int main(void){HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();
 for(unsigned q=0;q<2;++q){HSD_GObj* object=presentation_create(Ft_Kind_Mario,true);rig(object,q!=0);root_y_cases(object,q!=0);Fighter* fp=object->user_data;CHECK(helper(object,"Arm"));CHECK(!fp->take_dmg_cb&&!fp->death2_cb);CHECK(nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Damage)&&nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Death2));
  for(unsigned slot=0;slot<2;++slot){HSD_JObj original=*(HSD_JObj*)object->hsd_obj,copy=copies[0];native_reset(fp);HSD_JObj expected=*(HSD_JObj*)object->hsd_obj,expected_copy=copies[0];*(HSD_JObj*)object->hsd_obj=original;copies[0]=copy;CHECK(nativeScriptFighter_RunCallback(object,slot?NativeScriptFighterCallback_Death2:NativeScriptFighterCallback_Damage));CHECK(!memcmp(&expected.rotate,&((HSD_JObj*)object->hsd_obj)->rotate,sizeof(expected.rotate))&&!memcmp(&expected_copy.rotate,&copies[0].rotate,sizeof(copies[0].rotate)));CHECK(((HSD_JObj*)object->hsd_obj)->flags==expected.flags&&copies[0].flags==expected_copy.flags);CHECK(copies[1].rotate.x==9);++comparisons;}
  // Root index0 differs from the semantic-part mapping1 in this owned rig.
  CHECK(presentation_state(object,"resets",NULL).as.i64==2);
  uint8_t snapshot[65536];size_t bytes;CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));CHECK(helper(object,"Clear"));CHECK(!nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Damage)&&!nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Death2));CHECK(nativeScriptFighter_Load(snapshot,bytes));CHECK(nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Damage)&&nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Death2));
  CHECK(helper(object,"ArmAlternate"));bool matches=false;CHECK(nativeScriptFighter_CallbackMatches(object,NativeScriptFighterCallback_Damage,function("Alternate"),&matches)&&matches);CHECK(!nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Damage,function("BadParameter")));CHECK(!nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Damage,function("BadResult")));
  fp->take_dmg_cb=native_takeover;CHECK(!nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Damage));CHECK(helper(object,"Arm"));
  NativeScriptRuntimeContext context={.source=object,.source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};NativeScriptDiagnostic diagnostic;NativeScriptValue angle={.type=NativeScriptType_F32,.as.f32=0},output={.type=NativeScriptType_U64,.as.u64=99};HSD_JObj before=*(HSD_JObj*)object->hsd_obj;
  FighterPartsTable foreign=source.assets.parts;tables[fp->kind]=&foreign;CHECK(!nativeScriptBindingPresentation_Invoke(&context,NativeScriptBinding_BonesRootRotateX,&angle,1,&output,&diagnostic));CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&!memcmp(&before,object->hsd_obj,sizeof(before)));tables[fp->kind]=&source.assets.parts;
  HSD_JObj* saved=fp->parts[0].joint;fp->parts[0].joint=NULL;CHECK(!helper(object,"ResetRootRotation"));CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error);fp->parts[0].joint=saved;HSD_GObjFree(object);source.assets.ready=false;
 }
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS owned root reset callbacks\",\"checks\":%u,\"native_comparisons\":%u,\"euler_and_quaternion_copy\":true}\n",presentation_checks,comparisons);return 0;}
