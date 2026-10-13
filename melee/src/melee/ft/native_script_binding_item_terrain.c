#include "native_script_binding_item_terrain.h"
#include <melee/it/native_item_lifetime.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/it_3F14.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>

typedef struct TerrainInvocation {
    struct TerrainInvocation* previous;
    NativeScriptRuntimeContext* context;
    NativeItemLifetime source;
    NativeScriptDiagnostic* diagnostic;
    bool failed;
} TerrainInvocation;

// HSD_GObjEvent has no userdata. This pointer only borrows the current C stack
// during the engine's synchronous call; nested native/VM calls push and restore
// their own scope. Like the shared VM scratch, it is never a scene resource.
static TerrainInvocation* active;

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
static bool current(const TerrainInvocation* call)
{
    NativeScriptRuntimeContext* c=call->context;
    const NativeItemLifetime* s=&call->source;
    return c->source==s->object&&c->source_class==NativeCallbackSource_Item&&
        c->generation==s->generation&&c->scene_epoch==s->reference.lifetime&&
        nativeItemLifetime_Current(s)&&nativeScriptBindings_SourceAlive(c)&&
        s->program->article==s->article&&s->program->kind==s->kind&&
        s->program->owner==s->data->native_script_owner&&
        s->program->author_kind==s->data->native_article_author_kind;
}
static void stage_event(HSD_GObj* object,uint32_t hook)
{
    TerrainInvocation* call=active;
    if(!call)return;
    if(object!=call->source.object||!current(call)){
        call->failed=true;error(call->diagnostic,"Item terrain callback lost its retained source");return;
    }
    NativeScriptValue result;
    NativeScriptVMStatus status=nativeScriptItem_RunHook(object,hook,NULL,0,&result);
    if(status!=NativeScriptVM_Ok||!current(call)){
        call->failed=true;error(call->diagnostic,"Item terrain authored hook failed or retired its source");
    }
}
static void stage_left(HSD_GObj* object)
{stage_event(object,NativeScriptItemHook_StageLeft);}
static void stage_landed(HSD_GObj* object)
{stage_event(object,NativeScriptItemHook_StageLanded);}
static void stage_contact(HSD_GObj* object)
{stage_event(object,NativeScriptItemHook_StageContact);}
static bool bounce_model(TerrainInvocation* call)
{
    const NativeItemLifetime* s=&call->source;
    const ItemModelDesc* model=s->article?s->article->x10_modelDesc:NULL;
    if(!model||s->object_kind!=HSD_GObj_JObjKind)
        return error(call->diagnostic,"Item landing requires its native model descriptor and JObj");
    unsigned depth=(model->xC_bit_field>>6)&3;
    HSD_JObj* node=(HSD_JObj*)s->model;
    for(unsigned i=0;i<=depth;++i){
        HSD_ClassInfo* cls=node?HSD_CLASS_METHOD(node):NULL;
        if(!node||!cls||cls->head.obj_size<(int)sizeof(*node)||
           !hsdObjIsDescendantOf((HSD_Obj*)node,HSD_CLASS_INFO(&hsdJObj)))
            return error(call->diagnostic,"Item landing spin joint lacks its native class and extent");
        if(!current(call)||s->article->x10_modelDesc!=model||((model->xC_bit_field>>6)&3)!=depth||
           HSD_CLASS_METHOD(node)!=cls)
            return error(call->diagnostic,"Item landing model changed during class admission");
        if(i<depth)node=node->child;
    }
    return true;
}
bool nativeScriptBindingItemTerrain_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    bool land=semantic==NativeScriptBinding_ItemLandOnStage;
    bool follow=semantic==NativeScriptBinding_ItemFollowGround;
    bool basic=semantic==NativeScriptBinding_ItemBounceToGround;
    bool reset=semantic==NativeScriptBinding_ItemBounceToGroundWithContactReset;
    bool slide=semantic==NativeScriptBinding_ItemSlideOnGround;
    bool contact=semantic==NativeScriptBinding_ItemRespondToStage;
    bool impact=semantic==NativeScriptBinding_ItemRespondToStageWithImpactResponse;
    if(!out||(!land&&!follow&&!basic&&!reset&&!slide&&!contact&&!impact)||count!=((reset||impact)?1u:0u)||
       ((reset||impact)&&(!args||args[0].type!=NativeScriptType_Bool)))
        return error(d,"Item terrain operation differs from its typed signature");
    TerrainInvocation call={.context=c,.diagnostic=d};
    if(!c||c->source_class!=NativeCallbackSource_Item||!c->scene_epoch||
       !nativeItemLifetime_Capture(c->source,&call.source)||!call.source.source_owned||
       call.source.removing||!current(&call))
        return error(d,"Item terrain requires its live exact owned source");
    if(call.source.data->x378_itemColl.x0_gobj!=call.source.object||
       call.source.data->x378_itemColl.x34_flags.b1234==1)
        return error(d,"Item terrain requires its actual Item collision actor/category");
    if(!call.source.attributes||!it_804D6D28)
        return error(d,"Item terrain requires its native Item/common attributes");
    if((basic||reset)&&!bounce_model(&call))return false;
    if(!current(&call))return error(d,"Item terrain source changed before native collision");
    call.previous=active;active=&call;
    if(land)it_8026E414(c->source,stage_landed);
    else if(follow)it_8026D62C(c->source,stage_left);
    else if(slide)it_8026E8C4(c->source,stage_landed,stage_left);
    else if(impact&&args[0].as.u64)it_8026E5A0(c->source,stage_contact);
    else if(contact||impact)it_8026E71C(c->source,stage_contact);
    else if(reset&&args[0].as.u64)it_8026E15C(c->source,stage_landed);
    else it_8026E248(c->source,stage_landed);
    active=call.previous;
    if(call.failed||!current(&call))return error(d,"Item terrain source retired before publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
