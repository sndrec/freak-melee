#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_transition.h>
#include <melee/ft/native_script_binding_presentation.h>
#include <melee/ft/native_script_binding_item.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_binding_article_reference_resolve.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
#include <melee/ft/native_script_binding_effect.h>
#include <melee/ft/native_script_binding_ledge.h>
#include <melee/ft/native_script_binding_jump.h>
#include <melee/ft/native_script_binding_forms.h>
#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_math.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_effect_dispatch.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcoll.h>
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

static unsigned checks,cases,effects,winds,samples,mutations,joint_abis,scalar_abis;
static NativeScriptDiagnostic diagnostic;
/* Unexercised async-joint and Article paths must not acquire fake success. */
HSD_JObjInfo hsdJObj;
u8 HSD_GObj_JObjKind;
bool hsdObjIsDescendantOf(HSD_Obj* object, HSD_ClassInfo* parent)
{ (void)object; (void)parent; abort(); }
bool nativeScriptItem_IsAttached(Item_GObj* object)
{ (void)object; return false; }
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* object)
{ (void)object; return NULL; }
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* object)
{ (void)object; return NativeScriptVM_SourceRetired; }
bool nativeScriptItem_View(Item_GObj* object, uint32_t generation,
                           NativeScriptReceiverView* result)
{ (void)object; (void)generation; (void)result; return false; }
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"form feedback %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static NativeSourcePackage package;
static NativeScriptStorageInstance storage,other_storage;
static NativeScriptFighterProgram other_program;
static Fighter fighter;static FighterBone parts[140];static HSD_JObj joints[140];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
NativeMatchPool nativeGObjPool;
static _Alignas(32) uint8_t payload[4*256];static NativeMatchPoolSlot slots[4];static uint32_t free_order[4];
static HSD_GObj* object;static NativeMatchPoolReference reference;
static NativeScriptRuntimeContext context;
static NativeScriptVMStatus actor_status;static unsigned view_available,program_available;static bool package_available;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static unsigned mutation,mutate_on;static HSD_JObj* last_joint;static int last_effect,last_frames;static float last_scalar,last_wind[3];static Vec3 last_position;
static uintptr_t scalar_address;static const NativeScriptValue* caller_arguments;
enum { OnEffect=1,OnSample,OnWind };
enum { RetireGeneration=1,RetireStorage,FaultActor,RetireView,ChangeProgramIdentity,ChangeLayoutIdentity,
       ChangeParameterIdentity,ReplaceStorage,ReplacePackage,ReplaceProgram,ReplaceRig,ReplaceParts,ReuseSlot };

const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return package_available&&kind==Ft_Kind_CustomFirst?&package:NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{return program_available&&kind==Ft_Kind_CustomFirst?(program_available==2?&other_program:&package.program):NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* actor)
{return actor==object?actor_status:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* actor,uint32_t generation,NativeScriptReceiverView* result)
{if(!view_available||actor!=object||generation!=17)return false;*result=(NativeScriptReceiverView){&package.compiler.bank.declarations,view_available==2?&other_storage:&storage};return true;}
bool nativeModelEffect_Snapshot(HSD_GObj* actor,uint32_t* generation){(void)actor;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* actor,uint32_t generation){(void)actor;(void)generation;return NULL;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;abort();}
#include "source_contract.inc"
#include "dispatch_expected.inc"

static void change(unsigned stage)
{
    if(!mutation||stage!=mutate_on)return;++mutations;
    switch(mutation) {
    case RetireGeneration:++fighter.x8_spawnNum;break;
    case RetireStorage:storage.live=false;break;
    case FaultActor:actor_status=NativeScriptVM_Error;break;
    case RetireView:view_available=false;break;
    case ChangeProgramIdentity:++package.program.identity;break;
    case ChangeLayoutIdentity:++package.compiler.bank.storage_layout.identity;break;
    case ChangeParameterIdentity:++package.compiler.bank.storage_parameters.identity;break;
    case ReplaceStorage:other_storage=storage;view_available=2;break;
    case ReplacePackage:package_available=false;break;
    case ReplaceProgram:other_program=package.program;program_available=2;break;
    case ReplaceRig:tables[Ft_Kind_CustomFirst]=NULL;break;
    case ReplaceParts:fighter.parts=NULL;break;
    case ReuseSlot: {
        HSD_GObj previous=*object;CHECK(nativeMatchPool_Free(&nativeGObjPool,reference));
        NativeMatchPoolReference replacement;HSD_GObj* reused=nativeMatchPool_Allocate(&nativeGObjPool,&replacement);CHECK(reused==object);*reused=previous;
        CHECK(replacement.generation!=reference.generation&&nativeCallbackSource_Alive(object,17,NativeCallbackSource_Fighter));break;
    }
    default:abort();
    }
    mutation=0;
}
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj* actor,...)
{
    CHECK(actor==object);++effects;last_effect=(int)effect;va_list args;va_start(args,actor);
    NativeScriptEffectArguments family=nativeScriptEffectDispatch_Arguments((int32_t)effect);CHECK(family!=NativeScriptEffectArguments_Unsupported);
    if(family==NativeScriptEffectArguments_Position) {
        Vec3* value=va_arg(args,Vec3*);last_position=*value;
        CHECK(value!=(const Vec3*)&caller_arguments[1].as.f32);
        *value=(Vec3){37,38,39};CHECK(caller_arguments[1].as.f32!=37);
    } else last_joint=va_arg(args,HSD_JObj*);
    if(family==NativeScriptEffectArguments_JointScalar){float* value=va_arg(args,float*);scalar_address=(uintptr_t)value;last_scalar=*value;
        CHECK(value!=&fighter.facing_dir&&value!=&caller_arguments[2].as.f32);*value=37;CHECK(caller_arguments[2].as.f32!=37);}
    va_end(args);change(OnEffect);return(EF_SpawnResult){0};
}
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result)
{CHECK(!offset);++samples;last_joint=joint;*result=joint->translate;change(OnSample);}
void lb_800119DC(Vec3* position,int frames,float strength,float decay,float frequency)
{++winds;last_position=*position;last_frames=frames;last_wind[0]=strength;last_wind[1]=decay;last_wind[2]=frequency;change(OnWind);}
void efAsync_Spawn(HSD_GObj* actor,void* queue,u32 mode,u32 effect,HSD_JObj* joint,...)
{(void)actor;(void)queue;(void)mode;(void)effect;(void)joint;abort();}
void ftColl_CreateReflectHit(Fighter_GObj* actor,ReflectDesc* shape,HSD_GObjEvent callback)
{(void)actor;(void)shape;(void)callback;abort();}
void ftColl_8007AEF8(Fighter_GObj* actor){(void)actor;abort();}

static NativeScriptValue i32(int32_t v){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=v};}
static NativeScriptValue u32v(uint32_t v){return(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=v};}
static NativeScriptValue float_value(float v){return(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=v};}
static void setup(void)
{
    memset(&package,0,sizeof(package));memset(&nativeGObjPool,0,sizeof(nativeGObjPool));memset(tables,0,sizeof(tables));memset(heads,0,sizeof(heads));
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,payload,sizeof(payload),slots,free_order,4,sizeof(HSD_GObj),32,UINT64_C(0x100000002)));
    object=nativeMatchPool_Allocate(&nativeGObjPool,&reference);CHECK(object);memset(object,0,sizeof(*object));
    memset(&fighter,0,sizeof(fighter));fighter.gobj=object;fighter.kind=fighter.behavior_kind=Ft_Kind_CustomFirst;fighter.x8_spawnNum=17;fighter.parts=parts;fighter.facing_dir=-1;
    object->user_data=&fighter;heads[HSD_GOBJ_PLINK_FIGHTER]=object;
    package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;fighter.ft_data=&package.assets.character.data;gFtDataList[Ft_Kind_CustomFirst]=fighter.ft_data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=package.assets.motions;
    package.assets.facts.joints=140;package.assets.parts=(FighterPartsTable){package.assets.joint_to_part,package.assets.part_to_joint,140};tables[Ft_Kind_CustomFirst]=&package.assets.parts;
    for(unsigned j=0;j<140;++j){memset(&joints[j],0,sizeof(joints[j]));joints[j].translate=(Vec3){j+.25f,j+.5f,j+.75f};parts[j].joint=&joints[j];}
    for(unsigned p=0;p<54;++p)package.assets.part_to_joint[p]=(u8)((p+7)%140);
    package.program.bank=&package.compiler.bank;package.program.identity=88;package.compiler.bank.valid=package.compiler.bank.has_motion_layout=true;
    package.compiler.bank.motion_layout=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};package.assets.facts.moves=package.compiler.bank.motion_layout;
    package.compiler.bank.storage_layout.identity=55;package.compiler.bank.storage_layout.program=&package.compiler.bank.declarations;package.compiler.bank.storage_parameters.identity=66;
    storage=(NativeScriptStorageInstance){.layout=&package.compiler.bank.storage_layout,.parameters=&package.compiler.bank.storage_parameters,.generation=17,.identity=17,.owner=0,.live=true};
    context=(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=17,.scene_epoch=reference.lifetime};
    actor_status=NativeScriptVM_Ok;view_available=package_available=program_available=true;mutation=mutate_on=0;effects=winds=samples=0;scalar_address=0;
}
static bool invoke(uint32_t op,NativeScriptValue* args,unsigned count,NativeScriptValue* result)
{caller_arguments=args;return nativeScriptBindingFeedback_Invoke(&context,op,args,count,result,&diagnostic);}
static void reject(uint32_t op,NativeScriptValue* args,unsigned count)
{NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};unsigned e=effects,w=winds,s=samples;CHECK(!invoke(op,args,count,&result));CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);CHECK(effects==e&&winds==w&&samples==s);}
static void classifier(void)
{
    unsigned joint_count=0,scalar_count=0;
    for(unsigned id=0;id<65536;++id){CHECK(nativeScriptEffectDispatch_Arguments((int32_t)id)==expected_arguments[id]);joint_count+=expected_arguments[id]==1;scalar_count+=expected_arguments[id]==2;}
    // Exhaustive equality above checks the source-derived contract. Cohort
    // counts are reported rather than pinned to an obsolete catalog size.
    joint_abis=joint_count;scalar_abis=scalar_count;
    CHECK(nativeScriptEffectDispatch_Arguments(-1)==0&&nativeScriptEffectDispatch_Arguments(65536)==0);
    CHECK(nativeScriptEffectDispatch_Arguments(0x47A)==NativeScriptEffectArguments_JointScalar);++cases;
}
static void arguments(void)
{
    const uint32_t spawn[]={NativeScriptBinding_EffectsSpawn,NativeScriptBinding_EffectsSpawnWithScalar,NativeScriptBinding_EffectsSpawnOnJointWithScalar};
    for(unsigned op=0;op<3;++op)for(unsigned which=0;which<2;++which){setup();bool raw=op==2;unsigned index=raw?(which?43:104):(which?43:4);
        NativeScriptValue args[]={i32(op?0x47A:1276),u32v(index),float_value(-0.0f)},result={0};CHECK(invoke(spawn[op],args,op?3:2,&result));CHECK(result.type==NativeScriptType_Void&&effects==1);
        CHECK(last_joint==&joints[raw?index:package.assets.part_to_joint[index]]);if(op)CHECK(!memcmp(&last_scalar,&args[2].as.f32,4)&&scalar_address);++cases;}
    for(unsigned id=0;id<65536;++id)if(expected_arguments[id]==1||expected_arguments[id]==2){setup();bool scalar=expected_arguments[id]==2;NativeScriptValue args[]={i32((int32_t)id),u32v(scalar?104:4),float_value(-1.25f)},result={0};
        CHECK(invoke(scalar?NativeScriptBinding_EffectsSpawnOnJointWithScalar:NativeScriptBinding_EffectsSpawn,args,scalar?3:2,&result));CHECK(last_effect==(int)id&&last_joint==&joints[scalar?104:package.assets.part_to_joint[4]]);}
    ++cases;
    setup();NativeScriptValue args[]={i32(0x47A),u32v(4),float_value(1)};reject(NativeScriptBinding_EffectsSpawn,args,2);
    args[0]=i32(1276);reject(NativeScriptBinding_EffectsSpawnOnJointWithScalar,args,3);
    const int unsupported[]={-1,0x123,0x479,0x4CC,0x4BB,0x4B0,0x510,65536};
    for(unsigned i=0;i<sizeof(unsupported)/sizeof(*unsupported);++i){args[0]=i32(unsupported[i]);reject(NativeScriptBinding_EffectsSpawn,args,2);}++cases;
    args[0]=i32(1276);parts[package.assets.part_to_joint[4]].joint=NULL;reject(NativeScriptBinding_EffectsSpawn,args,2);parts[package.assets.part_to_joint[4]].joint=&joints[package.assets.part_to_joint[4]];
    args[1]=u32v(4);args[0]=u32v(1276);reject(NativeScriptBinding_EffectsSpawn,args,2);args[0]=i32(1276);reject(NativeScriptBinding_EffectsSpawn,args,1);++cases;
}
static void wind(void)
{
    setup();NativeScriptValue args[]={float_value(-0.0f),float_value(35.25f),float_value(-9.5f),i32(120),float_value(.4f),float_value(.003f),float_value(1.0471976f)},result={0};
    CHECK(invoke(NativeScriptBinding_EffectsWindAtPosition,args,7,&result));Vec3 expected={args[0].as.f32,args[1].as.f32,args[2].as.f32};CHECK(winds==1&&!samples&&last_frames==120&&!memcmp(&expected,&last_position,sizeof(expected)));
    CHECK(!memcmp(last_wind,&args[4].as.f32,4)&&!memcmp(last_wind+1,&args[5].as.f32,4)&&!memcmp(last_wind+2,&args[6].as.f32,4));++cases;
    setup();NativeScriptValue semantic[]={u32v(4),float_value(3),i32(120),float_value(.4f),float_value(.003f),float_value(1.0471976f)};CHECK(invoke(NativeScriptBinding_EffectsWind,semantic,6,&result));
    expected=joints[package.assets.part_to_joint[4]].translate;expected.x-=3;CHECK(samples==1&&winds==1&&!memcmp(&expected,&last_position,sizeof(expected)));++cases;
    args[0]=float_value(NAN);reject(NativeScriptBinding_EffectsWindAtPosition,args,7);args[0]=float_value(0);args[3]=u32v(120);reject(NativeScriptBinding_EffectsWindAtPosition,args,7);++cases;
}
static void positions(void)
{
    for(unsigned id=0;id<65536;++id)if(expected_arguments[id]==NativeScriptEffectArguments_Position) {
        setup();NativeScriptValue args[]={i32((int32_t)id),float_value(-0.0f),float_value(35.25f),float_value(-9.5f)},result={0};
        CHECK(invoke(NativeScriptBinding_EffectsSpawnAtPosition,args,4,&result));
        Vec3 expected={-0.0f,35.25f,-9.5f};CHECK(!memcmp(&last_position,&expected,sizeof(expected))&&effects==1&&last_effect==(int)id&&result.type==NativeScriptType_Void);++cases;
    }
    setup();NativeScriptValue args[]={i32(1285),float_value(1),float_value(2),float_value(3)};
    args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000505)};reject(NativeScriptBinding_EffectsSpawnAtPosition,args,4);
    args[0]=i32(-1);reject(NativeScriptBinding_EffectsSpawnAtPosition,args,4);
    args[0]=i32(1276);reject(NativeScriptBinding_EffectsSpawnAtPosition,args,4);
    args[0]=i32(1285);args[2]=float_value(NAN);reject(NativeScriptBinding_EffectsSpawnAtPosition,args,4);args[2]=float_value(2);
    for(unsigned m=1;m<=13;++m){
        setup();mutation=m;mutate_on=OnEffect;NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!invoke(NativeScriptBinding_EffectsSpawnAtPosition,args,4,&result));CHECK(result.type==NativeScriptType_U64&&result.as.u64==99&&effects==1);++cases;
    }
}
static void lifetimes(void)
{
    NativeScriptValue spawn[]={i32(1276),u32v(4)},scalar[]={i32(0x47A),u32v(43),float_value(-1.25f)},wind[]={u32v(4),float_value(3),i32(120),float_value(.4f),float_value(.003f),float_value(1.0471976f)};
    for(unsigned how=1;how<=ReuseSlot;++how)for(unsigned stage=1;stage<=3;++stage){setup();mutation=how;mutate_on=stage;
        uint32_t op=stage==OnEffect?NativeScriptBinding_EffectsSpawnOnJointWithScalar:NativeScriptBinding_EffectsWind;NativeScriptValue* args=stage==OnEffect?scalar:wind;unsigned count=stage==OnEffect?3:6;
        NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=123};CHECK(!invoke(op,args,count,&result));CHECK(result.type==NativeScriptType_U64&&result.as.u64==123);
        CHECK(stage==OnEffect?effects==1:stage==OnSample?(samples==1&&!winds):(samples==1&&winds==1));++cases;
    }
    setup();context.scene_epoch++;reject(NativeScriptBinding_EffectsSpawn,spawn,2);++cases;
    setup();storage.live=false;reject(NativeScriptBinding_EffectsSpawn,spawn,2);++cases;
    setup();actor_status=NativeScriptVM_Error;reject(NativeScriptBinding_EffectsSpawn,spawn,2);++cases;
    setup();package.ready=false;reject(NativeScriptBinding_EffectsSpawn,spawn,2);++cases;
    setup();package.program.bank=NULL;reject(NativeScriptBinding_EffectsSpawn,spawn,2);++cases;
    setup();fighter.ft_data=NULL;reject(NativeScriptBinding_EffectsSpawn,spawn,2);++cases;
    setup();package.assets.parts.parts_num=139;reject(NativeScriptBinding_EffectsSpawn,spawn,2);++cases;
}
int main(void)
{classifier();arguments();wind();positions();lifetimes();printf("{\"status\":\"PASS direct form feedback\",\"checks\":%u,\"cases\":%u,\"joint_abis\":%u,\"scalar_abis\":%u,\"retirement_mutations\":%u}\n",checks,cases,joint_abis,scalar_abis,mutations);return 0;}
