#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/mp/native_collision_snapshot.h>
#include <melee/mp/native_collision_state_internal.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpisland.h>
#include <melee/gr/grdynamicattr.h>
#include <melee/gr/stage.h>
#include <melee/gr/ground.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/native_class_snapshot.h>
#include <sysdolphin/baselib/native_objalloc_snapshot.h>
#include <dolphin/os/OSAlloc.h>
#include <dolphin/os/OSHeapSnapshot.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, cases, refusals;
#define CHECK(test) do { ++checks; if (!(test)) { fprintf(stderr, "line %d: %s\n", __LINE__, #test); exit(1); } } while (0)
static uint8_t saved[262144], after[262144], malformed[262145];
static Vec2 vertices[6];
static MapLine lines[3];
static MapJoint joints[3];
static MapCollData map;
static HSD_JObj* platform;
static grDynamicAttr_UnkStruct* timed_attribute;
static OSHeapHandle heap;
static size_t snapshot_bytes;

typedef struct Observation {
    bool floor, speed_valid;
    int line, attribute;
    Vec3 position, normal, speed;
    u32 flags;
    mp_UnkStruct0* island;
} Observation;

static Observation observe(void)
{
    Observation result = {0};
    result.floor = mpCheckFloor(0, 40, 0, -10, 0, &result.position, &result.line,
        &result.flags, &result.normal, -1, -1, -1, NULL, NULL);
    Vec3 point = {0, 21, 0};
    result.speed_valid = mpGetSpeed(2, &point, &result.speed);
    result.attribute = grDynamicAttr_801CA284(&point, 2);
    result.island = mpIsland_8005AB54(2);
    return result;
}

static void geometry(void)
{
    memset(&map, 0, sizeof(map)); memset(joints, 0, sizeof(joints));
    vertices[0]=(Vec2){-20,0}; vertices[1]=(Vec2){0,0};
    vertices[2]=(Vec2){0,0}; vertices[3]=(Vec2){20,0};
    vertices[4]=(Vec2){-5,16}; vertices[5]=(Vec2){5,16};
    for (unsigned i=0; i<3; ++i) {
        // Dynamic island records require the source's dynamic-kind bit as
        // well as the floor kind (mpIsland_8005B004 compares the low five bits).
        lines[i]=(MapLine){2*i,2*i+1,-1,-1,-1,-1,
            CollLine_Floor | (i==2 ? 0x10 : 0),0};
        joints[i].vtx_start=2*i; joints[i].vtx_count=2;
        joints[i].left_bound=vertices[2*i].x; joints[i].right_bound=vertices[2*i+1].x;
        joints[i].bottom_bound=joints[i].top_bound=vertices[2*i].y;
        if (i==2) { joints[i].dynamic_start=2; joints[i].dynamic_count=1; }
        else { joints[i].floor_start=i; joints[i].floor_count=1; }
    }
    // Alternate links cross native records whose flags were previously read
    // with the console's incorrect eight-byte stride.
    lines[0].next_id1=1; lines[1].prev_id1=0;
    map=(MapCollData){.verts=vertices,.vert_count=6,.lines=lines,.line_count=3,
        .floor_start=0,.floor_count=2,.dynamic_start=2,.dynamic_count=1,
        .joints=joints,.joint_count=3};
}

static void initialize(void)
{
    geometry(); stage_info.grkind=Gr_Kind_Battle;
    mpLibLoad(&map);
    CHECK(groundCollVtx==nativeCollisionVertices && groundCollLine==nativeCollisionLines);
    CHECK(groundCollJoint==nativeCollisionJoints);
    CHECK(mpLinesConnected(1,0) && mpLinesConnected(0,1));
    groundCollLine[0].flags &= ~LINE_FLAG_ENABLED;
    CHECK(!mpLinesConnected(1,0)); groundCollLine[0].flags |= LINE_FLAG_ENABLED;
    groundCollLine[0].flags |= LINE_FLAG_HIDDEN;
    CHECK(!mpLinesConnected(1,0)); groundCollLine[0].flags &= ~LINE_FLAG_HIDDEN;
    groundCollVtx[2].pos.x=3;
    CHECK(!mpLinesConnected(1,0)); groundCollVtx[2].pos.x=0;
    CHECK(mpLinesConnected(1,0));
    platform=HSD_JObjAlloc(); CHECK(platform);
    HSD_JObjSetScaleX(platform,1); HSD_JObjSetScaleY(platform,1); HSD_JObjSetScaleZ(platform,1);
    HSD_JObjSetTranslateX(platform,2); HSD_JObjSetTranslateY(platform,5);
    // This fixture supplies the retained model binding directly. The actual
    // matrix update, collision transformation and island rebuild execute below.
    groundCollJoint[2].x20=platform;
    mpLib_80055E9C(2);
    CHECK(groundCollVtx[4].pos.x==-3 && groundCollVtx[4].pos.y==21);
    Vec3 position={0,21,0};
    timed_attribute=grDynamicAttr_801CA0F8(7,&position,2,20,2); CHECK(timed_attribute);
    CHECK(grDynamicAttr_801CA0F8(9,&position,0,20,-1));
    Observation result=observe();
    CHECK(result.floor && result.line==2 && result.position.y==21);
    CHECK(result.speed_valid);
    CHECK(result.attribute==7);
    CHECK(result.island);
    CHECK(nativeCollisionState_Save(saved,sizeof(saved),&snapshot_bytes));
    CHECK(nativeCollisionState_ValidateSnapshot(saved,snapshot_bytes));
    ++cases;
}

static void mutate(void)
{
    HSD_JObjSetTranslateY(platform,11); mpLib_80055E9C(2);
    mpJointHide(1); mpLib_80057BC0(0);
    grDynamicAttr_801CA224(); grDynamicAttr_801CA224();
    CHECK(grDynamicAttr_801CA284(&(Vec3){0,21,0},2)==0);
    CHECK(nativeCollisionAttributeActive!=timed_attribute);
    for (grDynamicAttr_UnkStruct* free=nativeCollisionAttributeFree; free; free=free->next)
        CHECK(free!=timed_attribute);
    mpBoundingCheck(-1,-1,1,1);
    // Source descriptions are writable: the engine itself updates alternate
    // links and platform flags. Restore must own them as well as runtime lines.
    lines[0].next_id1=-1; lines[1].prev_id1=-1;
    vertices[0].x=-33; joints[0].left_bound=-33;
    mpLib_80458868[1]=(mpCollisionBox){12,13,14,15};
    mpLib_804D64CC=17;
    nativeCollisionQuery.line_id=2; nativeCollisionQuery.flags=0x9876;
    nativeCollisionQuery.scratch.right[4]=32;
}

static void reject(const void* data, size_t bytes)
{
    size_t before_bytes;
    CHECK(nativeCollisionState_Save(after,sizeof(after),&before_bytes));
    CHECK(!nativeCollisionState_ValidateSnapshot(data,bytes));
    CHECK(!nativeCollisionState_Load(data,bytes));
    size_t n;
    CHECK(nativeCollisionState_Save(malformed+1,sizeof(malformed)-1,&n));
    CHECK(n==before_bytes && !memcmp(after,malformed+1,n));
    ++refusals;
}

static void invalid_candidates(void)
{
    // Separate scratch for refusal evidence: reject() may overwrite malformed.
    uint8_t* bad=malloc(snapshot_bytes+1); CHECK(bad);
    for (unsigned mode=0; mode<14; ++mode) {
        memcpy(bad,saved,snapshot_bytes);
        NativeCollisionSnapshotHeader header;
        memcpy(&header,bad,sizeof(header));
        size_t n=snapshot_bytes;
        if (mode==0) --n; if (mode==1) ++n;
        if (mode==2) ++header.identity; if (mode==3) ++header.object_epoch;
        if (mode==4) header.source=(MapCollData*)(uintptr_t)1;
        if (mode==5) header.description.lines=(MapLine*)(uintptr_t)1;
        if (mode==6) header.description.line_count=NATIVE_COLLISION_LINES+1;
        if (mode==7) header.list_start=(CollJoint*)(uintptr_t)1;
        if (mode==8) header.list_end=NULL;
        if (mode==9) header.attribute_free=header.attribute_active;
        if (mode==10) header.attributes[0].next=&nativeCollisionAttributes[0];
        memcpy(bad,&header,sizeof(header));
        size_t line_offset=sizeof(header)+6*sizeof(CollVtx);
        size_t joint_offset=line_offset+3*sizeof(CollLine);
        if (mode==11) {
            CollLine row; memcpy(&row,bad+line_offset,sizeof(row)); row.x0=(MapLine*)(uintptr_t)1;
            memcpy(bad+line_offset,&row,sizeof(row));
        }
        if (mode==12) {
            CollJoint row; memcpy(&row,bad+joint_offset,sizeof(row)); row.next=&nativeCollisionJoints[0];
            memcpy(bad+joint_offset,&row,sizeof(row));
        }
        if (mode==13) {
            MapLine row=lines[0]; row.v0_idx=6;
            size_t offset=joint_offset+3*sizeof(CollJoint)+6*sizeof(Vec2);
            memcpy(bad+offset,&row,sizeof(row));
        }
        reject(bad,n);
    }
    free(bad);
    size_t untouched=101;
    CHECK(!nativeCollisionState_Save(vertices,sizeof(saved),&untouched) && untouched==101);
    CHECK(!nativeCollisionState_Save(groundCollVtx,sizeof(saved),&untouched));
    CHECK(!nativeCollisionState_Save(after,sizeof(after),(size_t*)lines));
    HSD_GObj_RenderCallbackDepth=1;
    CHECK(!nativeCollisionState_SnapshotBytes() && !nativeCollisionState_Load(saved,snapshot_bytes));
    HSD_GObj_RenderCallbackDepth=0;
    HSD_GObj* context=GObj_Create(77,0,0); CHECK(context);
    HSD_GObj* previous_context=HSD_GObj_804D7814;
    HSD_GObj_804D7814=context;
    CHECK(nativeCollisionState_SnapshotBytes()==snapshot_bytes);
    CHECK(nativeCollisionState_Save(after,sizeof(after),&untouched) && untouched==snapshot_bytes);
    CHECK(nativeCollisionState_Load(saved,snapshot_bytes) && HSD_GObj_804D7814==context);
    HSD_GObj_804D7814=previous_context;
    HSD_GObjFree(context);
    HSD_GObj_DelayedProcInfo.flags=1;
    CHECK(!nativeCollisionState_Load(saved,snapshot_bytes)); HSD_GObj_DelayedProcInfo.flags=0;
    ++cases;
}

static void connected_restore(void)
{
    Observation expected=observe();
    CHECK(nativeCollisionState_Save(saved,sizeof(saved),&snapshot_bytes));
    uint8_t* class_before=malloc(2*1024*1024),*class_after=malloc(2*1024*1024);
    uint8_t* obj_before=malloc(2*1024*1024),*obj_after=malloc(2*1024*1024);
    u32 os_capacity=OSHeapSnapshotBytes(heap),os_bytes;
    uint8_t* os_before=malloc(os_capacity),*os_after=malloc(os_capacity);
    CHECK(class_before&&class_after&&obj_before&&obj_after&&os_before&&os_after);
    size_t class_bytes,obj_bytes,n;
    CHECK(nativeClass_Save(class_before,2*1024*1024,&class_bytes));
    CHECK(nativeObjAlloc_Save(obj_before,2*1024*1024,&obj_bytes));
    CHECK(OSSaveHeapSnapshot(heap,os_before,os_capacity,&os_bytes));
    mutate();
    CHECK(nativeCollisionState_Save(after,sizeof(after),&n) && n==snapshot_bytes);
    CHECK(memcmp(after,saved,n));
    // Dynamic reclassification allocates a different island path. Destroy the
    // model too; companion graph/allocator ownership resurrects the saved JObj.
    mpLineSetPos(2,10,30,-10,30); mpLib_80055E24(2);
    groundCollJoint[2].x20=NULL; HSD_JObjRemoveAll(platform);
    HSD_JObj* future=HSD_JObjAlloc(); CHECK(future);
    CHECK(nativeCollisionState_ValidateSnapshot(saved,snapshot_bytes));
    CHECK(nativeClass_ValidateSnapshot(class_before,class_bytes));
    CHECK(nativeObjAlloc_ValidateSnapshot(obj_before,obj_bytes));
    CHECK(OSValidateHeapSnapshot(heap,os_before,os_bytes));
    CHECK(OSLoadHeapSnapshot(heap,os_before,os_bytes));
    CHECK(nativeObjAlloc_Load(obj_before,obj_bytes)); CHECK(nativeClass_Load(class_before,class_bytes));
    CHECK(nativeCollisionState_Load(saved,snapshot_bytes));
    CHECK(nativeCollisionState_Save(after,sizeof(after),&n) && n==snapshot_bytes && !memcmp(saved,after,n));
    CHECK(nativeClass_Save(class_after,2*1024*1024,&n) && n==class_bytes && !memcmp(class_before,class_after,n));
    CHECK(nativeObjAlloc_Save(obj_after,2*1024*1024,&n) && n==obj_bytes && !memcmp(obj_before,obj_after,n));
    u32 os_again;
    CHECK(OSSaveHeapSnapshot(heap,os_after,os_capacity,&os_again) && os_again==os_bytes && !memcmp(os_before,os_after,os_bytes));
    CHECK(groundCollJoint[2].x20==platform && timed_attribute->unk1C==2);
    Observation restored=observe(); CHECK(!memcmp(&expected,&restored,sizeof(expected)));
    mutate();
    CHECK(nativeCollisionState_Save(malformed,sizeof(malformed),&n) && n==snapshot_bytes && memcmp(after,malformed,n)!=0);
    // Recreate the exact same mutation from the same saved state, comparing all
    // collision payload/control bytes rather than only a semantic observation.
    CHECK(OSLoadHeapSnapshot(heap,os_before,os_bytes));
    CHECK(nativeObjAlloc_Load(obj_before,obj_bytes)); CHECK(nativeClass_Load(class_before,class_bytes));
    CHECK(nativeCollisionState_Load(saved,snapshot_bytes));
    observe(); mutate();
    CHECK(nativeCollisionState_Save(after,sizeof(after),&n) && n==snapshot_bytes && !memcmp(malformed,after,n));
    free(os_after); free(os_before); free(obj_after); free(obj_before); free(class_after); free(class_before);
    ++cases;
}


static void copy_only_history(void)
{
    static uint8_t frame_bytes[64*1024*1024];
    static NativeMatchArenaFrame frame;
    size_t bytes;
    CHECK(nativeCollisionState_Load(saved,snapshot_bytes));
    CHECK(nativeCollisionState_AdmitHistory(&nativeMatchSceneArena));
    CHECK(nativeMatchArena_BindFrame(&nativeMatchSceneArena,&frame,frame_bytes,sizeof(frame_bytes)));
    CHECK(nativeMatchArena_Save(&frame));
    for (unsigned repeat=0;repeat<2;++repeat) {
        // Exercise real collision/platform/attribute code and writable map data.
        mutate();
        CHECK(nativeCollisionState_Save(after,sizeof(after),&bytes));
        CHECK(bytes==snapshot_bytes && memcmp(after,saved,bytes));
        CHECK(nativeMatchArena_Load(&frame));
        CHECK(nativeCollisionState_Save(after,sizeof(after),&bytes));
        CHECK(bytes==snapshot_bytes && !memcmp(after,saved,bytes));
    }
    ++cases;
}

static void lifecycle(void)
{
    CHECK(nativeCollisionState_Load(saved,snapshot_bytes));
    uint32_t nan_bits=0x7fc01234;
    memcpy(&groundCollVtx[0].pos.x,&nan_bits,sizeof(nan_bits));
    size_t n; CHECK(nativeCollisionState_Save(after,sizeof(after),&n));
    groundCollVtx[0].pos.x=0;
    CHECK(nativeCollisionState_Load(after,n));
    uint32_t restored; memcpy(&restored,&groundCollVtx[0].pos.x,sizeof(restored)); CHECK(restored==nan_bits);
    CHECK(nativeCollisionState_Load(saved,snapshot_bytes));
    uint64_t epoch=nativeGObjPool.lifetime;
    // Same addresses and object epoch cannot make a previous map load valid.
    mpLibLoad(&map);
    CHECK(nativeGObjPool.lifetime==epoch && !nativeCollisionState_Load(saved,snapshot_bytes));
    MapCollData invalid=map; invalid.vert_count=NATIVE_COLLISION_VERTICES+1;
    CHECK(!nativeCollisionState_BeginScene(&invalid));
    invalid=map; invalid.joint_count=0; CHECK(!nativeCollisionState_BeginScene(&invalid));
    ++cases;
}

int main(void)
{
    enum { HEAP_BYTES=64*1024*1024 };
    uint8_t* memory=VirtualAlloc(NULL,HEAP_BYTES,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE); CHECK(memory);
    void* start=OSInitAlloc(memory,memory+HEAP_BYTES,1);
    heap=OSCreateHeap(start,memory+HEAP_BYTES); CHECK(heap>=0);
    HSD_SetHeap(heap); OSSetCurrentHeap(heap); HSD_ObjSetHeap(HEAP_BYTES,NULL); HSD_ObjInit(); HSD_IDSetup();
    ClassInfoInit(HSD_CLASS_INFO(&hsdJObj));
    HSD_GObjLibInitDataType config={.p_link_max=63,.gx_link_max=63,.gproc_pri_max=24};
    HSD_GObj_80391304(&config);
    initialize(); invalid_candidates(); connected_restore(); copy_only_history(); lifecycle();
    CHECK(nativeGObjPool_RetireScene()); OSDestroyHeap(heap);
    CHECK(VirtualFree(memory,0,MEM_RELEASE));
    printf("{\"status\":\"PASS\",\"cases\":%u,\"checks\":%u,\"refusals\":%u,\"snapshot_bytes\":%zu,\"native_line_stride\":%zu}\n",cases,checks,refusals,snapshot_bytes,sizeof(CollLine));
    return 0;
}
