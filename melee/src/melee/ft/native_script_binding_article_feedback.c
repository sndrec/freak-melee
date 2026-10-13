#include "native_script_binding_article_feedback.h"
#include "native_script_bindings.h"
#include "native_script_effect_dispatch.h"
#include "native_callback_lifetime.h"
#include "native_script_binding_item_model_resolve.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/types.h>
#include <melee/ef/efsync.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efasync.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

typedef struct ArticleFeedbackSource {
 NativeScriptRuntimeContext* context;NativeMatchPoolReference reference;
 const NativeScriptItemProgram* program;NativeScriptReceiverView view;
 Item* item;HSD_JObj* root;HSD_ClassInfo* root_class;
 uint8_t object_kind;
 uint64_t program_identity,layout_identity,parameter_identity;
} ArticleFeedbackSource;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool root_type(HSD_GObj* object,HSD_JObj* root,HSD_ClassInfo* expected)
{
 // The root comes from the live engine-owned GObj, never a script pointer.
 // HSD currently exposes class extent, but no allocation generation/span query.
 return root&&object->obj_kind==HSD_GObj_JObjKind&&HSD_CLASS_METHOD(root)==expected&&expected&&
  expected->head.obj_size>=(int)sizeof(*root)&&hsdObjIsDescendantOf((HSD_Obj*)root,HSD_CLASS_INFO(&hsdJObj));
}
static bool current(const ArticleFeedbackSource* source,NativeScriptDiagnostic* d)
{
 NativeScriptRuntimeContext* c=source->context;NativeScriptReceiverView view;
 if(nativeMatchPool_Resolve(source->reference)!=c->source||source->reference.lifetime!=c->scene_epoch||
    !nativeScriptBindings_SourceAlive(c)||nativeScriptItem_Status(c->source)!=NativeScriptVM_Ok||
    ((HSD_GObj*)c->source)->user_data!=source->item||nativeScriptItem_Spec(c->source)!=source->program||
    !source->program->bank||!source->program->bank->valid||source->program->identity!=source->program_identity||
    nativeScriptItem_Program(source->program->author_kind,source->program->owner)!=source->program||
    source->item->entity!=c->source||source->item->native_script_owner!=source->program->owner||
    source->item->kind!=source->program->kind||source->item->native_article_author_kind!=source->program->author_kind||
    source->program->article!=source->item->xC4_article_data||
    ((HSD_GObj*)c->source)->obj_kind!=source->object_kind||((HSD_GObj*)c->source)->hsd_obj!=source->root||
    (source->root&&!root_type(c->source,source->root,source->root_class))||
    !nativeScriptItem_View(c->source,c->generation,&view)||view.program!=source->view.program||
    view.program!=&source->program->bank->declarations||view.storage!=source->view.storage||!view.storage||
    !view.storage->live||view.storage->owner!=source->program->owner||view.storage->generation!=c->generation||
    view.storage->layout!=&source->program->bank->storage_layout||view.storage->layout->identity!=source->layout_identity||
    view.storage->parameters!=&source->program->bank->storage_parameters||view.storage->parameters->identity!=source->parameter_identity)
  return error(d,"Article feedback source, program, storage or runtime root retired");
 return true;
}
static bool capture(NativeScriptRuntimeContext* c,ArticleFeedbackSource* source,bool missing_root,NativeScriptDiagnostic* d)
{
 if(!c||c->source_class!=NativeCallbackSource_Item||!c->scene_epoch||
    !nativeMatchPool_Reference(&nativeGObjPool,c->source,&source->reference)||source->reference.lifetime!=c->scene_epoch||
    !nativeScriptBindings_SourceAlive(c)||nativeScriptItem_Status(c->source)!=NativeScriptVM_Ok)
  return error(d,"Article feedback requires a live source-owned Item");
 source->context=c;source->item=((HSD_GObj*)c->source)->user_data;source->program=nativeScriptItem_Spec(c->source);
 source->root=((HSD_GObj*)c->source)->hsd_obj;source->object_kind=((HSD_GObj*)c->source)->obj_kind;
 if(!source->item||!source->program||!source->program->bank||(!missing_root&&!source->root)||(source->root&&source->object_kind!=HSD_GObj_JObjKind)||
    !nativeScriptItem_View(c->source,c->generation,&source->view)||!source->view.storage||
    !source->view.storage->layout||!source->view.storage->parameters)return error(d,"Article feedback owned root/view is absent");
 source->root_class=source->root?HSD_CLASS_METHOD(source->root):NULL;source->program_identity=source->program->identity;
 source->layout_identity=source->view.storage->layout->identity;source->parameter_identity=source->view.storage->parameters->identity;
 return current(source,d);
}
static HSD_JObj* selected_joint(HSD_JObj* root,const NativeScriptItemModelSelector* s)
{
 HSD_JObj* node=root;
 for(unsigned step=0;node&&step<s->depth;++step){node=node->child;for(unsigned sibling=0;node&&sibling<s->path[step];++sibling)node=node->next;}
 return node;
}
static bool joint_effect(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 bool synchronous=(semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT;
 if((synchronous?count!=2:(count!=3&&count!=4))||!args||args[0].type!=NativeScriptType_I32||args[0].as.i64<INT32_MIN||args[0].as.i64>INT32_MAX||
    args[1].type!=NativeScriptType_Text)
  return error(d,"Item joint effect needs an exact Int effect and prepared path");
 if(!synchronous&&(args[2].type!=NativeScriptType_I32||
    (count==3?(args[2].as.i64!=0&&args[2].as.i64!=1&&args[2].as.i64!=3):
        (args[2].as.i64!=4||args[3].type!=NativeScriptType_F32||!isfinite(args[3].as.f32)))))
  return error(d,"Async Item joint effect needs mode0/1/3, or mode4 with a finite Float scalar");
 ArticleFeedbackSource source;if(!capture(context,&source,true,d))return false;
 const NativeScriptCompileBank* bank=source.program->bank;
 const NativeScriptItemModelSelector* selectors=source.program->model_selectors;
 uint32_t selector_count=source.program->model_selector_count;
 if(selector_count>NATIVE_SCRIPT_ITEM_MODEL_SELECTORS||(selector_count&&!selectors))return error(d,"Item joint effect selector table is invalid");
 unsigned low=0,high=selector_count;while(low<high){unsigned mid=low+(high-low)/2;if(selectors[mid].semantic<semantic)low=mid+1;else high=mid;}
 if(low==selector_count||selectors[low].semantic!=semantic)return error(d,"Item joint effect selector is not prepared for its exact owner");
 NativeScriptItemModelSelector selector=selectors[low];
 if(selector.model||selector.depth>12||selector.argument_count!=count||selector.channel!=(synchronous?0:args[2].as.i64)||
    selector.path_offset!=args[1].as.text.offset||selector.path_length!=args[1].as.text.length)
  return error(d,"Item joint effect literal path/mode differs from its immutable selector");
 int32_t id=(int32_t)args[0].as.i64;NativeScriptEffectArguments abi=nativeScriptEffectDispatch_Arguments(id);
 if(synchronous&&(id<0||id>UINT16_MAX||abi!=NativeScriptEffectArguments_Joint))
  return error(d,"Synchronous Item joint effect requires an exact native Joint-ABI effect ID");
 if(id>=0){
  uint32_t requirements=0;
  if(selector.channel==1||selector.channel==4){
   bool admitted=selector.channel==1?nativeScriptEffectDispatch_PositionRequirements(id,&requirements):
       nativeScriptEffectDispatch_PositionScalarRequirements(id,&requirements);
   if(!admitted||(requirements&NativeScriptEffectPosition_FighterParent))
    return error(d,"Async position effect recipe has an incompatible ABI or requires a Fighter parent");
  }else if(abi!=NativeScriptEffectArguments_Joint&&!(selector.channel==3&&abi==NativeScriptEffectArguments_JointScalar))
   return error(d,"Item joint effect ID has an incompatible native argument family");
 }
 HSD_JObj* node=selected_joint(source.root,&selector);HSD_ClassInfo* node_class=node?HSD_CLASS_METHOD(node):NULL;
 if(node&&(!node_class||node_class->head.obj_size<(int)sizeof(*node)||!hsdObjIsDescendantOf((HSD_Obj*)node,HSD_CLASS_INFO(&hsdJObj))))
  return error(d,"Item joint effect selected joint is not an engine JObj");
 if(!current(&source,d)||source.program->bank!=bank||source.program->model_selectors!=selectors||
    source.program->model_selector_count!=selector_count||memcmp(&selectors[low],&selector,sizeof(selector))||
    selected_joint(source.root,&selector)!=node||(node&&HSD_CLASS_METHOD(node)!=node_class))
  return error(d,"Item joint effect owned path changed before native dispatch");
 if(id>=0&&node){
  if(synchronous)efSync_Spawn(id,context->source,node);
  else if(selector.channel==4){float scalar=args[3].as.f32;efAsync_Spawn(context->source,&source.item->xBC0,4,id,node,&scalar);}
  else if(selector.channel==3)efAsync_Spawn(context->source,&source.item->xBC0,3,id,node,&source.item->facing_dir);
  else efAsync_Spawn(context->source,&source.item->xBC0,selector.channel,id,node);
 }
 if(!current(&source,d)||source.program->bank!=bank||source.program->model_selectors!=selectors||
    source.program->model_selector_count!=selector_count||memcmp(&selectors[low],&selector,sizeof(selector))||
    selected_joint(source.root,&selector)!=node||(node&&HSD_CLASS_METHOD(node)!=node_class))
  return error(d,"Item joint effect owned path changed during native dispatch");
 *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
static bool effect(const NativeScriptValue* value,NativeScriptEffectArguments abi,int32_t* result)
{
 uint64_t id;
 switch(value->type){
 case NativeScriptType_I32:case NativeScriptType_I64:if(value->as.i64<0)return false;id=(uint64_t)value->as.i64;break;
 case NativeScriptType_Bool:id=value->as.u64!=0;break;
 case NativeScriptType_U32:case NativeScriptType_U64:id=value->as.u64;break;
 default:return false;
 }
 if(id>UINT16_MAX||nativeScriptEffectDispatch_Arguments((int32_t)id)!=abi)return false;
 *result=(int32_t)id;return true;
}
bool nativeScriptBindingArticleFeedback_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
 const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 int32_t id=0;
 if(!out)return error(d,"Article feedback output is absent");
 if((semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_ASYNC_EFFECT||
    (semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT)
  return joint_effect(context,semantic,args,count,out,d);
 if(semantic==NativeScriptBinding_ItemSpawnEffectAtPosition||semantic==NativeScriptBinding_ItemSpawnWithScalarAtPosition){
  bool scalar=semantic==NativeScriptBinding_ItemSpawnWithScalarAtPosition;
  uint32_t requirements=0;NativeItemLifetime source;ArticleFeedbackSource rooted;
  if(count!=(scalar?5u:4u)||!args||!effect(&args[0],scalar?NativeScriptEffectArguments_PositionScalar:NativeScriptEffectArguments_Position,&id)||
     !(scalar?nativeScriptEffectDispatch_PositionScalarRequirements(id,&requirements):nativeScriptEffectDispatch_PositionRequirements(id,&requirements)))
   return error(d,"Position effect requires an integral exact ABI effect ID, XYZ and any declared scalar");
  if(requirements&NativeScriptEffectPosition_FighterParent)
   return error(d,"Position effect requires a Fighter parent and cannot be spawned by an Item");
  for(unsigned i=1;i<count;++i)if(args[i].type!=NativeScriptType_F32||!isfinite(args[i].as.f32))
   return error(d,"Position effect coordinates/scalar must be finite floats");
  if(!context||context->source_class!=NativeCallbackSource_Item||!context->scene_epoch||
     !nativeItemLifetime_Capture(context->source,&source)||!source.source_owned||
     source.reference.lifetime!=context->scene_epoch||source.generation!=context->generation||
     !nativeScriptBindings_SourceAlive(context))return error(d,"Position effect requires its live owned Item source");
  if((requirements&NativeScriptEffectPosition_Root)&&!capture(context,&rooted,false,d))return false;
  Vec3 position={args[1].as.f32,args[2].as.f32,args[3].as.f32};
  if(scalar){float value=args[4].as.f32;efSync_Spawn(id,context->source,&position,&value);}
  else efSync_Spawn(id,context->source,&position);
  if(!nativeItemLifetime_Current(&source)||!nativeScriptBindings_SourceAlive(context)||
     ((requirements&NativeScriptEffectPosition_Root)&&!current(&rooted,d)))
   return error(d,"Position effect retired or replaced its Item source");
  *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
 }
 if(semantic==NativeScriptBinding_ItemSpawnRootEffectWithOffset){
  if(count!=4||!args||!effect(&args[0],NativeScriptEffectArguments_JointOffset,&id))
   return error(d,"Root offset effect requires an integral Joint/Vec3-ABI effect ID and XYZ");
  for(unsigned i=1;i<4;++i)if(args[i].type!=NativeScriptType_F32||!isfinite(args[i].as.f32))
   return error(d,"Root effect offset coordinates must be finite floats");
  ArticleFeedbackSource source;if(!capture(context,&source,false,d))return false;
  Vec3 offset={args[1].as.f32,args[2].as.f32,args[3].as.f32};
  efSync_Spawn(id,context->source,source.root,&offset);
  if(!current(&source,d))return false;
  *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
 }
 if(semantic==NativeScriptBinding_ItemSpawnRootEffect){
  if(count!=1||!args||!effect(&args[0],NativeScriptEffectArguments_Joint,&id))return error(d,"Root effect requires an integral exact Joint-ABI effect ID");
 }else if(semantic==NativeScriptBinding_ItemDestroyEffects){if(count)return error(d,"Destroy effects takes no arguments");}
 else return error(d,"Unknown Article feedback operation");
 ArticleFeedbackSource source;if(!capture(context,&source,false,d))return false;
 if(semantic==NativeScriptBinding_ItemSpawnRootEffect)efSync_Spawn(id,context->source,source.root);
 else efLib_DestroyAll(context->source);
 if(!current(&source,d))return false;*out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
