#include <melee/gm/native_match_control_history.h>
#include "native_camera_snapshot.h"
#include "camera_state_internal.h"
#include "camera.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/gr/forward.h>
#include <melee/ft/kinds/ftMasterHand/ftmasterhanddamage0.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <stdlib.h>
#include <string.h>

// Resident boss-intro interest callback. Code does not move during history.
bool fn_8015B4EC(Vec3*);

// MELEE_ROLLBACK_HOST_STATE: scene lease, never restored by the page oracle.
static struct {
    uint64_t epoch;
    NativeMatchPool objects;
    NativeMatchPoolReference primary_object;
    NativeClassReference primary_backing, auxiliary_backing;
    HSD_CObj* primary;
    HSD_CObj* auxiliary;
    HSD_ClassInfo* primary_class;
    HSD_ClassInfo* auxiliary_class;
    bool active;
} scene;

void nativeCamera_BeginScene(void)
{
    if (scene.epoch==UINT64_MAX) abort();
    ++scene.epoch; scene.active=false;
    if (!nativeMatchPool_Validate(&nativeGObjPool) ||
        !nativeMatchPool_Reference(&nativeGObjPool,game_camera.gobj,&scene.primary_object)) return;
    HSD_GObj* object=game_camera.gobj;
    if (object->classifier!=0x10 || object->obj_kind!=HSD_GObj_CameraKind ||
        !(scene.primary_backing=nativeClass_Reference(object->hsd_obj,sizeof(HSD_CObj))).bytes ||
        !(scene.auxiliary_backing=nativeClass_Reference(cm_804D6464,sizeof(HSD_CObj))).bytes) return;
    scene.primary=object->hsd_obj; scene.auxiliary=cm_804D6464;
    scene.primary_class=HSD_CLASS_METHOD(scene.primary);
    scene.auxiliary_class=HSD_CLASS_METHOD(scene.auxiliary);
    if (!scene.primary_class || !scene.auxiliary_class ||
        scene.primary_class->head.obj_size<(int)sizeof(HSD_CObj) ||
        scene.auxiliary_class->head.obj_size<(int)sizeof(HSD_CObj)) return;
    scene.objects=nativeGObjPool; scene.active=true;
}

void nativeCamera_EndScene(void) { scene.active=false; }

static bool boundary(void)
{
    const NativeMatchPool* fixed=&scene.objects;
    return scene.active && nativeGmLoop_SnapshotBoundary() &&
        nativeGObjPool.lifetime==fixed->lifetime && nativeGObjPool.payload==fixed->payload &&
        nativeGObjPool.slots==fixed->slots && nativeGObjPool.free_order==fixed->free_order &&
        nativeGObjPool.capacity==fixed->capacity && nativeGObjPool.stride==fixed->stride &&
        nativeGObjPool.object_size==fixed->object_size &&
        nativeClass_Resolve(scene.primary_backing)==scene.primary &&
        nativeClass_Resolve(scene.auxiliary_backing)==scene.auxiliary;
}

static bool overlap(const void* first,size_t an,const void* second,size_t bn)
{
    uintptr_t a=(uintptr_t)first,b=(uintptr_t)second;
    return an>UINTPTR_MAX-a || bn>UINTPTR_MAX-b || (a<b+bn && b<a+an);
}

static bool independent(const void* data,size_t bytes)
{
    if (!data || !bytes || bytes>UINTPTR_MAX-(uintptr_t)data) return false;
#define EXCLUDE(root) if (overlap(data,bytes,&(root),sizeof(root))) return false
    EXCLUDE(scene); EXCLUDE(game_camera); EXCLUDE(cm_80453004);
    EXCLUDE(cm_803BCCA0); EXCLUDE(cm_804D6464); EXCLUDE(nativeGObjPool);
#undef EXCLUDE
    const NativeMatchPool* pool=&scene.objects;
    return !overlap(data,bytes,pool->payload,pool->stride*pool->capacity) &&
        !overlap(data,bytes,pool->slots,sizeof(*pool->slots)*pool->capacity) &&
        !overlap(data,bytes,pool->free_order,sizeof(*pool->free_order)*pool->capacity) &&
        !overlap(data,bytes,scene.primary,sizeof(*scene.primary)) &&
        !overlap(data,bytes,scene.auxiliary,sizeof(*scene.auxiliary)) &&
        !overlap(data,bytes,scene.primary_class,sizeof(*scene.primary_class)) &&
        !overlap(data,bytes,scene.auxiliary_class,sizeof(*scene.auxiliary_class));
}

static bool class_reference(NativeClassReference a,NativeClassReference b)
{
    return a.epoch==b.epoch && a.block==b.block && a.reserved==b.reserved &&
        a.offset==b.offset && a.bytes==b.bytes;
}

static bool pool_reference(NativeMatchPoolReference a,NativeMatchPoolReference b)
{
    return a.pool==b.pool && a.lifetime==b.lifetime && a.slot==b.slot && a.generation==b.generation;
}

// The union tags determine whether its bits are a callback, vector or index.
// Never interpret stale vector bits as a function address.
static bool callbacks(const Camera* camera)
{
    if (camera->x2D0.callback &&
        camera->x2D0.callback!=(void(*)(Camera_x2D0*))(Event)Camera_SetBounds &&
        camera->x2D0.callback!=(void(*)(Camera_x2D0*))(Event)fn_8002F908 &&
        camera->x2D0.callback!=(void(*)(Camera_x2D0*))(Event)fn_8002FBA0) return false;
    if (camera->x341_b1_b2==3 && camera->x344.cb &&
        camera->x344.cb!=fn_8015B4EC &&
        camera->x344.cb!=ftMh_MS_343_80151428) return false;
    // No source setter currently installs an eye callback; null is still legal.
    if (camera->x341_b3_b4==3 && camera->x35C.cb) return false;
    return true;
}

bool nativeCamera_Validate(const void* data,size_t bytes)
{
    if (!boundary() || bytes!=sizeof(NativeCameraSnapshot) || !independent(data,bytes)) return false;
    NativeCameraSnapshot state; memcpy(&state,data,sizeof(state));
    if (state.epoch!=scene.epoch || state.auxiliary!=scene.auxiliary ||
        !class_reference(state.primary_backing,scene.primary_backing) ||
        !class_reference(state.auxiliary_backing,scene.auxiliary_backing) ||
        !pool_reference(state.primary_object,scene.primary_object) ||
        state.camera.gobj!=(HSD_GObj*)(scene.objects.payload+scene.primary_object.slot*scene.objects.stride) ||
        (unsigned)state.camera.mode>CAMERA_DEBUG_FREE ||
        (unsigned)state.debug.last_mode>CAMERA_DEBUG_FREE || !callbacks(&state.camera)) return false;
    NativeMatchPoolReference ref=state.quake_object;
    if (!state.camera.quake_gobj) return !ref.pool && !ref.lifetime && !ref.slot && !ref.generation;
    return ref.pool==&nativeGObjPool && ref.lifetime==scene.objects.lifetime &&
        ref.slot<scene.objects.capacity && ref.generation &&
        state.camera.quake_gobj==(HSD_GObj*)(scene.objects.payload+ref.slot*scene.objects.stride) &&
        state.camera.quake_gobj!=state.camera.gobj;
}

static bool live_roots(const NativeCameraSnapshot* state)
{
    HSD_GObj* object=nativeMatchPool_Resolve(state->primary_object);
    if (object!=state->camera.gobj || object->classifier!=0x10 ||
        object->obj_kind!=HSD_GObj_CameraKind || object->hsd_obj!=scene.primary ||
        HSD_CLASS_METHOD(scene.primary)!=scene.primary_class ||
        HSD_CLASS_METHOD(scene.auxiliary)!=scene.auxiliary_class) return false;
    if (!state->camera.quake_gobj) return true;
    HSD_GObj* quake=nativeMatchPool_Resolve(state->quake_object);
    return quake==state->camera.quake_gobj && quake->classifier==HSD_GOBJ_CLASS_STAGE &&
        quake->obj_kind==HSD_GObj_JObjKind;
}

bool nativeCamera_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeCameraSnapshot);
    if (!boundary() || capacity<bytes || !independent(data,bytes) ||
        !independent(written,sizeof(*written)) || overlap(data,bytes,written,sizeof(*written))) return false;
    NativeCameraSnapshot state={0}; state.epoch=scene.epoch;
    memcpy(&state.camera,&game_camera,sizeof(game_camera));
    memcpy(&state.debug,&cm_80453004,sizeof(cm_80453004));
    memcpy(&state.coefficients,&cm_803BCCA0,sizeof(cm_803BCCA0));
    state.auxiliary=cm_804D6464;
    state.primary_backing=scene.primary_backing; state.auxiliary_backing=scene.auxiliary_backing;
    state.primary_object=scene.primary_object;
    if (state.camera.quake_gobj &&
        !nativeMatchPool_Reference(&nativeGObjPool,state.camera.quake_gobj,&state.quake_object)) return false;
    if (!nativeCamera_Validate(&state,bytes) || !live_roots(&state)) return false;
    memcpy(data,&state,bytes); *written=bytes; return true;
}

bool nativeCamera_Load(const void* data,size_t bytes)
{
    if (!nativeCamera_Validate(data,bytes)) return false;
    NativeCameraSnapshot state; memcpy(&state,data,sizeof(state));
    // GObj and class companions must restore first. Resolve/check every root
    // before changing any camera/debug/coefficient value; no draw or callbacks.
    if (!live_roots(&state)) return false;
    memcpy(&game_camera,&state.camera,sizeof(game_camera));
    memcpy(&cm_80453004,&state.debug,sizeof(cm_80453004));
    memcpy(&cm_803BCCA0,&state.coefficients,sizeof(cm_803BCCA0));
    cm_804D6464=state.auxiliary; return true;
}


bool nativeCamera_ValidateClassSnapshot(const void* data,size_t bytes,NativeClassSnapshotView classes)
{
    if(!nativeCamera_Validate(data,bytes))return false;
    NativeCameraSnapshot state;memcpy(&state,data,sizeof(state));
    return nativeClass_ResolveSnapshot(classes,state.primary_backing)==scene.primary &&
        nativeClass_ResolveSnapshot(classes,state.auxiliary_backing)==state.auxiliary &&
        nativeClass_SnapshotObjectType(classes,state.primary_backing,scene.primary_class) &&
        nativeClass_SnapshotObjectType(classes,state.auxiliary_backing,scene.auxiliary_class);
}

#ifdef MELEE_NATIVE
bool nativeCamera_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(game_camera);
    ROOT(cm_80453004);
    ROOT(cm_803BCCA0);
    ROOT(cm_804D6464);
#undef ROOT
    return true;
}
#endif
