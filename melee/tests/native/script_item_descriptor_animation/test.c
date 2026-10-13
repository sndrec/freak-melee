// Controlled transition inputs; production runtime/heap/scene kernels unchanged.
#pragma clang section data = ".data"
#include <melee/ft/native_fighter_runtime.c>
#pragma clang section data = ".mstate$m"
#include <melee/lb/lbheap.c>
#include <melee/it/native_article_model_scene.c>
#pragma clang section data = ".data"
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_motion_policy.h>
#include <melee/ft/native_script_binding_item_animation.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_article_animation_live.h>
#include <melee/gm/native_rollback_regions.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/list.h>
#include <melee/it/item.h>
#include <melee/it/iteffect.h>
#include <melee/ef/efsync.h>
#include <melee/it/native_article_animation_texture.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <limits.h>
#include <stdarg.h>
#include <sysdolphin/baselib/random.h>
#include <math.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum { HEAP_BYTES=64*1024*1024, OBS_NODES=128, OBS_TRACKS=64 };
static unsigned checks,cases,comparisons,frames,restores;
static unsigned texture_plans;
static unsigned arrow_cases,spawns;
static Vec3 spawned_position;
static HSD_GObj* spawned_parent;
static unsigned char* heap_bytes;
static NativeSourcePackage candidate;
static NativeFighterProject project;
static NativeScriptDiagnostic diagnostic;
static const char* declaration;
static bool synthetic_siblings;
static int32_t default_descriptor=-1;

static Item* data;
static Item_GObj* objects[3];
static NativeRollbackRegionSet regions;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"animation VM %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)

// Explicit silent imported leaf. This proof observes native effect arguments;
// it does not execute efSync's particle/model allocation or renderer branches.
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* parent,...)
{
    CHECK(id==0x421);va_list args;va_start(args,parent);
    const Vec3* position=va_arg(args,const Vec3*);CHECK(position);
    spawned_position=*position;spawned_parent=parent;++spawns;va_end(args);
    return (EF_SpawnResult){.kind=EF_SPAWN_NONE};
}
const EF_ModelRecipe* efSync_ModelRecipe(s32 id)
{(void)id;CHECK(false);return NULL;}

static void destroy_item(void* opaque)
{
    Item* item=opaque;
    for(unsigned slot=1;slot<=3;++slot)nativeArticleModel_Destroy(item->entity,slot);
    nativeScriptItem_Detach(item->entity);
}
static void stage_candidate(const char* path)
{
    CHECK(nativeFighterProject_Open(path,&project));
    candidate.address=&candidate;candidate.target=Ft_Kind_CustomFirst;
    candidate.owned_input.assets=(NativeSourceAssetInput){project.fighter_dat,project.fighter_size,
        project.animation_dat,project.animation_size,project.costume_dat,project.costume_size};
    CHECK(nativeSourceAssets_Import(&project.facts,&candidate.owned_input.assets,Ft_Kind_CustomFirst,&candidate.assets,&diagnostic));
    CHECK(nativeSourceBank_Allocate(&candidate.compiler,&diagnostic));
    static char source[16384];
    snprintf(source,sizeof(source),
        "fighter:\n    behavior independent\narticle %s states 4:\n"
        "    state int visits\n    state int op\n    state int descriptor\n"
        "    state 0 anim -1:\n        on physics:\n"
        "            item.state.visits = item.state.visits + 1\n"
        "            if item.state.op == 0:\n                item.set_animation_descriptor(item.state.descriptor)\n"
        "            if item.state.op == 1:\n                item.add_animation_descriptor(item.state.descriptor)\n"
        "            if item.state.op == 2:\n                item.advance_model_animation(0)\n"
        "            if item.state.op == 3 or item.state.op == 4:\n"
        "                random.float()\n                random.float()\n                random.float()\n"
        "                let float effect_x = 0\n                let float effect_y = 0\n                let float effect_z = 0\n"
        "                if item.state.op == 3:\n                    item.joint_world_position(0, \"\", effect_x, effect_y, effect_z)\n"
        "                else:\n                    item.joint_world_position(0, \"0\", effect_x, effect_y, effect_z)\n"
        "                item.spawn_effect_at_position(1057, effect_x, effect_y, effect_z)\n"
        "    state 1 anim -1:\n        on physics:\n            pass\n"
        "    state 2 anim -1:\n        on physics:\n            pass\n"
        "    state 3 anim -1:\n        on physics:\n            pass\n",declaration);
    NativeScriptSourceTree* tree=&candidate.compiler.bank.tree;
    tree->source=source;tree->source_bytes=(uint32_t)strlen(source);
    CHECK(nativeScriptSource_Parse(tree,&diagnostic));
    CHECK(nativeSourceMotionPolicy_Prepare(tree,ftData_MotionStateList,&candidate.assets,&diagnostic));
    CHECK(nativeSourceEntityAssets_Prepare(&candidate.entities,&candidate.assets,&candidate.owned_input.assets,&candidate.resources,tree,&diagnostic));
    if(synthetic_siblings){
        // Controlled pinned descriptors, separate from the genuine asset runs.
        static uint8_t stream[]={HSD_A_OP_CON,0x3f,0x80,0,0,0};
        static HSD_FObjDesc track={.type=HSD_A_J_TRAX,.length=sizeof(stream),.ad=stream};
        static HSD_AObjDesc controller={.fobjdesc=&track};
        static HSD_Joint child,first,second;
        static HSD_AnimJoint child_anim,first_anim,second_anim;
        static ItemStateDesc descriptor;static ItemStateArray states;
        static ItemModelDesc model;static Article article;
        child=(HSD_Joint){.scale={1,1,1}};second=(HSD_Joint){.scale={1,1,1}};
        first=(HSD_Joint){.child=&child,.next=&second,.scale={1,1,1}};
        child_anim=(HSD_AnimJoint){.aobjdesc=&controller};second_anim=(HSD_AnimJoint){.aobjdesc=&controller};
        first_anim=(HSD_AnimJoint){.child=&child_anim,.next=&second_anim,.aobjdesc=&controller};
        descriptor=(ItemStateDesc){.x0_anim_joint=&first_anim};states=(ItemStateArray){.x0_itemStateDesc=&descriptor,.count=1};
        NativeScriptItemAsset* binding=NULL;
        for(uint32_t i=0;i<candidate.entities.count;++i)
            if(!strcmp(candidate.entities.rows[i].declaration,declaration))binding=&candidate.entities.rows[i];
        CHECK(binding);article=*binding->article;model=*article.x10_modelDesc;
        model.x0_joint=&first;article.x10_modelDesc=&model;article.xC_itemStates=&states;
        binding->article=&article;
    }
    candidate.metadata=(NativeScriptItemMetadataContext){.motions=&candidate.assets.facts.moves,
        .assets=candidate.entities.rows,.asset_count=candidate.entities.count,.source_facts=&candidate.assets.facts,.source_assets=&candidate.assets};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
        .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&candidate.metadata,.motion_layout=&candidate.assets.facts.moves};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&candidate.compiler.bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&candidate.assets.facts.moves,&candidate.metadata,&candidate.program,&diagnostic));
    CHECK(nativeSourceItemVisuals_Prepare(&candidate.visuals,&candidate.program.items,&diagnostic));
    candidate.parameters.items=&candidate.metadata;candidate.parameters.fighter.bytes=project.fighter_dat;
    candidate.parameters.fighter.fighter_root=candidate.assets.facts.fighter_root;
    candidate.input_memory=malloc(1);CHECK(candidate.input_memory);candidate.ready=true;
    CHECK(nativeSourcePackage_Pin(&candidate));CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&candidate.program);
    slots[0]=(NativeFighterSlot){.source=&candidate,.fighter_kind=Ft_Kind_CustomFirst,.active=true};
}
static Item_GObj* create_item(unsigned at)
{
    const NativeScriptItemProgram* program=nativeScriptItem_Program(Ft_Kind_CustomFirst,candidate.program.items.programs[0].owner);CHECK(program);
    Item_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
    data[at]=(Item){.entity=object,.kind=program->kind,.x1C=(s32)(at+1),.msid=-1,
        .native_article_author_kind=program->author_kind,.native_script_owner=program->owner,
        .xC8_joint=program->article->x10_modelDesc->x0_joint,.x5D0_animFrameSpeed=2.5f,
        .xC4_article_data=(Article*)program->article,.xCC_item_attr=program->article->x0_common_attr};
    GObj_InitUserData(object,6,destroy_item,&data[at]);CHECK(nativeScriptItem_Attach(object,program));
    CHECK(nativeScriptItem_SelectState(object,0));
    HSD_JObj* primary=HSD_JObjLoadJoint(program->article->x10_modelDesc->x0_joint);CHECK(primary&&primary->child);
    HSD_GObjObject_80390A70(object,HSD_GObj_JObjKind,primary);
    data[at].xBBC_dynamicBoneTable=lbHeap_80015BD0(0,sizeof(*data[at].xBBC_dynamicBoneTable));CHECK(data[at].xBBC_dynamicBoneTable);
    memset(data[at].xBBC_dynamicBoneTable,0,sizeof(*data[at].xBBC_dynamicBoneTable));
    data[at].xBBC_dynamicBoneTable->bones[0]=primary;data[at].xBBC_dynamicBoneTable->bones[1]=primary->child;
    return object;
}
static NativeScriptValue state(Item_GObj* object,const char* name,const NativeScriptValue* write)
{
    Item* item=object->user_data;NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,(uint32_t)item->x1C,&view));
    for(uint32_t n=0;n<view.program->value_count;++n)if(view.program->values[n].owner==view.storage->owner&&
        nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
        uint32_t semantic=NATIVE_SCRIPT_STATE_SEMANTIC|n;
        if(write)CHECK(nativeScriptStorage_Write(view.storage,semantic,NULL,0,write,&diagnostic));
        NativeScriptValue output;CHECK(nativeScriptStorage_Read(view.storage,semantic,NULL,0,&output,&diagnostic));return output;
    }
    CHECK(false);return (NativeScriptValue){0};
}
static void run(Item_GObj* object,int operation)
{
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=operation};state(object,"op",&value);
    nativeScriptItem_BeginTick(object);value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=73};
    memset(&diagnostic,0,sizeof(diagnostic));
    NativeScriptVMStatus status=nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Physics,&value);
    if(status!=NativeScriptVM_Ok)fprintf(stderr,"operation %d status %d\n",operation,status);
    CHECK(status==NativeScriptVM_Ok);
    CHECK(value.type==NativeScriptType_Void);++cases;
}
typedef struct NodeObservation {
    uint32_t kind,flags;Vec3 translation,scale;Quaternion rotation;
    HSD_Material material; HSD_PEDesc pe; const void* images; const void* image; int tlut;
    HSD_AObj controller;HSD_FObj tracks[OBS_TRACKS];uint32_t track_count;bool animated;
} NodeObservation;
typedef struct GraphObservation { NodeObservation nodes[OBS_NODES];uint32_t count; } GraphObservation;
static void graph_observe(HSD_JObj* joint,GraphObservation* result)
{
    for(;joint;joint=joint->next){
        CHECK(result->count<OBS_NODES);NodeObservation* node=&result->nodes[result->count++];
        node->flags=joint->flags;node->translation=joint->translate;node->scale=joint->scale;node->rotation=joint->rotate;
        if(joint->aobj){
            node->animated=true;node->controller=*joint->aobj;node->controller.fobj=NULL;node->controller.hsd_obj=NULL;
            for(HSD_FObj* track=joint->aobj->fobj;track;track=track->next){
                CHECK(node->track_count<OBS_TRACKS);node->tracks[node->track_count]=*track;
                node->tracks[node->track_count++].next=NULL;
            }
        }
        for(HSD_DObj* drawable=joint->u.dobj;drawable;drawable=drawable->next){
            HSD_MObj* material=drawable->mobj;if(!material)continue;
            CHECK(result->count<OBS_NODES);NodeObservation* m=&result->nodes[result->count++];
            m->kind=1;m->flags=material->rendermode;
            if(material->mat)m->material=*material->mat;
            if(material->pe)m->pe=*material->pe;
            if(material->aobj){m->animated=true;m->controller=*material->aobj;m->controller.fobj=NULL;m->controller.hsd_obj=NULL;
                for(HSD_FObj* f=material->aobj->fobj;f;f=f->next){CHECK(m->track_count<OBS_TRACKS);m->tracks[m->track_count]=*f;m->tracks[m->track_count++].next=NULL;}}
            for(HSD_TObj* t=material->tobj;t;t=t->next){
                CHECK(result->count<OBS_NODES);NodeObservation* v=&result->nodes[result->count++];
                v->kind=2;v->flags=t->flags;v->translation=t->translate;v->scale=t->scale;v->rotation=t->rotate;
                v->images=t->imagetbl;v->image=t->imagedesc;v->tlut=t->tlut_no;
                if(t->aobj){v->animated=true;v->controller=*t->aobj;v->controller.fobj=NULL;v->controller.hsd_obj=NULL;
                    for(HSD_FObj* f=t->aobj->fobj;f;f=f->next){CHECK(v->track_count<OBS_TRACKS);v->tracks[v->track_count]=*f;v->tracks[v->track_count++].next=NULL;}}
            }
        }
        if(joint->child&&!(joint->flags&JOBJ_INSTANCE))graph_observe(joint->child,result);
    }
}
static void same_graph(HSD_JObj* actual,HSD_JObj* expected)
{
    static GraphObservation left,right;memset(&left,0,sizeof(left));memset(&right,0,sizeof(right));
    graph_observe(actual,&left);graph_observe(expected,&right);CHECK(!memcmp(&left,&right,sizeof(left)));++comparisons;
}
static const NativeScriptItemProgram* program(void)
{return nativeScriptItem_Program(Ft_Kind_CustomFirst,candidate.program.items.programs[0].owner);}

static void paired(unsigned operation,int32_t index)
{
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=index};
    state(objects[0],"descriptor",&value);
    uint64_t revision=nativeScriptItem_Revision(objects[0]);
    CommandInfo command=data[0].x524_cmd;int motion=data[0].msid;
    float frame=data[0].x5CC_currentAnimFrame;
    ssize_t heap_before=OSCheckHeap(HSD_GetHeap());
    run(objects[0],(int)operation);
    const ItemStateArray* states=program()->article->xC_itemStates;
    if(operation==2)HSD_JObjAnimAll(objects[1]->hsd_obj);
    else if(states&&(uint32_t)index<states->count){
        data[1].xD0_itemStateDesc=&states->x0_itemStateDesc[index];
        if(operation==0)Item_80268D34(objects[1],data[1].xD0_itemStateDesc);
        else HSD_JObjAddAnimAll(objects[1]->hsd_obj,data[1].xD0_itemStateDesc->x0_anim_joint,
            data[1].xD0_itemStateDesc->x4_matanim_joint,data[1].xD0_itemStateDesc->x8_parameters);
    }
    same_graph(objects[0]->hsd_obj,objects[1]->hsd_obj);
    CHECK(data[0].xD0_itemStateDesc==data[1].xD0_itemStateDesc);
    CHECK(data[0].msid==motion&&nativeScriptItem_Revision(objects[0])==revision);
    CHECK(!memcmp(&command,&data[0].x524_cmd,sizeof(command)));
    CHECK(!memcmp(&frame,&data[0].x5CC_currentAnimFrame,sizeof(frame)));
    CHECK(OSCheckHeap(HSD_GetHeap())==heap_before);
}

static bool invoke(bool set,NativeScriptValue argument,NativeScriptValue* output)
{
    NativeScriptRuntimeContext context={.source=objects[0],.source_class=NativeCallbackSource_Item,
        .generation=(uint32_t)data[0].x1C,.scene_epoch=nativeGObjPool.lifetime};
    return nativeScriptBindingItemAnimation_Invoke(&context,set?NativeScriptBinding_ItemSetAnimationDescriptor:
        NativeScriptBinding_ItemAddAnimationDescriptor,&argument,1,output,&diagnostic);
}
static void reject(bool set,NativeScriptValue argument)
{
    static GraphObservation before,after;memset(&before,0,sizeof(before));memset(&after,0,sizeof(after));
    graph_observe(objects[0]->hsd_obj,&before);ItemStateDesc* selected=data[0].xD0_itemStateDesc;
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=73};
    CHECK(!invoke(set,argument,&value));CHECK(value.type==NativeScriptType_I32&&value.as.i64==73);
    CHECK(data[0].xD0_itemStateDesc==selected);
    graph_observe(objects[0]->hsd_obj,&after);CHECK(!memcmp(&before,&after,sizeof(before)));++cases;
}
static void unsafe_cases(void)
{
    NativeScriptValue argument={.type=NativeScriptType_I32,.as.i64=default_descriptor};
    reject(true,(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=(int64_t)INT32_MAX+1});
    reject(false,(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=(int64_t)INT32_MIN-1});
    reject(true,(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1});
    reject(false,(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0});
    HSD_Joint foreign=*data[0].xC8_joint;HSD_Joint* shadow=data[0].xC8_joint;
    data[0].xC8_joint=&foreign;reject(true,argument);data[0].xC8_joint=shadow;
    int count=data[0].x374_dynamicBonesNum;data[0].x374_dynamicBonesNum=25;
    reject(true,argument);data[0].x374_dynamicBonesNum=count;
    HSD_SList callback={0};nativeAnimationGlobals.end_callbacks=&callback;
    reject(true,argument);reject(false,argument);nativeAnimationGlobals.end_callbacks=NULL;
    nativeAnimationGlobals.callback_depth=1;reject(true,argument);nativeAnimationGlobals.callback_depth=0;
    uint32_t slot=nativeAObjPool.free_order[nativeAObjPool.free_count-1];
    uint32_t generation=nativeAObjPool.slots[slot].generation;
    nativeAObjPool.slots[slot].generation=UINT32_MAX;reject(true,argument);
    nativeAObjPool.slots[slot].generation=generation;
    static HSD_AObj* controllers[NATIVE_AOBJ_CAPACITY];unsigned allocated=0;
    while(nativeAObjPool.free_count)controllers[allocated++]=HSD_AObjAlloc();
    reject(true,argument);reject(false,argument);
    while(allocated)HSD_AObjFree(controllers[--allocated]);
    static HSD_FObj* tracks[NATIVE_FOBJ_CAPACITY];
    while(nativeFObjPool.free_count)tracks[allocated++]=HSD_FObjAlloc();
    reject(true,argument);reject(false,argument);
    while(allocated)HSD_FObjFree(tracks[--allocated]);
    HSD_JObj* joint=objects[0]->hsd_obj;
    while(joint&&!joint->aobj)joint=joint->child;
    CHECK(joint&&joint->aobj);const HSD_AObjDesc* origin=joint->aobj->native_origin;
    HSD_AObjDesc wrong=*origin;joint->aobj->native_origin=&wrong;reject(true,argument);reject(false,argument);
    joint->aobj->native_origin=origin;same_graph(objects[0]->hsd_obj,objects[1]->hsd_obj);
    HSD_JObj* stack[128]={objects[0]->hsd_obj};unsigned depth=1;HSD_MObj* material=NULL;
    while(depth&&!material){HSD_JObj* j=stack[--depth];
        if(j->next)stack[depth++]=j->next;if(j->child)stack[depth++]=j->child;
        for(HSD_DObj* d=j->u.dobj;d;d=d->next)if(d->mobj&&d->mobj->tobj){material=d->mobj;break;}}
    if(material){
        HSD_TObj external={0};HSD_ImageDesc image={0};external.imagedesc=&image;
        u32 mode=material->rendermode;CHECK(!tobj_shadows&&!tobj_toon);
        material->rendermode|=RENDER_SHADOW;tobj_shadows=&external;reject(true,argument);reject(false,argument);
        tobj_shadows=NULL;material->rendermode=mode|RENDER_TOON;tobj_toon=&external;
        reject(true,argument);reject(false,argument);tobj_toon=NULL;material->rendermode=mode;
        GXTexMapID id=material->tobj->id;material->tobj->id=(GXTexMapID)(id^1);
        reject(true,argument);reject(false,argument);material->tobj->id=id;
    }
    HSD_JObj* root=objects[0]->hsd_obj;objects[0]->hsd_obj=NULL;
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=73};
    ItemStateDesc* selected=data[0].xD0_itemStateDesc;
    CHECK(invoke(false,argument,&value)&&value.type==NativeScriptType_Void&&data[0].xD0_itemStateDesc==selected);
    CHECK(invoke(true,argument,&value)&&value.type==NativeScriptType_Void);
    CHECK(data[0].xD0_itemStateDesc==program()->animation_descriptors[default_descriptor].descriptor);
    objects[0]->hsd_obj=root;same_graph(root,objects[1]->hsd_obj);++cases;
}
static HSD_TObj* first_texture(HSD_JObj* root)
{
    HSD_JObj* stack[128]={root};unsigned depth=1;
    while(depth){HSD_JObj* joint=stack[--depth];
        if(joint->next)stack[depth++]=joint->next;
        if(joint->child)stack[depth++]=joint->child;
        for(HSD_DObj* d=joint->u.dobj;d;d=d->next)
            if(d->mobj&&d->mobj->tobj)return d->mobj->tobj;
    }
    return NULL;
}
static void bomb_history(void)
{
    if(!strstr(declaration,"Bomb"))return;
    CHECK(program()->animation_descriptor_count==4);
    unsigned transitions=0;
    for(int32_t initial=0;initial<2;++initial){
        CHECK(program()->animation_descriptors[initial].set_valid);
        CHECK(program()->animation_descriptors[3].set_valid);
        paired(0,initial);
        HSD_TObj* texture=first_texture(objects[0]->hsd_obj);CHECK(texture&&texture->imagetbl);
        for(unsigned frame=0;frame<16;++frame){
            HSD_ImageDesc* before=texture->imagedesc;paired(2,initial);
            if(texture->imagedesc!=before)++transitions;
        }
        // Native RemoveAnim keeps borrowed table and current selected image.
        HSD_ImageDesc** table=texture->imagetbl;HSD_ImageDesc* image=texture->imagedesc;
        for(unsigned i=0;i<2;++i)HSD_JObjRemoveAnimAll(objects[i]->hsd_obj);
        CHECK(texture->imagetbl==table&&texture->imagedesc==image);
        paired(0,3); // Retained source's fast-fuse descriptor from actual 0/1 resources.
        for(unsigned frame=0;frame<48;++frame){
            HSD_ImageDesc* before=texture->imagedesc;paired(2,3);
            if(texture->imagedesc!=before)++transitions;
        }
        table=texture->imagetbl;image=texture->imagedesc;
        for(unsigned i=0;i<2;++i)HSD_JObjRemoveAnimAll(objects[i]->hsd_obj);
        CHECK(texture->imagetbl==table&&texture->imagedesc==image);
        paired(1,3);paired(2,3);
        NativeScriptValue index={.type=NativeScriptType_I32,.as.i64=3};
        HSD_ImageDesc* foreign_table[2]={0};texture->imagetbl=foreign_table;reject(true,index);texture->imagetbl=table;
        HSD_ImageDesc foreign_image={0};image=texture->imagedesc;texture->imagedesc=&foreign_image;
        reject(true,index);texture->imagedesc=image;
        CHECK(texture->aobj&&texture->aobj->fobj);HSD_FObj* track=texture->aobj->fobj;
        float old=track->p1;track->p1=NAN;reject(true,index);track->p1=2;reject(true,index);track->p1=old;
        unsigned op=track->op_intrp;track->op_intrp=HSD_A_OP_LIN;reject(true,index);track->op_intrp=(u8)op;
    }
    CHECK(transitions>=4);
    printf("{\"observation\":\"bomb_initial_and_fast_fuse\",\"initial_descriptors\":[0,1],\"selected_descriptor\":3,\"image_transitions\":%u,\"remove_retains_resources\":true}\n",transitions);
    paired(0,default_descriptor);++cases;
}
static void preparation_cases(void)
{
    NativeScriptItemProgram prepared=*program();Article article={0};prepared.article=&article;prepared.owner=0;
    CHECK(nativeScriptBindingItemAnimation_PrepareDescriptors(prepared.bank,&prepared,NULL,0,&diagnostic));
    CHECK(!article.x10_modelDesc); // Unused modelless owner must not need a graph.
    prepared=*program();prepared.article=&article;
    CHECK(nativeScriptBindingItemAnimation_PrepareDescriptors(prepared.bank,&prepared,NULL,0,&diagnostic));
    CHECK(prepared.animation_descriptors_prepared&&!prepared.animation_descriptor_count&&!prepared.animation_states);
    prepared=*program();NativeScriptItemAnimationDescriptor records[32];
    CHECK(!nativeScriptBindingItemAnimation_PrepareDescriptors(prepared.bank,&prepared,records,0,&diagnostic));
    CHECK(nativeScriptBindingItemAnimation_PrepareDescriptors(prepared.bank,&prepared,records,32,&diagnostic));
    CHECK(prepared.animation_descriptor_count==program()->animation_descriptor_count);
    for(uint32_t i=0;i<prepared.animation_descriptor_count;++i)
        CHECK(!memcmp(&records[i],&program()->animation_descriptors[i],sizeof(records[i])));
    ++cases;
}
static void texture_plan_cases(void)
{
    for(uint32_t seed=0;seed<256;++seed){
        HSD_TObjDesc descriptors[12]={0};HSD_TObj native[12]={0};
        NativeArticleAnimationTextureID plan[12];uint32_t count=seed%13,written=73;
        for(uint32_t i=0;i<count;++i){
            uint32_t flags=((seed>>(i%8))&3)==0?TEX_COORD_TOON:
                ((seed+i)%5==0?TEX_BUMP:TEX_COORD_UV);
            descriptors[i].blend_flags=flags;descriptors[i].id=(GXTexMapID)((seed+i*3)%8);
            native[i].flags=flags;native[i].id=descriptors[i].id;
            if(i+1<count){descriptors[i].next=&descriptors[i+1];native[i].next=&native[i+1];}
        }
        HSD_MObjDesc material={.texdesc=count?descriptors:NULL};
        CHECK(nativeArticleAnimation_TextureIDs(&material,plan,12,&written)&&written==count);
        HSD_TObjAssignResources(count?native:NULL);
        for(uint32_t i=0;i<count;++i)CHECK(plan[i].descriptor==&descriptors[i]&&plan[i].id==native[i].id);
        ++texture_plans;
    }
    HSD_TObjDesc cycle={0};cycle.next=&cycle;HSD_MObjDesc material={.texdesc=&cycle};
    NativeArticleAnimationTextureID plan[12];uint32_t written=73;
    CHECK(!nativeArticleAnimation_TextureIDs(&material,plan,12,&written)&&written==73);
    cycle.next=NULL;material.rendermode=RENDER_SHADOW;
    CHECK(nativeArticleAnimation_TextureIDs(&material,plan,12,&written)&&written==1);
    material.rendermode=RENDER_TOON;
    CHECK(nativeArticleAnimation_TextureIDs(&material,plan,12,&written)&&written==1);
    // Deliberately synthetic: raw IDs 2/7/2 become 0/1/2. Only the third
    // texture receives this actual native controller, not both raw-ID matches.
    uint8_t stream[]={HSD_A_OP_CON,0x3f,0x80,0,0,0};
    HSD_FObjDesc track={.type=HSD_A_T_TRAU,.length=sizeof(stream),.ad=stream};
    HSD_AObjDesc controller={.fobjdesc=&track};HSD_TexAnim texture={.id=2,.aobjdesc=&controller};
    HSD_TObjDesc descriptors[3]={{.id=2},{.id=7},{.id=2}};
    HSD_TObj native[3]={{.id=2},{.id=7},{.id=2}};
    for(unsigned i=0;i<2;++i){descriptors[i].next=&descriptors[i+1];native[i].next=&native[i+1];}
    HSD_MObjDesc model={.texdesc=descriptors};HSD_DObjDesc drawable={.mobjdesc=&model};
    HSD_Joint root={.u.dobjdesc=&drawable};HSD_MatAnim anim={.texanim=&texture};
    HSD_MatAnimJoint joint={.matanim=&anim};NativeArticleAnimationDemand demand;
    CHECK(nativeArticleAnimation_Census(&root,NULL,&joint,NULL,&demand));
    CHECK(demand.controllers==1&&demand.tracks==1&&!demand.uncovered);
    NativeArticleAnimationOrigin origins[4];written=73;
    NativeArticleAnimationApplication app={.target=&root,.matanim=&joint};
    CHECK(nativeArticleAnimation_PrepareApplications(&root,&app,1,origins,4,&written));
    CHECK(written==1&&origins[0].target==&descriptors[2]&&origins[0].controller==&controller);
    HSD_TObjAssignResources(native);HSD_TObjAddAnimAll(native,&texture);
    CHECK(!native[0].aobj&&!native[1].aobj&&native[2].aobj&&native[2].aobj->fobj);
    CHECK(native[2].aobj->native_origin==&controller&&native[2].aobj->fobj->native_origin==&track);
    HSD_TObjRemoveAnimAll(native);
    // Native constant image-index conversion: fractions in (-1,0) select zero.
    HSD_ImageDesc images[2]={0};HSD_ImageDesc* table[2]={&images[0],&images[1]};
    uint8_t image_stream[]={HSD_A_OP_CON,0,0};
    HSD_FObjDesc image_track={.type=HSD_A_T_TIMG,.frac_value=HSD_A_FRAC_S8|1,
        .length=sizeof(image_stream),.ad=image_stream};
    HSD_AObjDesc image_controller={.fobjdesc=&image_track};
    texture=(HSD_TexAnim){.id=2,.aobjdesc=&image_controller,.imagetbl=table,.n_imagetbl=2};
    anim.texanim=&texture;
    image_stream[1]=255;CHECK(nativeArticleAnimation_Census(&root,NULL,&joint,NULL,&demand)&&!demand.uncovered);
    image_stream[1]=254;CHECK(nativeArticleAnimation_Census(&root,NULL,&joint,NULL,&demand)&&(demand.uncovered&NativeArticleAnimation_Channel));
    image_stream[1]=4;CHECK(nativeArticleAnimation_Census(&root,NULL,&joint,NULL,&demand)&&(demand.uncovered&NativeArticleAnimation_Channel));
    image_stream[1]=2;CHECK(nativeArticleAnimation_Census(&root,NULL,&joint,NULL,&demand)&&!demand.uncovered);
    image_stream[0]=HSD_A_OP_LIN;CHECK(nativeArticleAnimation_Census(&root,NULL,&joint,NULL,&demand)&&(demand.uncovered&NativeArticleAnimation_Channel));
    image_stream[0]=HSD_A_OP_CON;texture.imagetbl=NULL;
    CHECK(!nativeArticleAnimation_Census(&root,NULL,&joint,NULL,&demand));
    texture.imagetbl=table;texture.n_imagetbl=513;CHECK(!nativeArticleAnimation_Census(&root,NULL,&joint,NULL,&demand));
    ++cases;
}
static void arrow_position_cases(void)
{
    for(uint32_t seed=0;seed<64;++seed)for(unsigned child=0;child<2;++child){
        for(unsigned i=0;i<2;++i){HSD_JObj* root=objects[i]->hsd_obj;
            HSD_JObjSetTranslateX(root,3.25f+(float)seed);HSD_JObjSetTranslateY(root,-7.5f);
            HSD_JObjSetRotationZ(root,.125f*(float)seed);HSD_JObjSetScaleX(root,1.25f);
            HSD_JObjSetTranslateY(root->child,2.5f);}
        *seed_ptr=seed;uint32_t before=spawns;run(objects[0],child?4:3);
        CHECK(spawns==before+1&&spawned_parent==objects[0]);
        Vec3 expected=spawned_position;uint32_t expected_seed=*seed_ptr;
        *seed_ptr=seed;Vec3 zero={0};
        it_80278800(objects[1],0x421,(int)child,&zero,&zero,1,0);
        CHECK(spawns==before+2&&spawned_parent==objects[1]);
        CHECK(*seed_ptr==expected_seed&&!memcmp(&expected,&spawned_position,sizeof(expected)));
        CHECK(expected_seed!=seed);++arrow_cases;
    }
    // Restore transforms before native descriptor/history comparisons.
    paired(0,default_descriptor);
}
static void compiler_negatives(void)
{
    static const char* const bodies[]={
        "item.set_animation_descriptor(true)","item.add_animation_descriptor(0.0)",
        "item.set_animation_descriptor()","item.add_animation_descriptor(0, 1)",
        "item.set_animation_descriptor(4294967295)","item.add_animation_descriptor(\"0\")"};
    NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
    for(unsigned i=0;i<sizeof(bodies)/sizeof(*bodies);++i){
        char source[2048];snprintf(source,sizeof(source),
            "fighter:\n    behavior independent\narticle %s states 4:\n    state 0 anim -1:\n        on physics:\n            %s\n",
            declaration,bodies[i]);
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
            .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&candidate.metadata,.motion_layout=&candidate.assets.facts.moves};
        const NativeScriptCompileBank* accepted=&candidate.compiler.bank;memset(&diagnostic,0,sizeof(diagnostic));
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));
        CHECK(diagnostic.message[0]&&accepted==&candidate.compiler.bank);
    }
    nativeSourceBank_Release(&scratch);++cases;
}
static bool captured(const void* pointer,size_t bytes)
{
    for(uint32_t i=0;i<regions.count;++i){uintptr_t p=(uintptr_t)pointer,b=(uintptr_t)regions.regions[i].memory;
        if(p>=b&&p-b<=regions.regions[i].size&&bytes<=regions.regions[i].size-(p-b))return true;}
    return false;
}
static void history_step(unsigned frame)
{
    CHECK(nativeRollbackPages_BeginFrame(frame));
    if(frame==0){paired(1,default_descriptor);paired(0,default_descriptor);}
    if(frame==1){paired(1,default_descriptor);paired(2,default_descriptor);}
    if(frame==2){paired(0,default_descriptor);paired(1,default_descriptor);}
    CHECK(nativeRollbackPages_EndFrame());++frames;
}
static void history(void)
{
    CHECK(nativeRollbackRegions_Build(&regions));
    CHECK(captured(&scene,sizeof(scene))&&captured(data,sizeof(*data)*3));
    CHECK(captured(nativeScriptItemActors,sizeof(nativeScriptItemActors)));
    CHECK(captured(&nativeAObjPool,sizeof(nativeAObjPool))&&captured(nativeAObjPool.payload,nativeAObjPool.stride*nativeAObjPool.capacity));
    CHECK(captured(&nativeFObjPool,sizeof(nativeFObjPool))&&captured(nativeFObjPool.payload,nativeFObjPool.stride*nativeFObjPool.capacity));
    CHECK(!captured(&candidate,sizeof(candidate))&&!captured(&checks,sizeof(checks)));
    uint64_t expected[3],baseline=nativeRollbackRegions_Hash(&regions);
    CHECK(nativeRollbackPages_Init(regions.regions,regions.count,regions.exact_regions,regions.exact_count,4,512));
    for(unsigned frame=0;frame<3;++frame){history_step(frame);expected[frame]=nativeRollbackRegions_Hash(&regions);}
    CHECK(nativeRollbackPages_ToFrame(1,3));++restores;CHECK(nativeRollbackRegions_Hash(&regions)==expected[0]);
    for(unsigned frame=1;frame<3;++frame){history_step(frame);CHECK(nativeRollbackRegions_Hash(&regions)==expected[frame]);}
    CHECK(nativeRollbackPages_ToFrame(0,3));++restores;CHECK(nativeRollbackRegions_Hash(&regions)==baseline);
    for(unsigned frame=0;frame<3;++frame){history_step(frame);CHECK(nativeRollbackRegions_Hash(&regions)==expected[frame]);}
    nativeRollbackPages_Shutdown();++cases;
}
int main(int argc,char** argv)
{
    CHECK(argc==3||argc==4);synthetic_siblings=argc==4;declaration=argv[2];heap_bytes=VirtualAlloc(NULL,HEAP_BYTES,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(heap_bytes);
    void* start=OSInitAlloc(heap_bytes,heap_bytes+HEAP_BYTES,1);OSHeapHandle heap=OSCreateHeap(start,heap_bytes+HEAP_BYTES);CHECK(heap>=0);
    HSD_SetHeap(heap);OSSetCurrentHeap(heap);HSD_ObjSetHeap(HEAP_BYTES,NULL);HSD_ObjInit();HSD_IDSetup();
    lbHeap_80431FA0.heap_array[0]=(struct Heap){.id=heap,.type=LbHeapType_Hsd,.status=LbHeapStatus_Create,.start=(uintptr_t)heap_bytes,.size=HEAP_BYTES};
    data=lbHeap_80015BD0(0,sizeof(*data)*3);CHECK(data);memset(data,0,sizeof(*data)*3);
    HSD_GObjLibInitDataType config={.p_link_max=63,.gx_link_max=63,.gproc_pri_max=24};HSD_GObj_80391304(&config);
    stage_candidate(argv[1]);compiler_negatives();preparation_cases();texture_plan_cases();CHECK(nativeArticleModelScene_Prepare());
    objects[0]=create_item(0);objects[1]=create_item(1);
    CHECK(program()->animation_descriptors_prepared&&program()->animation_descriptor_count);
    if(synthetic_siblings)CHECK(program()->animation_descriptor_count==1);
    static union CmdUnion script[1];
    for(unsigned i=0;i<2;++i){data[i].x524_cmd.u=script;data[i].x5CC_currentAnimFrame=19.25f;}
    uint32_t admitted=0;
    for(uint32_t i=0;i<program()->animation_descriptor_count;++i){
        const NativeScriptItemAnimationDescriptor* record=&program()->animation_descriptors[i];
        printf("{\"observation\":\"descriptor_admission\",\"owner\":\"%s\",\"index\":%u,\"set\":%s,\"add\":%s,\"add_uncovered\":%u}\n",
            declaration,i,record->set_valid?"true":"false",record->add_valid?"true":"false",record->add.uncovered);
        NativeScriptValue index={.type=NativeScriptType_I32,.as.i64=i};
        if(!record->set_valid)reject(true,index);
        if(!record->add_valid)reject(false,index);
        if(record->set_valid&&record->add_valid){
            if(default_descriptor<0)default_descriptor=(int32_t)i;
            paired(0,(int32_t)i);paired(1,(int32_t)i);++admitted;
        }
    }
    CHECK(admitted);paired(0,default_descriptor);paired(1,default_descriptor);bomb_history();unsafe_cases();
    if(synthetic_siblings){
        HSD_JObj* root=objects[0]->hsd_obj;CHECK(root->next&&root->aobj&&root->child->aobj&&root->next->aobj);
        CHECK(program()->animation_descriptors[0].set.controllers==3&&program()->animation_descriptors[0].add.controllers==2);
        HSD_AObj* sibling=root->next->aobj;NativeMatchPoolReference reference;
        CHECK(nativeMatchPool_Reference(&nativeAObjPool,sibling,&reference));paired(1,default_descriptor);
        CHECK(root->next->aobj==sibling&&nativeMatchPool_Resolve(reference)==sibling);
        printf("{\"observation\":\"synthetic_root_siblings\",\"set_controllers\":3,\"add_controllers\":2,\"retained_on_add\":true}\n");
    }
    paired(0,-1);paired(1,INT32_MIN);paired(0,INT32_MAX);paired(1,(int32_t)program()->animation_descriptor_count);
    for(unsigned i=0;i<2;++i){data[i].x374_dynamicBonesNum=1;data[i].xD4_dynamicBones[0].skeleton=((HSD_JObj*)objects[i]->hsd_obj)->child;}
    paired(1,default_descriptor);paired(0,default_descriptor);CHECK(!((HSD_JObj*)objects[0]->hsd_obj)->child->aobj);
    for(unsigned i=0;i<2;++i)data[i].x374_dynamicBonesNum=0;
    paired(1,default_descriptor);paired(2,default_descriptor);arrow_position_cases();history();
    printf("{\"observation\":\"descriptor_origins\",\"owner\":\"%s\",\"descriptors\":%u,\"admitted\":%u,\"origins\":%u}\n",
        declaration,program()->animation_descriptor_count,admitted,program()->animation_origin_count);
    CHECK(nativeArticleModelScene_Retire());
    for(unsigned i=0;i<2;++i){lbHeap_80015CA8(0,data[i].xBBC_dynamicBoneTable);data[i].xBBC_dynamicBoneTable=NULL;HSD_GObjFree(objects[i]);}
    CHECK(!nativeAObjPool.used&&!nativeFObjPool.used);CHECK(nativeSourcePackage_Unpin(&candidate));slots[0].active=false;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));NativeScriptFighterProgram* empty=calloc(1,sizeof(*empty));CHECK(empty);
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,empty);free(empty);CHECK(nativeSourcePackage_Release(&candidate));
    nativeFighterProject_Close(&project);CHECK(nativeGObjPool_RetireScene());lbHeap_80015CA8(0,data);OSDestroyHeap(heap);CHECK(VirtualFree(heap_bytes,0,MEM_RELEASE));
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_graph_comparisons\":%u,\"texture_plans\":%u,\"arrow_cases\":%u,\"frames\":%u,\"restores\":%u}\n",checks,cases,comparisons,texture_plans,arrow_cases,frames,restores);return 0;
}
