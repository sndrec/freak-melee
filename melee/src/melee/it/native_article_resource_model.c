#include "native_article_resource_model.h"
#include "native_article_model_pool.h"
#include "native_article_animation_live.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/gm/native_rollback_pages.h>
#include <melee/gm/native_match_arena.h>
#include <melee/lb/lbheap.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/jobj.h>
#include <string.h>
#include <math.h>

enum { RESOURCE_GRAPHS=NATIVE_ARTICLE_RESOURCE_COLLECTIONS*NATIVE_ARTICLE_RESOURCE_NODES };
// Captured .mstate registry and main-heap backing are history-owned. Every node
// stores its actual graph generation; no secondary Item slot represents a node.
static uint64_t snapshot_identity;
static struct {
    NativeArticleModelPool pool;
    NativeSourcePackage* packages[Ft_Kind_RuntimeCapacity];
    uint64_t epoch;
    uint32_t package_count;
} scene;

static bool boundary(void)
{
    return !(nativeMatchSceneArena.lifetime && nativeMatchSceneArena.sealed) && !nativeRollbackPages_IsInitialized()&&nativeScriptRuntime_Idle()&&
        !nativeHsdReservation_Active()&&!HSD_GObj_CurrentInvokedProc&&
        !HSD_GObj_CurrentInvokedProcGObj&&!HSD_GObj_RenderCallbackDepth&&!HSD_GObj_804D7818&&
        !HSD_GObj_DelayedProcInfo.flags;
}

bool nativeArticleResourceModel_OwnerReady(const NativeItemLifetime* owner)
{
    if(!owner||!owner->source_owned||!nativeItemLifetime_Current(owner)||
       !scene.epoch||scene.epoch!=nativeGObjPool.lifetime||
       owner->reference.lifetime!=scene.epoch||owner->program->resource_capacity<2||
       owner->program->resource_capacity>NATIVE_ARTICLE_RESOURCE_NODES)return false;
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(owner->program->author_kind);
    bool leased=false;
    for(uint32_t i=0;i<scene.package_count;++i)if(scene.packages[i]==package)leased=true;
    if(!leased||!package||package->address!=package||!package->ready||!package->assets.ready||
       owner->bank!=&package->compiler.bank||
       nativeScriptItem_Program(owner->program->author_kind,owner->program->owner)!=owner->program||
       package->program.items.count>NATIVE_SCRIPT_ENTITY_LIMIT)return false;
    for(uint32_t i=0;i<package->program.items.count;++i){
        const NativeScriptItemProgram* p=&package->program.items.programs[i];
        if(p->owner==owner->program->owner&&p->identity==owner->program_identity&&
           p->bank==owner->bank&&p->article==owner->article&&p->kind==owner->kind&&
           p->model_roots==owner->program->model_roots&&p->model_root_count==owner->program->model_root_count&&
           p->model_animations==owner->program->model_animations&&p->model_animation_count==owner->program->model_animation_count&&
           p->resource_capacity==owner->program->resource_capacity&&
           p->resource_reserved_part==owner->program->resource_reserved_part&&
           p->resource_selector_count==owner->program->resource_selector_count)return true;
    }
    return false;
}

bool nativeArticleResourceModel_Prepare(void)
{
    if(!boundary()||scene.epoch||snapshot_identity==UINT64_MAX||!nativeGObjPool.lifetime)return false;
    const HSD_Joint* roots[NATIVE_ARTICLE_MODEL_POOL_ROOTS];uint32_t count=0;
    for(unsigned kind=0;kind<Ft_Kind_RuntimeCapacity;++kind){
        const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage((FighterKind)kind);
        if(!package)continue;
        bool seen=false,needed=false;
        for(uint32_t i=0;i<scene.package_count;++i)if(scene.packages[i]==package)seen=true;
        if(seen)continue;
        if(!package->ready||package->address!=package||package->program.items.count>NATIVE_SCRIPT_ENTITY_LIMIT)goto failure;
        for(uint32_t i=0;i<package->program.items.count;++i){
            const NativeScriptItemProgram* p=&package->program.items.programs[i];
            if(!p->resource_capacity)continue;
            if(p->resource_capacity<2||p->resource_capacity>NATIVE_ARTICLE_RESOURCE_NODES||
               p->model_root_count>NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT||
               (p->model_root_count&&!p->model_roots))goto failure;
            needed=true;
            for(uint32_t r=0;r<p->model_root_count;++r){
                const HSD_Joint* root=p->model_roots[r].root;
                if(!root)goto failure;
                bool present=false;
                for(uint32_t n=0;n<count;++n)if(roots[n]==root)present=true;
                if(!present){if(count==NATIVE_ARTICLE_MODEL_POOL_ROOTS)goto failure;roots[count++]=root;}
            }
        }
        if(needed){
            if(scene.package_count==Ft_Kind_RuntimeCapacity||!nativeSourcePackage_Pin((NativeSourcePackage*)package))goto failure;
            scene.packages[scene.package_count++]=(NativeSourcePackage*)package;
        }
    }
    if(count){
        size_t bytes,alignment;
        if(!nativeArticleModelPool_Size(roots,count,RESOURCE_GRAPHS,&bytes,&alignment))goto failure;
        void* backing=lbHeap_80015BD0(0,bytes);
        if(!backing)goto failure;
        if(((uintptr_t)backing&(alignment-1))||
           !nativeArticleModelPool_Bind(&scene.pool,backing,bytes,roots,count,RESOURCE_GRAPHS,nativeGObjPool.lifetime)){
            lbHeap_80015CA8(0,backing);goto failure;
        }
    }
    scene.epoch=nativeGObjPool.lifetime;++snapshot_identity;return true;
failure:
    while(scene.package_count){
        bool released=nativeSourcePackage_Unpin(scene.packages[--scene.package_count]);
        HSD_ASSERTREPORT(0,released,"Resource graph package lease failed to unwind");
    }
    memset(&scene,0,sizeof(scene));return false;
}

static bool graph_reference(HSD_JObj* root,NativeMatchPoolReference* output)
{
    for(uint32_t i=0;i<scene.pool.reservation.count;++i)
        if(nativeMatchPool_Reference(&scene.pool.reservation.bins[i].pool,root,output))return true;
    return false;
}

bool nativeArticleResourceModel_Remove(ResourceCollection* set,ResourceNode* node)
{
    if(!set||!node)return false;
    if(!node->model)return node->descriptor==NULL&&node->graph.pool==NULL;
    bool registered=false;
    for(unsigned c=0;c<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++c)
        if(set==&nativeArticleResourceCollections[c]&&set->created<=NATIVE_ARTICLE_RESOURCE_NODES)
            for(unsigned n=0;n<set->created;++n)if(node==&set->nodes[n])registered=true;
    if(!registered||!set->generation||node->generation!=set->generation||
       node->graph.lifetime!=scene.epoch||nativeHsdReservation_Active())return false;
    bool owned_pool=false;
    for(uint32_t i=0;i<scene.pool.reservation.count;++i)
        if(node->graph.pool==&scene.pool.reservation.bins[i].pool)owned_pool=true;
    if(!owned_pool)return false;
    HSD_JObj* root=node->model;
    void* graph=nativeMatchPool_Resolve(node->graph);
    if(graph&&graph!=root)return false;
    // Endpoint and graph generations retire independently. Clear borrowers
    // even after endpoint loss, but never dereference a reused endpoint or
    // free a graph whose captured allocation generation has already retired.
    nativeArticleResource_DetachBorrowers(set,node);
    HSD_GObj* object=nativeMatchPool_Resolve(node->object);
    ItemLink* link=nativeMatchPool_Resolve(node->payload);
    if(object&&link&&object->classifier==HSD_GOBJ_CLASS_ITEMLINK&&
       object->user_data==link&&link==node->link&&link->gobj==object&&
       object->hsd_obj==root&&object->obj_kind==HSD_GObj_JObjKind){
        object->hsd_obj=NULL;object->obj_kind=HSD_GOBJ_OBJ_NONE;
    }
    node->model=NULL;node->descriptor=NULL;node->root_index=UINT32_MAX;
    memset(&node->graph,0,sizeof(node->graph));
    if(graph)HSD_JObjRemoveAll(root);
    return true;
}

bool nativeArticleResourceModel_Retire(void)
{
    if(!boundary())return false;
    if(!scene.epoch)return true;
    for(uint32_t i=0;i<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++i){
        ResourceCollection* set=&nativeArticleResourceCollections[i];
        if(set->active_operations)return false;
        for(uint32_t n=0;n<set->created;++n)
            if(set->nodes[n].model&&!nativeArticleResourceModel_Remove(set,&set->nodes[n]))return false;
    }
    void* backing=scene.pool.backing;
    if(backing&&!nativeArticleModelPool_Release(&scene.pool))return false;
    if(backing)lbHeap_80015CA8(0,backing);
    while(scene.package_count){
        if(!nativeSourcePackage_Unpin(scene.packages[scene.package_count-1]))return false;
        --scene.package_count;
    }
    memset(&scene,0,sizeof(scene));return true;
}

static void draw_resource(HSD_GObj* object,int pass)
{
    for(uint32_t c=0;c<NATIVE_ARTICLE_RESOURCE_COLLECTIONS;++c){
        ResourceCollection* set=&nativeArticleResourceCollections[c];
        for(uint32_t n=0;n<set->created;++n){
            ResourceNode* node=&set->nodes[n];
            if(nativeMatchPool_Resolve(node->object)!=object)continue;
            ResourceCapture view;
            if(!nativeArticleResource_BeginOperation(set->owner.object,n,&view))return;
            bool visible=view.link->x2C_b0!=0;
            bool matrices=visible&&view.model&&nativeArticleModelPool_BeginMatrices(&scene.pool,view.model);
            if(matrices)HSD_GObj_JObjCallback(object,pass);
            bool complete=!matrices||nativeHsdReservation_End(&scene.pool.reservation);
            bool current=nativeArticleResource_EndOperation(&view);
            HSD_ASSERTREPORT(0,complete&&(!visible||matrices)&&current,"Resource draw lost its admitted graph");
            return;
        }
    }
}

bool nativeArticleResource_LoadModel(Item_GObj* owner,unsigned index,uint32_t root_index)
{
    ResourceCapture view;
    if(nativeHsdReservation_Active()||!nativeArticleResource_Capture(owner,index,&view)||
       !nativeArticleResourceModel_OwnerReady(&view.owner)||!view.owner.program->model_roots||
       root_index>=view.owner.program->model_root_count)return false;
    const HSD_Joint* descriptor=view.owner.program->model_roots[root_index].root;
    HSD_JObj* root=nativeArticleModelPool_Construct(&scene.pool,descriptor);
    if(!root)return false;
    NativeMatchPoolReference reference;
    bool retained=nativeArticleResource_Current(&view)&&graph_reference(root,&reference);
    HSD_ASSERTREPORT(0,retained,"Resource construction lost its retained node");
    if(!retained){HSD_JObjRemoveAll(root);return false;}
    if(!nativeArticleResourceModel_Remove(view.collection,view.node)){HSD_JObjRemoveAll(root);return false;}
    view.node->model=root;view.node->descriptor=descriptor;
    view.node->root_index=root_index;view.node->graph=reference;
    HSD_GObjObject_80390A70(view.gobj,HSD_GObj_JObjKind,root);
    // A node gets one GX registration; replacements retain its existing link.
    if(!view.gobj->render_cb)GObj_SetupGXLink(view.gobj,draw_resource,6,0);
    return true;
}

static HSD_JObj* joint(const ResourceCapture* view,const uint8_t* path,unsigned depth)
{
    if(depth>12||(depth&&!path)||!view->model||
       nativeMatchPool_Resolve(view->graph)!=view->model)return NULL;
    HSD_JObj* node=view->model;
    for(unsigned i=0;node&&i<depth;++i){node=node->child;for(unsigned n=0;node&&n<path[i];++n)node=node->next;}
    return node;
}

float nativeArticleResource_ReadJoint(Item_GObj* owner,unsigned index,
    const uint8_t* path,unsigned depth,unsigned channel)
{
    ResourceCapture view;if(!nativeArticleResource_Capture(owner,index,&view))return 0;
    HSD_JObj* node=joint(&view,path,depth);
    if(!node||channel>15)return 0;
    float value=nativeModelJoint_Read(node,channel);
    return nativeArticleResource_Current(&view)?value:0;
}

bool nativeArticleResource_WriteJoint(Item_GObj* owner,unsigned index,
    const uint8_t* path,unsigned depth,unsigned channel,float value)
{
    ResourceCapture view;
    if(!isfinite(value)||!nativeArticleResource_Capture(owner,index,&view))return false;
    HSD_JObj* node=joint(&view,path,depth);if(!node||channel>15||channel==9)return false;
    nativeModelJoint_Write(node,channel,value);return nativeArticleResource_Current(&view);
}

bool nativeArticleResource_SetWorldMatrix(Item_GObj* owner,unsigned index,const Mtx matrix)
{
    ResourceCapture view;
    if(!matrix||!nativeArticleResource_Capture(owner,index,&view)||!view.model)return false;
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)if(!isfinite(matrix[r][c]))return false;
    HSD_JObjCopyMtx(view.model,(MtxPtr)matrix);view.model->flags|=0x03800000;
    HSD_JObjSetMtxDirty(view.model);return nativeArticleResource_Current(&view);
}

bool nativeArticleResource_ReadWorldMatrix(Item_GObj* owner,unsigned index,Mtx output)
{
    ResourceCapture view;
    if(!output||!nativeArticleResource_Capture(owner,index,&view)||!view.model)return false;
    Mtx value;C_MTXCopy(view.model->mtx,value);
    if(!nativeArticleResource_Current(&view))return false;
    C_MTXCopy(value,output);return true;
}

uint64_t nativeArticleResourceModel_SnapshotIdentity(void)
{ return scene.epoch == nativeGObjPool.lifetime && scene.epoch ? snapshot_identity : 0; }

bool nativeArticleResourceModel_BufferIndependent(const void* buffer,size_t bytes)
{
    uintptr_t p=(uintptr_t)buffer;
    if(!buffer || bytes>UINTPTR_MAX-p ||
       (p<(uintptr_t)&scene+sizeof(scene) && (uintptr_t)&scene<p+bytes) ||
       (p<(uintptr_t)&snapshot_identity+sizeof(snapshot_identity) && (uintptr_t)&snapshot_identity<p+bytes))return false;
    for(unsigned i=0;i<scene.package_count;++i)
        if(p<(uintptr_t)scene.packages[i]+sizeof(NativeSourcePackage) && (uintptr_t)scene.packages[i]<p+bytes)return false;
    return true;
}

bool nativeArticleResourceModel_SnapshotProgram(const NativeItemLifetime* owner)
{
    if (!nativeArticleResourceModel_SnapshotIdentity() || !owner || !owner->source_owned) return false;
    for (unsigned i=0;i<scene.package_count;++i) {
        const NativeSourcePackage* package=scene.packages[i];
        if (!package || !package->ready || package->address!=package || !package->leases ||
            package->program.items.count>NATIVE_SCRIPT_ENTITY_LIMIT) return false;
        for (unsigned j=0;j<package->program.items.count;++j) {
            const NativeScriptItemProgram* row=&package->program.items.programs[j];
            const NativeScriptItemProgram* actual=nativeScriptItem_Program(row->author_kind,row->owner);
            if (actual!=owner->program) continue;
            return actual && actual->bank && actual->bank->valid &&
                actual->bank==owner->bank && actual->identity==owner->program_identity &&
                actual->article==owner->article && actual->kind==owner->kind &&
                owner->view.program==&actual->bank->declarations &&
                actual->bank->storage_layout.identity==owner->layout_identity &&
                actual->bank->storage_parameters.identity==owner->parameter_identity;
        }
    }
    return false;
}

bool nativeArticleResourceModel_SnapshotGraph(const ResourceNode* node,
                                              const NativeScriptItemProgram* program)
{
    if (!node->model) return !node->descriptor && !node->graph.pool && !node->bound_part && !node->bound_capture;
    if (!program || node->root_index>=program->model_root_count || !program->model_roots ||
        node->descriptor!=program->model_roots[node->root_index].root) return false;
    for (unsigned i=0;i<scene.pool.reservation.count;++i) {
        const NativeMatchPool* pool=&scene.pool.reservation.bins[i].pool;
        if (node->graph.pool!=pool) continue;
        return pool->lifetime==scene.epoch && node->graph.lifetime==scene.epoch &&
            node->graph.slot<pool->capacity && node->graph.generation &&
            pool->object_size>=sizeof(HSD_JObj) && pool->payload &&
            pool->payload+node->graph.slot*pool->stride==(uint8_t*)node->model;
    }
    return false;
}
