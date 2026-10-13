#include "native_script_binding_held_items.h"
#include "native_script_bindings.h"
#include "native_callback_lifetime.h"
#include "ftlib.h"
#include "types.h"
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <string.h>

typedef struct HeldSource {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    Fighter* fighter;
    uint32_t kind;
} HeldSource;
typedef struct HeldSlot {
    Item_GObj* object;
    NativeMatchPoolReference reference;
    Item* item;
    uint32_t generation,kind;
} HeldSlot;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

// Bound and validate the actual registry prefix before native lifetime helpers
// inspect its data. An unlinked slot remains the audited missing-item result.
static bool registered(HSD_GObj* wanted,unsigned link,unsigned classifier,uint64_t epoch,
                       bool* found,NativeScriptDiagnostic* d)
{
    *found=false;
    if(!HSD_GObjPLinkHead)return true;
    uint32_t visited=0;
    for(HSD_GObj* node=HSD_GObjPLinkHead[link];node;node=node->next){
        NativeMatchPoolReference reference;
        if(visited++>=nativeGObjPool.capacity||!nativeMatchPool_Reference(&nativeGObjPool,node,&reference)||
           reference.lifetime!=epoch||node->classifier!=classifier)
            return error(d,"Held query registry is cyclic or contains an invalid physical node");
        if(node==wanted){*found=true;return true;}
    }
    return true;
}
static bool source_current(const HeldSource* saved,NativeScriptDiagnostic* d)
{
    HSD_GObj* object=saved->context.source;bool found;
    if(nativeMatchPool_Resolve(saved->reference)!=object||saved->reference.lifetime!=saved->context.scene_epoch||
       nativeGObjPool.lifetime!=saved->context.scene_epoch||object->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       object->user_data!=saved->fighter||!registered(object,HSD_GOBJ_PLINK_FIGHTER,HSD_GOBJ_CLASS_FIGHTER,saved->context.scene_epoch,&found,d)||
       !found||!nativeCallbackSource_Alive(object,saved->context.generation,NativeCallbackSource_Fighter)||
       saved->fighter->gobj!=object||(uint32_t)saved->fighter->kind!=saved->kind)
        return error(d,"Held query Fighter identity retired before publication");
    return true;
}
static bool source_capture(NativeScriptRuntimeContext* context,HeldSource* saved,NativeScriptDiagnostic* d)
{
    if(!context||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&saved->reference)||
       saved->reference.lifetime!=context->scene_epoch)return error(d,"Held query requires a physical Fighter in its pinned scene");
    HSD_GObj* object=context->source;bool found;
    if(object->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !registered(object,HSD_GOBJ_PLINK_FIGHTER,HSD_GOBJ_CLASS_FIGHTER,context->scene_epoch,&found,d)||!found||
       !object->user_data||!nativeCallbackSource_Alive(object,context->generation,NativeCallbackSource_Fighter))
        return error(d,"Held query source has no live Fighter identity");
    saved->context=*context;saved->fighter=object->user_data;saved->kind=(uint32_t)saved->fighter->kind;
    return source_current(saved,d);
}
static bool slot_current(const HeldSource* source,const HeldSlot* saved,NativeScriptDiagnostic* d)
{
    bool found;
    if(nativeMatchPool_Resolve(saved->reference)!=saved->object||saved->reference.lifetime!=source->context.scene_epoch||
       saved->object->classifier!=HSD_GOBJ_CLASS_ITEM||saved->object->user_data!=saved->item||
       !registered(saved->object,HSD_GOBJ_PLINK_ITEM,HSD_GOBJ_CLASS_ITEM,source->context.scene_epoch,&found,d)||!found||
       !nativeCallbackSource_Alive(saved->object,saved->generation,NativeCallbackSource_Item)||
       saved->item->entity!=saved->object||(uint32_t)saved->item->kind!=saved->kind||saved->item->native_removal_in_progress||
       source->fighter->item_gobj!=saved->object)
        return error(d,"Captured held Item identity or Fighter slot changed before publication");
    return true;
}
bool nativeScriptBindingHeldItems_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    if(!output||(semantic!=NativeScriptBinding_ItemsHeldId&&semantic!=NativeScriptBinding_ItemsHeldKind&&
       semantic!=NativeScriptBinding_ItemsHeldMotion&&semantic!=NativeScriptBinding_ItemsHeldFrame))
        return error(d,"Unsupported held Item observation");
    HeldSource source;
    if(!source_capture(context,&source,d))return false;
    Item_GObj* object=ftLib_GetItem(source.context.source);
    if(!source_current(&source,d)||source.fighter->item_gobj!=object)
        return error(d,"Fighter held slot changed during native selection");
    NativeScriptValue result=semantic==NativeScriptBinding_ItemsHeldFrame?
        (NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0}:
        (NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1};
    NativeMatchPoolReference reference;bool found=false;
    if(object&&nativeMatchPool_Reference(&nativeGObjPool,object,&reference)&&reference.lifetime==source.context.scene_epoch){
        if(object->classifier!=HSD_GOBJ_CLASS_ITEM)return error(d,"Held Fighter slot contains a live non-Item object");
        if(!registered(object,HSD_GOBJ_PLINK_ITEM,HSD_GOBJ_CLASS_ITEM,source.context.scene_epoch,&found,d))return false;
        if(found){
            uint32_t generation;
            if(!object->user_data||!nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&generation))
                return error(d,"Registered held Item data is invalid");
            Item* item=object->user_data;
            if(item->entity!=object)return error(d,"Registered held Item entity identity is invalid");
            if(!item->native_removal_in_progress){
                HeldSlot slot={.object=object,.reference=reference,.item=item,.generation=generation,.kind=(uint32_t)item->kind};
                if(semantic==NativeScriptBinding_ItemsHeldFrame)result.as.f32=item->x5CC_currentAnimFrame;
                else if(semantic==NativeScriptBinding_ItemsHeldMotion)result.as.i64=(int32_t)item->msid;
                else if(semantic==NativeScriptBinding_ItemsHeldKind)result.as.i64=(int32_t)item->kind;
                else {int32_t bits;memcpy(&bits,&generation,sizeof(bits));result.as.i64=bits;}
                if(!slot_current(&source,&slot,d))return false;
            }
        }
    }
    if(!source_current(&source,d)||source.fighter->item_gobj!=object)
        return error(d,"Held query source or slot changed before publication");
    *output=result;return true;
}
