#include <melee/gr/native_stage_start_snapshot.h>
#include <melee/gr/stage_state_internal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NativeMatchPool nativeGObjPool;
StageInfo stage_info;
static bool idle=true;
static unsigned checks,callbacks;
bool nativeGmLoop_SnapshotBoundary(void) {return idle;}
static void first(HSD_GObj* object) {(void)object;++callbacks;}
static void second(HSD_GObj* object) {(void)object;++callbacks;}
static void unknown(HSD_GObj* object) {(void)object;++callbacks;}
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void)
{
    HSD_GObj objects[8];NativeMatchPoolSlot slots[8];uint32_t free_order[8];
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),slots,free_order,8,sizeof(*objects),_Alignof(HSD_GObj),37));
    NativeMatchPoolReference a,b;HSD_GObj* one=nativeMatchPool_Allocate(&nativeGObjPool,&a);
    HSD_GObj* two=nativeMatchPool_Allocate(&nativeGObjPool,&b);CHECK(one && two);
    CHECK(nativeStageStart_BeginScene());CHECK(nativeStageStart_AdmitCallback(first));
    CHECK(nativeStageStart_AdmitCallback(second));CHECK(!nativeStageStart_AdmitCallback(NULL));
    stage_start_state.nodes[0]=(GroundStartCallback){NULL,one,first};
    stage_start_state.nodes[1]=(GroundStartCallback){&stage_start_state.nodes[0],two,second};
    stage_start_state.count=2;stage_info.x6A4=&stage_start_state.nodes[1];
    _Alignas(8) unsigned char saved[1024],after[1024],bad[1024],pool[4096];size_t bytes=0,pool_bytes=0;
    CHECK(nativeStageStart_Save(saved,sizeof(saved),&bytes));
    CHECK(bytes==sizeof(NativeStageStartSnapshotHeader)+2*sizeof(NativeStageStartSnapshotRecord));
    CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,pool,sizeof(pool),&pool_bytes));
    NativeMatchPoolSnapshotView view=nativeMatchSnapshotPool_View(&nativeGObjPool,pool,pool_bytes);
    CHECK(nativeStageStart_ValidateObjects(saved,bytes,view));
    memset(&stage_start_state,0,sizeof(stage_start_state));stage_info.x6A4=NULL;stage_start_state.started=true;
    CHECK(nativeMatchPool_Free(&nativeGObjPool,b));
    CHECK(nativeStageStart_ValidateObjects(saved,bytes,view));
    CHECK(!nativeStageStart_Load(saved,bytes));CHECK(stage_start_state.started && !stage_info.x6A4);
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,pool,pool_bytes));
    CHECK(nativeStageStart_Load(saved,bytes));
    CHECK(stage_start_state.count==2 && !stage_start_state.started && stage_info.x6A4==&stage_start_state.nodes[1]);
    CHECK(stage_start_state.nodes[1].callback==second && stage_start_state.nodes[0].callback==first);
    CHECK(stage_start_state.nodes[1].next==&stage_start_state.nodes[0] && !stage_start_state.nodes[0].next);
    CHECK(stage_start_state.nodes[1].gobj==two && stage_start_state.nodes[0].gobj==one);
    size_t count;CHECK(nativeStageStart_Save(after,sizeof(after),&count) && count==bytes && !memcmp(saved,after,bytes));
#define BAD(change) do {memcpy(bad,saved,bytes);change;CHECK(!nativeStageStart_Validate(bad,bytes));CHECK(!nativeStageStart_Load(bad,bytes));CHECK(nativeStageStart_Save(after,sizeof(after),&count) && !memcmp(saved,after,bytes));}while(0)
    BAD(((NativeStageStartSnapshotHeader*)bad)->epoch++);
    BAD(((NativeStageStartSnapshotHeader*)bad)->count=NATIVE_GOBJ_CAPACITY+1);
    BAD(((NativeStageStartSnapshotHeader*)bad)->started=2);
    BAD(((NativeStageStartSnapshotHeader*)bad)->reserved=1);
    BAD(((NativeStageStartSnapshotRecord*)(bad+sizeof(NativeStageStartSnapshotHeader)))->callback=unknown);
    BAD(((NativeStageStartSnapshotRecord*)(bad+sizeof(NativeStageStartSnapshotHeader)))->object.generation=0);
    BAD(((NativeStageStartSnapshotRecord*)(bad+sizeof(NativeStageStartSnapshotHeader)))->object.slot=8);
#undef BAD
    idle=false;size_t unchanged=SIZE_MAX;CHECK(!nativeStageStart_Save(after,sizeof(after),&unchanged) && unchanged==SIZE_MAX);
    CHECK(!nativeStageStart_Load(saved,bytes));idle=true;
    CHECK(!nativeStageStart_Save(stage_start_state.nodes,sizeof(stage_start_state.nodes),&unchanged));
    GroundStartCallback* next=stage_start_state.nodes[1].next;
    stage_start_state.nodes[1].next=&stage_start_state.nodes[1];
    CHECK(!nativeStageStart_Save(after,sizeof(after),&unchanged));stage_start_state.nodes[1].next=next;
    CHECK(callbacks==0);
    CHECK(nativeStageStart_BeginScene());stage_info.x6A4=NULL;
    CHECK(!nativeStageStart_Load(saved,bytes)); // Old callback definitions retired.
    CHECK(nativeStageStart_Save(after,sizeof(after),&count) && count==sizeof(NativeStageStartSnapshotHeader));
    printf("stage-start snapshot checks=%u bytes=%zu callbacks=%u\n",checks,bytes,callbacks);
    return 0;
}
