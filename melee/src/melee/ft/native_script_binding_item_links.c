#include "native_script_binding_item_links.h"
#include "native_script_bindings.h"
#include "native_script_actor_access.h"
#include "native_script_fighter.h"
#include "fighter.h"
#include <melee/it/native_item_lifetime.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

static bool creation_id(const NativeScriptValue* value,uint32_t* id)
{
    switch(value->type) {
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(value->as.i64<INT32_MIN||value->as.i64>UINT32_MAX)return false;
        *id=(uint32_t)value->as.i64;return true;
    case NativeScriptType_U32:case NativeScriptType_U64:
        if(value->as.u64>UINT32_MAX)return false;
        *id=(uint32_t)value->as.u64;return true;
    case NativeScriptType_Bool:*id=value->as.u64!=0;return true;
    default:return false;
    }
}
static bool slot(const NativeScriptValue* value,uint32_t* index)
{return creation_id(value,index)&&*index<4;}

// Creation IDs are unique within this scene. A link never follows an address
// from a previous allocation, or crosses a published fighter package bank.
static bool find(const NativeScriptCompileBank* bank,FighterKind author,
    uint32_t id,NativeItemLifetime* target)
{
    if(id==UINT32_MAX||!HSD_GObjPLinkHead)return false;
    uint32_t visited=0;
    for(Item_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
        object&&visited++<nativeGObjPool.capacity;object=object->next) {
        NativeMatchPoolReference reference;
        if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference)||
           !object->user_data || ((Item*)object->user_data)->x1C!=id)continue;
        NativeItemLifetime value;
        if(!nativeItemLifetime_Capture(object,&value)||!value.source_owned||value.removing||
           value.bank!=bank||value.data->native_article_author_kind!=author||
           !value.program||value.program->author_kind!=author||value.program->kind!=value.kind||
           value.program->article!=value.article||value.program->owner!=value.data->native_script_owner)return false;
        *target=value;return true;
    }
    return false;
}
static bool source(NativeScriptRuntimeContext* c,NativeItemLifetime* value,
    NativeScriptDiagnostic* d)
{
    // Destroyed callbacks retain their own storage and must be able to unlink
    // even after the native removal marker is set. New targets stay excluded.
    if(!c||c->source_class!=NativeCallbackSource_Item||!c->scene_epoch||
       !nativeItemLifetime_Capture(c->source,value)||value->reference.lifetime!=c->scene_epoch||
       value->generation!=c->generation||!value->source_owned||
       !value->program||value->program->article!=value->article||
       value->program->kind!=value->kind||value->program->owner!=value->data->native_script_owner||
       value->program->author_kind!=value->data->native_article_author_kind)
        return error(d,"Item links require their live exact owned source");
    return true;
}
static bool link_owned(NativeScriptRuntimeContext* c,const NativeScriptValue* args,
    NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    uint32_t source_id,target_id,index;
    if(!creation_id(&args[0],&source_id)||!slot(&args[1],&index)||!creation_id(&args[2],&target_id))
        return error(d,"Owned links require exact 32-bit creation IDs and a slot from 0 to 3");
    NativeMatchPoolReference reference;NativeScriptReceiverView view;
    if(!c||c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference)||reference.lifetime!=c->scene_epoch||
       !nativeScriptBindings_SourceAlive(c)||nativeScriptFighter_Status(c->source)!=NativeScriptVM_Ok||
       !nativeScriptActors_Lookup(NULL,c->source,c->source_class,c->generation,&view))
        return error(d,"Owned links require a live published Fighter caller");
    Fighter* fighter=((HSD_GObj*)c->source)->user_data;
    const NativeScriptFighterProgram* program=nativeScriptFighter_Program(fighter->kind);
    const NativeScriptCompileBank* bank=program?program->bank:NULL;
    if(!bank||!bank->valid||view.program!=&bank->declarations||!view.storage||!view.storage->live||
       view.storage->owner||view.storage->generation!=c->generation||
       view.storage->layout!=&bank->storage_layout||view.storage->parameters!=&bank->storage_parameters)
        return error(d,"Owned link caller differs from its published program/storage");
    NativeItemLifetime from,to,current;bool linked=false;
    if(source_id!=target_id&&find(bank,fighter->kind,source_id,&from)&&find(bank,fighter->kind,target_id,&to)&&
       from.data->owner==c->source&&to.data->owner==c->source&&
       nativeScriptItem_AttachmentOwner(from.object)==c->source&&nativeScriptItem_AttachmentOwner(to.object)==c->source&&
       (!find(bank,fighter->kind,from.data->native_article_links[index],&current)||current.object==to.object)) {
        from.data->native_article_links[index]=target_id;linked=true;
    }
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=linked};return true;
}
bool nativeScriptBindingItemLinks_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool owned=semantic==NativeScriptBinding_ArticlesLinkOwned;
    bool reciprocal=semantic==NativeScriptBinding_ItemLinkReciprocal;
    bool unlink=semantic==NativeScriptBinding_ItemUnlink;
    bool copy=semantic==NativeScriptBinding_ItemCopyLinkCollision;
    if((!owned&&!reciprocal&&!unlink&&!copy&&semantic!=NativeScriptBinding_ItemLinkId)||!args||!out||
       count!=(owned?3u:reciprocal?2u:1u))return error(d,"Invalid item link operation/signature");
    if(owned)return link_owned(c,args,out,d);
    uint32_t index,backlink=0;
    if(!slot(&args[0],&index)||(reciprocal&&!slot(&args[1],&backlink)))
        return error(d,"Item link slots must be integral values from 0 to 3");
    NativeItemLifetime from,to,back;
    if(!source(c,&from,d))return false;
    bool present=find(from.bank,from.data->native_article_author_kind,from.data->native_article_links[index],&to);
    NativeScriptValue result;
    if(copy) {
        if(from.removing)return error(d,"Collision copy requires a non-removing source Item");
        // Copy the complete native collision record, including its current
        // contacts and previous sweep state. No collision query or callback.
        if(present)from.data->x378_itemColl=to.data->x378_itemColl;
        result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=present};
    } else if(unlink) {
        // Remove every reciprocal edge, not just an assumed matching slot.
        // The target is retained and no callbacks run during this mutation.
        if(present)for(uint32_t i=0;i<4;++i)
            if(to.data->native_article_links[i]==from.generation)to.data->native_article_links[i]=UINT32_MAX;
        from.data->native_article_links[index]=UINT32_MAX;
        result=(NativeScriptValue){.type=NativeScriptType_Void};
    } else if(reciprocal) {
        bool matched=present&&find(from.bank,from.data->native_article_author_kind,to.data->native_article_links[backlink],&back)&&
            back.object==from.object;
        result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=matched};
    } else {
        uint32_t id=present?to.generation:UINT32_MAX;
        // Keep all creation-ID bits while returning the language's signed Int.
        int64_t signed_id=id<=INT32_MAX?(int64_t)id:(int64_t)id-INT64_C(0x100000000);
        result=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=signed_id};
    }
    *out=result;return true;
}
