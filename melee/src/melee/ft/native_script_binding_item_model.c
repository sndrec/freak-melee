#include "native_script_binding_item_model_resolve.h"
#include "native_script_actor_access.h"
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_article_model_scene.h>
#include <melee/it/native_article_model.h>
#include <melee/it/types.h>
#include <melee/it/itanimlist.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include "types.h"
#include <melee/lb/native_model_joint.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <math.h>
typedef struct ModelSource {
    NativeScriptRuntimeContext context;
    NativeMatchPoolReference reference;
    Item* item;
    const NativeScriptItemProgram* program;
    const NativeScriptCompileBank* bank;
    NativeScriptReceiverView view;
    HSD_JObj* root;
    HSD_ClassInfo* root_class;
    uint8_t object_kind;
    const NativeScriptStorageParameters* parameters;
    uint64_t identity,layout,parameter_identity;
    NativeArticleModelSceneSlot secondary;
    uint8_t model;
    bool dynamic;
    DynamicBoneTable* dynamic_table;
    HSD_JObj* dynamic_node;
    HSD_ClassInfo* dynamic_class;
    uint8_t dynamic_bone;
} ModelSource;
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool root_type(HSD_JObj* root,HSD_ClassInfo* expected)
{
    return root&&expected&&HSD_CLASS_METHOD(root)==expected&&
        expected->head.obj_size>=(int)sizeof(*root)&&
        hsdObjIsDescendantOf((HSD_Obj*)root,HSD_CLASS_INFO(&hsdJObj));
}
static bool current(const ModelSource* s,NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{
    NativeScriptReceiverView view;
    if(c->source!=s->context.source||c->source_class!=s->context.source_class||c->generation!=s->context.generation||c->scene_epoch!=s->context.scene_epoch||
       nativeMatchPool_Resolve(s->reference)!=c->source||s->reference.lifetime!=c->scene_epoch||((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_ITEM||
       ((HSD_GObj*)c->source)->user_data!=s->item||((HSD_GObj*)c->source)->obj_kind!=s->object_kind||((HSD_GObj*)c->source)->hsd_obj!=s->root||
       !nativeScriptBindings_SourceAlive(c)||nativeScriptItem_Status(c->source)!=NativeScriptVM_Ok||nativeScriptItem_Spec(c->source)!=s->program||
       s->program->identity!=s->identity||s->program->bank!=s->bank||!s->bank->valid||s->program->article!=s->item->xC4_article_data||
       s->program->owner!=s->item->native_script_owner||s->program->kind!=s->item->kind||s->program->author_kind!=s->item->native_article_author_kind||
       (s->root&&!root_type(s->root,s->root_class))||!nativeScriptItem_View(c->source,c->generation,&view)||
       view.program!=s->view.program||view.program!=&s->bank->declarations||view.storage!=s->view.storage||!view.storage||!view.storage->live||
       view.storage->owner!=s->program->owner||view.storage->generation!=c->generation||view.storage->layout!=&s->bank->storage_layout||
       view.storage->layout->identity!=s->layout||view.storage->parameters!=s->parameters||!s->parameters||s->parameters->identity!=s->parameter_identity)
        return error(d,"Item primary model, owner, program or storage retired before continuation");
    if(s->model && !nativeArticleModelScene_SlotCurrent(&s->secondary))
        return error(d,"Item secondary slot or graph generation changed before continuation");
    if(s->dynamic && (s->item->xBBC_dynamicBoneTable!=s->dynamic_table ||
       (s->dynamic_table&&s->dynamic_table->bones[s->dynamic_bone]!=s->dynamic_node) ||
       (s->dynamic_node&&!root_type(s->dynamic_node,s->dynamic_class))))
        return error(d,"Item dynamic table or selected joint changed before continuation");
    return true;
}
static bool capture(NativeScriptRuntimeContext* c,ModelSource* s,bool allow_missing_root,NativeScriptDiagnostic* d)
{
    if(!c||c->source_class!=NativeCallbackSource_Item||!c->scene_epoch||!nativeMatchPool_Reference(&nativeGObjPool,c->source,&s->reference)||
       s->reference.lifetime!=c->scene_epoch||((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_ITEM||
       !nativeScriptBindings_SourceAlive(c)||nativeScriptItem_Status(c->source)!=NativeScriptVM_Ok)
        return error(d,"Item primary model requires its live owned Item source");
    s->model=0;s->secondary=(NativeArticleModelSceneSlot){0};s->dynamic=false;
    s->context=*c;s->item=((HSD_GObj*)c->source)->user_data;s->program=nativeScriptItem_Spec(c->source);s->root=((HSD_GObj*)c->source)->hsd_obj;
    s->object_kind=((HSD_GObj*)c->source)->obj_kind;
    if(!s->item||!s->program||!s->program->bank||(!s->root&&!allow_missing_root)||(s->root&&s->object_kind!=HSD_GObj_JObjKind)||
       !nativeScriptItem_View(c->source,c->generation,&s->view)||s->view.program!=&s->program->bank->declarations||
       !s->view.storage||s->view.storage->layout!=&s->program->bank->storage_layout||
       s->view.storage->parameters!=&s->program->bank->storage_parameters)return error(d,"Item primary owned graph/view is absent");
    s->bank=s->program->bank;s->root_class=s->root?HSD_CLASS_METHOD(s->root):NULL;s->identity=s->program->identity;s->layout=s->view.storage->layout->identity;s->parameters=s->view.storage->parameters;s->parameter_identity=s->parameters->identity;
    return current(s,c,d);
}
static bool joint(ModelSource* s,uint32_t semantic,const NativeScriptValue* args,uint32_t count,HSD_JObj** output,unsigned* channel,NativeScriptDiagnostic* d)
{
    uint32_t domain=semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK;
    if((domain!=NATIVE_SCRIPT_ITEM_MODEL_PROPERTY&&domain!=NATIVE_SCRIPT_ITEM_MODEL_DYNAMIC_PROPERTY&&domain!=NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION&&domain!=NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION&&domain!=NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM)||s->program->model_selector_count>NATIVE_SCRIPT_ITEM_MODEL_SELECTORS||
       (s->program->model_selector_count&&!s->program->model_selectors))
        return error(d,"Unsupported Item model semantic/table");
    unsigned low=0,high=s->program->model_selector_count;
    while(low<high){unsigned mid=low+(high-low)/2;if(s->program->model_selectors[mid].semantic<semantic)low=mid+1;else high=mid;}
    if(low==s->program->model_selector_count||s->program->model_selectors[low].semantic!=semantic)return error(d,"Item model selector is not prepared for this exact owner");
    const NativeScriptItemModelSelector* p=&s->program->model_selectors[low];
    if(domain==NATIVE_SCRIPT_ITEM_MODEL_DYNAMIC_PROPERTY){
        if(!args||count!=1||p->argument_count!=1||p->dynamic_bone>=100||p->model||p->depth||
           args[0].type!=NativeScriptType_I32||args[0].as.i64!=p->dynamic_bone)
            return error(d,"Dynamic joint differs from its prepared literal table index");
        s->dynamic=true;s->dynamic_bone=p->dynamic_bone;s->dynamic_table=s->item->xBBC_dynamicBoneTable;
        s->dynamic_node=s->dynamic_table?s->dynamic_table->bones[p->dynamic_bone]:NULL;
        s->dynamic_class=NULL;
        if(s->dynamic_node){
            // Item_802682F0 fills this table in native preorder. Validate the
            // exact loaded descriptor identity at that ordinal, not a path alias.
            typedef struct Node {const HSD_Joint* descriptor;HSD_JObj* live;} Node;
            Node stack[100];unsigned pending=0,ordinal=0;
            const ItemModelDesc* model=s->program->article->x10_modelDesc;
            if(!model||!model->x0_joint||!s->root)return error(d,"Dynamic joint has no owned primary descriptor graph");
            stack[pending++]=(Node){model->x0_joint,s->root};bool found=false;
            while(pending){Node n=stack[--pending];
                if(!n.descriptor||!n.live||ordinal>=100||!root_type(n.live,HSD_CLASS_METHOD(n.live))||
                   n.live->id!=(uintptr_t)n.descriptor)
                    return error(d,"Dynamic joint graph differs from its native loaded descriptor preorder");
                if(ordinal++==p->dynamic_bone){found=n.live==s->dynamic_node;break;}
                if((n.descriptor->next!=NULL)!=(n.live->next!=NULL)||
                   (n.descriptor->child!=NULL)!=(n.live->child!=NULL))
                    return error(d,"Dynamic joint graph topology differs from its owned descriptor");
                if(n.live->next){if(pending==100)return error(d,"Dynamic joint graph capacity exceeded");stack[pending++]=(Node){n.descriptor->next,n.live->next};}
                if(n.live->child){if(pending==100)return error(d,"Dynamic joint graph capacity exceeded");stack[pending++]=(Node){n.descriptor->child,n.live->child};}
            }
            if(!found)return error(d,"Dynamic table entry is not its owned native preorder joint");
            s->dynamic_class=HSD_CLASS_METHOD(s->dynamic_node);
        }
        *output=s->dynamic_node;*channel=p->channel;return true;
    }
    unsigned path=p->argument_count-1;
    if(!args||count!=p->argument_count||(count!=1&&count!=2)||p->depth>12||args[path].type!=NativeScriptType_Text||
       args[path].as.text.offset!=p->path_offset||args[path].as.text.length!=p->path_length||
       p->model>3 || (count==1 && p->model) ||
       (count==2&&(args[0].type!=NativeScriptType_I32||args[0].as.i64!=p->model)))return error(d,"Item model selectors differ from their immutable literal recipe");
    if(s->model && s->model!=p->model)return error(d,"Item model projection changed within an operation");
    if(p->model && !s->model){
        if(!nativeArticleModelScene_CaptureSlot(s->context.source,p->model,&s->secondary))
            return error(d,"Item secondary projection requires its registered scene slot");
        s->model=p->model;
    }
    HSD_JObj* node=s->model?s->secondary.root:s->root;
    for(unsigned depth=0;node&&depth<p->depth;++depth){node=node->child;for(unsigned sibling=0;node&&sibling<p->path[depth];++sibling)node=node->next;}
    *output=node;*channel=p->channel;return true;
}
bool nativeScriptBindingItemModel_Read(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out)return error(d,"Item model read output is absent");ModelSource s;HSD_JObj* node;unsigned channel;
    if(!capture(c,&s,true,d)||!joint(&s,semantic,args,count,&node,&channel,d))return false;
    if(!s.model&&!s.root&&!s.dynamic)return error(d,"Item primary owned graph is absent");
    float scalar=nativeModelJoint_Read(node,channel);if(!current(&s,c,d))return false;
    *out=(channel==9||channel==10||channel==15)?(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=scalar!=0}:(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=scalar};return true;
}
bool nativeScriptBindingItemModel_Write(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(!value)return error(d,"Item model write value is absent");ModelSource s;HSD_JObj* node;unsigned channel;
    if(!capture(c,&s,true,d)||!joint(&s,semantic,args,count,&node,&channel,d))return false;
    if(!s.model&&!s.root&&!s.dynamic)return error(d,"Item primary owned graph is absent");
    if(channel==9||value->type!=((channel==10||channel==15)?NativeScriptType_Bool:NativeScriptType_F32))return error(d,"Item joint write type/access is invalid");
    nativeModelJoint_Write(node,channel,(channel==10||channel==15)?(float)(value->as.u64!=0):value->as.f32);return current(&s,c,d);
}
typedef struct AppearanceSource {
    struct ItemStateArray* states;
    ItemStateDesc* descriptors;
    ItemStateDesc* selected;
    ItemStateDesc animation;
    HSD_Joint* joint;
    u32 count;
    int motion;
    uint64_t revision;
} AppearanceSource;
static bool appearance_current(const ModelSource* s,const AppearanceSource* a,
    NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d)
{
    if(!current(s,c,d))return false;
    if(s->program->article->xC_itemStates!=a->states||a->states->x0_itemStateDesc!=a->descriptors||
       a->states->count!=a->count||s->item->xC8_joint!=a->joint||s->item->msid!=a->motion||
       nativeScriptItem_Revision(c->source)!=a->revision||s->item->xD0_itemStateDesc!=a->selected||
       a->selected->x0_anim_joint!=a->animation.x0_anim_joint||
       a->selected->x4_matanim_joint!=a->animation.x4_matanim_joint||
       a->selected->x8_parameters!=a->animation.x8_parameters||
       a->selected->xC_script!=a->animation.xC_script)
        return error(d,"Item appearance descriptor or state changed before continuation");
    return true;
}
static bool appearance(const ModelSource* s,NativeScriptRuntimeContext* c,
    int32_t descriptor,float frame,NativeScriptDiagnostic* d)
{
    struct ItemStateArray* states=s->program->article->xC_itemStates;
    // The source oracle admits only an index in this owned variable-length table.
    // The native routine itself assumes that admission and does not bounds check.
    if(!states)return error(d,"Item appearance state table is absent");
    if(descriptor<0||(uint32_t)descriptor>=states->count)return true;
    if(!states->x0_itemStateDesc)return error(d,"Item appearance descriptors are absent");
    AppearanceSource a={.states=states,.descriptors=states->x0_itemStateDesc,
        .selected=&states->x0_itemStateDesc[descriptor],.joint=s->item->xC8_joint,
        .count=states->count,.motion=s->item->msid,.revision=nativeScriptItem_Revision(c->source)};
    a.animation=*a.selected;s->item->xD0_itemStateDesc=a.selected;
    // it_80273670 order, with continuation checks before touching cached peers.
    HSD_JObjRemoveAnimAll(s->root);if(!appearance_current(s,&a,c,d))return false;
    if(a.joint){lb_8000B804(s->root->child,a.joint->child);if(!appearance_current(s,&a,c,d))return false;}
    HSD_JObjAddAnimAll(s->root,a.animation.x0_anim_joint,a.animation.x4_matanim_joint,a.animation.x8_parameters);
    if(!appearance_current(s,&a,c,d))return false;
    lb_8000BA0C(s->root,s->item->x5D0_animFrameSpeed);if(!appearance_current(s,&a,c,d))return false;
    HSD_JObjReqAnimAll(s->root,frame);if(!appearance_current(s,&a,c,d))return false;
    HSD_JObjAnimAll(s->root);if(!appearance_current(s,&a,c,d))return false;
    HSD_JObjRemoveAnimAll(s->root);if(!appearance_current(s,&a,c,d))return false;
    s->item->x524_cmd.u=NULL;return true;
}
typedef struct ScaleOwner {
    HSD_GObj* object;
    NativeMatchPoolReference reference;
    Fighter* data;
    uint32_t generation;
    FighterKind kind;
} ScaleOwner;
static bool scale_owner_current(const ScaleOwner* owner,const ModelSource* s)
{
    return nativeMatchPool_Resolve(owner->reference)==owner->object&&
        owner->reference.lifetime==s->context.scene_epoch&&
        owner->object->classifier==HSD_GOBJ_CLASS_FIGHTER&&owner->object->user_data==owner->data&&
        nativeCallbackSource_Alive(owner->object,owner->generation,NativeCallbackSource_Fighter)&&
        owner->data->gobj==owner->object&&owner->data->kind==owner->kind&&s->item->owner==owner->object;
}
static bool scale_owner(const ModelSource* s,ScaleOwner* out,NativeScriptDiagnostic* d)
{
    ScaleOwner owner={.object=s->item->owner};
    if(!owner.object||!nativeMatchPool_Reference(&nativeGObjPool,owner.object,&owner.reference)||
       owner.reference.lifetime!=s->context.scene_epoch||owner.object->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !nativeCallbackSource_Snapshot(owner.object,NativeCallbackSource_Fighter,&owner.generation))
        return error(d,"Item inherited scale requires its live current Fighter owner");
    owner.data=owner.object->user_data;owner.kind=owner.data->kind;
    if(!scale_owner_current(&owner,s))return error(d,"Item scale owner changed before native access");
    *out=owner;return true;
}
static HSD_JObj* spin_joint(const ModelSource* s,const ItemModelDesc* descriptor)
{
    HSD_JObj* node=s->root;unsigned depth=(descriptor->xC_bit_field>>6)&3;
    for(unsigned i=0;node&&i<depth;++i)node=node->child;
    return node;
}
bool nativeScriptBindingItemModel_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_ItemCloneModel||semantic==NativeScriptBinding_ItemLoadModel||semantic==NativeScriptBinding_ItemDestroyModel||
       semantic==NativeScriptBinding_ItemSetModelDrawEnabled){
        bool load=semantic==NativeScriptBinding_ItemLoadModel;
        bool draw=semantic==NativeScriptBinding_ItemSetModelDrawEnabled;
        bool clone=semantic==NativeScriptBinding_ItemCloneModel;
        if(!out||!args||count!=(load||draw?2:1)||args[0].type!=NativeScriptType_I32||
           args[0].as.i64<1||args[0].as.i64>3||
           (load&&(args[1].type!=NativeScriptType_I32||args[1].as.i64<0||args[1].as.i64>UINT32_MAX))||
           (draw&&(args[1].type!=NativeScriptType_Bool||args[1].as.u64>1)))
            return error(d,"Secondary model operation arguments are invalid");
        ModelSource s;
        if(!capture(c,&s,true,d))return false;
        unsigned slot=(unsigned)args[0].as.i64;
        NativeArticleModelSceneSlot before;
        if(!nativeArticleModelScene_CaptureSlot(c->source,slot,&before))
            return error(d,"Secondary model operation requires its exact owned slot");
        bool loaded=false;
        if(clone) {
            if(!s.program->clones_primary_model)
                return error(d,"Primary model cloning was not prepared for this Article owner");
            if(!before.root && s.root) {
                Quaternion rotation;
                HSD_JObjGetRotation(s.root,&rotation);
                Vec3 scale={s.item->scl,s.item->scl,s.item->scl};
                if(!isfinite(scale.x)||!isfinite(rotation.x)||!isfinite(rotation.y)||
                   !isfinite(rotation.z)||!isfinite(rotation.w))
                    return error(d,"Primary model cloning requires finite root rotation and Item scale");
                loaded=nativeArticleModelScene_Load(c->source,slot,NATIVE_ARTICLE_MODEL_PRIMARY_ROOT);
                if(!current(&s,c,d))return false;
                if(loaded) {
                    NativeArticleModelSceneSlot fresh;
                    if(!nativeArticleModelScene_CaptureSlot(c->source,slot,&fresh)||!fresh.root)
                        return error(d,"Primary model clone lost its registered independent graph");
                    HSD_JObjSetRotation(fresh.root,&rotation);
                    if(!current(&s,c,d)||!nativeArticleModelScene_SlotCurrent(&fresh))return false;
                    HSD_JObjSetScale(fresh.root,&scale);
                    if(!current(&s,c,d)||!nativeArticleModelScene_SlotCurrent(&fresh))return false;
                }
            }
        } else if(load){
            if((uint64_t)args[1].as.i64>=s.program->model_root_count)
                return error(d,"Secondary model root ordinal differs from its prepared owner");
            loaded=nativeArticleModelScene_Load(c->source,slot,(unsigned)args[1].as.i64);
        }else if(draw){
            uint8_t mask=(uint8_t)(1u<<(slot-1));
            if(args[1].as.u64)s.item->native_model_draw_disabled_mask&=(uint8_t)~mask;
            else s.item->native_model_draw_disabled_mask|=mask;
        }else nativeArticleModel_Destroy(c->source,slot);
        if(!current(&s,c,d))return false;
        NativeArticleModelSceneSlot after;
        if(!nativeArticleModelScene_CaptureSlot(c->source,slot,&after))
            return error(d,"Secondary model slot retired before operation completion");
        if((draw||((load||clone)&&!loaded))&&!nativeArticleModelScene_SlotCurrent(&before))
            return error(d,"Secondary model operation changed a retained graph unexpectedly");
        *out=load||clone?(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=loaded}:
            (NativeScriptValue){.type=NativeScriptType_Void};
        return true;
    }
    if((semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM){
        if(!out||!args||count!=13||args[12].type!=NativeScriptType_Bool||args[12].as.u64>1)
            return error(d,"Item joint transform arguments are invalid");
        for(unsigned i=2;i<12;++i)if(args[i].type!=NativeScriptType_F32)
            return error(d,"Item joint transform requires float components");
        ModelSource s;HSD_JObj* node;unsigned channel;
        if(!capture(c,&s,true,d)||!joint(&s,semantic,args,2,&node,&channel,d))return false;
        bool finite=true;
        for(unsigned i=2;i<12;++i)if(!isfinite(args[i].as.f32))finite=false;
        if(node&&finite){
            Vec3 position={args[2].as.f32,args[3].as.f32,args[4].as.f32};
            Quaternion rotation={args[5].as.f32,args[6].as.f32,args[7].as.f32,args[8].as.f32};
            Vec3 scale={args[9].as.f32,args[10].as.f32,args[11].as.f32};
            if(args[12].as.u64){
                HSD_JObjSetTranslateWithMtxDirty(node,&position);
                HSD_JObjSetRotationWithMtxDirty(node,&rotation);
                HSD_JObjSetScaleWithMtxDirty(node,&scale);
            }else{
                HSD_JObjSetTranslate(node,&position);
                HSD_JObjSetRotation(node,&rotation);
                HSD_JObjSetScale(node,&scale);
            }
        }
        if(!current(&s,c,d))return false;
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if((semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION){
        if(!out||!args||count!=5||args[2].type!=NativeScriptType_F32||args[3].type!=NativeScriptType_F32||args[4].type!=NativeScriptType_F32)
            return error(d,"Item matrix rotation arguments are invalid");
        ModelSource s;HSD_JObj* node;unsigned channel;
        if(!capture(c,&s,true,d)||!joint(&s,semantic,args,2,&node,&channel,d)||channel!=11)return false;
        Vec3 rotation={0};HSD_ClassInfo* selected_class=node?HSD_CLASS_METHOD(node):NULL;
        if(node){if(!root_type(node,selected_class))return error(d,"Item matrix rotation selected joint class is invalid");
            HSD_MtxGetRotation(node->mtx,&rotation);
        }
        if(!current(&s,c,d))return false;
        HSD_JObj* retained;unsigned retained_channel;
        if(!joint(&s,semantic,args,2,&retained,&retained_channel,d)||retained!=node||retained_channel!=channel||
           (node&&!root_type(node,selected_class)))return error(d,"Item matrix rotation selected joint changed before publication");
        args[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=rotation.x};
        args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=rotation.y};
        args[4]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=rotation.z};
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if((semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK)==NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION){
        if(!out||!args||count!=5||args[2].type!=NativeScriptType_F32||args[3].type!=NativeScriptType_F32||args[4].type!=NativeScriptType_F32)
            return error(d,"Item world position arguments are invalid");
        ModelSource s;HSD_JObj* node;unsigned channel;
        if(!capture(c,&s,true,d)||!joint(&s,semantic,args,2,&node,&channel,d)||channel!=11)return false;
        Vec3 position={0};HSD_ClassInfo* selected_class=node?HSD_CLASS_METHOD(node):NULL;
        if(node){if(!root_type(node,selected_class))return error(d,"Item world position selected joint class is invalid");
            bool claimed=s.model!=0;
            if(claimed&&!nativeArticleModelScene_BeginMatrices(c->source,s.model))
                return error(d,"Item world position could not claim secondary matrix storage");
            lb_8000B1CC(node,NULL,&position);
            if(claimed&&!nativeArticleModelScene_EndMatrices())
                return error(d,"Item world position lost its retained secondary matrix claim");
        }
        if(!current(&s,c,d))return false;
        HSD_JObj* retained;unsigned retained_channel;
        if(!joint(&s,semantic,args,2,&retained,&retained_channel,d)||retained!=node||retained_channel!=channel||
           (node&&!root_type(node,selected_class)))return error(d,"Item world position selected joint changed before publication");
        args[2]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.x};
        args[3]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.y};
        args[4]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=position.z};
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    bool advance=semantic==NativeScriptBinding_ItemAdvanceAnimation||semantic==NativeScriptBinding_ItemAdvanceModelAnimation;
    bool bake=semantic==NativeScriptBinding_ItemSetAppearance;
    bool reset=semantic==NativeScriptBinding_ItemResetSpin;
    bool inherit=semantic==NativeScriptBinding_ItemInheritOwnerScale;
    bool attribute=semantic==NativeScriptBinding_ItemScaleByAttribute;
    bool instance=semantic==NativeScriptBinding_ItemSetInstanceScale;
    bool hide=semantic==NativeScriptBinding_ItemHideModel;
    bool show=semantic==NativeScriptBinding_ItemShowModel;
    bool rate=semantic==NativeScriptBinding_ItemSetModelAnimationRate;
    if(!out||((advance||reset||inherit||hide||show)?count!=0:bake?count!=2||!args||args[0].type!=NativeScriptType_I32||
        args[0].as.i64<INT32_MIN||args[0].as.i64>INT32_MAX||args[1].type!=NativeScriptType_F32:
        count!=1||!args||args[0].type!=NativeScriptType_F32)||
       (!advance&&!bake&&!reset&&!inherit&&!attribute&&!instance&&!hide&&!show&&!rate&&semantic!=NativeScriptBinding_ItemSetScale&&semantic!=NativeScriptBinding_ItemSetAnimationFrame))return error(d,"Item primary model operation arguments are invalid");
    ModelSource s;if(!capture(c,&s,false,d))return false;
    if(hide)it_8026BB44(c->source);
    else if(show)it_8026BB20(c->source);
    else if(rate)lb_8000BA0C(s.root,args[0].as.f32);
    else if(reset){
        const ItemModelDesc* descriptor=s.program->article->x10_modelDesc;
        if(!descriptor)return error(d,"Item spin reset owned model descriptor is absent");
        HSD_JObj* selected=spin_joint(&s,descriptor);
        HSD_ClassInfo* selected_class=selected?HSD_CLASS_METHOD(selected):NULL;
        if(!root_type(selected,selected_class)||(selected->flags&JOBJ_USE_QUATERNION))
            return error(d,"Item spin reset requires its selected Euler joint");
        uint8_t bits=descriptor->xC_bit_field;
        it_80274740(c->source);
        if(!current(&s,c,d))return false;
        if(s.program->article->x10_modelDesc!=descriptor||descriptor->xC_bit_field!=bits||
           spin_joint(&s,descriptor)!=selected||!root_type(selected,selected_class))
            return error(d,"Item spin descriptor or selected joint changed before completion");
    }
    else if(attribute){
        ItemAttr* attributes=s.program->article->x0_common_attr;
        if(!attributes||s.item->xCC_item_attr!=attributes)return error(d,"Item attribute scale requires its exact owned attributes");
        it_8026BAE8(c->source,args[0].as.f32);
        if(!current(&s,c,d))return false;
        if(s.program->article->x0_common_attr!=attributes||s.item->xCC_item_attr!=attributes)
            return error(d,"Item scale attributes changed before completion");
    }
    else if(instance)it_80274484(c->source,s.root,args[0].as.f32);
    else if(inherit){
        ScaleOwner owner;if(!scale_owner(&s,&owner,d))return false;
        it_80274594(c->source);
        if(!current(&s,c,d))return false;
        if(!scale_owner_current(&owner,&s))return error(d,"Item scale owner retired or changed before completion");
    }
    else if(bake){if(!appearance(&s,c,(int32_t)args[0].as.i64,args[1].as.f32,d))return false;}
    else if(advance){
        HSD_JObjAnimAll(s.root);if(!current(&s,c,d))return false;
        if(semantic==NativeScriptBinding_ItemAdvanceAnimation){
            float frame=lbGetJObjCurrFrame(s.root);if(!current(&s,c,d))return false;
            s.item->x5CC_currentAnimFrame=frame;
            it_802799E4(c->source);if(!current(&s,c,d))return false;
        }
    }
    else if(semantic==NativeScriptBinding_ItemSetScale){Vec3 scale={args[0].as.f32,args[0].as.f32,args[0].as.f32};HSD_JObjSetScale(s.root,&scale);}
    else {
        lb_8000BA0C(s.root,s.item->x5D0_animFrameSpeed);if(!current(&s,c,d))return false;
        HSD_JObjReqAnimAll(s.root,args[0].as.f32);if(!current(&s,c,d))return false;
        HSD_JObjAnimAll(s.root);if(!current(&s,c,d))return false;
        float frame=lbGetJObjCurrFrame(s.root);if(!current(&s,c,d))return false;s.item->x5CC_currentAnimFrame=frame;
    }
    if(!current(&s,c,d))return false;*out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
