#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/it/native_item_pool.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_rollback_state.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma clang section data = ""
static uint8_t *saved,*invalid,*after,*oracle;
static uint8_t object_saved[1024*1024];
static size_t buffer_bytes,sparse_bytes,full_bytes;
static unsigned checks,cases,callbacks,cancelled,detached;
static Item* retiring;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u: %s\n",(unsigned)__LINE__,#x);exit(1);}}while(0)
void OSReport(char* text,...){(void)text;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason)
{fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
void HSD_LObjRemoveAll(HSD_LObj* p){(void)p;abort();}
void HSD_JObjRemoveAll(HSD_JObj* p){(void)p;abort();}
void HSD_LObj_803668EC(HSD_LObj* p){(void)p;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* p){(void)p;abort();}
void HSD_JObjDispAll(HSD_JObj* p,Mtx m,u32 f,u32 r){(void)p;(void)m;(void)f;(void)r;abort();}
void HSD_FogSet(HSD_Fog* p){(void)p;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* p){(void)p;abort();}
void HSD_CObjEndCurrent(void){abort();}

// Resource and VM internals are controlled boundaries. The actual destructor
// must call them in order, with its still-live record and creation number.
void nativeArticleResource_CancelGeneration(Item_GObj* object,uint32_t generation)
{
    CHECK(retiring && retiring->entity==object && (uint32_t)retiring->x1C==generation);
    CHECK(nativeItemPool_Slot(retiring)!=NULL && cancelled==detached);++cancelled;
}
void nativeScriptItem_Detach(Item_GObj* object)
{
    CHECK(retiring && retiring->entity==object && nativeItemPool_Slot(retiring)!=NULL);
    CHECK(cancelled==detached+1);++detached;
}
// Exact complete model-table builder and userdata destructor from item.c.
#include "item_bodies.inc"

static void initialize(void)
{
    CHECK(nativeGObjPool_RetireScene());
    HSD_GObjLibInitDataType config;HSD_GObj_803912E0(&config);
    config.p_link_max=10;config.gx_link_max=3;config.gproc_pri_max=3;
    HSD_GObj_80391304(&config);
    CHECK(nativeItemPool.lifetime==nativeGObjPool.lifetime);
    CHECK(nativeItemPool.used==0 && nativeItemPool.capacity==NATIVE_GOBJ_CAPACITY);
    CHECK(nativeMatchPool_Validate(&nativeItemPool));
}
static Item* create(int id)
{
    // Actual constructor's allocation/publication sequence. Its article asset
    // selection, rendering registration and gameplay initialization are not run.
    HSD_GObj* root=GObj_Create(HSD_GOBJ_CLASS_ITEM,9,0);if(!root)return NULL;
    NativeMatchPoolReference reference;
    Item* actor=nativeMatchPool_Allocate(&nativeItemPool,&reference);
    if(!actor){HSD_GObjFree(root);return NULL;}
    NativeItemPoolSlot zero={0};CHECK(memcmp(actor,&zero,sizeof(zero))==0);
    actor->entity=root;actor->x1C=id;
    GObj_InitUserData(root,6,Item_OnUserDataRemove,actor);
    return actor;
}
static void remove_item(Item* actor)
{
    retiring=actor;HSD_GObjFree(actor->entity);retiring=NULL;
}
static void deferred_remove(HSD_GObj* root)
{
    size_t bytes=123;
    CHECK(!nativeItemPool_Save(after,buffer_bytes,&bytes) && bytes==123);
    CHECK(!nativeItemPool_RetireScene());
    unsigned used=nativeItemPool.used;
    retiring=root->user_data;HSD_GObjFree(root);
    CHECK(nativeItemPool.used==used && HSD_GObj_DelayedProcInfo.delay_remove_gobj);
    ++callbacks;
}
static void lifecycle(void)
{
    initialize();Item* a=create(1),*b=create(2);
    NativeMatchPoolReference retired,live;
    CHECK(nativeMatchPool_Reference(&nativeItemPool,a,&retired));
    memset(&nativeItemPool_Slot(a)->bones,0xAB,sizeof(DynamicBoneTable));
    HSD_GObj* root=a->entity;remove_item(a);
    CHECK(nativeItemPool.used==1 && nativeMatchPool_Resolve(retired)==NULL);
    CHECK(nativeItemPool_Slot(a)==NULL);
    a=create(3);CHECK(a->entity==root);
    CHECK(nativeMatchPool_Reference(&nativeItemPool,a,&live));
    CHECK(live.slot==retired.slot && live.generation==retired.generation+1);
    retiring=a;GObj_RemoveUserData(root);retiring=NULL;
    CHECK(nativeItemPool.used==1 && root->user_data==NULL);
    HSD_GObjFree(root);CHECK(nativeItemPool.used==1);
    HSD_GObj_SetupProc(b->entity,deferred_remove,0);HSD_GObj_80390CFC();retiring=NULL;
    CHECK(callbacks==1 && nativeItemPool.used==0 && nativeGObjProcPool.used==0);
    CHECK(cancelled==3 && detached==3 && nativeMatchPool_Validate(&nativeItemPool));++cases;
}
static void model_table(void)
{
    initialize();Item* a=create(4);NativeItemPoolSlot* slot=nativeItemPool_Slot(a);
    ItemModelDesc model={0};Article article={.x10_modelDesc=&model};a->xC4_article_data=&article;
    CHECK(Item_802682F0(a->entity) && a->xBBC_dynamicBoneTable==NULL);
    HSD_JObj nodes[101]={0};model.x4_bone_count=3;
    nodes[0].child=&nodes[1];nodes[1].parent=&nodes[0];nodes[1].next=&nodes[2];nodes[2].parent=&nodes[0];
    a->entity->hsd_obj=&nodes[0];
    CHECK(Item_802682F0(a->entity) && a->xBBC_dynamicBoneTable==&slot->bones);
    for(unsigned i=0;i<3;++i)CHECK(slot->bones.bones[i]==&nodes[i]);
    for(unsigned i=3;i<100;++i)CHECK(slot->bones.bones[i]==NULL);
    memset(nodes,0,sizeof(nodes));model.x4_bone_count=100;
    for(unsigned i=0;i<99;++i)nodes[i].next=&nodes[i+1];
    CHECK(Item_802682F0(a->entity));
    for(unsigned i=0;i<100;++i)CHECK(slot->bones.bones[i]==&nodes[i]);
    nodes[99].next=&nodes[100];CHECK(!Item_802682F0(a->entity));
    // Failed construction owns exactly the same one slot. Normal caller cleanup
    // retires it; no second allocation/free or spill beyond the 100 entries.
    CHECK(nativeItemPool.used==1 && slot->bones.bones[99]==&nodes[99]);
    a->entity->hsd_obj=NULL;remove_item(a);CHECK(nativeItemPool.used==0);++cases;
}
static void restore(void)
{
    initialize();Item* a=create(11),*b=create(12),*c=create(13);
    NativeItemPoolSlot* own=nativeItemPool_Slot(a);
    a->xBBC_dynamicBoneTable=&own->bones;own->bones.bones[99]=(HSD_JObj*)c->entity;
    a->owner=c->entity;a->pos.x=37.5f;a->native_article_links[3]=0x12345678;
    // The semantic digest excludes idle query scratch and disabled/empty
    // victim payload. The actual owner snapshot must still retain every byte.
    a->x378_itemColl.contact=(Vec3){31.0f,-12.0f,9.0f};
    a->x5D4_hitboxes[0].hit.hurt_coll_pos=(Vec3){-42.0f,17.0f,3.0f};
    a->x5D4_hitboxes[0].hit.coll_distance=0.25f;
    a->x5D4_hitboxes[0].hit.victims_1[0].x4=16;
    a->x5D4_hitboxes[1].hit.victims_2[0].victim=c->entity;
    a->x524_cmd.frame_count=40.0f;
    memset(a->xDD4_itemVar.native_article.state,0x5A,sizeof(a->xDD4_itemVar.native_article.state));
    remove_item(b);NativeItemPoolSlot expected=*own;
    NativeMatchPoolReference reference,future;
    CHECK(nativeMatchPool_Reference(&nativeItemPool,a,&reference));
    size_t object_bytes;CHECK(nativeItemPool_Save(saved,buffer_bytes,&sparse_bytes));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,object_saved,sizeof(object_saved),&object_bytes));
    CHECK(sparse_bytes==sizeof(NativeMatchPoolSnapshotHeader)+
        (2*NATIVE_ITEM_CAPACITY-2)*sizeof(uint32_t)+2*(sizeof(uint32_t)+sizeof(NativeItemPoolSlot)));
    NativeRollbackStateRegion region;CHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1);
    oracle=malloc(region.size);CHECK(oracle!=NULL);memcpy(oracle,region.memory,region.size);
    HSD_GObj* root=a->entity;remove_item(a);
    a=create(99);CHECK(a->entity==root);a->pos.x=-1;
    CHECK(nativeMatchPool_Reference(&nativeItemPool,a,&future));
    CHECK(nativeItemPool_ValidateSnapshot(saved,sparse_bytes));
    CHECK(nativeItemPool_Load(saved,sparse_bytes));
    CHECK(nativeMatchPool_Resolve(reference)==a && nativeMatchPool_Resolve(future)==NULL);
    CHECK(memcmp(a,&expected,sizeof(expected))==0);
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,object_saved,object_bytes));
    CHECK(a->entity->user_data==a && a->xBBC_dynamicBoneTable==&nativeItemPool_Slot(a)->bones);
    // Scheduler roots/graphs/resource state are not yet in an explicit aggregate.
    memcpy(region.memory,oracle,region.size);free(oracle);oracle=NULL;
    CHECK(nativeItemPool_Save(after,buffer_bytes,&object_bytes));
    CHECK(object_bytes==sparse_bytes && memcmp(saved,after,sparse_bytes)==0);
    remove_item(a);a=create(99);
    CHECK(nativeMatchPool_Reference(&nativeItemPool,a,&reference));
    CHECK(reference.slot==future.slot && reference.generation==future.generation);
    remove_item(a);remove_item(c);++cases;
}
static void reject(const void* snapshot,size_t bytes)
{
    size_t before_bytes,after_bytes;
    CHECK(nativeItemPool_Save(after,buffer_bytes,&before_bytes));
    CHECK(!nativeItemPool_ValidateSnapshot(snapshot,bytes));
    CHECK(!nativeItemPool_Load(snapshot,bytes));
    CHECK(nativeItemPool_Save(object_saved,sizeof(object_saved),&after_bytes));
    CHECK(after_bytes==before_bytes && memcmp(after,object_saved,before_bytes)==0);
}
static void invalid_candidates(void)
{
    initialize();Item* a=create(5);size_t bytes;
    CHECK(nativeItemPool_Save(saved,buffer_bytes,&bytes));
    NativeMatchPoolSnapshotHeader header;memcpy(&header,saved,sizeof(header));
    size_t offset=sizeof(header)+(header.capacity+header.free_count)*sizeof(uint32_t)+sizeof(uint32_t);
    HSD_GObj* bad_roots[]={NULL,(HSD_GObj*)1,(HSD_GObj*)(nativeGObjPool.payload+1),
        (HSD_GObj*)(nativeGObjPool.payload+nativeGObjPool.capacity*nativeGObjPool.stride)};
    for(unsigned i=0;i<sizeof(bad_roots)/sizeof(*bad_roots);++i) {
        memcpy(invalid,saved,bytes);memcpy(invalid+offset+offsetof(Item,entity),&bad_roots[i],sizeof(bad_roots[i]));
        reject(invalid,bytes);++cases;
    }
    DynamicBoneTable* wrong=&((NativeItemPoolSlot*)a)[1].bones;
    memcpy(invalid,saved,bytes);memcpy(invalid+offset+offsetof(Item,xBBC_dynamicBoneTable),&wrong,sizeof(wrong));
    reject(invalid,bytes);++cases;
    reject(saved,bytes-1);reject(saved,sizeof(header)-1);++cases;
    memcpy(invalid,saved,bytes);header.used=header.capacity+1;memcpy(invalid,&header,sizeof(header));reject(invalid,bytes);++cases;
    HSD_GObj* root=a->entity;a->entity=(HSD_GObj*)1;
    size_t untouched=456;CHECK(!nativeItemPool_Save(invalid,buffer_bytes,&untouched) && untouched==456);a->entity=root;
    root->user_data_kind=4;CHECK(!nativeItemPool_Save(invalid,buffer_bytes,&untouched));root->user_data_kind=6;
    root->user_data=NULL;CHECK(!nativeItemPool_Save(invalid,buffer_bytes,&untouched));root->user_data=a;
    a->xBBC_dynamicBoneTable=wrong;CHECK(!nativeItemPool_Save(invalid,buffer_bytes,&untouched));a->xBBC_dynamicBoneTable=NULL;
    CHECK(nativeItemPool_Save(invalid,buffer_bytes,&untouched));++cases;
    HSD_GObj_804D7818=root;CHECK(!nativeItemPool_Load(saved,bytes));HSD_GObj_804D7818=NULL;
    HSD_GObj_DelayedProcInfo.flags=1;CHECK(!nativeItemPool_Save(invalid,buffer_bytes,&untouched));HSD_GObj_DelayedProcInfo.flags=0;++cases;
    NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeItemPool,a,&reference));
    initialize();CHECK(nativeMatchPool_Resolve(reference)==NULL && !nativeItemPool_Load(saved,bytes));++cases;
}
static void exhaustion(void)
{
    initialize();
    for(unsigned i=0;i<NATIVE_ITEM_CAPACITY;++i)CHECK(create((int)i+1));
    CHECK(nativeGObjPool.used==NATIVE_GOBJ_CAPACITY && nativeItemPool.used==NATIVE_ITEM_CAPACITY);
    CHECK(create(1)==NULL);CHECK(nativeItemPool_Save(saved,buffer_bytes,&full_bytes));
    CHECK(full_bytes==sizeof(NativeMatchPoolSnapshotHeader)+NATIVE_ITEM_CAPACITY*(2*sizeof(uint32_t)+sizeof(NativeItemPoolSlot)));
    CHECK(nativeMatchPool_Validate(&nativeItemPool));
    while(HSD_GObjPLinkHead[9])remove_item(HSD_GObjPLinkHead[9]->user_data);
    CHECK(nativeItemPool.used==0 && nativeGObjPool.used==0);
    size_t bytes;CHECK(nativeItemPool_Save(saved,buffer_bytes,&bytes));
    CHECK(bytes==sizeof(NativeMatchPoolSnapshotHeader)+2*NATIVE_ITEM_CAPACITY*sizeof(uint32_t));
    CHECK(nativeItemPool_Load(saved,bytes));++cases;
}

#include "link_cases.h"

int main(void)
{
    buffer_bytes=sizeof(NativeMatchPoolSnapshotHeader)+NATIVE_ITEM_CAPACITY*(sizeof(NativeItemPoolSlot)+2*sizeof(uint32_t));
    saved=malloc(buffer_bytes);invalid=malloc(buffer_bytes);after=malloc(buffer_bytes);CHECK(saved && invalid && after);
    lifecycle();model_table();restore();invalid_candidates();exhaustion();link_lifecycle();link_restore();link_exhaustion();CHECK(nativeGObjPool_RetireScene());
    CHECK(nativeItemPool.lifetime==0 && nativeItemPool.used==0 && cancelled==detached);
    printf("{\"cases\":%u,\"checks\":%u,\"actor_bytes\":%zu,\"record_bytes\":%zu,\"capacity\":%u,\"sparse_snapshot_bytes\":%zu,\"full_snapshot_bytes\":%zu,\"link_record_bytes\":%zu,\"link_sparse_bytes\":%zu,\"link_full_bytes\":%zu}\n",
        cases,checks,sizeof(Item),sizeof(NativeItemPoolSlot),NATIVE_ITEM_CAPACITY,sparse_bytes,full_bytes,sizeof(ItemLink),link_sparse_bytes,link_full_bytes);
    free(saved);free(invalid);free(after);return 0;
}
