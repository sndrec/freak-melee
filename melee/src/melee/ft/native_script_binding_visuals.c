#include "native_script_binding_visuals.h"
#include "native_script_bindings.h"
#include "native_script_fighter.h"
#include "native_source_package.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include "native_fighter_pose.h"
#include "ftcostume_assets.h"
#include "native_costume_runtime.h"
#include "ftcommon_assets.h"
#include "fighter.h"
#include "ftcolanim.h"
#include "ftparts.h"
#include "ftanim.h"
#include <melee/lb/types.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

typedef struct VisualSource {
 NativeScriptRuntimeContext* context;
 NativeMatchPoolReference reference;
 const NativeSourcePackage* package;
 const NativeScriptFighterProgram* program;
 NativeScriptReceiverView view;
 Fighter* fighter;FighterBone* parts;void* root;
 const struct ftData_x8* model_data;FtPartsDesc model;
 const FtPartsVisLookup* lookups[5];DObjList draws,secondary_draws;
 const void* import_arena;size_t import_used,import_size;
 uint64_t program_identity,layout_identity,parameter_identity;
} VisualSource;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
bool nativeScriptBindingVisuals_Read(NativeScriptRuntimeContext* c,uint32_t semantic,
 NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 NativeMatchPoolReference reference;NativeFighterLifetime life;
 if(semantic!=NativeScriptBinding_VisualsHeldItemVisible||!out||!c||
    c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||
    !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference)||reference.lifetime!=c->scene_epoch||
    !nativeScriptBindings_SourceAlive(c)||!nativeScriptBindings_SourceFighterLayout(c,d)||
    !nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,&life)||!life.source_owned||
    life.object!=c->source||life.generation!=c->generation||life.reference.lifetime!=c->scene_epoch)
  return error(d,"Held-item visibility needs its live selected source Fighter");
 NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=life.data->x221E_b3};
 if(!nativeFighterLifetime_Current(&life)||!nativeScriptBindings_SourceFighterLayout(c,d))
  return error(d,"Held-item visibility source, program or storage retired");
 *out=value;return true;
}
static bool current(const VisualSource* source,NativeScriptDiagnostic* d)
{
 NativeScriptRuntimeContext* c=source->context;NativeScriptReceiverView view;
 if(nativeMatchPool_Resolve(source->reference)!=c->source||source->reference.lifetime!=c->scene_epoch||
    !nativeScriptBindings_SourceAlive(c)||((HSD_GObj*)c->source)->user_data!=source->fighter||
    nativeFighterRuntime_SourcePackage(source->fighter->kind)!=source->package||
    nativeScriptFighter_Program(source->fighter->kind)!=source->program||
    source->program->identity!=source->program_identity||source->program->bank!=&source->package->compiler.bank||
    nativeScriptBindings_SourceFighterLayout(c,d)!=&source->program->bank->motion_layout||
    source->fighter->parts!=source->parts||((HSD_GObj*)c->source)->hsd_obj!=source->root||
    source->fighter->ft_data->x8!=source->model_data||
    source->model_data->x0.model_num!=source->model.model_num||source->model_data->x0.costume_count!=source->model.costume_count||
    source->model_data->x0.vis_table!=source->model.vis_table||source->fighter->x5AC.model_num!=source->model.model_num||
    source->package->assets.character.import.arena!=source->import_arena||
    source->package->assets.character.import.arena_used!=source->import_used||source->package->assets.character.import.arena_size!=source->import_size||
    source->fighter->dobj_list.data!=source->draws.data||source->fighter->dobj_list.count!=source->draws.count||
    source->fighter->x203C.data!=source->secondary_draws.data||source->fighter->x203C.count!=source->secondary_draws.count||
    !source->parts||source->parts[0].joint!=source->root||
    !ftPartsTable||ftPartsTable[source->fighter->kind]!=&source->package->assets.parts||
    !source->package->assets.parts.parts_num||source->package->assets.parts.parts_num>MAX_FT_PARTS||
    source->package->assets.parts.parts_num!=source->package->assets.facts.joints+source->package->assets.facts.omission_count||
    source->package->assets.parts.part_to_joint!=source->package->assets.part_to_joint||
    source->package->assets.parts.joint_to_part!=source->package->assets.joint_to_part||
    !nativeScriptFighter_View(c->source,c->generation,&view)||view.program!=source->view.program||
    view.storage!=source->view.storage||!view.storage||!view.storage->live||
    view.storage->layout!=&source->program->bank->storage_layout||view.storage->layout->identity!=source->layout_identity||
    view.storage->parameters!=&source->program->bank->storage_parameters||view.storage->parameters->identity!=source->parameter_identity)
  return error(d,"Visual source, program, storage or owned rig retired");
 for(unsigned channel=0;channel<5;++channel)
  if(source->fighter->x5AC.xC[channel]!=source->lookups[channel])return error(d,"Owned visibility channel changed before publication");
 return true;
}
static bool capture(NativeScriptRuntimeContext* c,VisualSource* source,NativeScriptDiagnostic* d)
{
 if(!c||c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||
    !nativeMatchPool_Reference(&nativeGObjPool,c->source,&source->reference)||source->reference.lifetime!=c->scene_epoch||
    !nativeScriptBindings_SourceFighterLayout(c,d))return error(d,"Visual operation requires a live bound source Fighter");
 source->context=c;source->fighter=((HSD_GObj*)c->source)->user_data;source->parts=source->fighter->parts;source->root=((HSD_GObj*)c->source)->hsd_obj;
 source->package=nativeFighterRuntime_SourcePackage(source->fighter->kind);source->program=nativeScriptFighter_Program(source->fighter->kind);
 if(!source->package||!source->program||!source->parts||!source->root||
    !nativeScriptFighter_View(c->source,c->generation,&source->view)||!source->view.storage||
    !source->view.storage->layout||!source->view.storage->parameters)return error(d,"Visual source owned view is absent");
 source->program_identity=source->program->identity;source->layout_identity=source->view.storage->layout->identity;
 source->model_data=&source->package->assets.character.model_data;source->model=source->model_data->x0;
 for(unsigned channel=0;channel<5;++channel)source->lookups[channel]=source->fighter->x5AC.xC[channel];
 source->draws=source->fighter->dobj_list;source->secondary_draws=source->fighter->x203C;
 source->import_arena=source->package->assets.character.import.arena;
 source->import_used=source->package->assets.character.import.arena_used;source->import_size=source->package->assets.character.import.arena_size;
 source->parameter_identity=source->view.storage->parameters->identity;return current(source,d);
}
static bool number(const NativeScriptValue* value,int32_t minimum,int32_t maximum,int32_t* result)
{
 uint64_t unsigned_value;
 switch(value->type){
 case NativeScriptType_I32:case NativeScriptType_I64:
  if(value->as.i64<minimum||value->as.i64>maximum)return false;*result=(int32_t)value->as.i64;return true;
 case NativeScriptType_Bool:unsigned_value=value->as.u64!=0;break;
 case NativeScriptType_U32:case NativeScriptType_U64:unsigned_value=value->as.u64;break;
 default:return false;
 }
 if(unsigned_value>(uint32_t)maximum)return false;*result=(int32_t)unsigned_value;return true;
}
static bool span(const HSD_DatImport* import,const void* pointer,size_t bytes)
{
 uintptr_t start=(uintptr_t)import->arena,at=(uintptr_t)pointer;
 return start&&pointer&&import->arena_used<=import->arena_size&&at>=start&&at-start<=import->arena_used&&bytes<=import->arena_used-(at-start);
}
static bool visibility_group_valid(const VisualSource* source,int32_t index,bool selection,bool clear_channel,NativeScriptDiagnostic* d)
{
 Fighter* fp=source->fighter;const FtCharacterAssets* assets=&source->package->assets.character;
 const FtPartsDesc* model=&assets->model_data.x0;const HSD_DatImport* import=&assets->import;
 if(!selection&&!model->model_num&&fp->ft_data->x8==&assets->model_data&&!fp->x5AC.model_num)return true;
 if(fp->ft_data->x8!=&assets->model_data||!model->model_num||model->model_num>11||fp->x5AC.model_num!=model->model_num||
    (index>=0&&(uint32_t)index>=model->model_num)||!model->costume_count||model->costume_count>8||
    !span(import,model->vis_table,(size_t)model->costume_count*sizeof(*model->vis_table)))
  return error(d,"Visibility group differs from its owned imported model table");
 uint32_t variant=nativeCostumeRuntime_ModelVariant(fp);if(variant>=model->costume_count)variant=0;
 for(unsigned channel=0;channel<5;++channel){
  const FtPartsVisLookup* lookup=fp->x5AC.xC[channel];
  if(channel<4){const void* expected=model->vis_table[variant][channel]?model->vis_table[variant][channel]:model->vis_table[0][channel];
   if(lookup!=expected)return error(d,"Visibility channel is not the selected owned model variant");
  }else if(lookup){bool owned=false;
   for(unsigned slot=0;slot<FT_CHARACTER_ITEM_CAPACITY;++slot)
    if((source->package->assets.facts.layout.visibility_mask&(1u<<slot))&&assets->visibility_assets[slot]==lookup)owned=true;
   if(!owned)return error(d,"Outline visibility channel is not a declared owned lookup");
  }
  if(!lookup)continue;
  if(!span(import,lookup,(size_t)model->model_num*sizeof(*lookup)))return error(d,"Visibility lookup exceeds its owned imported span");
  bool traversed=clear_channel&&channel==0&&fp->x5AC.cleared[0];
  for(uint32_t row_index=0;row_index<model->model_num;++row_index){
   if(!traversed&&(int32_t)row_index!=index)continue;
   const FtPartsVisLookup* row=&lookup[row_index];
   if(row->x0<0||row->x0>256||(row->x0&&!span(import,row->x4,(size_t)row->x0*sizeof(*row->x4))))
    return error(d,"Visibility alternatives exceed their owned imported table");
   if(traversed)for(int choice=0;choice<row->x0;++choice){
    const TempS* meshes=&row->x4[choice];
    if(meshes->x0<0||meshes->x0>256||(meshes->x0&&!span(import,meshes->x4,(size_t)meshes->x0)))
     return error(d,"Visibility mesh ordinals exceed their owned imported table");
    for(int draw=0;draw<meshes->x0;++draw){
     uint32_t ordinal=meshes->x4[draw];
     if(!fp->dobj_list.data||ordinal>=fp->dobj_list.count)
      return error(d,"Visibility mesh ordinal is absent from the current owned model");
    }
   }
  }
 }
 // Native selectors are compared to alternatives, never used as array indices.
 // An absent selector hides every alternative; an empty row still stores the
 // selector. Stock reset/auxiliary hooks rely on this defined behavior.
 return true;
}
typedef struct MaterialSelection {
 const FtPartsVisLookup* row;const TempS* meshes;DObjList list;
 int alternatives,count;TempS* choices;u8* indices;
 HSD_DObj* draws[256];HSD_MObj* materials[256];HSD_AObj* animations[256];
} MaterialSelection;
static bool materials_current(const VisualSource* source,const MaterialSelection* selected,int index,NativeScriptDiagnostic* d)
{
 if(!current(source,d))return false;
 if(selected->row->x0!=selected->alternatives||selected->row->x4!=selected->choices||
    selected->meshes->x0!=selected->count||selected->meshes->x4!=selected->indices)
  return error(d,"Selected material-animation mesh table changed during evaluation");
 int first=index<0?0:index,limit=index<0?selected->count:index+1;
 for(int i=first;i<limit;++i){
  unsigned ordinal=selected->indices[i];
  if(ordinal>=selected->list.count||selected->list.data[ordinal]!=selected->draws[i]||
     (selected->draws[i]&&selected->draws[i]->mobj!=selected->materials[i])||
     (selected->materials[i]&&selected->materials[i]->aobj!=selected->animations[i]))
   return error(d,"Selected material or animation identity changed during evaluation");
 }
 return true;
}
static bool material_animation(const VisualSource* source,int table,int index,int variant,float* frame,bool freeze,NativeScriptDiagnostic* d)
{
 if(!visibility_group_valid(source,index,false,false,d))return false;
 const FtPartsVisLookup* lookup=source->lookups[table];
 if(!lookup)return true; // Native absent-channel behavior.
 const FtPartsVisLookup* row=&lookup[index];const HSD_DatImport* import=&source->package->assets.character.import;
 if(variant>=row->x0)return error(d,"Material-animation alternative is absent in its selected channel");
 const TempS* meshes=&row->x4[variant];
 MaterialSelection selected={.row=row,.meshes=meshes,.alternatives=row->x0,.choices=row->x4,
  .count=meshes->x0,.indices=meshes->x4,.list=table==2?source->secondary_draws:source->draws};
 if(selected.count<0||selected.count>256||
    (selected.count&&(!span(import,selected.indices,(size_t)selected.count)||!selected.list.data)))
  return error(d,"Material-animation mesh indices exceed the owned imported table");
 float end=*frame;
 // Validate every target and all lengths before changing the first animation.
 for(int i=0;i<selected.count;++i){
  unsigned ordinal=selected.indices[i];if(ordinal>=(unsigned)selected.list.count)
   return error(d,"Material-animation mesh index exceeds its selected native draw list");
  HSD_DObj* draw=selected.list.data[ordinal];HSD_MObj* material=draw?draw->mobj:NULL;
  HSD_AObj* animation=material?material->aobj:NULL;
  selected.draws[i]=draw;selected.materials[i]=material;selected.animations[i]=animation;
  if(freeze&&animation){
   float candidate=HSD_AObjGetEndFrame(animation);
   if(!isfinite(candidate))return error(d,"Material-animation length is not finite");
   if(end==0)end=candidate;
   else if(end!=candidate)return error(d,"Selected material-animation lengths differ");
  }
 }
 if(!materials_current(source,&selected,-1,d))return false;
 for(int i=0;i<selected.count;++i){
  // Check the next node before using it and retain it across each native
  // leaf. Whole-selection scans bracket the loop, keeping this path linear.
  if(!materials_current(source,&selected,i,d))return false;
  if(freeze)HSD_AObjSetRate(selected.animations[i],0);
  else{
   HSD_MObjReqAnim(selected.materials[i],*frame);
   if(!materials_current(source,&selected,i,d))return false;
   HSD_MObjAnim(selected.materials[i]);
  }
  if(!materials_current(source,&selected,i,d))return false;
 }
 if(!materials_current(source,&selected,-1,d))return false;
 if(freeze)*frame=end;
 return true;
}
static bool part_animation_valid(const VisualSource* source,int32_t group,int32_t variant,float duration,bool applying,NativeScriptDiagnostic* d)
{
 Fighter* fp=source->fighter;const FtCharacterAssets* assets=&source->package->assets.character;
 const HSD_DatImport* import=&assets->import;
 if(group<0||group>=5||variant<0||variant>=FT_CHARACTER_PART_ANIMATION_VARIANTS||!isfinite(duration)||
    !Fighter_804D6540||Fighter_804D6540[fp->kind]!=&source->package->assets.omissions||
    fp->ft_data->x1C!=assets->part_animation_groups||
    assets->part_animation_groups[group]!=&assets->part_animation_group_data[group])
  return error(d,"Part animation needs an owned group, present variant and finite duration");
 const struct ftData_x1C* selected=assets->part_animation_groups[group];
 if(!span(import,selected->x8,FT_CHARACTER_PART_ANIMATION_VARIANTS*sizeof(*selected->x8))||!selected->x8[variant])
  return error(d,"Part animation variant is absent from the owned imported group");
 // Validate the entire native traversal before applying its first pose. A
 // local stack avoids mutating ftAnim's traversal scratch during admission.
 HSD_AnimJoint* stack[30];unsigned depth=0,visited=0;float end_frame=0;bool found_animation=false;
 HSD_AnimJoint* node=selected->x8[variant];unsigned part=selected->x0;
 unsigned count=source->package->assets.parts.parts_num;
 while(node){
  if(++visited>count||!span(import,node,sizeof(*node)))return error(d,"Part animation tree exceeds its owned rig/import");
  while(part<count&&ftParts_8007506C(fp->kind,part))++part;
  if(part>=count||(applying&&!fp->parts[part].flags_b0&&node->aobjdesc&&!fp->parts[part].x4_jobj2)||
     (node->aobjdesc&&(!span(import,node->aobjdesc,sizeof(*node->aobjdesc))||!isfinite(node->aobjdesc->end_frame))))
   return error(d,"Part animation has an absent bone, pose joint or invalid animation descriptor");
  if(node->aobjdesc&&!found_animation){end_frame=node->aobjdesc->end_frame;found_animation=true;}
  ++part;
  if(node->child){if(depth>=29)return error(d,"Part animation exceeds native traversal depth");stack[depth++]=node;node=node->child;}
  else if(node->next)node=node->next;
  else {node=NULL;while(depth){node=stack[--depth]->next;if(node)break;}}
 }
 if(duration!=0&&!isfinite(end_frame/duration))return error(d,"Part animation blend rate is not finite");
 return current(source,d);
}
bool nativeScriptBindingVisuals_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
 const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 if(semantic==NativeScriptBinding_ColorClearAttack){
  if(!out||count)return error(d,"Attack color clearing takes no arguments");
  VisualSource source;if(!capture(context,&source,d))return false;
  ftCo_800BFFAC(source.fighter);
  if(!current(&source,d))return false;
  *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
 }
 bool query=semantic==NativeScriptBinding_VisualsCurrentGroup||semantic==NativeScriptBinding_VisualsPreviousGroup;
 bool selection=semantic==NativeScriptBinding_VisualsSetGroup||semantic==NativeScriptBinding_VisualsSetCurrentGroup;
 if(semantic==NativeScriptBinding_VisualsSetGroupMaterialFrame||semantic==NativeScriptBinding_VisualsInitializeGroupMaterialAnimation){
  int32_t table,index,variant;
  if(!out||!args||count!=4||!number(&args[0],0,4,&table)||!number(&args[1],0,10,&index)||
     !number(&args[2],0,255,&variant)||args[3].type!=NativeScriptType_F32||!isfinite(args[3].as.f32))
   return error(d,"Material animation expects integral channel/group/alternative and finite Float frame");
  VisualSource source;if(!capture(context,&source,d)||(uint32_t)index>=source.model.model_num)
   return error(d,"Material-animation group is absent from its owned model");
  float frame=args[3].as.f32;bool freeze=semantic==NativeScriptBinding_VisualsInitializeGroupMaterialAnimation;
  if(!material_animation(&source,table,index,variant,&frame,freeze,d)||!current(&source,d))return false;
  *out=freeze?(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=frame}:(NativeScriptValue){.type=NativeScriptType_Void};return true;
 }
 if(semantic==NativeScriptBinding_VisualsApplyPartAnimation||semantic==NativeScriptBinding_VisualsSetPartAnimation||
    semantic==NativeScriptBinding_VisualsShowPartAnimation||semantic==NativeScriptBinding_VisualsHidePartAnimation){
  bool applying=semantic==NativeScriptBinding_VisualsApplyPartAnimation;
  bool setting=semantic==NativeScriptBinding_VisualsSetPartAnimation;
  bool hiding=semantic==NativeScriptBinding_VisualsHidePartAnimation;
  uint32_t expected=applying?3:setting?2:1;int32_t group,variant=-1;float duration=0;
  if(!out||!args||count!=expected||!number(&args[0],0,4,&group)||
     ((applying||setting)&&!number(&args[1],setting?-1:0,FT_CHARACTER_PART_ANIMATION_VARIANTS-1,&variant))||
     (applying&&(args[2].type!=NativeScriptType_F32||!isfinite(args[2].as.f32))))
   return error(d,"Part animation needs a group, valid variant and finite blend duration");
  VisualSource source;if(!capture(context,&source,d))return false;Fighter* fp=source.fighter;
  if(applying)duration=args[2].as.f32;
  if(!applying&&!setting)variant=hiding?fp->x8B0[group].x11:fp->x8B0[group].x10;
  // Unselected/inactive slots are native no-ops, even without a group asset.
  if(variant==-1&&!applying){
   if(setting)ftAnim_80070FB4(context->source,group,-1);
   if(!current(&source,d))return false;
   *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
  }
  if(!part_animation_valid(&source,group,variant,duration,applying||(!setting&&!hiding),d))return false;
  const FtCharacterAssets* assets=&source.package->assets.character;
  const struct ftData_x1C* selected=assets->part_animation_groups[group];
  HSD_AnimJoint** variants=selected->x8;HSD_AnimJoint* animation=variants[variant];
  HSD_JObj* blend=fp->x8AC_animSkeleton;HSD_Joint* costume=fp->x108_costume_joint;FigaTree* clip=fp->x590;
  NativeFighterPoseAssets restoration={0};const void* costume_arena=NULL;size_t costume_used=0,costume_size=0;
  if(hiding){
   if(!nativeFighterRuntime_PoseAssets(context->source,&restoration)||!restoration.costume||
      !nativeFighterPose_Validate(context->source,NativeFighterPose_RestorePartAnimation,
       clip&&fp->x8A4_animBlendFrames!=0?NativeFighterPose_Blend:NativeFighterPose_Live,selected->x0,NULL,clip))
    return error(d,"Part animation restoration needs its owned costume, clip and live rig");
   costume_arena=restoration.costume->import.arena;costume_used=restoration.costume->import.arena_used;
   costume_size=restoration.costume->import.arena_size;
  }
  if(!current(&source,d))return false;
  if(applying)ftAnim_ApplyPartAnim(context->source,group,variant,duration);
  else if(setting)ftAnim_80070FB4(context->source,group,variant);
  else if(hiding)ftAnim_80070CC4(context->source,group);
  else ftAnim_80070C48(context->source,group);
  if(!current(&source,d))return false;
  if(!Fighter_804D6540||Fighter_804D6540[fp->kind]!=&source.package->assets.omissions||
     fp->ft_data->x1C!=assets->part_animation_groups||assets->part_animation_groups[group]!=selected||
     selected->x8!=variants||variants[variant]!=animation||fp->x8AC_animSkeleton!=blend||
     fp->x108_costume_joint!=costume||fp->x590!=clip)
   return error(d,"Part animation assets or restoration rig changed during application");
  if(hiding){
   NativeFighterPoseAssets now;
   if(!nativeFighterRuntime_PoseAssets(context->source,&now)||now.character!=restoration.character||
      now.costume!=restoration.costume||now.action_count!=restoration.action_count||
      now.animation_bytes!=restoration.animation_bytes||now.animation_size!=restoration.animation_size||
      now.animation_arena!=restoration.animation_arena||now.animation_arena_size!=restoration.animation_arena_size||
      now.costume->import.arena!=costume_arena||now.costume->import.arena_used!=costume_used||
      now.costume->import.arena_size!=costume_size)
    return error(d,"Part animation restoration assets retired before output");
  }
  *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
 }
 uint32_t expected=semantic==NativeScriptBinding_VisualsSetVisible||query?1:
  selection||semantic==NativeScriptBinding_ColorStart?2:UINT32_MAX;
 if(!out||expected==UINT32_MAX||count!=expected||!args)return error(d,"Visual argument contract is invalid");
 int32_t first=0,second=0;
 if(semantic==NativeScriptBinding_VisualsSetVisible){
  if(args[0].type!=NativeScriptType_Bool||args[0].as.u64>1)return error(d,"Visibility needs one normalized Bool");
 }else if(query){
  if(args[0].type!=NativeScriptType_Bool&&args[0].type!=NativeScriptType_I32&&args[0].type!=NativeScriptType_U32&&
     args[0].type!=NativeScriptType_I64&&args[0].type!=NativeScriptType_U64)return error(d,"Visibility query needs an integral group index");
  if(!number(&args[0],0,10,&first))first=-1;
 }else if(selection){
  if(!number(&args[0],0,10,&first)||!number(&args[1],-1,INT8_MAX,&second))return error(d,"Group/value must be integral and within native signed8 selection bounds");
 }else if(!number(&args[0],0,FT_COMMON_COLOR_ENTRY_COUNT+FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT-1,&first)||
          args[1].type!=NativeScriptType_Bool||args[1].as.u64>1)return error(d,"Color needs an actual table ID and normalized Bool mode");
 VisualSource source;if(!capture(context,&source,d))return false;
 struct Fighter_804D653C_t* colors=Fighter_804D653C;struct Fighter_804D653C_t* special=Fighter_804D6538;
 if(semantic==NativeScriptBinding_VisualsSetVisible)source.fighter->invisible=!args[0].as.u64;
 else if(selection){
  bool live=semantic==NativeScriptBinding_VisualsSetCurrentGroup;
  if(!visibility_group_valid(&source,first,true,live,d)||!current(&source,d))return false;
  if(live)ftParts_80074B0C(context->source,first,second);
  else ftParts_80074A4C(context->source,first,second);
 }else if(query){
  if(first>=0&&(uint32_t)first>=source.model.model_num)first=-1;
  if(!visibility_group_valid(&source,first,false,false,d)||!current(&source,d))return false;
  int32_t selected=first<0?-1:semantic==NativeScriptBinding_VisualsCurrentGroup?
   source.fighter->x5F4_arr[first].idx:source.fighter->x5F4_arr[first].prev;
  if(!current(&source,d))return false;
  *out=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=selected};return true;
 }else{
  ColorOverlay* overlay;uint32_t limit;
  if(first>=FT_COMMON_COLOR_ENTRY_COUNT){if(!special)return error(d,"Special color table is absent");overlay=&source.fighter->x508;limit=FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT;}
  else {if(!colors)return error(d,"Common color table is absent");overlay=colors[first].unk5?&source.fighter->x488:&source.fighter->x408;limit=FT_COMMON_COLOR_ENTRY_COUNT;}
  if(overlay->x28_colanim.i<0||(uint32_t)overlay->x28_colanim.i>=limit)return error(d,"Existing color overlay index exceeds its selected table");
  (void)ftCo_800BFFD0(source.fighter,first,args[1].as.u64!=0);
  if(Fighter_804D653C!=colors||Fighter_804D6538!=special)return error(d,"Color tables changed during selection");
 }
 if(!current(&source,d))return false;*out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
