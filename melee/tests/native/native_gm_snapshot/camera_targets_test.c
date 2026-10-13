#include <melee/cm/camera.h>
#include <melee/cm/camera_subjects_internal.h>
#include <melee/cm/native_camera_subject_snapshot.h>
#include <melee/gm/gm_1A45.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TestCameraSnapshot {
    NativeCameraSubjectSnapshotHeader header;
    CmSubject records[NATIVE_CAMERA_SUBJECT_CAPACITY];
} TestCameraSnapshot;
static TestCameraSnapshot saved, endpoint;
static CmSubject* targets[8];
static size_t bytes;
unsigned fixture_camera_checks, fixture_camera_refusals;
#define CHECK(x) do { ++fixture_camera_checks; if (!(x)) { \
    fprintf(stderr,"camera line %d: %s\n",__LINE__,#x);exit(1); } } while (0)

void fixture_camera_init(void)
{
    CameraSubjects_Init(70);
    CHECK(nativeCameraSubjects_SnapshotBytes() == sizeof(saved.header)+70*sizeof(CmSubject));
    for (unsigned i=0;i<8;++i) targets[i]=Camera_80029044(CmSubjectState_Auto);
    CHECK(cm_804D6460==targets[0] && cm_804D6468==targets[7]);
}

void fixture_camera_activity(void)
{
    TestCameraSnapshot state;
    size_t written=313;
    CHECK(!nativeCameraSubjects_Save(&state,sizeof(state),&written) && written==313);
    CHECK(!nativeCameraSubjects_Validate(&saved,bytes));
    CHECK(!nativeCameraSubjects_Load(&saved,bytes));
}

void fixture_camera_tick(void)
{
    unsigned frame=gm_801A4BA8();
    for (unsigned i=0;i<8;++i) {
        targets[i]->pos.x=(float)frame+i;
        targets[i]->pos.y=(float)i-frame;
        targets[i]->ext.h.y=(float)frame/3;
        targets[i]->state_timer=(s16)frame;
    }
    // Releasing an interior target retains its old next link. Reallocation
    // moves it to the active tail without changing the stable borrowed address.
    if (frame%5==2) {
        Camera_800290D4(targets[4]);
        CHECK(Camera_80029020()==targets[4]);
    }
    CmSubject* transient=Camera_80029044(CmSubjectState_Inactive);
    transient->x60.z=(float)frame;
    Camera_800290D4(transient);
    cm_804D645C[69].x54.x=(float)frame;
}

static void reject(const TestCameraSnapshot* bad)
{
    TestCameraSnapshot before,after;
    size_t a,b;
    CHECK(nativeCameraSubjects_Save(&before,sizeof(before),&a));
    CHECK(!nativeCameraSubjects_Validate(bad,bytes));
    CHECK(!nativeCameraSubjects_Load(bad,bytes));
    CHECK(nativeCameraSubjects_Save(&after,sizeof(after),&b));
    CHECK(a==b && !memcmp(&before,&after,a));
    ++fixture_camera_refusals;
}

static void malformed(void)
{
    TestCameraSnapshot bad=saved;
    ++bad.header.epoch;reject(&bad);
    bad=saved;++bad.header.object_epoch;reject(&bad);
    bad=saved;bad.header.count=69;reject(&bad);
    bad=saved;bad.header.reserved=1;reject(&bad);
    bad=saved;bad.header.backing=(CmSubject*)1;reject(&bad);
    bad=saved;bad.header.free_head=(CmSubject*)1;reject(&bad);
    bad=saved;bad.header.active_tail=targets[0];reject(&bad);
    bad=saved;bad.header.active_head=NULL;reject(&bad);
    bad=saved;bad.records[69].prev=&cm_804D645C[69];reject(&bad);
    bad=saved;bad.records[69].next=(CmSubject*)1;reject(&bad);
    bad=saved;bad.records[69].next=(CmSubject*)((char*)cm_804D645C+1);reject(&bad);
    bad=saved;bad.records[69].next=&cm_804D645C[70];reject(&bad);
    bad=saved;bad.records[69].state=(CmSubjectState)3;reject(&bad);
    bad=saved;bad.records[0].prev=targets[1];reject(&bad);
    bad=saved;bad.header.free_head=targets[0];reject(&bad);
    bad=saved;bad.records[69].prev=NULL;bad.header.free_head=NULL;reject(&bad);

    size_t written=313;
    CHECK(!nativeCameraSubjects_Save(&bad,bytes-1,&written) && written==313);
    CHECK(!nativeCameraSubjects_Save(&bad,sizeof(bad),(size_t*)&bad));
    CHECK(!nativeCameraSubjects_Save(cm_804D645C,sizeof(bad),&written));
    CHECK(!nativeCameraSubjects_Save(&bad,sizeof(bad),(size_t*)&cm_804D6458));
    CHECK(!nativeCameraSubjects_Load(&saved,bytes-1));
    CHECK(!nativeCameraSubjects_Load(NULL,bytes));
    // Unaligned candidate bytes are accepted without unaligned typed reads.
    unsigned char unaligned[sizeof(saved)+1];
    memcpy(unaligned+1,&saved,bytes);
    CHECK(nativeCameraSubjects_Validate(unaligned+1,bytes));
    CHECK(nativeCameraSubjects_Load(unaligned+1,bytes));

    // Free records retain next/history bytes. This is valid, not an active edge.
    Camera_800290D4(targets[0]);
    CHECK(targets[0]->next!=NULL);
    CHECK(nativeCameraSubjects_Save(&bad,sizeof(bad),&written));
    targets[0]->next=NULL;targets[0]->x60.x=913;
    CHECK(nativeCameraSubjects_Load(&bad,written));
    CHECK(targets[0]->next==bad.records[0].next && targets[0]->x60.x==bad.records[0].x60.x);
    CHECK(nativeCameraSubjects_Load(&saved,bytes));
    // Bad live topology must not write either destination or its byte count.
    CmSubject* next=cm_804D645C[69].next;
    cm_804D645C[69].next=(CmSubject*)1;
    memset(&bad,0xA5,sizeof(bad));written=313;
    CHECK(!nativeCameraSubjects_Save(&bad,sizeof(bad),&written) && written==313);
    for (unsigned i=0;i<sizeof(bad);++i) CHECK(((u8*)&bad)[i]==0xA5);
    cm_804D645C[69].next=next;
    CHECK(nativeCameraSubjects_Load(&saved,bytes));

    uint64_t lifetime=nativeGObjPool.lifetime;
    ++nativeGObjPool.lifetime;
    CHECK(!nativeCameraSubjects_Validate(&saved,bytes));
    CHECK(!nativeCameraSubjects_Load(&saved,bytes));
    nativeGObjPool.lifetime=lifetime;
    // Empty and full active chains are legal endpoints, not special cases that
    // may lose history. All allocation identities repeat after a restore.
    while (cm_804D6460) Camera_800290D4(cm_804D6460);
    CHECK(nativeCameraSubjects_Save(&bad,sizeof(bad),&written));
    for (unsigned i=0;i<70;++i) CHECK(Camera_80029020()!=NULL);
    CHECK(!cm_804D6458);
    CHECK(nativeCameraSubjects_Load(&bad,written));
    CHECK(!cm_804D6460 && !cm_804D6468 && cm_804D6458!=NULL);
    for (unsigned i=0;i<70;++i) CHECK(Camera_80029020()!=NULL);
    CHECK(nativeCameraSubjects_Save(&bad,sizeof(bad),&written));
    while (cm_804D6460) Camera_800290D4(cm_804D6460);
    CHECK(nativeCameraSubjects_Load(&bad,written));
    CHECK(!cm_804D6458 && cm_804D6460 && cm_804D6468);
    CHECK(nativeCameraSubjects_Load(&saved,bytes));
}

void fixture_camera_sample(unsigned frame,bool repeated,bool retired)
{
    if (retired) {
        CHECK(!nativeCameraSubjects_Validate(&saved,bytes));
        CHECK(!nativeCameraSubjects_Load(&saved,bytes));
        return;
    }
    if (frame==8 && !repeated) {
        CHECK(nativeCameraSubjects_Save(&saved,sizeof(saved),&bytes));
        malformed();
    }
    if (frame!=24) return;
    TestCameraSnapshot current;size_t current_bytes;
    CHECK(nativeCameraSubjects_Save(&current,sizeof(current),&current_bytes));
    if (!repeated) {
        memcpy(&endpoint,&current,current_bytes);
        CHECK(nativeCameraSubjects_Load(&saved,bytes));
    } else {
        CHECK(bytes==current_bytes && !memcmp(&endpoint,&current,bytes));
    }
}

void fixture_camera_retired(void)
{
    CHECK(!nativeCameraSubjects_Load(&saved,bytes));
    // Reinitializing equal-capacity storage retires history even at the same
    // address; all 70 records are again on the original free chain.
    CameraSubjects_Init(70);
    CHECK(cm_804D6458==cm_804D645C && !cm_804D6460 && !cm_804D6468);
    for (unsigned i=0;i<70;++i)
        CHECK(cm_804D645C[i].prev==(i==69 ? NULL : &cm_804D645C[i+1]));
    CHECK(!nativeCameraSubjects_Validate(&saved,bytes));
    CameraSubjects_Init(NATIVE_CAMERA_SUBJECT_CAPACITY);
    for (unsigned i=0;i<NATIVE_CAMERA_SUBJECT_CAPACITY;++i)
        CHECK(Camera_80029020()==&cm_804D645C[i]);
    CHECK(!cm_804D6458);
    CameraSubjects_Init(1);
    CHECK(Camera_80029020()==cm_804D645C && !cm_804D6458);
    Camera_800290D4(cm_804D645C);
    CHECK(cm_804D6458==cm_804D645C && !cm_804D6460 && !cm_804D6468);
}
