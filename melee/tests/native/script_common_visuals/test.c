#include <melee/ft/native_script_binding_visuals.h>
#include <melee/ft/native_fighter_pose.h>
#include <stdlib.h>
bool nativeFighterPose_Validate(Fighter_GObj* g,NativeFighterPoseOperation op,int32_t destination,int32_t start,HSD_Joint* graph,FigaTree* clip)
{(void)g;(void)op;(void)destination;(void)start;(void)graph;(void)clip;abort();}
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
#include <melee/ft/native_script_binding_article_reference_resolve.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcolanim.h>
#include <melee/lb/lb_013B.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <melee/ft/ftanim.h>
#include <sysdolphin/baselib/debug.h>
#include <math.h>
#include <melee/ft/native_fighter_visuals.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,comparisons,group_comparisons,query_comparisons,color_clear_comparisons;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"visuals %u: %s\n",__LINE__,#v);exit(1);}}while(0)
static NativeSourcePackage source;static NativeScriptFighterProgram program;static NativeScriptStorageInstance storage;
static Fighter fighter;static HSD_GObj object;static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static FighterBone parts[3];static HSD_JObj root,replaced;static MotionState motions[8];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
NativeMatchPool nativeGObjPool;static NativeMatchPoolSlot slots[1];static uint32_t free_order[1];static uint64_t epoch=UINT64_C(0x12300000000);
static struct Fighter_804D653C_t colors[FT_COMMON_COLOR_ENTRY_COUNT],special[FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT];
struct Fighter_804D653C_t *Fighter_804D653C=colors,*Fighter_804D6538=special;
struct Fighter_804D6540_t** Fighter_804D6540;
static union ColorOverlay_x8_t commands[FT_COMMON_COLOR_ENTRY_COUNT+FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT];
static struct Arena {void* variants[2][4];FtPartsVisLookup lookups[4][3];TempS selections[4][3][256];u8 mesh_ordinals[3];} arena;
static HSD_DObj draws[3];static HSD_DObj* draw_pointers[3];
static HSD_DObj secondary_draws[3];static HSD_DObj* secondary_pointers[3];
static HSD_MObj materials[3];static HSD_AObj material_animations[3];
static struct MaterialCall {HSD_MObj* material;float frame;unsigned operation;} material_calls[12];
static unsigned material_call_count,material_comparisons,material_retirements,material_inject_at;
static unsigned variant,inject;static NativeScriptVMStatus actor_status;static NativeScriptRuntimeContext context;
static NativeScriptDiagnostic diagnostic;
// This direct primitive cohort does not execute the source-owned property
// reader. The connected held-item fixture owns that actor/lifetime proof.
bool nativeScriptFighter_IsAttached(Fighter_GObj* actor){(void)actor;abort();}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;abort();}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind==source.target?&source:NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){return kind==source.target?&program:NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* actor){return actor==&object?actor_status:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* actor,uint32_t generation,NativeScriptReceiverView* view){
 if(actor!=&object||generation!=17||!storage.live)return false;*view=(NativeScriptReceiverView){.program=&source.compiler.bank.declarations,.storage=&storage};return true;}
uint8_t nativeCostumeRuntime_ModelVariant(Fighter* actor){CHECK(actor==&fighter);return(uint8_t)variant;}
bool nativeModelEffect_Snapshot(HSD_GObj* actor,uint32_t* generation){(void)actor;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* actor,uint32_t generation){(void)actor;(void)generation;return NULL;}
// Test-only injection after all original native statements, just before return.
// Original parts/color primitives have no script callbacks today. This proves
// the binding's continuation guard under simulated lifecycle-capable reentry.
static void after_native(void){switch(inject){
 case 1:++fighter.x8_spawnNum;break;
 case 2:heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;object.user_data=(void*)1;break;
 case 3:storage.live=false;break;
 case 4:actor_status=NativeScriptVM_Error;break;
 case 5:object.hsd_obj=&replaced;break;
 case 6:++source.compiler.bank.storage_parameters.identity;break;
 case 7:Fighter_804D653C=NULL;break;
 case 8:fighter.x5AC.xC[0]=NULL;break;
 case 9:source.assets.character.model_data.x0.vis_table=NULL;break;
 case 10:++source.assets.character.import.arena_used;break;
 case 11:fighter.dobj_list.data=NULL;break;
 case 12:fighter.x203C.data=NULL;break;
 case 13:draws[0].mobj=NULL;break;
 case 14:materials[0].aobj=NULL;break;
 default:break;
}}
// The material renderer/evaluator remains a controlled leaf; preserve exact
// operation order and frame arguments and inject retirement at each boundary.
void HSD_MObjReqAnim(HSD_MObj* material,float frame){
 if(!material)return;CHECK(material_call_count<12);
 material_calls[material_call_count++]=(struct MaterialCall){material,frame,0};if(material_call_count==material_inject_at)after_native();}
void HSD_MObjAnim(HSD_MObj* material){
 if(!material)return;CHECK(material_call_count<12);
 material_calls[material_call_count++]=(struct MaterialCall){material,0,1};if(material_call_count==material_inject_at)after_native();}
// The connected capture-scheduler fixture owns partial-body evaluation.
void ftAnim_ApplyPartAnim(Fighter_GObj* actor,s32 group,s32 choice,float duration)
{(void)actor;(void)group;(void)choice;(void)duration;abort();}
void ftAnim_80070FB4(Fighter_GObj* actor,s32 group,s32 choice)
{(void)actor;(void)group;(void)choice;abort();}
void ftAnim_80070C48(Fighter_GObj* actor,s32 group)
{(void)actor;(void)group;abort();}
void ftAnim_80070CC4(Fighter_GObj* actor,int group)
{(void)actor;(void)group;abort();}
bool nativeFighterRuntime_PoseAssets(Fighter_GObj* actor,NativeFighterPoseAssets* assets)
{(void)actor;(void)assets;abort();}
u32 ftParts_8007506C(FighterKind kind,int part){(void)kind;(void)part;abort();}
void __assert(char* file,u32 line,char* expression){(void)file;(void)line;(void)expression;abort();}
void OSReport(char* format,...){(void)format;abort();}
#include "source_contract.inc"
#include "native_visuals.inc"
static NativeScriptValue integer(int64_t value){return(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=value};}
static NativeScriptValue boolean(unsigned value){return(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xdeadbeef1234)};}
static void setup(void){
 CHECK(nativeMatchPool_Bind(&nativeGObjPool,&object,sizeof(object),slots,free_order,1,sizeof(object),_Alignof(HSD_GObj),++epoch));
 NativeMatchPoolReference ref;CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&ref)==&object);
 memset(&source,0,sizeof(source));memset(&fighter,0,sizeof(fighter));memset(&arena,0,sizeof(arena));memset(heads,0,sizeof(heads));
 source.address=&source;source.ready=source.assets.ready=true;source.target=Ft_Kind_CustomFirst;source.compiler.bank.valid=source.compiler.bank.has_motion_layout=true;
 source.compiler.bank.motion_layout=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=8};source.assets.facts.moves=source.compiler.bank.motion_layout;
 source.assets.facts.joints=3;source.assets.parts=(FighterPartsTable){.parts_num=3,.part_to_joint=source.assets.part_to_joint,.joint_to_part=source.assets.joint_to_part};
 source.assets.motions=motions;source.program.bank=&source.compiler.bank;source.program.identity=123;program=source.program;
 source.compiler.bank.storage_layout.identity=456;source.compiler.bank.storage_parameters.identity=789;
 storage=(NativeScriptStorageInstance){.layout=&source.compiler.bank.storage_layout,.parameters=&source.compiler.bank.storage_parameters,.owner=0,.generation=17,.identity=17,.live=true};
 source.assets.character.import.arena=(uint8_t*)&arena;source.assets.character.import.arena_used=source.assets.character.import.arena_size=sizeof(arena);
 source.assets.character.model_data.x0=(FtPartsDesc){.model_num=3,.costume_count=2,.vis_table=arena.variants};
 source.assets.character.data.x8=&source.assets.character.model_data;
 for(unsigned channel=0;channel<4;++channel){arena.variants[0][channel]=arena.lookups[channel];
  for(unsigned group=0;group<3;++group){arena.lookups[channel][group]=(FtPartsVisLookup){.x0=(int)(group==2?130:group+2),.x4=arena.selections[channel][group]};}}
 arena.lookups[1][0].x0=1; // An absent alternative here still hides this channel.
 arena.variants[1][0]=NULL;arena.variants[1][1]=arena.lookups[1];
 object.user_data=&fighter;object.hsd_obj=&root;fighter.gobj=&object;fighter.kind=fighter.behavior_kind=source.target;fighter.x8_spawnNum=17;
 fighter.ft_data=&source.assets.character.data;fighter.parts=parts;parts[0].joint=&root;
 fighter.x5AC.model_num=3;for(unsigned channel=0;channel<4;++channel)fighter.x5AC.xC[channel]=arena.lookups[channel];
 memset(draws,0,sizeof(draws));
 memset(secondary_draws,0,sizeof(secondary_draws));memset(materials,0,sizeof(materials));
 memset(material_animations,0,sizeof(material_animations));memset(material_calls,0,sizeof(material_calls));material_call_count=material_inject_at=0;
 for(unsigned i=0;i<3;++i){draw_pointers[i]=&draws[i];draws[i].flags=0x20;arena.mesh_ordinals[i]=(u8)i;}
 fighter.dobj_list=(DObjList){.count=3,.data=draw_pointers};
 fighter.x203C=(DObjList){.count=3,.data=secondary_pointers};
 for(unsigned i=0;i<3;++i){secondary_pointers[i]=&secondary_draws[i];
  draws[i].mobj=secondary_draws[i].mobj=&materials[i];materials[i].aobj=&material_animations[i];
  material_animations[i].end_frame=24;material_animations[i].framerate=1;}
 for(unsigned group=0;group<3;++group)for(int choice=0;choice<arena.lookups[0][group].x0;++choice)
  arena.selections[0][group][choice]=(TempS){.x0=1,.x4=&arena.mesh_ordinals[group]};
 gFtDataList[fighter.kind]=fighter.ft_data;ftData_CharacterStateTables[fighter.kind]=motions;tables[fighter.kind]=&source.assets.parts;
 heads[HSD_GOBJ_PLINK_FIGHTER]=&object;actor_status=NativeScriptVM_Ok;inject=variant=0;Fighter_804D653C=colors;Fighter_804D6538=special;
 for(unsigned i=0;i<FT_COMMON_COLOR_ENTRY_COUNT;++i)colors[i]=(struct Fighter_804D653C_t){.unk=&commands[i],.unk4=i%5,.unk5=i&1};
 for(unsigned i=0;i<FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT;++i)special[i]=(struct Fighter_804D653C_t){.unk=&commands[123+i],.unk4=i%5};
 context=(NativeScriptRuntimeContext){.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17,.scene_epoch=epoch};
}
static void same(const Fighter* expected){CHECK(!memcmp(expected,&fighter,sizeof(fighter)));++comparisons;}
static void successful(void){
 for(unsigned visible=0;visible<2;++visible){setup();NativeScriptValue args[]={boolean(visible)},out;Fighter expected=fighter;expected.invisible=!visible;
  CHECK(nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_VisualsSetVisible,args,1,&out,&diagnostic)&&out.type==NativeScriptType_Void);same(&expected);}
 for(unsigned model=0;model<3;++model)for(int group=0;group<3;++group)for(int value=-1;value<(group==2?128:group+2);++value){
  setup();variant=model?model==1?1:99:0;Fighter before=fighter;ftParts_80074A4C(&object,group,value);Fighter expected=fighter;fighter=before;
  NativeScriptValue args[]={integer(group),integer(value)},out;CHECK(nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_VisualsSetGroup,args,2,&out,&diagnostic)&&out.type==NativeScriptType_Void);same(&expected);}
 setup();source.assets.facts.layout.visibility_mask=1;source.assets.character.visibility_assets[0]=arena.lookups[3];fighter.x5AC.xC[4]=arena.lookups[3];
 NativeScriptValue outline_args[]={integer(0),integer(1)},outline_out;Fighter initial=fighter;ftParts_80074A4C(&object,0,1);Fighter outline_expected=fighter;fighter=initial;
 CHECK(nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_VisualsSetGroup,outline_args,2,&outline_out,&diagnostic));same(&outline_expected);
 for(int id=0;id<FT_COMMON_COLOR_ENTRY_COUNT+FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT;++id)for(unsigned mode=0;mode<2;++mode)for(int previous=0;previous<5;++previous){
  setup();fighter.x408.x28_colanim.i=fighter.x488.x28_colanim.i=previous;fighter.x508.x28_colanim.i=previous;
  fighter.x408.x0_timer=fighter.x488.x0_timer=fighter.x508.x0_timer=77;
  Fighter before=fighter;(void)ftCo_800BFFD0(&fighter,id,mode!=0);Fighter expected=fighter;fighter=before;
  NativeScriptValue args[]={integer(id),boolean(mode)},out;CHECK(nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_ColorStart,args,2,&out,&diagnostic)&&out.type==NativeScriptType_Void);same(&expected);}
}
static void rejection(uint32_t semantic,NativeScriptValue* args,unsigned count){
 Fighter before=fighter;NativeScriptValue out=sentinel();CHECK(!nativeScriptBindingVisuals_Invoke(&context,semantic,args,count,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_C(0xdeadbeef1234));CHECK(!memcmp(&before,&fighter,sizeof(fighter)));
}
static void absent_selector_cases(void)
{
 const int choices[]={-1,0,1,2,3,127};
 for(unsigned empty=0;empty<2;++empty)for(unsigned live=0;live<2;++live)
 for(int index=0;index<3;++index)for(unsigned choice=0;choice<6;++choice) {
  setup();if(empty)for(unsigned channel=0;channel<4;++channel)arena.lookups[channel][index].x0=0;
  Fighter before=fighter;HSD_DObj before_draws[3],expected_draws[3];memcpy(before_draws,draws,sizeof(draws));
  if(live)ftParts_80074B0C(&object,index,choices[choice]);else ftParts_80074A4C(&object,index,choices[choice]);
  Fighter expected=fighter;memcpy(expected_draws,draws,sizeof(draws));fighter=before;memcpy(draws,before_draws,sizeof(draws));
  NativeScriptValue args[]={integer(index),integer(choices[choice])},out;
  bool accepted=nativeScriptBindingVisuals_Invoke(&context,live?NativeScriptBinding_VisualsSetCurrentGroup:NativeScriptBinding_VisualsSetGroup,args,2,&out,&diagnostic);
  if(!accepted)fprintf(stderr,"empty=%u live=%u group=%d selector=%d: %s\n",empty,live,index,choices[choice],diagnostic.message);
  CHECK(accepted);
  CHECK(out.type==NativeScriptType_Void);same(&expected);CHECK(!memcmp(expected_draws,draws,sizeof(draws)));if(live)++group_comparisons;
 }
}
static void failures(void){
 NativeScriptValue args[]={integer(0),integer(0)};
 const int64_t bad[]={-2,128,255,256,INT64_MAX};
 for(unsigned i=0;i<5;++i){setup();args[0]=integer(2);args[1]=integer(bad[i]);rejection(NativeScriptBinding_VisualsSetGroup,args,2);}
 setup();args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)};args[1]=integer(-1);rejection(NativeScriptBinding_VisualsSetGroup,args,2);
 setup();args[0]=integer(3);rejection(NativeScriptBinding_VisualsSetGroup,args,2);
 setup();args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};args[1]=integer(0);rejection(NativeScriptBinding_VisualsSetGroup,args,2);
 for(unsigned failure=0;failure<10;++failure){setup();args[0]=integer(0);args[1]=integer(1);
  switch(failure){case 0:source.ready=false;break;case 1:storage.live=false;break;case 2:++context.scene_epoch;break;
   case 3:source.assets.parts.part_to_joint=NULL;break;case 4:fighter.x5AC.model_num=4;break;
   case 5:fighter.x5AC.xC[0]=NULL;break;case 6:arena.lookups[0][0].x0=257;break;
   case 7:arena.lookups[0][0].x4=NULL;break;case 8:source.assets.character.import.arena_used=1;break;
   case 9:fighter.x5AC.xC[4]=arena.lookups[0];break;}
  rejection(NativeScriptBinding_VisualsSetGroup,args,2);
 }
 for(unsigned failure=0;failure<6;++failure){setup();args[0]=integer(0);args[1]=boolean(0);
  switch(failure){case 0:args[0]=integer(-1);break;case 1:args[0]=integer(129);break;case 2:args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};break;
   case 3:Fighter_804D653C=NULL;break;case 4:fighter.x408.x28_colanim.i=123;break;case 5:args[1]=integer(1);break;}
  rejection(NativeScriptBinding_ColorStart,args,2);
 }
 setup();args[0]=integer(123);args[1]=boolean(0);Fighter_804D6538=NULL;rejection(NativeScriptBinding_ColorStart,args,2);
 setup();fighter.x508.x28_colanim.i=6;rejection(NativeScriptBinding_ColorStart,args,2);
 setup();args[0]=integer(0);rejection(NativeScriptBinding_VisualsSetVisible,args,1);
 setup();args[0]=boolean(1);context.source_class=NativeCallbackSource_Item;rejection(NativeScriptBinding_VisualsSetVisible,args,1);
}
static void retirement(void){
 for(unsigned which=0;which<2;++which)for(unsigned mode=1;mode<=(which?7:6);++mode){setup();inject=mode;
  NativeScriptValue args[]={integer(which?1:0),which?boolean(1):integer(-1)},out=sentinel();
  CHECK(!nativeScriptBindingVisuals_Invoke(&context,which?NativeScriptBinding_ColorStart:NativeScriptBinding_VisualsSetGroup,args,2,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_C(0xdeadbeef1234));}
}
static void live_groups(void){
 for(unsigned cleared=0;cleared<2;++cleared)for(unsigned same_value=0;same_value<2;++same_value)
  for(int group=0;group<3;++group)for(int value=-1;value<(group==2?128:group+2);++value){
   setup();fighter.x5AC.cleared[0]=(u8)cleared;fighter.x5AC.cleared[1]=1;
   fighter.x5F4_arr[group].idx=(s8)(same_value?value:value==0?1:0);fighter.x5F4_arr[group].prev=1;
   Fighter before=fighter;HSD_DObj old_draws[3],expected_draws[3];memcpy(old_draws,draws,sizeof(draws));
   ftParts_80074B0C(&object,group,value);Fighter expected=fighter;memcpy(expected_draws,draws,sizeof(draws));
   fighter=before;memcpy(draws,old_draws,sizeof(draws));
   NativeScriptValue args[]={integer(group),integer(value)},out=sentinel();
   CHECK(nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_VisualsSetCurrentGroup,args,2,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_Void);same(&expected);CHECK(!memcmp(expected_draws,draws,sizeof(draws)));++group_comparisons;
   CHECK(fighter.x5F4_arr[group].prev==1&&fighter.x5AC.cleared[1]==1);
   if(same_value)CHECK(fighter.x5AC.cleared[0]==cleared&&!fighter.x221D_b2);
   else CHECK(!fighter.x5AC.cleared[0]&&fighter.x221D_b2);
  }
 setup();fighter.x5F4_arr[0].prev=1;
 NativeScriptValue args[]={integer(0),integer(-1)},out;
 CHECK(nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_VisualsSetCurrentGroup,args,2,&out,&diagnostic));
 CHECK(fighter.x5F4_arr[0].idx==-1&&fighter.x5F4_arr[0].prev==1);
 ftParts_80074A8C(&object);CHECK(fighter.x5F4_arr[0].idx==1&&!fighter.x221D_b2);
 // Actual DObjSetFlags accepts a null list entry as a no-op. The list's
 // ordinal/data footprint remains bounded before the native traversal.
 setup();fighter.x5AC.cleared[0]=1;draw_pointers[1]=NULL;
 CHECK(nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_VisualsSetCurrentGroup,args,2,&out,&diagnostic));
 CHECK((draws[0].flags&DOBJ_HIDDEN)&&(draws[2].flags&DOBJ_HIDDEN)&&draws[1].flags==0x20);
 for(unsigned negative=0;negative<8;++negative){setup();fighter.x5AC.cleared[0]=1;
  switch(negative){case 0:arena.lookups[0][2].x0=257;break;
   case 1:arena.selections[0][2][0].x0=-1;break;case 2:arena.selections[0][2][0].x0=257;break;
   case 3:arena.selections[0][2][0].x4=NULL;break;case 4:arena.mesh_ordinals[2]=3;break;
   case 5:fighter.dobj_list.data=NULL;break;case 6:args[0]=integer(3);break;
   case 7:args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};break;}
  rejection(NativeScriptBinding_VisualsSetCurrentGroup,args,2);args[0]=integer(0);
 }
 for(unsigned mutation=1;mutation<=11;++mutation){if(mutation==7)continue;setup();inject=mutation;out=sentinel();
  CHECK(!nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_VisualsSetCurrentGroup,args,2,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_C(0xdeadbeef1234));
 }
}
static void group_queries(void){
 const uint32_t ids[]={NativeScriptBinding_VisualsCurrentGroup,NativeScriptBinding_VisualsPreviousGroup};
 for(unsigned which=0;which<2;++which){
  const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(ids[which]);
  CHECK(descriptor&&descriptor->result_type==NativeScriptType_I32&&descriptor->argument_count==1&&descriptor->arguments[0].numeric_any);
  CHECK((descriptor->flags&NativeScriptFlag_Pure)&&!(descriptor->flags&(NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_SourceOnly)));
  for(int group=0;group<3;++group)for(int stored=-1;stored<=127;++stored){setup();
   fighter.x5F4_arr[group].idx=(s8)stored;fighter.x5F4_arr[group].prev=(s8)(126-stored);
   int expected=which?nativeFighterVisuals_PreviousGroup(&fighter,group):nativeFighterVisuals_CurrentGroup(&fighter,group);
   NativeScriptValue index=integer(group),out=sentinel();Fighter before=fighter;
   CHECK(nativeScriptBindingVisuals_Invoke(&context,ids[which],&index,1,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_I32&&out.as.i64==expected);CHECK(!memcmp(&before,&fighter,sizeof(fighter)));++query_comparisons;
  }
  const NativeScriptValue indexes[]={integer(-1),integer(3),integer(10),integer(11),integer(INT64_MIN),integer(INT64_MAX),
   {.type=NativeScriptType_U64,.as.u64=UINT64_MAX},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)},
   {.type=NativeScriptType_U32,.as.u64=UINT32_MAX}};
  for(unsigned i=0;i<sizeof(indexes)/sizeof(*indexes);++i){setup();NativeScriptValue out=sentinel();
   CHECK(nativeScriptBindingVisuals_Invoke(&context,ids[which],&indexes[i],1,&out,&diagnostic));CHECK(out.type==NativeScriptType_I32&&out.as.i64==-1);
  }
  setup();source.assets.character.model_data.x0.model_num=fighter.x5AC.model_num=0;
  NativeScriptValue index=integer(0),out;CHECK(nativeScriptBindingVisuals_Invoke(&context,ids[which],&index,1,&out,&diagnostic)&&out.as.i64==-1);
  setup();index=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};rejection(ids[which],&index,1);
  for(unsigned invalid=0;invalid<6;++invalid){setup();index=integer(0);
   switch(invalid){case 0:source.ready=false;break;case 1:storage.live=false;break;case 2:++context.generation;break;
    case 3:fighter.x5AC.model_num=4;break;case 4:fighter.x5AC.xC[0]=NULL;break;case 5:context.source_class=NativeCallbackSource_Item;break;}
   rejection(ids[which],&index,1);
  }
 }
}
static NativeScriptValue floating(float value){return(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};}
static void material_setup(unsigned channel){
 setup();arena.lookups[channel][0].x0=2;
 arena.selections[channel][0][1]=(TempS){.x0=3,.x4=arena.mesh_ordinals};
}
static void material_cases(void){
 const uint32_t methods[]={NativeScriptBinding_VisualsSetGroupMaterialFrame,NativeScriptBinding_VisualsInitializeGroupMaterialAnimation};
 for(unsigned freeze=0;freeze<2;++freeze)for(unsigned channel=0;channel<4;++channel)for(unsigned missing=0;missing<3;++missing){
  material_setup(channel);NativeScriptValue args[]={integer(channel),integer(0),integer(1),floating(freeze?0:3.5f)},out=sentinel();
  if(missing==1){draw_pointers[1]=secondary_pointers[1]=NULL;}
  if(missing==2)materials[1].aobj=NULL;
  float expected_end=0;
  if(freeze)expected_end=nativeFighterVisuals_InitializeGroupMaterialAnimation(&fighter,channel,0,1,0);
  else nativeFighterVisuals_SetGroupMaterialFrame(&fighter,channel,0,1,3.5f);
  struct MaterialCall expected[12];memcpy(expected,material_calls,sizeof(expected));unsigned expected_calls=material_call_count;
  HSD_AObj expected_animations[3];memcpy(expected_animations,material_animations,sizeof(expected_animations));
  for(unsigned i=0;i<3;++i)material_animations[i].framerate=1;
  memset(material_calls,0,sizeof(material_calls));material_call_count=0;
  CHECK(nativeScriptBindingVisuals_Invoke(&context,methods[freeze],args,4,&out,&diagnostic));
  CHECK(out.type==(freeze?NativeScriptType_F32:NativeScriptType_Void));if(freeze)CHECK(out.as.f32==expected_end);
  CHECK(material_call_count==expected_calls&&!memcmp(expected,material_calls,sizeof(expected)));
  CHECK(!memcmp(expected_animations,material_animations,sizeof(expected_animations)));++material_comparisons;
 }
 for(unsigned freeze=0;freeze<2;++freeze){
  setup();fighter.x5AC.xC[3]=NULL;arena.variants[0][3]=NULL;
  NativeScriptValue args[]={integer(3),integer(0),integer(0),floating(17)},out=sentinel();
  CHECK(nativeScriptBindingVisuals_Invoke(&context,methods[freeze],args,4,&out,&diagnostic));
  CHECK(!material_call_count);if(freeze)CHECK(out.type==NativeScriptType_F32&&out.as.f32==17);
  for(unsigned invalid=0;invalid<12;++invalid){material_setup(0);
   args[0]=integer(0);args[1]=integer(0);args[2]=integer(1);args[3]=floating(freeze?0:3);
   switch(invalid){case 0:args[0]=integer(-1);break;case 1:args[0]=integer(5);break;
    case 2:args[1]=integer(3);break;case 3:args[2]=integer(2);break;
    case 4:args[0]=floating(0);break;case 5:args[3]=floating(NAN);break;
    case 6:args[3]=floating(INFINITY);break;case 7:arena.mesh_ordinals[2]=250;break;
    case 8:arena.selections[0][0][1].x0=257;break;case 9:arena.selections[0][0][1].x4=NULL;break;
    case 10:storage.live=false;break;case 11:context.source_class=NativeCallbackSource_Item;break;}
   rejection(methods[freeze],args,4);CHECK(!material_call_count);
   for(unsigned i=0;i<3;++i)CHECK(material_animations[i].framerate==1);
  }
 }
 for(unsigned invalid=0;invalid<3;++invalid){material_setup(0);
  NativeScriptValue args[]={integer(0),integer(0),integer(1),floating(0)};
  if(invalid==0)material_animations[2].end_frame=25;
  if(invalid==1)material_animations[2].end_frame=NAN;
  if(invalid==2)args[3]=floating(15);
  rejection(methods[1],args,4);for(unsigned i=0;i<3;++i)CHECK(material_animations[i].framerate==1);
 }
 const unsigned modes[]={1,2,3,4,5,6,8,9,10,11,12,13,14};
 for(unsigned boundary=1;boundary<=2;++boundary)for(unsigned n=0;n<sizeof(modes)/sizeof(*modes);++n){
  material_setup(0);NativeScriptValue args[]={integer(0),integer(0),integer(1),floating(8)},out=sentinel();
  inject=modes[n];material_inject_at=boundary;
  CHECK(!nativeScriptBindingVisuals_Invoke(&context,methods[0],args,4,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_C(0xdeadbeef1234));
  CHECK(material_call_count==boundary);++material_retirements;
 }
}
static void clear_attack_color_cases(void)
{
 static struct ColorOverlay_UnkInner inner;
 for(unsigned enabled=0;enabled<4;++enabled)for(int priority=-2;priority<=2;++priority){
  setup();fighter.x488.x8_ptr1=&commands[3];fighter.x488.x28_colanim.ptr=&inner;
  fighter.x488.x4_pri=priority;fighter.x488.x7C_color_enable=(enabled&1)!=0;fighter.x488.x7C_flag2=(enabled&2)!=0;
  fighter.x488.x0_timer=47;fighter.x488.xC_loop=2;
  Fighter before=fighter;ftCo_800BFFAC(&fighter);Fighter expected=fighter;fighter=before;
  NativeScriptValue out=sentinel();CHECK(nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_ColorClearAttack,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void);same(&expected);++color_clear_comparisons;
 }
 setup();NativeScriptValue argument=integer(0);rejection(NativeScriptBinding_ColorClearAttack,&argument,1);
 setup();storage.live=false;rejection(NativeScriptBinding_ColorClearAttack,NULL,0);
 setup();context.source_class=NativeCallbackSource_Item;rejection(NativeScriptBinding_ColorClearAttack,NULL,0);
 const unsigned mutations[]={1,2,3,4,5,6,8,9,10,11,12};
 for(unsigned n=0;n<sizeof(mutations)/sizeof(*mutations);++n){
  setup();fighter.x488.x8_ptr1=&commands[3];fighter.x488.x4_pri=1;inject=mutations[n];NativeScriptValue out=sentinel();
  CHECK(!nativeScriptBindingVisuals_Invoke(&context,NativeScriptBinding_ColorClearAttack,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_C(0xdeadbeef1234));
  CHECK(!fighter.x488.x8_ptr1&&fighter.x488.x4_pri==0);
 }
}
int main(void){successful();absent_selector_cases();failures();retirement();live_groups();group_queries();material_cases();clear_attack_color_cases();printf("{\"status\":\"PASS owned common visuals\",\"checks\":%u,\"native_comparisons\":%u,\"current_setter_comparisons\":%u,\"query_comparisons\":%u,\"material_comparisons\":%u,\"material_retirements\":%u,\"color_clear_comparisons\":%u}\n",checks,comparisons,group_comparisons,query_comparisons,material_comparisons,material_retirements,color_clear_comparisons);return 0;}
