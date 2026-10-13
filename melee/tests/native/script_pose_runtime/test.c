#include <melee/ft/native_script_binding_pose.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_math.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
#include <melee/ft/native_script_binding_article_reference_resolve.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_pose.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftdata.h>
#include <melee/it/types.h>
#include <melee/lb/lbanim.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned checks,cases,calls,mutations,mutation;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"pose %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static NativeSourcePackage package;
static NativeScriptStorageInstance storage,other_storage;
static Fighter fighter;static FighterBone parts[5];static HSD_JObj live,blend,other;
static HSD_Joint graph_descriptor,costume_descriptor;static FigaTree clip;
static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
NativeMatchPool nativeGObjPool;
static _Alignas(32) uint8_t payload[4*256];static NativeMatchPoolSlot slots[4];static uint32_t free_order[4];
static HSD_GObj* object;static NativeMatchPoolReference reference;static NativeScriptRuntimeContext context;
static NativeScriptVMStatus status;static bool package_available,view_available,assets_available,valid_topology;
static unsigned last_operation,last_destination,last_start,last_clip,last_graph;
static float last_a,last_b;static HSD_JObj* last_skeleton;

const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return package_available&&kind==Ft_Kind_CustomFirst?&package:NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&package.program:NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* actor){return actor==object?status:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* actor,uint32_t generation,NativeScriptReceiverView* result)
{if(!view_available||actor!=object||generation!=17)return false;*result=(NativeScriptReceiverView){&package.compiler.bank.declarations,&storage};return true;}
bool nativeModelEffect_Snapshot(HSD_GObj* actor,uint32_t* generation){(void)actor;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* actor,uint32_t generation){(void)actor;(void)generation;return NULL;}
bool nativeFighterRuntime_PoseAssets(Fighter_GObj* actor,NativeFighterPoseAssets* result)
{if(actor!=object||!assets_available)return false;*result=(NativeFighterPoseAssets){.character=&package.assets.character,.costume=&package.assets.costume,.action_count=311,.pose_graph_count=2,.animation_bytes=(const uint8_t*)&clip,.animation_size=sizeof(clip),.animation_arena=&clip,.animation_arena_size=sizeof(clip)};return true;}
FigaTree* nativeFighterRuntime_PoseClip(Fighter_GObj* actor,int32_t row){CHECK(actor==object);last_clip=row;return row==305?&clip:NULL;}
HSD_Joint* nativeFighterRuntime_PoseGraph(Fighter_GObj* actor,int32_t row){CHECK(actor==object);last_graph=row;return row==1?&graph_descriptor:NULL;}
bool nativeFighterPose_Validate(Fighter_GObj* actor,NativeFighterPoseOperation operation,int32_t destination,int32_t start,HSD_Joint* graph,FigaTree* animation)
{CHECK(actor==object);last_operation=operation;last_destination=destination;last_start=start;
 if(operation==NativeFighterPose_LoadAnimation)CHECK(animation==&clip&&!graph);
 else if(operation==NativeFighterPose_CopyGraph||operation==NativeFighterPose_BlendGraph)CHECK(graph==&graph_descriptor&&!animation);
 else CHECK(!graph&&!animation);return valid_topology;}
#include "source_contract.inc"

static void change(void)
{
    if(!mutation)return;++mutations;
    switch(mutation) {
    case 1:++fighter.x8_spawnNum;break;
    case 2:storage.live=false;break;
    case 3:status=NativeScriptVM_Error;break;
    case 4:view_available=false;break;
    case 5:++package.program.identity;break;
    case 6:++package.compiler.bank.storage_layout.identity;break;
    case 7:++package.compiler.bank.storage_parameters.identity;break;
    case 8:package_available=false;break;
    case 9:fighter.parts=NULL;break;
    case 10:object->hsd_obj=&other;break;
    case 11:fighter.x8AC_animSkeleton=&other;break;
    case 12:assets_available=false;break;
    case 13: {
        HSD_GObj previous=*object;CHECK(nativeMatchPool_Free(&nativeGObjPool,reference));
        NativeMatchPoolReference replacement;HSD_GObj* reused=nativeMatchPool_Allocate(&nativeGObjPool,&replacement);
        CHECK(reused==object);*reused=previous;CHECK(replacement.generation!=reference.generation);break;
    }
    default:abort();
    }
    mutation=0;
}
static void native_call(Fighter* actor,unsigned operation,unsigned destination,int start)
{CHECK(actor==&fighter&&last_operation==operation&&last_destination==destination&&last_start==(unsigned)start);++calls;change();}
void ftAnim_8006F4C8(Fighter* actor,bool destination,FigaTree* animation)
{CHECK(animation==&clip&&last_clip==305);native_call(actor,NativeFighterPose_LoadAnimation,destination,0);}
void ftAnim_80070710(HSD_JObj* skeleton,float frame)
{CHECK(skeleton==(last_destination?&blend:&live));last_skeleton=skeleton;last_a=frame;native_call(&fighter,NativeFighterPose_RequestFrame,last_destination,0);}
void HSD_JObjAnimAll(HSD_JObj* skeleton)
{CHECK(skeleton==(last_destination?&blend:&live));last_skeleton=skeleton;native_call(&fighter,NativeFighterPose_Evaluate,last_destination,0);}
HSD_Joint* ftAnim_8006F994(Fighter* actor,HSD_JObj* skeleton,HSD_Joint* costume)
{CHECK(actor==&fighter&&skeleton==(last_destination?&blend:&live)&&costume==&costume_descriptor);change();return &costume_descriptor;}
void ftAnim_8006FA58(Fighter* actor,Fighter_Part start,HSD_Joint* graph)
{CHECK(graph==(last_operation==NativeFighterPose_CopyCostume?&costume_descriptor:&graph_descriptor));native_call(actor,last_operation,0,start);}
void ftAnim_8006FB88(Fighter* actor,Fighter_Part start,HSD_Joint* graph)
{CHECK(graph==(last_operation==NativeFighterPose_CopyCostume?&costume_descriptor:&graph_descriptor));native_call(actor,last_operation,1,start);}
void ftAnim_80070010(Fighter* actor,Fighter_Part start,float a,float b,HSD_Joint* graph)
{CHECK(graph==&graph_descriptor);last_a=a;last_b=b;native_call(actor,NativeFighterPose_BlendGraph,0,start);}
void ftAnim_80070108(Fighter* actor,Fighter_Part start,float a,float b,HSD_Joint* graph)
{CHECK(graph==&graph_descriptor);last_a=a;last_b=b;native_call(actor,NativeFighterPose_BlendGraph,1,start);}
void ftAnim_8006FE9C(Fighter* actor,Fighter_Part start,float a,float b)
{last_a=a;last_b=b;native_call(actor,NativeFighterPose_BlendToLive,0,start);}
void ftAnim_8006FF74(Fighter* actor,Fighter_Part start){native_call(actor,NativeFighterPose_CopyToLive,0,start);}

static NativeScriptValue i(int64_t value){return(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=value};}
static NativeScriptValue f(float value){return(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};}
static NativeScriptValue selector(unsigned type,uint64_t row){return(NativeScriptValue){.type=type,.as.u64=row};}
static void setup(void)
{
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,payload,sizeof(payload),slots,free_order,4,sizeof(HSD_GObj),32,5));object=nativeMatchPool_Allocate(&nativeGObjPool,&reference);CHECK(object);memset(object,0,sizeof(*object));
    memset(&package,0,sizeof(package));memset(&fighter,0,sizeof(fighter));memset(heads,0,sizeof(heads));
    package.address=&package;package.target=Ft_Kind_CustomFirst;package.ready=package.assets.ready=true;
    NativeScriptCompileBank* bank=&package.compiler.bank;bank->valid=bank->has_motion_layout=true;bank->storage_layout.identity=31;bank->storage_parameters.identity=41;
    bank->pose_graph_count=package.assets.facts.pose_count=2;bank->pose_graph_mask=package.assets.facts.layout.joint_graph_mask=48;
    bank->pose_symbols[0].row=1;bank->pose_symbol_count=1;package.assets.parts.parts_num=5;
    package.program.bank=bank;package.program.identity=23;fighter.kind=fighter.behavior_kind=Ft_Kind_CustomFirst;fighter.gobj=object;fighter.x8_spawnNum=17;
    fighter.ft_data=&package.assets.character.data;gFtDataList[fighter.kind]=fighter.ft_data;ftData_CharacterStateTables[fighter.kind]=package.assets.motions;
    fighter.parts=parts;parts[1].joint=&live;parts[1].x4_jobj2=&blend;fighter.x8AC_animSkeleton=&blend;fighter.x108_costume_joint=&costume_descriptor;
    object->user_data=&fighter;object->hsd_obj=&live;heads[HSD_GOBJ_PLINK_FIGHTER]=object;
    storage=(NativeScriptStorageInstance){.layout=&bank->storage_layout,.parameters=&bank->storage_parameters,.owner=0,.generation=17,.live=true};other_storage=storage;
    context=(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=17,.scene_epoch=reference.lifetime};
    status=NativeScriptVM_Ok;package_available=view_available=assets_available=valid_topology=true;calls=mutation=0;
}
static bool invoke(unsigned op,NativeScriptValue* arguments,unsigned count,NativeScriptValue* result)
{return nativeScriptBindingPose_Invoke(&context,op,arguments,count,result,&diagnostic);}
static void reject(unsigned op,NativeScriptValue* args,unsigned count)
{unsigned before=calls;NativeScriptValue output=i(99);CHECK(!invoke(op,args,count,&output));CHECK(calls==before&&output.type==NativeScriptType_I64&&output.as.i64==99);++cases;}
int main(void)
{
    const unsigned operations[]={NativeScriptBinding_PoseLoadAnimation,NativeScriptBinding_PoseRequestFrame,NativeScriptBinding_PoseEvaluate,NativeScriptBinding_PoseCopyCostume,NativeScriptBinding_PoseCopyGraph,NativeScriptBinding_PoseBlendGraph,NativeScriptBinding_PoseBlendToLive,NativeScriptBinding_PoseCopyToLive};
    const unsigned counts[]={2,2,1,2,3,5,3,1};
    for(unsigned op=0;op<8;++op)for(unsigned destination=0;destination<(op<6?2:1);++destination) {
        setup();NativeScriptValue args[]={selector(NativeScriptType_PoseDestination,destination),i(1),selector(NativeScriptType_PoseGraph,1),f(-.25f),f(1.75f)};
        if(op==0)args[1]=i(305);if(op==1)args[1]=f(-2.5f);if(op>=6){args[0]=i(1);args[1]=f(-.25f);args[2]=f(1.75f);}
        NativeScriptValue out={0};CHECK(invoke(operations[op],args,counts[op],&out));CHECK(calls==1&&out.type==NativeScriptType_Void);
        if(op==1)CHECK(last_a==-2.5f);if(op==5||op==6)CHECK(last_a==-.25f&&last_b==1.75f);++cases;
        for(unsigned m=1;m<=13;++m) {
            setup();mutation=m;out=i(99);CHECK(!invoke(operations[op],args,counts[op],&out));CHECK(out.type==NativeScriptType_I64&&out.as.i64==99);
            CHECK(calls==(op==3?0:1));++cases;
        }
    }
    setup();NativeScriptValue args[]={selector(NativeScriptType_PoseDestination,0),i(1),selector(NativeScriptType_PoseGraph,1),f(.25f),f(.75f)};
    valid_topology=false;reject(NativeScriptBinding_PoseBlendGraph,args,5);valid_topology=true;
    args[0]=i(0);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[0]=selector(NativeScriptType_PoseDestination,2);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[0]=selector(NativeScriptType_PoseDestination,0);
    args[2]=i(1);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[2]=selector(NativeScriptType_PoseGraph,0);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[2]=selector(NativeScriptType_PoseGraph,2);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[2]=selector(NativeScriptType_PoseGraph,1);
    args[1]=i(INT64_C(0x100000001));reject(NativeScriptBinding_PoseBlendGraph,args,5);args[1]=i(-1);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[1]=i(5);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[1]=i(1);
    args[3]=f(NAN);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[3]=f(.25f);args[4]=f(INFINITY);reject(NativeScriptBinding_PoseBlendGraph,args,5);args[4]=f(.75f);
    context.source_class=NativeCallbackSource_Item;reject(NativeScriptBinding_PoseBlendGraph,args,5);
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"retirement_mutations\":%u,\"native_operations\":8}\n",checks,cases,mutations);return 0;
}
