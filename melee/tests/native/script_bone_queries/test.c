#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_binding_presentation.h>

#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_pool.h>

#include <melee/ft/ftdata.h>
#include <sysdolphin/baselib/gobjplink.h>




#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "throw_angle_source.h"
#pragma clang section data = ""
enum{N=1024,F=32,V=64};
static char text[16384];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N];static uint32_t arguments[N];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch call_graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
NativeScriptCompileBank presentation_bank;unsigned presentation_checks;static NativeSourcePackage source;
#include "native_leaves.h"
static NativeScriptDiagnostic diagnostic;
static HSD_JObj joints[4],replacement_joint;static Vec3 scale_storage[4];static HSD_RObj observer;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];static MotionState motions[1];
HSD_JObjInfo hsdJObj;
static HSD_GObj* active,*replacement_object;
static unsigned comparisons,cases,mutations,mode,class_mode,helper_calls;
static NativeScriptReceiverView retained;
static uint64_t old_layout,old_params,old_epoch;static uint32_t old_generation;
void presentation_check(bool result,const char* expression,unsigned line)
{++presentation_checks;if(!result){fprintf(stderr,"bone query %u: %s (%u:%u %s)\n",line,expression,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return source.ready&&source.target==kind?&source:NULL;}
static void initialize_bank(void)
{
 NativeScriptCompileBank* bank=&source.compiler.bank;
 *bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=call_graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void compiler_negatives(void)
{
 const char* calls[]={
  "bones.world_position(1.5, state.x, state.y, state.z)",
  "bones.index(1.5)",
  "bones.world_position(2, state.x, state.x, state.z)",
  "bones.world_position(2, state.index, state.y, state.z)",
  "bones.world_position(2, state.x, state.y)",
  "bones.world_position(2, state.x, state.y, state.z, 1)",
  "bones.world_position(2, state.x, state.y, state.z, 1, 2, 3, 4)",
  "bones.index(LeftThumb, 1)",
  "bones.world_position(state.x, state.x, state.y, state.z)"
 };
 NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
 for(unsigned i=0;i<sizeof(calls)/sizeof(calls[0]);++i){
  char bad[1024];snprintf(bad,sizeof(bad),"fighter:\n    behavior independent\nstate float x\nstate float y\nstate float z\nstate int index\nfunc Bad():\n    %s\n",calls[i]);
  NativeScriptCompileRequest request={.source=bad,.source_bytes=(uint32_t)strlen(bad)};
  const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));CHECK(!scratch.bank.valid);++cases;
 }
 nativeSourceBank_Release(&scratch);
}
static void prepare(void)
{
 source.address=&source;source.ready=source.assets.ready=true;source.target=Ft_Kind_CustomFirst;initialize_bank();
 const char* script="fighter:\n    behavior independent\nstate float x\nstate float y\nstate float z\nstate int index\nstate int count\nstate int after\n"
 "func Query():\n    state.count = bones.count\n    state.index = bones.index(LeftThumb)\n    bones.world_position(state.index, state.x, state.y, state.z)\n    state.after = state.after + 1\n"
 "func Offset():\n    bones.world_position(bones.index(LeftThumb), state.x, state.y, state.z, 0.25, -0.5, 1.75)\n    state.after = state.after + 1\n"
 "func NamedOffset():\n    bones.world_position_offset(bones.index(LeftThumb), state.x, state.y, state.z, 0.25, -0.5, 1.75)\n"
 "func Raw():\n    bones.world_position(1, state.x, state.y, state.z)\n"
 "func ReadRootScale():\n    state.x = bones.root.scale_x\n    state.y = bones.root.scale_y\n    state.z = bones.root.scale_z\n    state.after = state.after + 1\n"
 "func SetRootScale():\n    bones.root.set_scale(state.x, state.y, state.z)\n    state.after = state.after + 1\n"
 "func Translate():\n    bones.set_translation(state.index, state.x, state.y, state.z)\n"
 "state int shield_age\nstate int tilt_age\nfunc WriteAges():\n    input.analog_shield_age = state.shield_age\n    input.horizontal_tilt_age = state.tilt_age\nfunc ReadAges():\n    state.shield_age = input.analog_shield_age\n    state.tilt_age = input.horizontal_tilt_age\n"
 THROW_ANGLE_SOURCE
 "func Invalid():\n    bones.world_position(-1, state.x, state.y, state.z)\n    state.after = state.after + 1\n";
 source.assets.facts.moves=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=script,.source_bytes=(uint32_t)strlen(script),.motion_layout=&source.assets.facts.moves};
 const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&source.compiler.bank,&request,&accepted,&diagnostic));
 CHECK(nativeScriptFighter_PrepareProgram(source.target,&source.compiler.bank,&source.assets.facts.moves,NULL,&source.program,&diagnostic));
 CHECK(nativeScriptFighter_CanPublish(source.target));nativeScriptFighter_PublishProgram(source.target,&source.program);
 source.assets.facts.joints=4;source.assets.parts=(FighterPartsTable){.parts_num=4,.part_to_joint=source.assets.part_to_joint,.joint_to_part=source.assets.joint_to_part};
 for(unsigned i=0;i<FT_COMMON_PART_COUNT;++i)source.assets.part_to_joint[i]=2;
 source.assets.motions=motions;tables[source.target]=&source.assets.parts;ftPartsTable=tables;
 gFtDataList[source.target]=&source.assets.character.data;ftData_CharacterStateTables[source.target]=motions;
 presentation_bank=source.compiler.bank;
}
static void class_initialize(void)
{
 HSD_CLASS_INFO(&hsdJObj)->head.flags|=1;
 if(class_mode==1)retained.storage->live=false;
 if(class_mode==2)((Fighter*)active->user_data)->parts[2].joint=&replacement_joint;
 if(class_mode==3)active->hsd_obj=&replacement_joint;
}
static void rig(HSD_GObj* object,unsigned shape)
{
 Fighter* fp=object->user_data;NativeFighterPoolSlot* pool=nativeFighterPool_Slot(fp);CHECK(pool);
 fp->ft_data=&source.assets.character.data;gFtDataList[source.target]=fp->ft_data;ftData_CharacterStateTables[source.target]=motions;
 fp->parts=pool->parts;memset(joints,0,sizeof(joints));memset(&replacement_joint,0,sizeof(replacement_joint));
 memset(&hsdJObj,0,sizeof(hsdJObj));HSD_CLASS_INFO(&hsdJObj)->head.flags=1;HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);
 HSD_CLASS_INFO(&hsdJObj)->head.info_init=class_initialize;hsdJObj.make_mtx=HSD_JObjMakeMatrix;
 for(unsigned i=0;i<4;++i){HSD_CLASS_METHOD(&joints[i])=HSD_CLASS_INFO(&hsdJObj);joints[i].scl=&scale_storage[i];
  joints[i].translate=(Vec3){1.25f+i,-2.5f+i,3.75f-i};joints[i].scale=(Vec3){1.5f,.75f,-2};
  joints[i].rotate=(Quaternion){.x=.2f,.y=-.35f,.z=.6f};joints[i].flags=JOBJ_MTX_DIRTY;fp->parts[i].joint=&joints[i];}
 joints[0].child=&joints[1];joints[1].parent=&joints[0];joints[1].child=&joints[2];
 if(shape)joints[2].parent=&joints[1];
 if(shape==2){joints[2].flags|=JOBJ_USE_QUATERNION;joints[2].rotate=(Quaternion){.x=.2f,.y=-.3f,.z=.1f,.w=.9f};}
 object->hsd_obj=&joints[0];object->obj_kind=HSD_GObj_JObjKind;
 CHECK(nativeScriptFighter_View(object,fp->x8_spawnNum,&retained));active=object;mode=class_mode=0;
}
static uint32_t function(const char* name)
{for(unsigned i=0;i<source.compiler.bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&source.compiler.bank.tree,declarations[i].name,name))return i;CHECK(false);return 0;}
static NativeScriptVMStatus helper(HSD_GObj* object,const char* name)
{NativeScriptValue out={0};++helper_calls;return nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function(name),NULL,0,&out,&diagnostic);}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)object->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue floating(float value){return(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};}
static NativeScriptValue integer(int value){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=value};}
static void seed(HSD_GObj* object)
{NativeScriptValue v=floating(91);presentation_state(object,"x",&v);v=floating(92);presentation_state(object,"y",&v);v=floating(93);presentation_state(object,"z",&v);}
static void restores(void)
{
 source.compiler.bank.storage_layout.identity=old_layout;source.compiler.bank.storage_parameters.identity=old_params;
 nativeGObjPool.lifetime=old_epoch;Fighter* fp=active->user_data;fp->x8_spawnNum=old_generation;
 fp->parts=nativeFighterPool_Slot(fp)->parts;fp->parts[2].joint=&joints[2];
 joints[2].parent=&joints[1];HSD_CLASS_METHOD(&joints[2])=HSD_CLASS_INFO(&hsdJObj);
 HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);retained.storage->live=true;
 tables[source.target]=&source.assets.parts;
}
void HSD_RObjUpdateAll(HSD_RObj* r,void* obj,HSD_ObjUpdateFunc update)
{
 (void)r;(void)obj;(void)update;if(!mode)return;++mutations;
 Fighter* fp=active->user_data;
 switch(mode){
 case 1:retained.storage->live=false;break;
 case 2:++fp->x8_spawnNum;break;
 case 3:++source.compiler.bank.storage_layout.identity;break;
 case 4:++source.compiler.bank.storage_parameters.identity;break;
 case 5:fp->parts[2].joint=&replacement_joint;break;
 case 6:HSD_CLASS_METHOD(&joints[2])=NULL;break;
 case 7:tables[source.target]=NULL;break;
 case 8:fp->parts=NULL;break;
 case 9:joints[2].parent=NULL;break;
 case 10:++nativeGObjPool.lifetime;break;
 case 11:HSD_GObjFree(active);replacement_object=presentation_create(source.target,false);break;
 default:abort();}
 mode=0;
}
static void native_pairs(void)
{
 for(unsigned shape=0;shape<3;++shape)for(unsigned offsets=0;offsets<3;++offsets)for(unsigned iteration=0;iteration<6;++iteration){
  HSD_GObj* object=presentation_create(source.target,true);rig(object,shape);
  Vec3 offset={.25f,-.5f,1.75f};joints[2].translate.x+=(float)iteration*.3f;
  Vec3 expected;lb_8000B1CC(&joints[2],offsets?&offset:NULL,&expected);
  for(unsigned i=0;i<4;++i)joints[i].flags|=JOBJ_MTX_DIRTY;
  CHECK(helper(object,offsets==0?"Query":offsets==1?"Offset":"NamedOffset")==NativeScriptVM_Ok);
  const char* names[]={"x","y","z"};float components[]={expected.x,expected.y,expected.z};
  for(unsigned i=0;i<3;++i){NativeScriptValue got=presentation_state(object,names[i],NULL);CHECK(got.type==NativeScriptType_F32&&!memcmp(&got.as.f32,&components[i],4));}
  if(!offsets){CHECK(presentation_state(object,"index",NULL).as.i64==nativeFighterRig_Id(&source.assets.rig,2));CHECK(presentation_state(object,"count",NULL).as.i64==4);}
  ++comparisons;++cases;HSD_GObjFree(object);
 }
 HSD_GObj* object=presentation_create(source.target,true);rig(object,1);Vec3 expected;lb_8000B1CC(&joints[1],NULL,&expected);
 CHECK(helper(object,"Raw")==NativeScriptVM_Ok);CHECK(presentation_state(object,"x",NULL).as.f32==expected.x);CHECK(expected.x!=joints[2].mtx[0][3]);++cases;HSD_GObjFree(object);
}
static void retirement(void)
{
 for(unsigned m=1;m<=10;++m){HSD_GObj* object=presentation_create(source.target,true);rig(object,1);seed(object);
  old_layout=source.compiler.bank.storage_layout.identity;old_params=source.compiler.bank.storage_parameters.identity;old_epoch=nativeGObjPool.lifetime;old_generation=((Fighter*)object->user_data)->x8_spawnNum;
  joints[2].robj=&observer;mode=m;
  CHECK(helper(object,"Query")!=NativeScriptVM_Ok);restores();
  CHECK(presentation_state(object,"x",NULL).as.f32==91&&presentation_state(object,"y",NULL).as.f32==92&&presentation_state(object,"z",NULL).as.f32==93);
  CHECK(presentation_state(object,"after",NULL).as.i64==0);++cases;HSD_GObjFree(object);
 }
}
static void direct_negatives(void)
{
 HSD_GObj* object=presentation_create(source.target,true);rig(object,1);NativeScriptRuntimeContext c=context(object);
 NativeScriptValue args[]={integer(2),floating(91),floating(92),floating(93)},out={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesWorldPosition,args,3,&out,&diagnostic));
 args[0]=floating(2);CHECK(!nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesWorldPosition,args,4,&out,&diagnostic));args[0]=integer(2);
 args[2]=integer(92);CHECK(!nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesWorldPosition,args,4,&out,&diagnostic));args[2]=floating(92);
 for(unsigned i=0;i<3;++i){args[0]=integer(i==0?-1:i==1?4:2);HSD_JObj* save=((Fighter*)object->user_data)->parts[2].joint;if(i==2)((Fighter*)object->user_data)->parts[2].joint=NULL;
  CHECK(!nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesWorldPosition,args,4,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&args[1].as.f32==91&&args[2].as.f32==92&&args[3].as.f32==93);((Fighter*)object->user_data)->parts[2].joint=save;++cases;}
 args[0]=integer(2);HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj)-1;
 CHECK(!nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesWorldPosition,args,4,&out,&diagnostic));HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);
 for(unsigned i=1;i<=2;++i){class_mode=i;HSD_CLASS_INFO(&hsdJObj)->head.flags=0;CHECK(!nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesWorldPosition,args,4,&out,&diagnostic));retained.storage->live=true;((Fighter*)object->user_data)->parts[2].joint=&joints[2];++cases;}
 HSD_GObjFree(object);
 // Actual physical pool reuse; the admitted graph is fixture-owned static storage,
 // so native sampling can finish while its source GObj/actor is retired.
 object=presentation_create(source.target,true);rig(object,1);c=context(object);
 args[0]=integer(2);joints[2].robj=&observer;mode=11;replacement_object=NULL;
 CHECK(!nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesWorldPosition,args,4,&out,&diagnostic));
 CHECK(replacement_object==object);CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&args[1].as.f32==91&&args[2].as.f32==92&&args[3].as.f32==93);
 ++cases;HSD_GObjFree(replacement_object);
}
static void root_scale_cases(void)
{
 static const Vec3 scales[]={{1.25f,-2,.5f},{0,-0.0f,3},{.125f,8,-1},{INFINITY,-INFINITY,NAN}};
 for(unsigned root_index=0;root_index<=3;root_index+=3)
  for(unsigned independent=0;independent<2;++independent)
   for(unsigned sample=0;sample<sizeof(scales)/sizeof(scales[0]);++sample){
    HSD_GObj* object=presentation_create(source.target,true);rig(object,1);
    object->hsd_obj=&joints[root_index];HSD_JObj* root=object->hsd_obj;
    root->scale=scales[sample];root->flags=independent?JOBJ_MTX_INDEP_SRT:0;
    CHECK(helper(object,"ReadRootScale")==NativeScriptVM_Ok);
    const char* fields[]={"x","y","z"};float values[]={root->scale.x,root->scale.y,root->scale.z};
    for(unsigned axis=0;axis<3;++axis){NativeScriptValue value=presentation_state(object,fields[axis],NULL);
     CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&values[axis],sizeof(float)));}
    // Use the real native setter as the reference, including matrix-dirty flags.
    Vec3 desired=scales[(sample+1)%4];static HSD_JObj before[4],expected[4];
    memcpy(before,joints,sizeof(joints));HSD_JObjSetScale(root,&desired);memcpy(expected,joints,sizeof(joints));memcpy(joints,before,sizeof(joints));
    values[0]=desired.x;values[1]=desired.y;values[2]=desired.z;
    for(unsigned axis=0;axis<3;++axis){NativeScriptValue value=floating(values[axis]);presentation_state(object,fields[axis],&value);}
    CHECK(helper(object,"SetRootScale")==NativeScriptVM_Ok&&!memcmp(joints,expected,sizeof(joints)));
    CHECK(((Fighter*)object->user_data)->x34_scale.y==1);
    HSD_GObjFree(object);++cases;++comparisons;
   }
 for(unsigned setter=0;setter<2;++setter)for(unsigned invalid=0;invalid<8;++invalid){
  HSD_GObj* object=presentation_create(source.target,true);rig(object,1);seed(object);
  NativeScriptRuntimeContext c=context(object);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99},original=out;
  NativeScriptValue args[]={floating(4),floating(5),floating(6)};unsigned count=3;
  switch(invalid){
  case 0:object->hsd_obj=NULL;break;
  case 1:object->obj_kind=HSD_GObj_JObjKind+1;break;
  case 2:HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj)-1;break;
  case 3:++c.generation;break;
  case 4:++c.scene_epoch;break;
  case 5:retained.storage->live=false;break;
  case 6:class_mode=1;HSD_CLASS_INFO(&hsdJObj)->head.flags=0;break;
  case 7:class_mode=3;HSD_CLASS_INFO(&hsdJObj)->head.flags=0;break;
  }
  static HSD_JObj before[4];memcpy(before,joints,sizeof(joints));
  bool ok=setter?nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesRootSetScale,args,count,&out,&diagnostic):
   nativeScriptBindingPresentation_Read(&c,NativeScriptBinding_BonesRootScaleX,&out,&diagnostic);
  CHECK(!ok&&!memcmp(&out,&original,sizeof(out))&&!memcmp(joints,before,sizeof(joints)));
  // Restore deliberately invalid fixture identities before native GObj cleanup.
  object->hsd_obj=&joints[0];object->obj_kind=HSD_GObj_JObjKind;
  HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);HSD_CLASS_INFO(&hsdJObj)->head.flags=1;class_mode=0;
  retained.storage->live=true;HSD_GObjFree(object);++cases;
 }
}

static void translations(void)
{
 const Vec3 samples[]={{-0.0f,0,1},{2.5f,-4.25f,8},{INFINITY,-INFINITY,NAN}};
 const u32 flags[]={0,JOBJ_MTX_INDEP_SRT,JOBJ_MTX_INDEP_PARENT,JOBJ_MTX_DIRTY};
 for(unsigned index=0;index<4;++index)for(unsigned sample=0;sample<3;++sample)for(unsigned f=0;f<4;++f){
  HSD_GObj* object=presentation_create(source.target,true);rig(object,1);
  for(unsigned n=0;n<4;++n)joints[n].flags=flags[f];
  static HSD_JObj before[4],expected[4];memcpy(before,joints,sizeof(joints));
  HSD_JObjSetTranslate(&joints[index],&samples[sample]);memcpy(expected,joints,sizeof(joints));memcpy(joints,before,sizeof(joints));
  NativeScriptValue value=integer(index);presentation_state(object,"index",&value);
  const char* names[]={"x","y","z"};float components[]={samples[sample].x,samples[sample].y,samples[sample].z};
  for(unsigned n=0;n<3;++n){value=floating(components[n]);presentation_state(object,names[n],&value);}
  CHECK(helper(object,"Translate")==NativeScriptVM_Ok&&!memcmp(joints,expected,sizeof(joints)));
  CHECK(((Fighter*)object->user_data)->cur_pos.x==0&&((Fighter*)object->user_data)->x34_scale.y==1);
  HSD_GObjFree(object);++cases;++comparisons;
 }
 for(unsigned invalid=0;invalid<10;++invalid){
  HSD_GObj* object=presentation_create(source.target,true);rig(object,1);
  NativeScriptRuntimeContext c=context(object);NativeScriptValue values[]={integer(2),floating(1),floating(2),floating(3)},out={.type=NativeScriptType_U64,.as.u64=99};
  Fighter* fighter=object->user_data;FighterBone* original=fighter->parts;unsigned count=4;
  switch(invalid){
  case 0:values[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=INT64_MAX};break;
  case 1:values[0]=floating(2);break;
  case 2:values[2]=integer(2);break;
  case 3:count=3;break;
  case 4:fighter->parts[2].joint=NULL;break;
  case 5:fighter->parts=NULL;break;
  case 6:++c.generation;break;
  case 7:++c.scene_epoch;break;
  case 8:class_mode=1;HSD_CLASS_INFO(&hsdJObj)->head.flags=0;break;
  case 9:class_mode=3;HSD_CLASS_INFO(&hsdJObj)->head.flags=0;break;
  }
  static HSD_JObj before[4];memcpy(before,joints,sizeof(joints));
  CHECK(!nativeScriptBindingPresentation_Invoke(&c,NativeScriptBinding_BonesSetTranslation,values,count,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
  // Class initializers can intentionally replace class/root metadata. Compare
  // the selected translation and flags, which must not have been mutated.
  for(unsigned n=0;n<4;++n)CHECK(!memcmp(&joints[n].translate,&before[n].translate,sizeof(Vec3))&&joints[n].flags==before[n].flags);
  fighter->parts=original;fighter->parts[2].joint=&joints[2];retained.storage->live=true;
  HSD_CLASS_INFO(&hsdJObj)->head.flags=1;class_mode=0;HSD_GObjFree(object);++cases;
 }
 for(unsigned n=0;n<2;++n){HSD_GObj* object=presentation_create(source.target,true);rig(object,1);
  NativeScriptValue value=integer(n?-1:4);presentation_state(object,"index",&value);seed(object);
  static HSD_JObj before[4];memcpy(before,joints,sizeof(joints));
  CHECK(helper(object,"Translate")==NativeScriptVM_Ok&&!memcmp(before,joints,sizeof(joints)));HSD_GObjFree(object);++cases;}
}

static void input_ages(void)
{
 HSD_GObj* object=presentation_create(source.target,true);rig(object,1);Fighter* fighter=object->user_data;
 const int ages[]={0,1,2,127,253,254};NativeScriptRuntimeContext c=context(object);
 for(unsigned n=0;n<6;++n){NativeScriptValue shield=integer(ages[n]),tilt=integer(ages[5-n]);
  presentation_state(object,"shield_age",&shield);presentation_state(object,"tilt_age",&tilt);
  fighter->x673=73;fighter->x671_timer_lstick_tilt_y=41;
  CHECK(helper(object,"WriteAges")==NativeScriptVM_Ok&&fighter->trigger_analog_timer==ages[n]&&fighter->x670_timer_lstick_tilt_x==ages[5-n]);
  CHECK(fighter->x673==73&&fighter->x671_timer_lstick_tilt_y==41);
  fighter->trigger_analog_timer=ages[5-n];fighter->x670_timer_lstick_tilt_x=ages[n];
  CHECK(helper(object,"ReadAges")==NativeScriptVM_Ok);
  CHECK(presentation_state(object,"shield_age",NULL).as.i64==ages[5-n]&&presentation_state(object,"tilt_age",NULL).as.i64==ages[n]);++cases;
 }
 const uint32_t ids[]={NativeScriptBinding_InputAnalogShieldAge,NativeScriptBinding_InputHorizontalTiltAge};
 const NativeScriptValue invalid[]={{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I32,.as.i64=255},
  {.type=NativeScriptType_I64,.as.i64=INT64_MAX},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x1000000fe)},
  {.type=NativeScriptType_Bool,.as.u64=2},{.type=NativeScriptType_F32,.as.f32=1}};
 for(unsigned id=0;id<2;++id)for(unsigned n=0;n<6;++n){u8 shield=fighter->trigger_analog_timer,tilt=fighter->x670_timer_lstick_tilt_x;
  CHECK(!nativeScriptBindings_Write(&c,ids[id],NULL,0,&invalid[n],&diagnostic));
  CHECK(fighter->trigger_analog_timer==shield&&fighter->x670_timer_lstick_tilt_x==tilt&&fighter->x673==73&&fighter->x671_timer_lstick_tilt_y==41);++cases;}
 const NativeScriptValue accepted[]={{.type=NativeScriptType_Bool,.as.u64=1},{.type=NativeScriptType_U64,.as.u64=254},{.type=NativeScriptType_I64,.as.i64=0}};
 for(unsigned id=0;id<2;++id)for(unsigned n=0;n<3;++n){CHECK(nativeScriptBindings_Write(&c,ids[id],NULL,0,&accepted[n],&diagnostic));++cases;}
 HSD_GObjFree(object);
}

static void stable_ids(void)
{
 NativeFighterRig* map=&source.assets.rig;
 map->count=4;map->next_id=500001;
 for(unsigned n=0;n<MAX_FT_PARTS;++n)map->ids[n]=n;
 map->ids[2]=500000;map->ids[139]=2;
 CHECK(nativeFighterRig_Index(map,diagnostic.message,sizeof(diagnostic.message)));
 // Run the same real compiler/actor/VM/matrix comparisons through an ID that
 // differs from its physical slot, including semantic-role -> ID -> joint.
 native_pairs();
 HSD_GObj* object=presentation_create(source.target,true);rig(object,1);
 NativeScriptValue value=integer(500000);presentation_state(object,"index",&value);
 seed(object);CHECK(helper(object,"Translate")==NativeScriptVM_Ok);
 CHECK(joints[2].translate.x==91&&joints[2].translate.y==92&&joints[2].translate.z==93);
 CHECK(joints[1].translate.x==2.25f);++cases;
 // The original numeric ID now refers to the reserved external slot, not
 // the physical body joint. Neither that ID nor a deleted ID may alias it.
 for(unsigned n=0;n<2;++n){value=integer(n?139:2);presentation_state(object,"index",&value);
  HSD_JObj before[4];memcpy(before,joints,sizeof(joints));
  CHECK(helper(object,"Translate")==NativeScriptVM_Ok&&!memcmp(before,joints,sizeof(joints)));++cases;}
 HSD_GObjFree(object);
 memset(map,0,sizeof(*map));
}

#include "throw_angle_cases.h"
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);compiler_negatives();prepare();native_pairs();throw_angle_cases();retirement();direct_negatives();root_scale_cases();translations();input_ages();stable_ids();
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS bone queries\",\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u,\"retirement_mutations\":%u,\"actor_vm_calls\":%u}\n",presentation_checks,cases,comparisons,mutations,helper_calls);return 0;
}
