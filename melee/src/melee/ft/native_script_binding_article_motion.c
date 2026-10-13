#include "native_script_binding_article_motion.h"
#include "types.h"
#include "native_fighter_lifetime.h"
#include "ftparts.h"
#include "ftdata.h"
#include "native_source_package.h"
#include <melee/it/native_item_lifetime.h>
#include <melee/it/it_2725.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <math.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/types.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>

typedef struct Source {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    Item* data;
    const NativeScriptItemProgram* spec;
    NativeScriptReceiverView view;
    uint64_t layout,parameters;
} Source;
typedef struct Relation {
    HSD_GObj* object;
    NativeMatchPoolReference reference;
    const void* data;
    uint32_t classifier,generation,kind;
} Relation;
static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool live(const Source* s)
{
    HSD_GObj* object=s->context.source;NativeScriptRuntimeContext c=s->context;
    NativeScriptReceiverView view;const NativeScriptCompileBank* bank=s->spec->bank;
    return nativeMatchPool_Resolve(s->reference)==object && s->reference.lifetime==c.scene_epoch &&
        object->classifier==HSD_GOBJ_CLASS_ITEM && object->user_data==s->data && nativeScriptBindings_SourceAlive(&c) &&
        nativeScriptItem_Status(object)==NativeScriptVM_Ok && nativeScriptItem_Spec(object)==s->spec && bank->valid &&
        nativeScriptItem_View(object,c.generation,&view) && view.program==&bank->declarations && view.program==s->view.program &&
        view.storage==s->view.storage && view.storage && view.storage->live && view.storage->owner==s->spec->owner &&
        view.storage->generation==c.generation && view.storage->layout==&bank->storage_layout && view.storage->layout->identity==s->layout &&
        view.storage->parameters==&bank->storage_parameters && view.storage->parameters->identity==s->parameters;
}
static bool capture(NativeScriptRuntimeContext* c,Source* s,NativeScriptDiagnostic* d)
{
    if(!c || c->source_class!=NativeCallbackSource_Item || !c->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&s->reference) || s->reference.lifetime!=c->scene_epoch ||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_ITEM || !nativeScriptBindings_SourceAlive(c))
        return error(d,"Article motion source has no live Item identity in its scene");
    s->context=*c;s->data=((HSD_GObj*)c->source)->user_data;s->spec=nativeScriptItem_Spec(c->source);
    if(!s->spec || !s->spec->bank || !nativeScriptItem_View(c->source,c->generation,&s->view) ||
       !s->view.storage || !s->view.storage->layout || !s->view.storage->parameters)
        return error(d,"Article motion source has no exact owned program/storage");
    s->layout=s->view.storage->layout->identity;s->parameters=s->view.storage->parameters->identity;
    return live(s)?true:error(d,"Article motion source is retired, faulted or has changed schema");
}
static bool relation_live(const Relation* r,uint64_t epoch)
{
    if(!r->object)return true;
    if(nativeMatchPool_Resolve(r->reference)!=r->object || r->reference.lifetime!=epoch ||
       r->object->classifier!=r->classifier || r->object->user_data!=r->data)return false;
    if(r->classifier!=HSD_GOBJ_CLASS_FIGHTER)return true;
    const Fighter* f=r->data;
    return nativeCallbackSource_Alive(r->object,r->generation,NativeCallbackSource_Fighter) &&
        f->gobj==r->object && (uint32_t)f->kind==r->kind;
}
static bool relation(HSD_GObj* object,uint64_t epoch,Relation* r)
{
    if(!object)return true;
    Relation value={.object=object};
    if(!nativeMatchPool_Reference(&nativeGObjPool,object,&value.reference) || value.reference.lifetime!=epoch)return false;
    value.classifier=object->classifier;value.data=object->user_data;
    if(value.classifier==HSD_GOBJ_CLASS_FIGHTER) {
        if(!value.data || !nativeCallbackSource_Snapshot(object,NativeCallbackSource_Fighter,&value.generation))return false;
        value.kind=(uint32_t)((const Fighter*)value.data)->kind;
    }
    if(!relation_live(&value,epoch))return false;*r=value;return true;
}
// Release reads the native primary joint and attachment bone, independently of
// authored model selectors. Admission does not evaluate or allocate a matrix.
static bool release_joint(HSD_JObj* joint)
{
    HSD_ClassInfo* type=joint?HSD_CLASS_METHOD(joint):NULL;
    return type && type->head.obj_size>=(int)sizeof(*joint) &&
        hsdObjIsDescendantOf((HSD_Obj*)joint,HSD_CLASS_INFO(&hsdJObj));
}
bool nativeScriptBindingArticleMotion_Read(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)selectors;
    if(!out || count || (semantic!=NativeScriptBinding_ItemAttachmentActive&&semantic!=NativeScriptBinding_ItemCurrentOwner&&
       semantic!=NativeScriptBinding_ItemContactFighter&&semantic!=NativeScriptBinding_ItemReflector&&semantic!=NativeScriptBinding_ItemReflectionOwnerTransferDisabled))
        return error(d,"Unsupported Article ownership comparison or selectors");
    Source s={0};if(!capture(c,&s,d))return false;
    if(semantic==NativeScriptBinding_ItemReflectionOwnerTransferDisabled){
        NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=s.data->xDCC_flag.b1!=0};
        if(!live(&s))return error(d,"Article source changed during reflection policy query");
        *out=value;return true;
    }
    if(semantic==NativeScriptBinding_ItemContactFighter||semantic==NativeScriptBinding_ItemReflector){
        HSD_GObj** field=semantic==NativeScriptBinding_ItemReflector?&s.data->xC64_reflectGObj:&s.data->xCF4_fighterGObjUnk;
        HSD_GObj* object=*field;Relation selected={0};uint32_t id=0;
        bool captured=object&&relation(object,c->scene_epoch,&selected);
        if(captured&&selected.classifier==HSD_GOBJ_CLASS_FIGHTER)id=selected.generation;
        if(!live(&s)||*field!=object)
            return error(d,"Article contact identity changed before handle publication");
        if(captured&&!relation_live(&selected,c->scene_epoch))
            return error(d,"Captured contact target changed before handle publication");
        *out=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=id};return true;
    }
    if(semantic==NativeScriptBinding_ItemCurrentOwner) {
        HSD_GObj* object=s.data->owner;Relation selected={0};uint32_t id=0;
        bool present=object && relation(object,c->scene_epoch,&selected) && selected.classifier==HSD_GOBJ_CLASS_FIGHTER;
        if(present)id=selected.generation;
        if(!live(&s))return error(d,"Article source changed during current-owner query");
        if(present&&(!relation_live(&selected,c->scene_epoch)||s.data->owner!=object))
            return error(d,"Captured damage owner changed before handle publication");
        *out=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=id};return true;
    }
    HSD_GObj* current=s.data->owner;Fighter_GObj* attachment=nativeScriptItem_AttachmentOwner(c->source);
    Relation owner={0},attached={0};bool result=false;
    // Attachment activity compares the revocable control relation. Missing
    // initial identities are false; no dangling userdata is inspected.
    if(current && attachment && relation(current,c->scene_epoch,&owner) &&
       relation(attachment,c->scene_epoch,&attached) && attached.classifier==HSD_GOBJ_CLASS_FIGHTER)
        result=owner.object==attached.object;
    if(!live(&s))return error(d,"Article source changed during ownership comparison");
    if(result && (!relation_live(&owner,c->scene_epoch) || !relation_live(&attached,c->scene_epoch) ||
                  s.data->owner!=current || nativeScriptItem_AttachmentOwner(c->source)!=attachment))
        return error(d,"Captured matching ownership relation changed before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=result};return true;
}
bool nativeScriptBindingArticleMotion_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool release=semantic==NativeScriptBinding_ItemReleaseAttachment;
    bool preserve_form=semantic==NativeScriptBinding_ItemReleaseAttachmentPreserveHeld;
    if(release || preserve_form) {
        if(!c || !out || !args || count!=(preserve_form?4u:3u) ||
           c->source_class!=NativeCallbackSource_Item || !c->scene_epoch ||
           args[0].type!=NativeScriptType_F32 || args[1].type!=NativeScriptType_F32 ||
           args[2].type!=NativeScriptType_F32 || !isfinite(args[0].as.f32) ||
           !isfinite(args[1].as.f32) || !isfinite(args[2].as.f32) ||
           (preserve_form && args[3].type!=NativeScriptType_Bool))
            return error(d,"Attachment release requires finite Float XYZ and optional Bool preserve_held");
        NativeItemLifetime source;
        if(!nativeItemLifetime_Capture(c->source,&source) || !source.source_owned ||
           source.reference.lifetime!=c->scene_epoch || source.generation!=c->generation ||
           !nativeScriptBindings_SourceAlive(c) || !source.article || !source.attributes ||
           source.program->article!=source.article || source.article->x0_common_attr!=source.attributes ||
           !source.article->x10_modelDesc || source.object_kind!=HSD_GObj_JObjKind ||
           !release_joint((HSD_JObj*)source.model))
            return error(d,"Attachment release requires its exact live Item program/assets and primary joint");
        Item* item=source.data;
        HSD_GObj* native_owner=item->xDC8_word.flags.x0?item->x51C:item->owner;
        NativeMatchPoolReference owner_reference;
        NativeFighterLifetime owner;
        if(!nativeMatchPool_Reference(&nativeGObjPool,native_owner,&owner_reference) ||
           owner_reference.lifetime!=c->scene_epoch || native_owner->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
           !nativeFighterLifetime_Capture(native_owner->user_data,&owner))
            return error(d,"Attachment release native owner is not a retained Fighter");
        Fighter* fp=owner.data;
        FighterPartsTable* parts=(uint32_t)fp->kind<Ft_Kind_RuntimeCapacity && ftPartsTable?ftPartsTable[fp->kind]:NULL;
        const NativeSourcePackage* package=owner.source_owned?nativeFighterRuntime_SourcePackage(fp->kind):NULL;
        if(!parts || !parts->parts_num || parts->parts_num>MAX_FT_PARTS || !fp->parts ||
           (unsigned)item->xDC4>=parts->parts_num || !release_joint(fp->parts[item->xDC4].joint) ||
           (owner.source_owned && (!package || !package->ready || !package->assets.ready ||
            parts!=&package->assets.parts || fp->ft_data!=&package->assets.character.data)))
            return error(d,"Attachment release native Fighter part is absent or differs from owned assets");
        const ItemModelDesc* model=source.article->x10_modelDesc;
        const HSD_Joint* shadow=item->xC8_joint;
        if(shadow!=model->x0_joint)
            return error(d,"Attachment release shadow differs from its owned primary descriptor");
        FighterBone* owner_parts=fp->parts;
        HSD_JObj* owner_joint=owner_parts[item->xDC4].joint;
        uint32_t owner_part_count=parts->parts_num;
        unsigned owner_part=(unsigned)item->xDC4;
        int bone=model->x8_bone_attach_id;
        HSD_JObj* joint=(HSD_JObj*)source.model;
        if(bone<0 || bone>=100 || model->x4_bone_count>100)
            return error(d,"Attachment release Item bone exceeds the native model table");
        if(model->x4_bone_count) {
            if((unsigned)bone>=model->x4_bone_count || !item->xBBC_dynamicBoneTable)
                return error(d,"Attachment release Item bone table is absent");
            joint=item->xBBC_dynamicBoneTable->bones[bone];
        } else for(int i=0;i<bone;++i) {
            if(!release_joint(joint))return error(d,"Attachment release Item joint path is absent");
            joint=joint->child;
        }
        if(!release_joint(joint))return error(d,"Attachment release Item attach joint is absent");
        // The creator test is intentionally separate from the native release
        // owner selection (which can use x51C). Reflection may change owner.
        Fighter_GObj* creator=nativeScriptItem_Creator(c->source);
        bool preserve=preserve_form && args[3].as.u64 && creator && item->owner==creator;
        NativeFighterLifetime held_owner;NativeItemLifetime held={0};Item_GObj* held_object=NULL;
        if(preserve) {
            if(!nativeFighterLifetime_Capture(creator->user_data,&held_owner))
                return error(d,"Attachment release held-slot Fighter identity is unavailable");
            held_object=held_owner.data->item_gobj;
            if(held_object && (!nativeItemLifetime_Capture(held_object,&held) ||
                               held.reference.lifetime!=c->scene_epoch))
                return error(d,"Attachment release saved held Item has retired");
        }
        Vec3 velocity={args[0].as.f32,args[1].as.f32,args[2].as.f32};
        // Native it_80273B50 applies x4_throw_speed_mul exactly once.
        it_8027429C(c->source,&velocity);
        if(c->source!=source.object || c->source_class!=NativeCallbackSource_Item ||
           c->generation!=source.generation || c->scene_epoch!=source.reference.lifetime ||
           !nativeItemLifetime_Current(&source) || !nativeFighterLifetime_Current(&owner) ||
           !nativeScriptBindings_SourceAlive(c))
            return error(d,"Attachment release participant retired or changed program/storage");
        if(item->xC4_article_data->x10_modelDesc!=model || item->xC8_joint!=shadow ||
           model->x0_joint!=shadow || !ftPartsTable || ftPartsTable[fp->kind]!=parts ||
           parts->parts_num!=owner_part_count || fp->parts!=owner_parts ||
           owner_parts[owner_part].joint!=owner_joint)
            return error(d,"Attachment release primary descriptor or admitted Fighter part changed");
        if(preserve) {
            if(!nativeFighterLifetime_Current(&held_owner) ||
               (held_object && !nativeItemLifetime_Current(&held)))
                return error(d,"Attachment release saved held relation retired before restoration");
            held_owner.data->item_gobj=held_object;
        }
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    bool debug=semantic==NativeScriptBinding_ItemInheritOwnerDebugDisplay;
    bool basic=semantic==NativeScriptBinding_ItemStageCollision;
    bool mode=semantic==NativeScriptBinding_ItemStageCollisionMode;
    if(!out || (!debug&&!basic&&!mode) || count!=(mode?1u:0u) ||
       (mode&&(!args||args[0].type!=NativeScriptType_Bool)))
        return error(d,"Article motion operation differs from its typed signature");
    Source s={0};if(!capture(c,&s,d))return false;
    Relation owner={0};
    if(debug) {
        if(!relation(s.data->owner,c->scene_epoch,&owner))return error(d,"Debug display owner has no live physical identity");
        db_80225DD8(c->source,owner.object);
    } else if(basic || args[0].as.u64)it_8026D9A0(c->source);
    else it_8026DA08(c->source);
    if(!live(&s))return error(d,"Article motion source retired/faulted before publication");
    if(debug && (!relation_live(&owner,c->scene_epoch)||s.data->owner!=owner.object))
        return error(d,"Captured debug display owner changed during native operation");
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
