#include "native_script_binding_fighter_items.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include "ftparts.h"
#include "ftcommon.h"
#include "native_script_moves.h"
#include "kinds/ftCommon/ftpickupitem.h"
#include "kinds/ftCommon/ftCo_ItemThrow.h"
#include "kinds/ftCommon/forward.h"
#include "types.h"
#include <melee/it/item.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_26B1.h>
#include <melee/it/types.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itfoods.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <math.h>

typedef struct ItemView {
    Item_GObj* object;
    NativeMatchPoolReference reference;
    Item* data;
    const Article* article;
    const ItemModelDesc* descriptor;
    const ItemAttr* attributes;
    const ItemLogicTable* logic;
    HSD_JObj* model;
    Fighter_GObj* creator;
    uint32_t creator_generation;
    FighterKind creator_kind;
    const NativeScriptItemProgram* spec;
    NativeScriptReceiverView view;
    uint64_t layout,parameters;
    uint32_t generation,kind;
    bool removal_in_progress;
} ItemView;

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

static bool caller_current(NativeScriptRuntimeContext* context,const NativeFighterLifetime* caller,
                           NativeScriptDiagnostic* d)
{
    return context->source==caller->object && context->source_class==NativeCallbackSource_Fighter &&
        context->generation==caller->generation && context->scene_epoch==caller->reference.lifetime &&
        nativeFighterLifetime_Current(caller) && nativeScriptBindings_SourceAlive(context) &&
        nativeScriptBindings_SourceFighterLayout(context,d)==&caller->program->bank->motion_layout;
}

static bool policy_current(NativeScriptRuntimeContext* context,
                           const NativeFighterLifetime* fighter)
{
    return context->source==fighter->object &&
        context->source_class==NativeCallbackSource_Fighter &&
        context->generation==fighter->generation &&
        context->scene_epoch==fighter->reference.lifetime &&
        nativeFighterLifetime_Current(fighter) && nativeScriptBindings_SourceAlive(context);
}

static bool policy_capture(NativeScriptRuntimeContext* context,
                          NativeFighterLifetime* fighter,NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if(!context || context->source_class!=NativeCallbackSource_Fighter ||
       !context->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference) ||
       reference.lifetime!=context->scene_epoch ||
       ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       !nativeScriptBindings_SourceAlive(context) ||
       !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,fighter) ||
       !policy_current(context,fighter))
        return error(d,"Pickup policy requires its retained physical Fighter identity");
    return true;
}

static bool item_current(const ItemView* saved)
{
    Item_GObj* object=saved->object;
    if(nativeMatchPool_Resolve(saved->reference)!=object || saved->reference.lifetime!=nativeGObjPool.lifetime ||
       object->classifier!=HSD_GOBJ_CLASS_ITEM || object->user_data!=saved->data ||
       !nativeCallbackSource_Alive(object,saved->generation,NativeCallbackSource_Item) ||
       saved->data->entity!=object || (uint32_t)saved->data->kind!=saved->kind ||
       saved->data->xC4_article_data!=saved->article || saved->data->xCC_item_attr!=saved->attributes ||
       saved->data->xB8_itemLogicTable!=saved->logic || object->hsd_obj!=saved->model ||
       (saved->article && saved->article->x10_modelDesc!=saved->descriptor) ||
       saved->data->native_removal_in_progress!=saved->removal_in_progress)
        return false;
    if(!saved->spec)return !saved->data->native_script_owner && !nativeScriptItem_IsAttached(object) &&
        saved->data->native_creator==saved->creator &&
        saved->data->native_creator_generation==saved->creator_generation &&
        saved->data->native_creator_kind==saved->creator_kind;
    NativeScriptReceiverView view;
    return nativeScriptItem_Status(object)==NativeScriptVM_Ok && nativeScriptItem_Spec(object)==saved->spec &&
        nativeScriptItem_View(object,saved->generation,&view) &&
        view.program==&saved->spec->bank->declarations && view.program==saved->view.program &&
        view.storage==saved->view.storage && view.storage && view.storage->live &&
        view.storage->owner==saved->spec->owner && view.storage->generation==saved->generation &&
        view.storage->layout==&saved->spec->bank->storage_layout && view.storage->layout->identity==saved->layout &&
        view.storage->parameters==&saved->spec->bank->storage_parameters && view.storage->parameters->identity==saved->parameters;
}

static bool item_capture(Item_GObj* object,uint64_t epoch,ItemView* saved,NativeScriptDiagnostic* d)
{
    ItemView value={.object=object};
    if(!nativeMatchPool_Reference(&nativeGObjPool,object,&value.reference) || value.reference.lifetime!=epoch ||
       object->classifier!=HSD_GOBJ_CLASS_ITEM ||
       !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&value.generation))
        return error(d,"Fighter Item target has no current physical identity");
    value.data=object->user_data;value.kind=(uint32_t)value.data->kind;
    value.article=value.data->xC4_article_data;
    value.descriptor=value.article?value.article->x10_modelDesc:NULL;
    value.attributes=value.data->xCC_item_attr;value.logic=value.data->xB8_itemLogicTable;value.model=object->hsd_obj;
    value.creator=value.data->native_creator;value.creator_generation=value.data->native_creator_generation;
    value.creator_kind=value.data->native_creator_kind;
    value.removal_in_progress=value.data->native_removal_in_progress;
    if(nativeScriptItem_IsAttached(object) || value.data->native_script_owner) {
        value.spec=nativeScriptItem_Spec(object);
        if(!value.spec || !nativeScriptItem_View(object,value.generation,&value.view) ||
           !value.view.storage || !value.view.storage->layout || !value.view.storage->parameters)
            return error(d,"Fighter Item target has no live source specification/storage");
        value.layout=value.view.storage->layout->identity;
        value.parameters=value.view.storage->parameters->identity;
    }
    if(!item_current(&value))return error(d,"Fighter Item target is faulted, retiring or has changed its identity");
    *saved=value;return true;
}

static bool handle_word(const NativeScriptValue* value,uint32_t* output,NativeScriptDiagnostic* d)
{
    switch(value->type) {
    case NativeScriptType_I32:case NativeScriptType_I64:
        if(value->as.i64<INT32_MIN || value->as.i64>UINT32_MAX)break;
        *output=(uint32_t)value->as.i64;return true;
    case NativeScriptType_U32:case NativeScriptType_U64:
        if(value->as.u64>UINT32_MAX)break;
        *output=(uint32_t)value->as.u64;return true;
    case NativeScriptType_Bool:
        if(value->as.u64>1)return error(d,"Fighter Item Bool handle is not canonical");
        *output=(uint32_t)value->as.u64;return true;
    default:return error(d,"Fighter Item handle must be integral; Float identities are not admitted");
    }
    return error(d,"Fighter Item handle exceeds its native 32-bit identity");
}

static bool select_item(NativeScriptRuntimeContext* context,const NativeFighterLifetime* caller,
                        uint32_t handle,bool existing,ItemView* output,bool* found,NativeScriptDiagnostic* d)
{
    *found=false;
    if(handle==UINT32_MAX || !HSD_GObjPLinkHead)return true;
    Item_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];uint32_t visited=0;
    while(object) {
        NativeMatchPoolReference reference;
        if(visited++==nativeGObjPool.capacity || !nativeMatchPool_Reference(&nativeGObjPool,object,&reference) ||
           reference.lifetime!=caller->reference.lifetime || object->classifier!=HSD_GOBJ_CLASS_ITEM)
            return error(d,"Fighter Item list is cyclic or contains an invalid scene node");
        if(object->user_data && ((Item*)object->user_data)->x1C==handle) {
            if(!existing) {
                const NativeScriptItemProgram* spec=nativeScriptItem_Spec(object);
                if(spec) {
                    if(spec->bank!=caller->program->bank || spec->author_kind!=caller->kind ||
                       nativeScriptItem_Creator(object)!=caller->object)return true;
                } else {
                    const Item* item=object->user_data;
                    if(item->native_script_owner || nativeScriptItem_IsAttached(object) ||
                       item->native_creator!=caller->object || item->native_creator_generation!=caller->generation ||
                       item->native_creator_kind!=caller->kind)return true;
                }
            }
            if(!item_capture(object,caller->reference.lifetime,output,d))return false;
            if(!caller_current(context,caller,d))return error(d,"Fighter Item caller changed during selection");
            *found=true;return true;
        }
        object=object->next;
    }
    return true;
}

bool nativeScriptBindingFighterItems_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_ItemsRejectHealingPickup) {
        NativeFighterLifetime fighter;
        if(!output || !policy_capture(context,&fighter,d))return false;
        NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=fighter.data->x2222_b4!=0};
        if(!policy_current(context,&fighter))return error(d,"Pickup policy Fighter retired before publication");
        *output=value;return true;
    }
    if(semantic!=NativeScriptBinding_ItemsAuxiliaryId && semantic!=NativeScriptBinding_ItemsHeldUseCategory && semantic!=NativeScriptBinding_ItemsHoldKind && semantic!=NativeScriptBinding_ItemsHeavy &&
       semantic!=NativeScriptBinding_ItemsReopenBlocked)
        return error(d,"Unknown held Item query");
    if(!context || !output || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context))return error(d,"Held Item query requires its live source Fighter");
    NativeFighterLifetime caller;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller) || !caller.source_owned ||
       !caller_current(context,&caller,d))return error(d,"Held Item query requires its exact owned actor/program");
    if(semantic==NativeScriptBinding_ItemsReopenBlocked) {
        *output=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=caller.data->x2221_b5!=0};return true;
    }
    Item_GObj* object=semantic==NativeScriptBinding_ItemsAuxiliaryId?caller.data->x1984_heldItemSpec:caller.data->item_gobj;
    NativeScriptValue result=semantic==NativeScriptBinding_ItemsHeavy?
        (NativeScriptValue){.type=NativeScriptType_Bool}:(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=semantic==NativeScriptBinding_ItemsHoldKind?0:-1};
    NativeMatchPoolReference reference;
    if(object && nativeMatchPool_Reference(&nativeGObjPool,object,&reference) && reference.lifetime==caller.reference.lifetime) {
        // A malformed live slot is an error, not a nullable absent Item. Do
        // not cast its userdata until its physical class has been validated.
        if(object->classifier!=HSD_GOBJ_CLASS_ITEM)return error(d,"Held slot has a non-Item physical class");
        ItemView target;if(!item_capture(object,caller.reference.lifetime,&target,d))return false;
        if(target.removal_in_progress)return error(d,"Held Item is being removed");
        if(semantic==NativeScriptBinding_ItemsHeavy) {
            if(!target.attributes)return error(d,"Held Item weight attributes are absent");
            result.as.u64=itIsHeavy(object);
        } else if(semantic==NativeScriptBinding_ItemsHeldUseCategory) {
            if(!target.data->xCC_item_attr)return error(d,"Held Item use-category attributes are absent");
            result.as.i64=itGetUseKind(object);
        } else if(semantic==NativeScriptBinding_ItemsHoldKind) {
            if(!target.attributes)return error(d,"Held Item hold-category attributes are absent");
            result.as.i64=itGetHoldKind(object);
        } else result.as.i64=(int32_t)target.generation;
        if(!item_current(&target))return error(d,"Held Item changed during query");
    }
    // Check the physical caller before reading its selected slot: a query
    // result must never publish data from a retired or replaced selection.
    if(!caller_current(context,&caller,d))return error(d,"Held Item caller changed before query publication");
    Item_GObj* selected=semantic==NativeScriptBinding_ItemsAuxiliaryId?caller.data->x1984_heldItemSpec:caller.data->item_gobj;
    if(selected!=object)return error(d,"Held Item slot changed before query publication");
    *output=result;return true;
}

bool nativeScriptBindingFighterItems_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* input,NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_ItemsRejectHealingPickup) {
        NativeFighterLifetime fighter;
        if(!input || input->type!=NativeScriptType_Bool || input->as.u64>1)
            return error(d,"Healing pickup policy requires a canonical Bool");
        if(!policy_capture(context,&fighter,d))return false;
        fighter.data->x2222_b4=input->as.u64!=0;
        return policy_current(context,&fighter)?true:error(d,"Pickup policy Fighter retired after mutation");
    }
    NativeFighterLifetime caller;
    if(semantic!=NativeScriptBinding_ItemsReopenBlocked || !input || input->type!=NativeScriptType_Bool ||
       input->as.u64>1 || !context || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context) ||
       !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller) || !caller.source_owned ||
       !caller_current(context,&caller,d))return error(d,"Item reopen policy requires its live source and canonical Bool");
    caller.data->x2221_b5=input->as.u64!=0;return true;
}

// The native pickup notifier prefers the temporary x1978 slot. Capture that
// exact item rather than assuming the ordinary held slot is its recipient.
static bool capture_slot(const NativeFighterLifetime* caller,bool pickup,ItemView* target,
                         bool* found,NativeScriptDiagnostic* d)
{
    Item_GObj* object=pickup&&caller->data->x1978?caller->data->x1978:caller->data->item_gobj;
    *found=object!=NULL;
    return !object || item_capture(object,caller->reference.lifetime,target,d);
}

static bool common_policy(NativeScriptRuntimeContext* context,const NativeFighterLifetime* caller,
                          uint32_t semantic,const NativeScriptValue* arguments,NativeScriptValue* result,
                          NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_ItemsReleaseHeld) {
        for(unsigned n=0;n<4;++n)
            if(arguments[n].type!=NativeScriptType_F32 || !isfinite(arguments[n].as.f32))
                return error(d,"Held Item release velocity and damage multiplier require finite F32 values");
        ItemView target;bool found;
        if(!capture_slot(caller,false,&target,&found,d))return false;
        if(found) {
            if(!target.article || !target.descriptor || !target.attributes || !target.model ||
               (!target.spec && !target.logic))
                return error(d,"Held Item release requires its native model, attributes and drop behavior");
            Vec3 velocity={arguments[0].as.f32,arguments[1].as.f32,arguments[2].as.f32};
            Item_8026ABD8(target.object,&velocity,arguments[3].as.f32);
            if(!caller_current(context,caller,d))return error(d,"Held Item release caller faulted or retired");
            // A dropped callback may destroy the Item. Never dereference a
            // retired pool slot, including one immediately reused by a spawn.
            if(nativeMatchPool_Resolve(target.reference) && !item_current(&target))
                return error(d,"Held Item release recipient faulted or changed identity");
        }
        return true;
    }
    if(semantic==NativeScriptBinding_ItemsSpawnFood) {
        for(unsigned n=0;n<3;++n)
            if(arguments[n].type!=NativeScriptType_F32 || !isfinite(arguments[n].as.f32))
                return error(d,"Food spawn coordinates require finite F32 values");
        Vec3 position={arguments[0].as.f32,arguments[1].as.f32,arguments[2].as.f32};
        Item_GObj* object=it_8028FAF4(caller->object,&position);
        if(!caller_current(context,caller,d))return error(d,"Food spawn caller retired during construction");
        if(object) {
            ItemView target;if(!item_capture(object,caller->reference.lifetime,&target,d))return false;
            if(target.kind!=It_Kind_Foods || target.spec || target.creator)
                return error(d,"Food constructor did not preserve its ordinary unowned identity");
        }
        *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=object!=NULL};return true;
    }
    if(semantic==NativeScriptBinding_ItemsCreateEngine) {
        if(arguments[0].type!=NativeScriptType_I32 || arguments[0].as.i64<0 ||
           arguments[0].as.i64>=It_Kind_Kuriboh)return error(d,"Engine Item kind is outside the ordinary common-item family");
        for(unsigned n=1;n<=7;++n)
            if(arguments[n].type!=NativeScriptType_F32 || !isfinite(arguments[n].as.f32))
                return error(d,"Engine Item position, velocity and facing require finite F32 values");
        uint32_t group;if(!handle_word(&arguments[8],&group,d))return false;
        if(arguments[9].type!=NativeScriptType_F32 || !isfinite(arguments[9].as.f32) ||
           arguments[9].as.f32<INT16_MIN || arguments[9].as.f32>INT16_MAX)
            return error(d,"Engine Item damage is outside its finite native s16 domain");
        Vec3 position={arguments[1].as.f32,arguments[2].as.f32,arguments[3].as.f32};
        SpawnItem spawn={.kind=(ItemKind)arguments[0].as.i64};
        Item_InitSpawn(&spawn,caller->object,&position,arguments[7].as.f32);
        spawn.vel=(Vec3){arguments[4].as.f32,arguments[5].as.f32,arguments[6].as.f32};
        spawn.x40=group;spawn.x3C_damage=(s16)arguments[9].as.f32;
        Item_GObj* object=Item_CreateEngine(&spawn);
        if(!caller_current(context,caller,d))return error(d,"Engine Item caller retired during construction");
        *result=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1};
        if(!object)return true; // the native NULL boundary includes callback cleanup
        ItemView target;if(!item_capture(object,caller->reference.lifetime,&target,d))return false;
        if(target.spec || target.data->native_creator!=caller->object ||
           target.data->native_creator_generation!=caller->generation || target.data->native_creator_kind!=caller->kind)
            return error(d,"Engine Item construction did not preserve its ordinary creator identity");
        result->as.i64=(int32_t)target.generation;return true;
    }
    ItemView target;bool found;
    if(!capture_slot(caller,semantic==NativeScriptBinding_ItemsNotifyPickup,&target,&found,d))return false;
    if(semantic==NativeScriptBinding_ItemsNotifyPickup) {
        if(arguments[0].type!=NativeScriptType_Bool || arguments[0].as.u64>1)
            return error(d,"Item pickup notification requires a canonical Bool");
        ftpickupitem_80094818(caller->object,arguments[0].as.u64!=0);
    } else if(semantic==NativeScriptBinding_ItemsNotifyHeldNeutral) {
        result->type=NativeScriptType_Bool;result->as.u64=0;
        if(found && target.spec && !target.removal_in_progress &&
           nativeScriptItem_HookBound(target.object,NativeScriptItemHook_HolderNeutral)) {
            NativeScriptValue value;
            if(nativeScriptItem_RunHook(target.object,NativeScriptItemHook_HolderNeutral,NULL,0,&value)!=NativeScriptVM_Ok ||
               value.type!=NativeScriptType_Void)return error(d,"Held Item neutral callback faulted or retired");
            if(!caller_current(context,caller,d))return error(d,"Held Item neutral caller retired during its callback");
            result->as.u64=caller->data->item_gobj==target.object;
        }
    } else {
        if(arguments[0].type!=NativeScriptType_I32 || arguments[0].as.i64<ftCo_MS_LightThrowF ||
           arguments[0].as.i64>ftCo_MS_HeavyThrowLw4 ||
           !nativeScriptMoves_Contains(&caller->program->bank->motion_layout,(uint32_t)arguments[0].as.i64))
            return error(d,"Held Item throw motion is outside the published common throw table");
        if(found) {
            if(!target.data->xCC_item_attr)return error(d,"Held Item throw attributes are absent");
            ftCo_800957F4(caller->object,(int)arguments[0].as.i64);
        }
    }
    if(!caller_current(context,caller,d))return error(d,"Held Item policy caller faulted or retired");
    if(found && !item_current(&target))return error(d,"Held Item policy recipient changed before publication");
    return true;
}

static bool attach(NativeScriptRuntimeContext* context,const NativeFighterLifetime* caller,
                   const ItemView* target,int32_t raw,bool notify,bool* attached,NativeScriptDiagnostic* d)
{
    *attached=false;
    Fighter* fighter=caller->data;
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(caller->kind);
    if(package)raw=nativeFighterRig_Resolve(&package->assets.rig,raw);
    if(!package || !fighter->parts || !ftPartsTable ||
       ftPartsTable[caller->kind]!=&package->assets.parts ||
       package->assets.parts.parts_num!=package->assets.facts.joints+package->assets.facts.omission_count ||
       !package->assets.parts.parts_num || package->assets.parts.parts_num>MAX_FT_PARTS ||
       raw<0 || (uint32_t)raw>=package->assets.parts.parts_num || !fighter->parts[raw].joint ||
       !target->model || !target->article || !target->descriptor || !target->attributes ||
       (!target->spec && !target->logic) ||
       (target->spec && target->article!=target->spec->article))
        return true; // native invalid-rig/target admission is a healthy false
    const FighterBone* parts=fighter->parts;
    HSD_JObj* joint=parts[raw].joint;
    HSD_JObj* root=target->object->hsd_obj;
    // Actor attachment provenance remains separate. Pickup can change native
    // damage/pending ownership and can explicitly clear its control relation.
    Item_8026AB54(target->object,caller->object,(Fighter_Part)raw);
    if(!caller_current(context,caller,d) || !item_current(target))
        return error(d,"Fighter or Item faulted/retired during attachment pickup");
    if(fighter->parts!=parts || parts[raw].joint!=joint || target->object->hsd_obj!=root ||
       ftPartsTable[caller->kind]!=&package->assets.parts)
        return error(d,"Fighter Item attachment model peers changed during pickup");
    if(notify) {
        // This legacy flag repeats debug-display inheritance, not a Fighter
        // item-pickup event. Preserve its actual position after Item pickup.
        db_80225DD8(target->object,caller->object);
        if(!caller_current(context,caller,d) || !item_current(target))
            return error(d,"Fighter or Item changed during attachment debug inheritance");
    }
    *attached=true;return true;
}

static bool inherit_scale(NativeScriptRuntimeContext* context,const NativeFighterLifetime* caller,
                          const ItemView* target,bool remember,bool* inherited,NativeScriptDiagnostic* d)
{
    *inherited=false;
    if(!target->model || !target->article || !target->descriptor || !target->attributes)return true;
    Fighter_GObj* owner=target->data->owner;
    NativeMatchPoolReference owner_reference;uint32_t generation;
    if(!nativeMatchPool_Reference(&nativeGObjPool,owner,&owner_reference) ||
       owner_reference.lifetime!=caller->reference.lifetime ||
       !nativeCallbackSource_Snapshot(owner,NativeCallbackSource_Fighter,&generation))return true;
    NativeFighterLifetime current_owner;
    if(!nativeFighterLifetime_Capture(owner->user_data,&current_owner))
        return error(d,"Fighter Item scale owner is faulted or has no stable identity");
    // it_80274594 multiplies the existing Item scale by the actual current
    // damage owner's instance Y scale and updates native model/collision data.
    // Original creator and revocable attachment owner are not substituted.
    it_80274594(target->object);
    if(!caller_current(context,caller,d) || !item_current(target) ||
       !nativeFighterLifetime_Current(&current_owner))
        return error(d,"Fighter, Item or captured scale owner retired during scale inheritance");
    if(remember)target->data->throw_scale=target->data->scl;
    *inherited=true;return true;
}

bool nativeScriptBindingFighterItems_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    bool clear=semantic==NativeScriptBinding_ItemsClearHeld || semantic==NativeScriptBinding_ItemsClearAuxiliary;
    bool attachment=semantic==NativeScriptBinding_ItemsAttach;
    bool scale=semantic==NativeScriptBinding_ItemsInheritOwnerScale;
    bool existing=semantic==NativeScriptBinding_ItemsBindExistingHeld || semantic==NativeScriptBinding_ItemsBindExistingAuxiliary;
    bool presentation=semantic==NativeScriptBinding_ItemsSetVisible || semantic==NativeScriptBinding_ItemsSetPaused ||
        semantic==NativeScriptBinding_ItemsNotifyDrop;
    bool bind=existing || semantic==NativeScriptBinding_ItemsBindHeld || semantic==NativeScriptBinding_ItemsBindAuxiliary ||
        semantic==NativeScriptBinding_ArticlesBindHeld || semantic==NativeScriptBinding_ArticlesBindAuxiliary;
    bool policy=semantic==NativeScriptBinding_ItemsNotifyPickup || semantic==NativeScriptBinding_ItemsNotifyHeldNeutral ||
        semantic==NativeScriptBinding_ItemsThrowHeld || semantic==NativeScriptBinding_ItemsReleaseHeld || semantic==NativeScriptBinding_ItemsCreateEngine ||
        semantic==NativeScriptBinding_ItemsSpawnFood;
    uint32_t expected=semantic==NativeScriptBinding_ItemsSpawnFood?3:semantic==NativeScriptBinding_ItemsCreateEngine?10:
        semantic==NativeScriptBinding_ItemsReleaseHeld?4:semantic==NativeScriptBinding_ItemsNotifyHeldNeutral?0:clear?0:attachment?3:(scale||presentation)?2:1;
    if((!clear&&!attachment&&!scale&&!bind&&!presentation&&!policy) || !context || !output || count!=expected || (count&&!arguments) ||
       context->source_class!=NativeCallbackSource_Fighter || !nativeScriptBindings_SourceAlive(context))
        return error(d,"Fighter Item operation requires its live source and exact signature");
    NativeFighterLifetime caller;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller) || !caller.source_owned ||
       !caller_current(context,&caller,d))return error(d,"Fighter Item operation requires its exact owned actor/program");
    bool auxiliary=semantic==NativeScriptBinding_ItemsClearAuxiliary || semantic==NativeScriptBinding_ItemsBindAuxiliary ||
        semantic==NativeScriptBinding_ItemsBindExistingAuxiliary || semantic==NativeScriptBinding_ArticlesBindAuxiliary;
    if(policy) {
        NativeScriptValue result={.type=NativeScriptType_Void};
        if(!common_policy(context,&caller,semantic,arguments,&result,d) || !caller_current(context,&caller,d))return false;
        *output=result;return true;
    }
    NativeScriptValue result={.type=clear?NativeScriptType_Void:NativeScriptType_Bool};
    if(clear) {
        if(auxiliary)caller.data->x1984_heldItemSpec=NULL;
        else caller.data->item_gobj=NULL;
    } else {
        uint32_t handle;if(!handle_word(&arguments[0],&handle,d))return false;
        if((attachment&&(arguments[1].type!=NativeScriptType_I32 || arguments[1].as.i64<INT32_MIN ||
                         arguments[1].as.i64>INT32_MAX || arguments[2].type!=NativeScriptType_Bool || arguments[2].as.u64>1)) ||
           ((scale||presentation)&&(arguments[1].type!=NativeScriptType_Bool || arguments[1].as.u64>1)))
            return error(d,"Fighter Item raw joint or Bool flag has the wrong type/value");
        ItemView target;bool found;
        if(!select_item(context,&caller,handle,existing||presentation,&target,&found,d))return false;
        if(found) {
            bool succeeded=true;
            if(attachment) {if(!attach(context,&caller,&target,(int32_t)arguments[1].as.i64,arguments[2].as.u64!=0,&succeeded,d))return false;}
            else if(scale) {if(!inherit_scale(context,&caller,&target,arguments[1].as.u64!=0,&succeeded,d))return false;}
            else if(presentation) {
                bool flag=arguments[1].as.u64!=0;
                if(semantic==NativeScriptBinding_ItemsSetVisible) {
                    if(!target.object->hsd_obj)succeeded=false;
                    else if(flag)it_8026BB20(target.object);
                    else it_8026BB44(target.object);
                } else if(semantic==NativeScriptBinding_ItemsSetPaused) {
                    if(flag)it_8026B724(target.object);else it_8026B73C(target.object);
                } else ftCommon_8007E6DC(caller.object,target.object,flag);
            }
            else {
                if(auxiliary)caller.data->x1984_heldItemSpec=target.object;
                else caller.data->item_gobj=target.object;
            }
            if(!item_current(&target))return error(d,"Fighter Item target changed before publication");
            result.as.u64=succeeded;
        }
    }
    if(!caller_current(context,&caller,d))return error(d,"Fighter Item caller changed before publication");
    *output=result;return true;
}
