#include "native_script_binding_signal.h"
#include "native_script_binding_signal_resolve.h"
#include "native_script_actor_access.h"
#include "native_script_fighter.h"
#include "native_callback_lifetime.h"
#include "fighter.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <math.h>
#include <stdio.h>

typedef struct Caller {
 NativeScriptRuntimeContext* context;NativeMatchPoolReference reference;
 const NativeScriptCompileBank* bank;NativeScriptReceiverView view;
 const void* data;
 uint64_t layout,parameters;
} Caller;
typedef struct Target {
 Item_GObj* object;NativeMatchPoolReference reference;uint32_t generation;
 const NativeScriptItemProgram* program;NativeScriptReceiverView view;
} Target;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool alive(const Caller* caller)
{
 NativeScriptRuntimeContext* c=caller->context;NativeScriptReceiverView view;
 if(nativeMatchPool_Resolve(caller->reference)!=c->source||caller->reference.lifetime!=c->scene_epoch||
    !nativeScriptBindings_SourceAlive(c)||((HSD_GObj*)c->source)->user_data!=caller->data||!caller->bank->valid)return false;
 if(c->source_class==NativeCallbackSource_Fighter){
  if(nativeScriptFighter_Status(c->source)!=NativeScriptVM_Ok)return false;
  const NativeScriptFighterProgram* p=nativeScriptFighter_Program(((Fighter*)((HSD_GObj*)c->source)->user_data)->kind);
  if(!p||p->bank!=caller->bank)return false;
 }else{const NativeScriptItemProgram* p=nativeScriptItem_Spec(c->source);
  if(nativeScriptItem_Status(c->source)!=NativeScriptVM_Ok||!p||p->bank!=caller->bank||p->owner!=caller->view.storage->owner)return false;}
 return nativeScriptActors_Lookup(NULL,c->source,c->source_class,c->generation,&view)&&
  view.program==&caller->bank->declarations&&view.program==caller->view.program&&view.storage==caller->view.storage&&view.storage&&
  view.storage->live&&view.storage->generation==c->generation&&view.storage->layout==&caller->bank->storage_layout&&
  view.storage->layout->identity==caller->layout&&view.storage->parameters==&caller->bank->storage_parameters&&view.storage->parameters->identity==caller->parameters;
}
static bool capture(NativeScriptRuntimeContext* c,Caller* caller,NativeScriptDiagnostic* d)
{
 if(!c||!c->scene_epoch||(c->source_class!=NativeCallbackSource_Fighter&&c->source_class!=NativeCallbackSource_Item)||
    !nativeMatchPool_Reference(&nativeGObjPool,c->source,&caller->reference)||caller->reference.lifetime!=c->scene_epoch||
    !nativeScriptBindings_SourceAlive(c)||!nativeScriptActors_Lookup(NULL,c->source,c->source_class,c->generation,&caller->view)||
    !caller->view.program||!caller->view.storage||!caller->view.storage->layout||!caller->view.storage->parameters)
  return error(d,"Signal caller has no live owned program/storage in its scene");
 caller->context=c;
 caller->data=((HSD_GObj*)c->source)->user_data;
 if(c->source_class==NativeCallbackSource_Fighter){const NativeScriptFighterProgram* p=nativeScriptFighter_Program(((Fighter*)((HSD_GObj*)c->source)->user_data)->kind);caller->bank=p?p->bank:NULL;}
 else{const NativeScriptItemProgram* p=nativeScriptItem_Spec(c->source);caller->bank=p?p->bank:NULL;}
 if(!caller->bank)return error(d,"Signal caller program is absent");
 caller->layout=caller->view.storage->layout->identity;caller->parameters=caller->view.storage->parameters->identity;
 return alive(caller)?true:error(d,"Signal caller retired/faulted during capture");
}
static bool function(const NativeScriptCompileBank* bank,uint32_t id,uint32_t* owner,NativeScriptDiagnostic* d)
{
 const NativeScriptProgramIndex* p=&bank->declarations;
 if(id>=p->function_count)return error(d,"Signal code ID exceeds its exact caller bank");
 const NativeScriptFunctionDeclaration* f=&p->functions[id];
 if(!f->callback||!f->signature_resolved||!f->owner||f->owner>=bank->tree.count||
    bank->tree.nodes[f->owner].kind!=NativeScriptSource_Article||f->result_type!=NativeScriptType_Void||f->parameter_count!=1||
    f->parameter_first>=p->parameter_count||p->parameters[f->parameter_first].type!=NativeScriptType_F32)
  return error(d,"Signal code ID differs from the owned Article Void/one-F32 ABI");
 *owner=f->owner;return true;
}
static bool target(const Caller* caller,Item_GObj* object,uint32_t owner,Target* output)
{
 Target t={.object=object};
 if(!object||!nativeMatchPool_Reference(&nativeGObjPool,object,&t.reference)||t.reference.lifetime!=caller->reference.lifetime||
    !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&t.generation))return false;
 t.program=nativeScriptItem_Spec(object);
 if(!t.program||t.program->bank!=caller->bank||t.program->owner!=owner||
    !nativeScriptItem_View(object,t.generation,&t.view)||t.view.program!=&caller->bank->declarations||!t.view.storage||
    !t.view.storage->live||t.view.storage->owner!=owner||t.view.storage->generation!=t.generation||
    t.view.storage->layout!=&caller->bank->storage_layout||t.view.storage->parameters!=&caller->bank->storage_parameters)return false;
 *output=t;return true;
}
static bool handle(const NativeScriptValue* value,uint32_t* output,NativeScriptDiagnostic* d)
{
 uint64_t bits;
 switch(value->type){
 case NativeScriptType_I32:case NativeScriptType_I64:
  // The old Int handle was passed as uint32_t; retain all signed32 bit patterns,
  // including -1's explicit absent sentinel, without truncating wider values.
  if(value->as.i64<INT32_MIN||value->as.i64>UINT32_MAX)return error(d,"Signal handle exceeds its exact creation-ID width");
  bits=(uint32_t)value->as.i64;break;
 case NativeScriptType_U32:case NativeScriptType_U64:bits=value->as.u64;break;
 case NativeScriptType_Bool:bits=value->as.u64!=0;break;
 default:return error(d,"Signal handle must be integral");
 }
 if(bits>UINT32_MAX)return error(d,"Signal handle exceeds its exact creation-ID width");*output=(uint32_t)bits;return true;
}
static bool signal_id(const Target* t,uint32_t function,uint32_t* id,NativeScriptDiagnostic* d)
{
 if(t->program->signal_count>NATIVE_SCRIPT_ITEM_SIGNALS||(t->program->signal_count&&!t->program->signals))return error(d,"Prepared signal table is invalid");
 bool found=false;
 for(uint32_t i=0;i<t->program->signal_count;++i)if(t->program->signals[i].function==function){
  if(found)return error(d,"Prepared signal code ID is ambiguous");*id=t->program->signals[i].signal_id;found=true;}
 return found?true:error(d,"Signal code ID is not in the recipient's prepared table");
}
bool nativeScriptBindingSignal_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,
 uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 if(!out||(semantic&NATIVE_SCRIPT_SIGNAL_DOMAIN_MASK)!=NATIVE_SCRIPT_SIGNAL_DOMAIN)return error(d,"Signal semantic/output is invalid");
 uint32_t op=(semantic&NATIVE_SCRIPT_SIGNAL_OPERATION_MASK)>>20,code=semantic&NATIVE_SCRIPT_SIGNAL_FUNCTION_MASK;
 bool self=op==NativeScriptSignal_Self;
 bool owned=op==NativeScriptSignal_Owned;
 if((!self&&!owned&&op!=NativeScriptSignal_Target)||count!=(self?2u:4u)||!args||args[self?0:2].type!=NativeScriptType_Text||
    args[self?1:3].type!=NativeScriptType_F32||
    (!self&&args[1].type!=NativeScriptType_I32))return error(d,"Signal arguments differ from their compiled signature");
 if(!c||(self&&c->source_class!=NativeCallbackSource_Item)||(owned&&c->source_class!=NativeCallbackSource_Fighter)||
    (!self&&!owned&&c->source_class!=NativeCallbackSource_Fighter&&c->source_class!=NativeCallbackSource_Item))return error(d,"Signal operation has the wrong source context");
 uint32_t number=0;if(!self&&!handle(&args[0],&number,d))return false;
 Caller caller;if(!capture(c,&caller,d))return false;
 const NativeScriptTextValue* name=&args[self?0:2].as.text;
 if(!caller.bank->code.source||caller.bank->code.source!=caller.bank->tree.source||!name->length||
    name->offset>caller.bank->code.source_bytes||name->length>caller.bank->code.source_bytes-name->offset)
  return error(d,"Signal literal text span exceeds its exact caller source");
 uint32_t owner;if(!function(caller.bank,code,&owner,d))return false;
 // Generic legacy target dispatch rejects nonfinite payloads as undelivered.
 // Local item.signal historically called the authored handler directly.
 if(!self&&!isfinite(args[3].as.f32)){if(!alive(&caller))return error(d,"Signal caller retired before payload rejection");*out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return true;}
 Target t={0};
 if(self){if(owner!=caller.view.storage->owner||!target(&caller,c->source,owner,&t))return error(d,"Self signal belongs to a different Article owner");}
 else if(number!=UINT32_MAX&&HSD_GObjPLinkHead){
  uint32_t visited=0;
  for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];object&&visited++<nativeGObjPool.capacity;object=object->next){
   if(object->user_data&&((Item*)object->user_data)->x1C==number&&
       !((Item*)object->user_data)->native_removal_in_progress&&
       (!owned || (((Item*)object->user_data)->owner==c->source &&
                   nativeScriptItem_AttachmentOwner(object)==c->source))&&
       target(&caller,object,owner,&t))break;
  }
 }
 if(!t.object){if(!alive(&caller))return error(d,"Signal caller retired before absence publication");*out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};return true;}
 uint32_t id;if(!signal_id(&t,code,&id,d))return false;
 NativeScriptVMStatus before=nativeScriptItem_Status(t.object);
 if(before!=NativeScriptVM_Ok)return error(d,before==NativeScriptVM_Error?"Signal recipient is faulted":"Signal recipient retired before dispatch");
 NativeScriptValue result={0};NativeScriptVMStatus status=nativeScriptItem_RunSignal(t.object,id,args[self?1:3].as.f32,&result);
 if(!alive(&caller))return error(d,"Signal caller retired/faulted during dispatch");
 if(status==NativeScriptVM_Error)return error(d,"Authored signal failed in its recipient");
 // Generic legacy dispatch reported true after entering the callback even if it
 // removed the recipient. Never reacquire/read a reused target allocation here.
 if(status!=NativeScriptVM_Ok&&status!=NativeScriptVM_SourceRetired)return error(d,"Unknown signal execution status");
 *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};return true;
}
