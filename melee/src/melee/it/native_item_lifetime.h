#ifndef MELEE_IT_NATIVE_ITEM_LIFETIME_H
#define MELEE_IT_NATIVE_ITEM_LIFETIME_H
#include "native_script_item_actor.h"
#include "types.h"
#include <melee/ft/native_callback_lifetime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

// Retained actual Item identity across synchronous native callbacks. Ordinary
// state/owner changes remain gameplay mutations, not actor retirement.
typedef struct NativeItemLifetime {
    Item_GObj* object;
    Item* data;
    NativeMatchPoolReference reference;
    const Article* article;
    const ItemAttr* attributes;
    const void* model;
    const NativeScriptItemProgram* program;
    const NativeScriptCompileBank* bank;
    NativeScriptReceiverView view;
    uint64_t program_identity,layout_identity,parameter_identity;
    uint32_t generation;
    ItemKind kind;
    u8 object_kind;
    bool source_owned,removing;
} NativeItemLifetime;
static inline bool nativeItemLifetime_Current(const NativeItemLifetime* s)
{
    Item_GObj* object=s->object;
    if(nativeMatchPool_Resolve(s->reference)!=object||s->reference.lifetime!=nativeGObjPool.lifetime||
       object->classifier!=HSD_GOBJ_CLASS_ITEM||object->user_data!=s->data||
       object->obj_kind!=s->object_kind||object->hsd_obj!=s->model||
       !nativeCallbackSource_Alive(object,s->generation,NativeCallbackSource_Item)||
       s->data->entity!=object||s->data->kind!=s->kind||s->data->xC4_article_data!=s->article||
       s->data->xCC_item_attr!=s->attributes||s->data->native_removal_in_progress!=s->removing||
       nativeScriptItem_IsAttached(object)!=s->source_owned)return false;
    if(!s->source_owned)return s->data->native_script_owner==0;
    NativeScriptReceiverView view;
    return nativeScriptItem_Status(object)==NativeScriptVM_Ok&&nativeScriptItem_Spec(object)==s->program&&
        s->program->identity==s->program_identity&&s->program->bank==s->bank&&s->bank&&s->bank->valid&&
        nativeScriptItem_View(object,s->generation,&view)&&view.program==&s->bank->declarations&&
        view.program==s->view.program&&view.storage==s->view.storage&&view.storage&&view.storage->live&&
        view.storage->owner==s->program->owner&&view.storage->generation==s->generation&&
        view.storage->layout==&s->bank->storage_layout&&view.storage->layout->identity==s->layout_identity&&
        view.storage->parameters==&s->bank->storage_parameters&&view.storage->parameters->identity==s->parameter_identity;
}
static inline bool nativeItemLifetime_Capture(Item_GObj* object,NativeItemLifetime* output)
{
    if(!output)return false;NativeItemLifetime value={.object=object};
    if(!nativeMatchPool_Reference(&nativeGObjPool,object,&value.reference)||object->classifier!=HSD_GOBJ_CLASS_ITEM||
       !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&value.generation))return false;
    value.data=object->user_data;value.kind=value.data->kind;value.article=value.data->xC4_article_data;
    value.attributes=value.data->xCC_item_attr;value.model=object->hsd_obj;value.object_kind=object->obj_kind;
    value.removing=value.data->native_removal_in_progress;value.source_owned=nativeScriptItem_IsAttached(object);
    if(value.source_owned){
        value.program=nativeScriptItem_Spec(object);
        if(!value.program||!value.program->bank||!nativeScriptItem_View(object,value.generation,&value.view)||
           !value.view.storage||!value.view.storage->layout||!value.view.storage->parameters)return false;
        value.bank=value.program->bank;value.program_identity=value.program->identity;
        value.layout_identity=value.view.storage->layout->identity;value.parameter_identity=value.view.storage->parameters->identity;
    }
    if(!nativeItemLifetime_Current(&value))return false;*output=value;return true;
}
#endif
