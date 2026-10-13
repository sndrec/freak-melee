#include <melee/gm/native_match_control_history.h>
#include "camera_subjects_internal.h"
#include "camera.h"
#include "native_camera_subject_snapshot.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/debug.h>
#include <stdlib.h>
#include <string.h>

// Actual camera target records and roots, retained in the page oracle while
// other owners migrate. No secondary copy or per-frame allocation.
static CmSubject subjects[NATIVE_CAMERA_SUBJECT_CAPACITY];
CmSubject* cm_804D6458;
CmSubject* cm_804D645C;
CmSubject* cm_804D6460;
CmSubject* cm_804D6468;

// Scene identity is host control and must not be rewound by the broad oracle.
#pragma clang section data = "" bss = ""
static struct {
    uint64_t epoch, object_epoch;
    NativeMatchPool objects;
    uint32_t count;
    unsigned activity;
    bool active;
} subject_scene;

void CameraSubjects_Init(int count)
{
    HSD_ASSERTREPORT(0, count > 0 && count <= NATIVE_CAMERA_SUBJECT_CAPACITY,
                     "Invalid native camera target capacity: %d", count);
    if (subject_scene.epoch == UINT64_MAX) abort();
    ++subject_scene.epoch;
    subject_scene.active = false;
    subject_scene.object_epoch = nativeGObjPool.lifetime;
    subject_scene.count = (uint32_t)count;
    ++subject_scene.activity;
    cm_804D6458 = subjects;
    cm_804D645C = subjects;
    memset(subjects, 0, (size_t)count*sizeof(*subjects));
    for (int i = 0; i < count-1; ++i) subjects[i].prev = &subjects[i+1];
    subjects[count-1].prev = NULL;
    cm_804D6460 = NULL;
    cm_804D6468 = NULL;
    --subject_scene.activity;
}

void Camera_80028F5C(CmSubject* subject, CmSubjectState state)
{
    if (subject != NULL) {
        subject->state = state;
        subject->pos.z = 0.0f;
        subject->pos.y = 0.0f;
        subject->pos.x = 0.0f;
        subject->bone_pos = subject->pos;
        subject->facing_dir = 0.0f;
        subject->on_ledge = false;
        subject->force_inactive = false;
        subject->was_framed = false;
        subject->state_timer = 0;
        subject->ext.h.x = -1.0f;
        subject->ext.h.y = 1.0f;
        subject->ext.v.x = 1.0f;
        subject->ext.v.y = -1.0f;
        subject->ext.v.z = 1.0f;
        subject->target_ext.h = subject->ext.h;
        subject->target_ext.v = subject->ext.v;
        subject->x54.x = 0.0f;
        subject->x54.y = 0.0f;
        subject->x54.z = 0.0f;
        subject->x60.x = 0.0f;
        subject->x60.y = 0.0f;
        subject->x60.z = 0.0f;
    }
}

CmSubject* Camera_80029020(void)
{
    return Camera_80029044(0);
}

CmSubject* Camera_80029044(int arg0)
{
    CmSubject* subject = cm_804D6458;

    if (cm_804D6458 == NULL) {
        OSReport("couldn't get CmSubject struct.\n", arg0);
        while (true) {
        };
    }

    cm_804D6458 = subject->prev;
    subject->next = NULL;
    if (cm_804D6460 != NULL) {
        cm_804D6468->next = subject;
    } else {
        cm_804D6460 = subject;
    }
    subject->prev = cm_804D6468;
    cm_804D6468 = subject;
    Camera_80028F5C(subject, arg0);
    return subject;
}

void Camera_800290D4(CmSubject* subject)
{
    if (subject->next != 0) {
        subject->next->prev = subject->prev;
    } else {
        cm_804D6468 = subject->prev;
    }

    if (subject->prev != 0) {
        subject->prev->next = subject->next;
    } else {
        cm_804D6460 = subject->next;
    }

    subject->prev = cm_804D6458;
    cm_804D6458 = subject;
}

void nativeCameraSubjects_BeginScene(void)
{
    if (subject_scene.epoch == UINT64_MAX) abort();
    ++subject_scene.epoch;
    subject_scene.active = false;
    if (!subject_scene.count || subject_scene.object_epoch != nativeGObjPool.lifetime ||
        !nativeMatchPool_Validate(&nativeGObjPool)) return;
    subject_scene.objects = nativeGObjPool;
    subject_scene.active = true;
}

void nativeCameraSubjects_EndScene(void) { subject_scene.active = false; }

static bool boundary(void)
{
    const NativeMatchPool* fixed = &subject_scene.objects;
    return subject_scene.active && !subject_scene.activity &&
        nativeGmLoop_SnapshotBoundary() && cm_804D645C == subjects &&
        subject_scene.count && subject_scene.count <= NATIVE_CAMERA_SUBJECT_CAPACITY &&
        nativeGObjPool.lifetime == subject_scene.object_epoch &&
        nativeGObjPool.payload == fixed->payload && nativeGObjPool.slots == fixed->slots &&
        nativeGObjPool.free_order == fixed->free_order && nativeGObjPool.capacity == fixed->capacity &&
        nativeGObjPool.stride == fixed->stride && nativeGObjPool.object_size == fixed->object_size;
}

size_t nativeCameraSubjects_SnapshotBytes(void)
{
    return sizeof(NativeCameraSubjectSnapshotHeader) + subject_scene.count*sizeof(*subjects);
}

static bool overlap(const void* first, size_t an, const void* second, size_t bn)
{
    uintptr_t a = (uintptr_t)first, b = (uintptr_t)second;
    return an > UINTPTR_MAX-a || bn > UINTPTR_MAX-b || (a < b+bn && b < a+an);
}

static bool independent(const void* data, size_t bytes)
{
    if (!data || !bytes || bytes > UINTPTR_MAX-(uintptr_t)data) return false;
#define EXCLUDE(root) if (overlap(data, bytes, &(root), sizeof(root))) return false
    EXCLUDE(subjects); EXCLUDE(subject_scene); EXCLUDE(nativeGObjPool);
    EXCLUDE(cm_804D6458); EXCLUDE(cm_804D645C); EXCLUDE(cm_804D6460); EXCLUDE(cm_804D6468);
#undef EXCLUDE
    const NativeMatchPool* pool = &subject_scene.objects;
    return !overlap(data, bytes, pool->payload, pool->stride*pool->capacity) &&
        !overlap(data, bytes, pool->slots, sizeof(*pool->slots)*pool->capacity) &&
        !overlap(data, bytes, pool->free_order, sizeof(*pool->free_order)*pool->capacity);
}

// Candidate pointers are numeric slot claims, never dereferenced. The free
// chain uses prev; retail deliberately retains obsolete next links on release.
static bool slot_index(CmSubject* subject, uint32_t count, uint32_t* index)
{
    uintptr_t value = (uintptr_t)subject, base = (uintptr_t)subjects;
    if (value < base || value-base >= count*sizeof(*subjects) ||
        (value-base)%sizeof(*subjects)) return false;
    *index = (uint32_t)((value-base)/sizeof(*subjects));
    return true;
}

static bool topology(const NativeCameraSubjectSnapshotHeader* header, const void* records)
{
    uint8_t seen[NATIVE_CAMERA_SUBJECT_CAPACITY] = {0};
    uint32_t total = 0, index;
    // Even stale links on free nodes must stay in this owner's requested prefix.
    for (uint32_t i = 0; i < header->count; ++i) {
        CmSubject node;
        memcpy(&node, (const u8*)records+i*sizeof(node), sizeof(node));
        if ((node.next && !slot_index(node.next, header->count, &index)) ||
            (node.prev && !slot_index(node.prev, header->count, &index)) ||
            (unsigned)node.state > CmSubjectState_Auto) return false;
    }
    CmSubject* previous = NULL;
    for (CmSubject* at = header->active_head; at;) {
        if (!slot_index(at, header->count, &index) || seen[index]) return false;
        CmSubject node;
        memcpy(&node, (const u8*)records+index*sizeof(node), sizeof(node));
        if (node.prev != previous) return false;
        seen[index] = 1; ++total; previous = at; at = node.next;
    }
    if (previous != header->active_tail) return false;
    for (CmSubject* at = header->free_head; at;) {
        if (!slot_index(at, header->count, &index) || seen[index]) return false;
        CmSubject node;
        memcpy(&node, (const u8*)records+index*sizeof(node), sizeof(node));
        seen[index] = 1; ++total; at = node.prev;
    }
    return total == header->count;
}

bool nativeCameraSubjects_Validate(const void* data, size_t bytes)
{
    if (!boundary() || bytes != nativeCameraSubjects_SnapshotBytes() ||
        !independent(data, bytes)) return false;
    NativeCameraSubjectSnapshotHeader header;
    memcpy(&header, data, sizeof(header));
    return header.epoch == subject_scene.epoch && header.object_epoch == subject_scene.object_epoch &&
        header.backing == subjects && header.count == subject_scene.count && !header.reserved &&
        topology(&header, (const u8*)data+sizeof(header));
}

bool nativeCameraSubjects_Save(void* data, size_t capacity, size_t* written)
{
    size_t bytes = nativeCameraSubjects_SnapshotBytes();
    if (!boundary() || capacity < bytes || !independent(data, bytes) ||
        !independent(written, sizeof(*written)) || overlap(data, bytes, written, sizeof(*written))) return false;
    NativeCameraSubjectSnapshotHeader header = {0};
    header.epoch = subject_scene.epoch;
    header.object_epoch = subject_scene.object_epoch;
    header.backing = subjects; header.free_head = cm_804D6458;
    header.active_head = cm_804D6460; header.active_tail = cm_804D6468;
    header.count = subject_scene.count;
    if (!topology(&header, subjects)) return false;
    memcpy(data, &header, sizeof(header));
    memcpy((u8*)data+sizeof(header), subjects, header.count*sizeof(*subjects));
    *written = bytes;
    return true;
}

bool nativeCameraSubjects_Load(const void* data, size_t bytes)
{
    if (!nativeCameraSubjects_Validate(data, bytes)) return false;
    NativeCameraSubjectSnapshotHeader header;
    memcpy(&header, data, sizeof(header));
    // Complete validation precedes all writes; no callbacks or allocation.
    memcpy(subjects, (const u8*)data+sizeof(header), header.count*sizeof(*subjects));
    cm_804D6458 = header.free_head;
    cm_804D6460 = header.active_head;
    cm_804D6468 = header.active_tail;
    return true;
}

#ifdef MELEE_NATIVE
bool nativeCameraSubjects_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(cm_804D6458);
    ROOT(cm_804D645C);
    ROOT(cm_804D6460);
    ROOT(cm_804D6468);
    if(!nativeMatchArena_AddRoot(arena,subjects,subject_scene.count*sizeof(*subjects)))return false;
#undef ROOT
    return true;
}
#endif
