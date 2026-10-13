#include "native_script_binding_article_reference.h"
#include "native_script_receiver_semantics.h"
#include "native_script_expression.h"
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool handle(const NativeScriptValue* value,uint32_t* out,NativeScriptDiagnostic* d)
{
 uint64_t bits;
 switch(value->type){
 case NativeScriptType_I32:case NativeScriptType_I64:
  if(value->as.i64<INT32_MIN||value->as.i64>UINT32_MAX)return error(d,"Article reference handle exceeds creation-ID width");bits=(uint32_t)value->as.i64;break;
 case NativeScriptType_U32:case NativeScriptType_U64:bits=value->as.u64;break;
 case NativeScriptType_Bool:bits=value->as.u64!=0;break;
 default:return error(d,"Article reference handle must be integral");
 }
 if(bits>UINT32_MAX)return error(d,"Article reference handle exceeds creation-ID width");*out=(uint32_t)bits;return true;
}
static bool null(const NativeScriptValue* value)
{return value->type==NativeScriptType_Object&&!value->as.object.identity&&!value->as.object.generation&&
 value->as.object.kind==(NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|NativeCallbackSource_Item);}
static bool retained(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
 const NativeScriptValue* value,uint32_t owner,const NativeScriptReceiverRecord** record,bool* exists,NativeScriptDiagnostic* d)
{
 if(!nativeScriptReceiver_Exists(scope,caller,value,exists,d))return false;
 if(null(value)){*record=NULL;return true;}
 if(value->type==NativeScriptType_Object&&!value->as.object.identity&&!value->as.object.generation)
  return error(d,"Article reference null token has a different source class");
 // Exists may report false for a stale record; a numeric/state/model projection
 // must still fault on that captured identity rather than silently default.
 return nativeScriptReceiver_ValidateArticle(scope,caller,value,owner,record,d);
}
static const NativeScriptProgramIndex* expected(const NativeScriptReceiverScope* scope)
{
 if(!scope||!scope->pool||!scope->pool->frames||scope->frame.slot+1!=scope->pool->depth||scope->frame.slot>=scope->pool->frame_capacity||
 scope->pool->frames[scope->frame.slot].serial!=scope->frame.serial)return NULL;
 return scope->pool->frames[scope->frame.slot].expected_program;
}
static bool owner_node(const NativeScriptProgramIndex* p,uint32_t owner,NativeScriptDiagnostic* d)
{
 return p&&p->tree&&p->tree->nodes&&p->tree->count<=p->tree->capacity&&owner&&owner<p->tree->count&&p->tree->nodes[owner].kind==NativeScriptSource_Article?
  true:error(d,"Article reference owner is absent from the exact caller program");
}
static bool index(const NativeScriptValue* value,uint32_t count,bool* inside,NativeScriptDiagnostic* d)
{
 uint64_t number;
 switch(value->type){
 case NativeScriptType_I32:case NativeScriptType_I64:if(value->as.i64<0){*inside=false;return true;}number=(uint64_t)value->as.i64;break;
 case NativeScriptType_U32:case NativeScriptType_U64:number=value->as.u64;break;
 case NativeScriptType_Bool:number=value->as.u64!=0;break;
 default:return error(d,"Article state index must be integral");
 }
 *inside=number<count;return true;
}
bool nativeScriptBindingArticleReference_Read(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
 uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 if(!out||!selectors||!count)return error(d,"Article reference read has no captured selector");
 uint32_t category=semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK,id=semantic&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK;
 bool query;if(!nativeScriptReceiver_Exists(scope,caller,&selectors[0],&query,d))return false;
 if(selectors[0].type==NativeScriptType_Object&&!selectors[0].as.object.identity&&!selectors[0].as.object.generation&&!null(&selectors[0]))
  return error(d,"Article reference null token has a different source class");
 const NativeScriptProgramIndex* p=expected(scope);
 if(category==NATIVE_SCRIPT_RECEIVER_ARTICLE_EXISTS){
  if(!owner_node(p,id,d))return false;
  if(count!=1)return error(d,"Article existence requires one captured object");bool exists=query;
  if(exists){const NativeScriptReceiverRecord* record;if(!nativeScriptReceiver_ValidateArticle(scope,caller,&selectors[0],id,&record,d))return false;}
  *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=exists};return true;
 }
 uint32_t owner=id;const NativeScriptValueDeclaration* field=NULL;
 bool physical=category==NATIVE_SCRIPT_RECEIVER_ARTICLE_PHYSICAL;
 uint32_t component=id&7;
 if(physical) {
  owner=id>>3;
  if(count!=1||component>NativeScriptArticlePhysical_NormalZ)return error(d,"Invalid Article physical vector projection");
 }
 if(category==NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE){
  if(!p||id>=p->value_count)return error(d,"Article state declaration exceeds the caller's exact program");field=&p->values[id];owner=field->owner;
  if(!owner||!p->tree||owner>=p->tree->count||p->tree->nodes[owner].kind!=NativeScriptSource_Article||field->parameter||
     (field->type!=NativeScriptType_Bool&&field->type!=NativeScriptType_I32&&field->type!=NativeScriptType_F32)||!field->count||count!=(field->array?2u:1u))
   return error(d,"Article state projection differs from its readonly declaration");
 }else if(!physical&&(category!=NATIVE_SCRIPT_RECEIVER_ARTICLE_OWNER||count!=1))return error(d,"Unsupported Article reference read");
 if(!owner_node(p,owner,d))return false;
 const NativeScriptReceiverRecord* record=NULL;
 if(!null(&selectors[0])&&!nativeScriptReceiver_ValidateArticle(scope,caller,&selectors[0],owner,&record,d))return false;
 NativeScriptValue result={.type=field?field->type:physical?NativeScriptType_F32:NativeScriptType_U32};
 if(field){
  bool inside=true;if(field->array&&!index(&selectors[1],field->count,&inside,d))return false;
  if(record&&inside){if(record->view.program!=p||!nativeScriptStorage_Read(record->view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|id,
       field->array?&selectors[1]:NULL,field->array?1:0,&result,d))return false;}
 }else if(physical&&record) {
  const Item* item=((HSD_GObj*)record->actor)->user_data;
  const Vec3* vector=&item->pos;
  if(component>=NativeScriptArticlePhysical_NormalX) {
   const NativeScriptItemProgram* program=nativeScriptItem_Spec(record->actor);
   if(!program||program->recipe)return error(d,"Article surface normal requires ordinary authored Article storage");
   vector=&item->xDD4_itemVar.native_article.surface_normal;component-=NativeScriptArticlePhysical_NormalX;
  }
  const float* value=component==0?&vector->x:component==1?&vector->y:&vector->z;
  memcpy(&result.as.f32,value,sizeof(*value));
 }else if(record){
  Item* item=((HSD_GObj*)record->actor)->user_data;HSD_GObj* owner_object=item->owner;
  if(!item->native_removal_in_progress&&owner_object){
   NativeMatchPoolReference reference;uint32_t generation;
   if(nativeMatchPool_Reference(&nativeGObjPool,owner_object,&reference)&&reference.lifetime==caller->scene_epoch&&
      nativeCallbackSource_Snapshot(owner_object,NativeCallbackSource_Fighter,&generation)){
    const Fighter* fighter=owner_object->user_data;
    if(fighter&&fighter->gobj==owner_object&&nativeMatchPool_Resolve(reference)==owner_object&&
       nativeCallbackSource_Alive(owner_object,generation,NativeCallbackSource_Fighter))result.as.u64=generation;
   }
  }
  const NativeScriptReceiverRecord* after;if(!nativeScriptReceiver_ValidateArticle(scope,caller,&selectors[0],owner,&after,d)||after!=record||((Item*)((HSD_GObj*)record->actor)->user_data)->owner!=owner_object)
   return error(d,"Article current owner changed during snapshot");
 }
 if(record){const NativeScriptReceiverRecord* after;if(!nativeScriptReceiver_ValidateArticle(scope,caller,&selectors[0],owner,&after,d)||after!=record)return false;}
 *out=result;return true;
}
static bool root_type(HSD_GObj* object,HSD_JObj* root,HSD_ClassInfo* info)
{return root&&object->obj_kind==HSD_GObj_JObjKind&&info&&HSD_CLASS_METHOD(root)==info&&info->head.obj_size>=(int)sizeof(*root)&&hsdObjIsDescendantOf((HSD_Obj*)root,HSD_CLASS_INFO(&hsdJObj));}
typedef struct ArticleBoneJoint {
 HSD_JObj* joint;
 HSD_JObj* parent;
 HSD_ClassInfo* info;
 int extent;
} ArticleBoneJoint;
static bool bone_position(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
 const NativeScriptValue* captured,uint32_t owner,const NativeScriptReceiverRecord* record,
 NativeScriptValue* args,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 HSD_GObj* gobj=record->actor;Item* item=gobj->user_data;
 const Article* article=item->xC4_article_data;const ItemModelDesc* model=article?article->x10_modelDesc:NULL;
 DynamicBoneTable* table=item->xBBC_dynamicBoneTable;int64_t index=args[2].as.i64;
 if(item->native_removal_in_progress||index<0||index>=100||!model||!table||
    (uint64_t)index>=model->x4_bone_count||!table->bones[index]) {
  *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return true;
 }
 if(model->x4_bone_count>100)return error(d,"Article bone table exceeds native capacity");
 const uint32_t count=model->x4_bone_count;HSD_JObj* selected=table->bones[index];
 HSD_JObj* root=gobj->hsd_obj;const uint8_t root_kind=gobj->obj_kind;
 ArticleBoneJoint joints[100];unsigned used=0;const NativeScriptReceiverRecord* after;
 for(HSD_JObj* joint=selected;joint;joint=joint->parent) {
  if(used==100)return error(d,"Article bone parent chain exceeds native capacity");
  for(unsigned i=0;i<used;++i)if(joints[i].joint==joint)return error(d,"Article bone parent chain contains a cycle");
  HSD_ClassInfo* info=HSD_CLASS_METHOD(joint);int extent=info?info->head.obj_size:0;
  if(!info||extent<(int)sizeof(*joint)||!hsdObjIsDescendantOf((HSD_Obj*)joint,HSD_CLASS_INFO(&hsdJObj)))
   return error(d,"Article bone parent chain contains an untyped native joint");
  if(!nativeScriptReceiver_ValidateArticle(scope,caller,captured,owner,&after,d)||after!=record||
     gobj->user_data!=item||gobj->hsd_obj!=root||gobj->obj_kind!=root_kind||item->native_removal_in_progress||
     item->xC4_article_data!=article||article->x10_modelDesc!=model||model->x4_bone_count!=count||
     item->xBBC_dynamicBoneTable!=table||table->bones[index]!=selected||
     HSD_CLASS_METHOD(joint)!=info||info->head.obj_size!=extent)
   return error(d,"Article bone identity changed during joint admission");
  joints[used++]=(ArticleBoneJoint){joint,joint->parent,info,extent};
 }
 Vec3 position={0};
 for(unsigned pass=0;pass<2;++pass) {
  if(!nativeScriptReceiver_ValidateArticle(scope,caller,captured,owner,&after,d)||after!=record||
     gobj->user_data!=item||gobj->hsd_obj!=root||gobj->obj_kind!=root_kind||item->native_removal_in_progress||
     item->xC4_article_data!=article||article->x10_modelDesc!=model||model->x4_bone_count!=count||
     item->xBBC_dynamicBoneTable!=table||table->bones[index]!=selected)
   return error(d,"Article bone identity or table changed during world-position query");
  for(unsigned i=0;i<used;++i)if(joints[i].joint->parent!=joints[i].parent||
      HSD_CLASS_METHOD(joints[i].joint)!=joints[i].info||joints[i].info->head.obj_size!=joints[i].extent)
   return error(d,"Article bone parent chain changed during world-position query");
  if(pass==0)lb_8000B1CC(selected,NULL,&position);
 }
 args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.x};
 args[4]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.y};
 args[5]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.z};
 *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
 return true;
}
bool nativeScriptBindingArticleReference_Invoke(const NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* caller,
 uint32_t semantic,const NativeScriptValue* selectors,uint32_t selector_count,NativeScriptValue* args,
 uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 (void)selectors;uint32_t category=semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK,owner=semantic&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK;
 bool bone=category==NATIVE_SCRIPT_RECEIVER_ARTICLE_BONE_POSITION;
 if(!out||selector_count||!args||!owner||(category!=NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT&&category!=NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION&&!bone)||
    count!=(category==NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT?2u:bone?6u:5u)||args[1].type!=NativeScriptType_I32)return error(d,"Article capture/model query differs from its compiled contract");
 if(category!=NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT)for(unsigned i=bone?3:2;i<count;++i)if(args[i].type!=NativeScriptType_F32)return error(d,"Article position outputs must be captured F32 destinations");
 if(bone&&(args[2].type!=NativeScriptType_I32||args[2].as.i64<INT32_MIN||args[2].as.i64>INT32_MAX))
  return error(d,"Article bone index requires a well-formed Int");
 uint32_t number;if(!handle(&args[0],&number,d))return false;NativeScriptValue object;
 if(!nativeScriptReceiver_CaptureArticle(scope,caller,number,owner,&object,d))return false;
 if(category==NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT){*out=object;return true;}
 const NativeScriptReceiverRecord* record;bool exists;if(!retained(scope,caller,&object,owner,&record,&exists,d))return false;
 if(!record){*out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return true;}
 if(bone)return bone_position(scope,caller,&object,owner,record,args,out,d);
 HSD_GObj* gobj=record->actor;Item* item=gobj->user_data;
 if(item->native_removal_in_progress||!gobj->hsd_obj||gobj->obj_kind!=HSD_GObj_JObjKind){*out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return true;}
 HSD_JObj* root=gobj->hsd_obj;HSD_ClassInfo* info=HSD_CLASS_METHOD(root);
 if(!root_type(gobj,root,info))return error(d,"Primary model is not an engine-typed JObj root");
 Vec3 position;HSD_JObjGetTranslation(root,&position);
 const NativeScriptReceiverRecord* after;
 if(!nativeScriptReceiver_ValidateArticle(scope,caller,&object,owner,&after,d)||after!=record||gobj->hsd_obj!=root||
    ((Item*)gobj->user_data)->native_removal_in_progress||!root_type(gobj,root,info))return error(d,"Primary model/root changed during translation read");
 args[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.x};args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.y};args[4]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.z};
 *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};return true;
}
