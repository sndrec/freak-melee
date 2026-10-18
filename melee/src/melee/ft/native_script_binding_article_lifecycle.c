#include "native_script_binding_article_lifecycle.h"
#include "native_script_actor_access.h"
#include "native_script_fighter.h"
#include "native_source_package.h"
#include "ftparts.h"
#include "types.h"
#include <melee/it/item.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>

typedef struct Actor {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    const void* data;
    const NativeScriptCompileBank* bank;
    const NativeScriptItemProgram* item;
    const NativeScriptFighterProgram* fighter;
    NativeScriptReceiverView view;
    uint64_t layout,parameters;
} Actor;

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

static bool live(const Actor* a)
{
    HSD_GObj* object=a->context.source;
    NativeScriptReceiverView view;
    NativeScriptRuntimeContext context=a->context;
    if(nativeMatchPool_Resolve(a->reference)!=object ||
       a->reference.lifetime!=context.scene_epoch || object->user_data!=a->data ||
       !nativeScriptBindings_SourceAlive(&context) || !a->bank->valid)return false;
    if(context.source_class==NativeCallbackSource_Item) {
        if(nativeScriptItem_Status(object)!=NativeScriptVM_Ok ||
           nativeScriptItem_Spec(object)!=a->item || a->item->bank!=a->bank)return false;
    } else if(nativeScriptFighter_Program(((const Fighter*)a->data)->kind)!=a->fighter ||
              a->fighter->bank!=a->bank ||
              !nativeScriptBindings_SourceFighterLayout(&context,NULL))return false;
    return nativeScriptActors_Lookup(NULL,object,context.source_class,context.generation,&view) &&
        view.program==&a->bank->declarations && view.program==a->view.program &&
        view.storage==a->view.storage && view.storage && view.storage->live &&
        view.storage->owner==a->view.storage->owner &&
        view.storage->generation==context.generation &&
        view.storage->layout==&a->bank->storage_layout && view.storage->layout->identity==a->layout &&
        view.storage->parameters==&a->bank->storage_parameters &&
        view.storage->parameters->identity==a->parameters &&
        (a->item?view.storage->owner==a->item->owner:view.storage->owner==0);
}

static bool capture(NativeScriptRuntimeContext* c,Actor* a,NativeScriptDiagnostic* d)
{
    if(!c || !c->scene_epoch ||
       (c->source_class!=NativeCallbackSource_Fighter && c->source_class!=NativeCallbackSource_Item) ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&a->reference) ||
       a->reference.lifetime!=c->scene_epoch || !nativeScriptBindings_SourceAlive(c))
        return error(d,"Article lifecycle source has no live physical identity in its scene");
    a->context=*c;a->data=((HSD_GObj*)c->source)->user_data;
    if(c->source_class==NativeCallbackSource_Item) {
        a->item=nativeScriptItem_Spec(c->source);a->fighter=NULL;
        a->bank=a->item?a->item->bank:NULL;
    } else {
        a->fighter=nativeScriptFighter_Program(((const Fighter*)a->data)->kind);a->item=NULL;
        a->bank=a->fighter?a->fighter->bank:NULL;
    }
    if(!a->bank || !nativeScriptActors_Lookup(NULL,c->source,c->source_class,c->generation,&a->view) ||
       !a->view.storage || !a->view.storage->layout || !a->view.storage->parameters)
        return error(d,"Article lifecycle source has no selected owned program/storage");
    a->layout=a->view.storage->layout->identity;a->parameters=a->view.storage->parameters->identity;
    return live(a)?true:error(d,"Article lifecycle source is retired, faulted or has changed schema");
}

static bool integral(const NativeScriptValue* v,int64_t* number,NativeScriptDiagnostic* d)
{
    switch(v->type) {
    case NativeScriptType_Bool:*number=v->as.u64!=0;return true;
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(v->as.i64<INT32_MIN || v->as.i64>UINT32_MAX)break;
        *number=v->as.i64;return true;
    case NativeScriptType_U32:case NativeScriptType_U64:
        if(v->as.u64>UINT32_MAX)break;
        *number=(int64_t)v->as.u64;return true;
    default:return error(d,"Article lifecycle selector must be integral");
    }
    return error(d,"Article lifecycle selector exceeds native 32-bit width");
}

static bool attach(Actor* caller,const NativeScriptValue* args,uint32_t count,NativeScriptDiagnostic* d)
{
    int64_t raw;
    if(!integral(&args[0],&raw,d))return false;
    if(count==2 && args[1].type!=NativeScriptType_Bool)
        return error(d,"Article attachment registration flag must be Bool");
    Item* item=(Item*)caller->data;
    Fighter_GObj* owner=nativeScriptItem_AttachmentOwner(caller->context.source);
    if(!owner || item->owner!=owner)return true;
    uint32_t generation;
    if(!nativeCallbackSource_Snapshot(owner,NativeCallbackSource_Fighter,&generation))
        return error(d,"Article attachment owner retired during selection");
    NativeScriptRuntimeContext context={.source=owner,.source_class=NativeCallbackSource_Fighter,
        .generation=generation,.actor_id=generation,.scene_epoch=caller->context.scene_epoch};
    Actor recipient={0};
    if(!capture(&context,&recipient,d))return false;
    if(recipient.bank!=caller->bank)return error(d,"Article attachment owner belongs to a different source program");
    const Fighter* fighter=recipient.data;
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
    if(!package || !fighter->parts || !ftPartsTable ||
       ftPartsTable[fighter->kind]!=&package->assets.parts ||
       package->assets.parts.parts_num!=package->assets.facts.joints+package->assets.facts.omission_count ||
       !package->assets.parts.parts_num || package->assets.parts.parts_num>MAX_FT_PARTS)
        return error(d,"Article attachment owner has no exact imported rig");
    raw=nativeFighterRig_Resolve(&package->assets.rig,raw);
    if(raw<0 || (uint64_t)raw>=package->assets.parts.parts_num)return true;
    if(!fighter->parts[raw].joint || !caller->context.source ||
       !((HSD_GObj*)caller->context.source)->hsd_obj || !item->xC4_article_data ||
       item->xC4_article_data!=caller->item->article ||
       !item->xC4_article_data->x10_modelDesc)
        return error(d,"Article attachment model or selected owner joint is absent");
    Item_8026AB54(caller->context.source,owner,(Fighter_Part)raw);
    if(!live(caller) || !live(&recipient))return error(d,"Article or attachment owner retired/faulted during pickup");
    if(count==2 && args[1].as.u64)((Fighter*)recipient.data)->x1984_heldItemSpec=caller->context.source;
    return true;
}

typedef struct PhysicalIdentity {
    HSD_GObj* object;
    NativeMatchPoolReference reference;
    const void* data;
    uint32_t generation,source_class,kind;
} PhysicalIdentity;

static bool physical_current(const PhysicalIdentity* target,uint64_t epoch)
{
    if(!target->object)return true;
    HSD_GObj* object=target->object;
    const unsigned classes[]={HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_CLASS_EFFECT};
    if(target->source_class>NativeCallbackSource_ModelEffect ||
       nativeMatchPool_Resolve(target->reference)!=object || target->reference.lifetime!=epoch ||
       object->classifier!=classes[target->source_class] || object->user_data!=target->data ||
       !nativeCallbackSource_Alive(object,target->generation,target->source_class))return false;
    if(target->source_class==NativeCallbackSource_Fighter) {
        const Fighter* fighter=target->data;
        return fighter->gobj==object && (uint32_t)fighter->kind==target->kind;
    }
    if(target->source_class==NativeCallbackSource_Item) {
        const Item* item=target->data;
        return item->entity==object && (uint32_t)item->kind==target->kind;
    }
    return true;
}

static bool attachment_owner_is(NativeScriptRuntimeContext* c,const NativeScriptValue* argument,
    NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    NativeItemLifetime source;
    if(!c || c->source_class!=NativeCallbackSource_Item || !c->scene_epoch ||
       !argument || argument->type!=NativeScriptType_Object ||
       !nativeItemLifetime_Capture(c->source,&source) ||
       source.reference.lifetime!=c->scene_epoch || source.generation!=c->generation ||
       !source.source_owned || !source.article || !source.attributes ||
       source.program->article!=source.article || source.program->kind!=source.kind ||
       source.program->owner!=source.data->native_script_owner ||
       source.program->author_kind!=source.data->native_article_author_kind)
        return error(d,"Attachment-owner comparison requires its live owned Item and Object input");

    void* resolved=NULL;uint32_t generation;
    // A valid Item/Effect target is a healthy nonmatch, just as the original
    // native pointer comparison. Invalid/stale supplied tokens still fail.
    if(!nativeScriptReceiver_ResolveExternal(c,argument,c->scene_epoch,UINT32_MAX,
            &resolved,&generation,d))return false;
    PhysicalIdentity target={.object=resolved,.generation=generation,
        .source_class=argument->as.object.kind&NATIVE_SCRIPT_RECEIVER_SOURCE_MASK};
    if(resolved) {
        target.reference=(NativeMatchPoolReference){&nativeGObjPool,c->scene_epoch,
            argument->as.object.identity-1,argument->as.object.generation};
        if(nativeMatchPool_Resolve(target.reference)!=resolved)
            return error(d,"Attachment comparison target changed before capture");
        target.data=target.object->user_data;
        if(target.source_class==NativeCallbackSource_Fighter) {
            if(target.object->classifier!=HSD_GOBJ_CLASS_FIGHTER)
                return error(d,"Attachment comparison target has the wrong native class");
            target.kind=(uint32_t)((const Fighter*)target.data)->kind;
        } else if(target.source_class==NativeCallbackSource_Item) {
            if(target.object->classifier!=HSD_GOBJ_CLASS_ITEM)
                return error(d,"Attachment comparison target has the wrong native class");
            target.kind=(uint32_t)((const Item*)target.data)->kind;
        }
        if(!physical_current(&target,c->scene_epoch))
            return error(d,"Attachment comparison target is not its live physical instance");
    }
    Fighter_GObj* attachment=nativeScriptItem_AttachmentOwner(c->source);
    PhysicalIdentity owner={.object=attachment,.source_class=NativeCallbackSource_Fighter};
    if(attachment) {
        if(!nativeMatchPool_Reference(&nativeGObjPool,attachment,&owner.reference) ||
           owner.reference.lifetime!=c->scene_epoch || attachment->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
           !nativeCallbackSource_Snapshot(attachment,NativeCallbackSource_Fighter,&owner.generation))
            return error(d,"Captured attachment owner retired during comparison");
        owner.data=attachment->user_data;owner.kind=(uint32_t)((const Fighter*)owner.data)->kind;
        if(!physical_current(&owner,c->scene_epoch))
            return error(d,"Captured attachment owner changed during comparison");
    }
    bool equal=attachment && target.object==attachment;
    // Recheck the captured relation only; never select a replacement target or
    // query another program's private state to complete this comparison.
    if(!nativeItemLifetime_Current(&source) || !physical_current(&target,c->scene_epoch) ||
       !physical_current(&owner,c->scene_epoch) ||
       nativeScriptItem_AttachmentOwner(c->source)!=attachment ||
       !nativeItemLifetime_Current(&source) || !physical_current(&target,c->scene_epoch) ||
       !physical_current(&owner,c->scene_epoch))
        return error(d,"Attachment comparison identity or relation changed before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=equal};return true;
}

bool nativeScriptBindingArticleLifecycle_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_ItemAttachmentOwnerIs) {
        if(!out || !args || count!=1)return error(d,"Attachment-owner comparison requires one Object argument");
        return attachment_owner_is(c,args,out,d);
    }
    bool attachment=semantic==NativeScriptBinding_ItemAttachToOwner || semantic==NativeScriptBinding_ItemAttachToOwnerAux;
    bool clear=semantic==NativeScriptBinding_ItemClearCurrentOwner ||
        semantic==NativeScriptBinding_ItemClearAttachmentOwner || semantic==NativeScriptBinding_ItemDetachOwner;
    if(!out || (!attachment && !clear) ||
       (attachment?(count!=(semantic==NativeScriptBinding_ItemAttachToOwnerAux?2u:1u) || !args):count!=0))
        return error(d,"Article lifecycle semantic or arguments are invalid");
    if(!c || c->source_class!=NativeCallbackSource_Item)
        return error(d,"Article lifecycle operation has the wrong source class");
    Actor caller={0};if(!capture(c,&caller,d))return false;
    if(attachment){if(!attach(&caller,args,count,d))return false;}
    else {
        Item* item=(Item*)caller.data;
        if(semantic!=NativeScriptBinding_ItemClearAttachmentOwner)item->owner=NULL;
        if(semantic!=NativeScriptBinding_ItemClearCurrentOwner && !nativeScriptItem_ClearAttachmentOwner(c->source))
            return error(d,"Article attachment relation could not be cleared");
    }
    if(!live(&caller))return error(d,"Article lifecycle caller retired/faulted before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
