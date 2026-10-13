#include <melee/gm/native_snapshot_actor_graph.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/fighter.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/item.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Slots=4,Bytes=4*sizeof(NativeFighterPoolSlot)+4096 };
NativeMatchPool nativeGObjPool,nativeFighterPool,nativeGroundPool,nativeItemPool;
static NativeMatchPool* pools[]={&nativeGObjPool,&nativeFighterPool,&nativeGroundPool,&nativeItemPool};
typedef struct Backing {
    _Alignas(32) uint8_t payload[Bytes];
    NativeMatchPoolSlot metadata[Slots];
    uint32_t free_order[Slots];
} Backing;
static Backing backing[4],before[4];
static NativeMatchPool descriptors[4];
static uint8_t saved[4][Bytes],bad[4][Bytes];
static size_t lengths[4];
static unsigned checks,rejected,callbacks;
#define CHECK(c) do {++checks;if(!(c)){fprintf(stderr,"line %u: %s\n",__LINE__,#c);exit(1);}}while(0)
void Fighter_Unload_8006DABC(void* p) {(void)p;++callbacks;abort();}
void Item_OnUserDataRemove(void* p) {(void)p;++callbacks;abort();}
void nativeGroundPool_Release(void* p) {(void)p;++callbacks;abort();}
static void reflection_remove(void* p) {(void)p;++callbacks;abort();}

static NativeSnapshotActorGraph views(uint8_t bytes[4][Bytes])
{
    return (NativeSnapshotActorGraph){
        nativeMatchSnapshotPool_View(pools[0],bytes[0],lengths[0]),
        nativeMatchSnapshotPool_View(pools[1],bytes[1],lengths[1]),
        nativeMatchSnapshotPool_View(pools[2],bytes[2],lengths[2]),
        nativeMatchSnapshotPool_View(pools[3],bytes[3],lengths[3])};
}
static void untouched(void)
{
    CHECK(!memcmp(before,backing,sizeof(backing)));
    for(unsigned i=0;i<4;++i)CHECK(!memcmp(&descriptors[i],pools[i],sizeof(descriptors[i])));
    CHECK(callbacks==0);
}
int main(void)
{
    size_t sizes[]={sizeof(HSD_GObj),sizeof(NativeFighterPoolSlot),sizeof(Ground),sizeof(NativeItemPoolSlot)};
    for(unsigned i=0;i<4;++i)
        CHECK(nativeMatchPool_Bind(pools[i],backing[i].payload,sizeof(backing[i].payload),backing[i].metadata,
                                   backing[i].free_order,Slots,sizes[i],32,101));
    NativeMatchPoolReference root_refs[4],actor_refs[3];
    HSD_GObj* roots[4];
    for(unsigned i=0;i<4;++i) {roots[i]=nativeMatchPool_Allocate(pools[0],&root_refs[i]);CHECK(roots[i]);}
    NativeFighterPoolSlot* fighter=nativeMatchPool_Allocate(pools[1],&actor_refs[0]);
    Ground* ground=nativeMatchPool_Allocate(pools[2],&actor_refs[1]);
    NativeItemPoolSlot* item=nativeMatchPool_Allocate(pools[3],&actor_refs[2]);
    CHECK(fighter && ground && item);
    fighter->actor.gobj=roots[0];ground->gobj=roots[1];item->actor.entity=roots[2];
    *roots[0]=(HSD_GObj){.classifier=HSD_GOBJ_CLASS_FIGHTER,.user_data_kind=4,.user_data=fighter,.user_data_remove_func=Fighter_Unload_8006DABC};
    *roots[1]=(HSD_GObj){.classifier=HSD_GOBJ_CLASS_STAGE,.user_data_kind=3,.user_data=ground,.user_data_remove_func=nativeGroundPool_Release};
    *roots[2]=(HSD_GObj){.classifier=HSD_GOBJ_CLASS_ITEM,.user_data_kind=6,.user_data=item,.user_data_remove_func=Item_OnUserDataRemove};
    *roots[3]=(HSD_GObj){.classifier=HSD_GOBJ_CLASS_STAGE,.user_data_kind=3,.user_data=(void*)1,.user_data_remove_func=reflection_remove};
    for(unsigned i=0;i<4;++i)CHECK(nativeMatchSnapshotPool_Save(pools[i],saved[i],Bytes,&lengths[i]));
    NativeSnapshotActorGraph graph=views(saved);CHECK(nativeSnapshotActorGraph_Validate(&graph));
    // Mutate/reuse live slots, leaving the old candidate immutable. Its roots
    // must still agree without restoration or consulting current occupancy.
    CHECK(nativeMatchPool_Free(pools[0],root_refs[0]));
    NativeMatchPoolReference replacement;HSD_GObj* newer=nativeMatchPool_Allocate(pools[0],&replacement);
    CHECK(newer==roots[0]);newer->classifier=0xFFFF;
    CHECK(nativeMatchPool_Free(pools[1],actor_refs[0]));
    NativeMatchPoolReference newer_actor;
    CHECK(nativeMatchPool_Allocate(pools[1],&newer_actor)==fighter);
    CHECK(!nativeMatchPool_Resolve(root_refs[0]) && !nativeMatchPool_Resolve(actor_refs[0]));
    memcpy(before,backing,sizeof(before));for(unsigned i=0;i<4;++i)descriptors[i]=*pools[i];
    CHECK(nativeSnapshotActorGraph_Validate(&graph));untouched();
#define REJECT(change) do { \
    memcpy(bad,saved,sizeof(bad));NativeSnapshotActorGraph edit=views(bad); \
    void* r_data=(void*)nativeMatchSnapshotPool_Find(edit.objects,root_refs[0].slot); \
    void* e_data=(void*)nativeMatchSnapshotPool_Find(edit.objects,root_refs[3].slot); \
    void* f_data=(void*)nativeMatchSnapshotPool_Find(edit.fighters,actor_refs[0].slot); \
    void* g_data=(void*)nativeMatchSnapshotPool_Find(edit.ground,actor_refs[1].slot); \
    void* i_data=(void*)nativeMatchSnapshotPool_Find(edit.items,actor_refs[2].slot); \
    HSD_GObj r_value,e_value;Fighter f_value;Ground g_value;Item i_value; \
    memcpy(&r_value,r_data,sizeof(r_value));memcpy(&e_value,e_data,sizeof(e_value)); \
    memcpy(&f_value,f_data,sizeof(f_value));memcpy(&g_value,g_data,sizeof(g_value));memcpy(&i_value,i_data,sizeof(i_value)); \
    HSD_GObj* root=&r_value;HSD_GObj* extra=&e_value;Fighter* f=&f_value;Ground* g=&g_value;Item* it=&i_value; \
    (void)root;(void)extra;(void)f;(void)g;(void)it;change; \
    memcpy(r_data,&r_value,sizeof(r_value));memcpy(e_data,&e_value,sizeof(e_value)); \
    memcpy(f_data,&f_value,sizeof(f_value));memcpy(g_data,&g_value,sizeof(g_value));memcpy(i_data,&i_value,sizeof(i_value)); \
    edit=views(bad);CHECK(edit.objects.records && edit.fighters.records && edit.ground.records && edit.items.records); \
    CHECK(!nativeSnapshotActorGraph_Validate(&edit));untouched();++rejected; \
} while(0)
    REJECT(root->user_data=(void*)1);
    REJECT(root->user_data_kind=6);
    REJECT(root->user_data_remove_func=NULL);
    REJECT(root->classifier=HSD_GOBJ_CLASS_ITEM);
    REJECT(f->gobj=NULL);
    REJECT(f->gobj=roots[2]);
    REJECT(g->gobj=roots[0]);
    REJECT(it->entity=(void*)1);
    REJECT(*extra=*root);
    REJECT(extra->user_data_remove_func=nativeGroundPool_Release);
#undef REJECT
    NativeSnapshotActorGraph missing=graph;memset(&missing.items,0,sizeof(missing.items));
    CHECK(!nativeSnapshotActorGraph_Validate(&missing));untouched();
    ++nativeGroundPool.lifetime;CHECK(!nativeSnapshotActorGraph_Validate(&graph));
    CHECK(callbacks==0);
    printf("{\"checks\":%u,\"malformed_graphs\":%u,\"callbacks\":%u,\"old_candidate_after_live_reuse\":true}\n",checks,rejected,callbacks);
    return 0;
}
