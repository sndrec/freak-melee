#include <melee/gm/native_match_control_history.h>
#include "native_stage_snapshot.h"
#include "native_ground_pool.h"
#include "stage.h"
#include "stage_start_state.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <stdlib.h>
#include <string.h>

typedef struct StageBackingCache {
    void* pointer;
    NativeClassReference backing;
} StageBackingCache;

enum { ParameterColorOffset=offsetof(GroundParam,xB8),
       ParameterColorEnd=offsetof(GroundParam,xD8)+sizeof(GXColor) };
_Static_assert(ParameterColorEnd-ParameterColorOffset==9*sizeof(GXColor),
               "The actual Ground color fields must form the declared span.");

// Host lease, not mutable simulation state. No allocation at save/load/tick.
static struct {
    uint64_t epoch;
    NativeMatchPool objects, ground;
    StKind selected_stage;
    StageInfo resident[2];
    bool admitted[2];
    GroundParam parameter;
    HSD_ClassInfo* joint_types[GROUND_JOINT_POINT_COUNT];
    HSD_ClassInfo* camera_types[STAGE_CAMERA_COUNT];
    StageBackingCache joints[GROUND_JOINT_POINT_COUNT];
    StageBackingCache cameras[STAGE_CAMERA_COUNT];
    unsigned joint_type_count, camera_type_count;
    bool active;
} scene;

static bool residents(const StageInfo* stage,unsigned phase)
{
    if(phase>1 || !scene.admitted[phase])return false;
    const StageInfo* fixed=&scene.resident[phase];
    uintptr_t head=(uintptr_t)stage->x6A4,base=(uintptr_t)stage_start_state.nodes;
    bool pending=!head || (head>=base && head-base<sizeof(stage_start_state.nodes) &&
        (head-base)%sizeof(*stage_start_state.nodes)==0);
    return stage->grkind==fixed->grkind && pending &&
        stage->itemdata==fixed->itemdata && stage->coll_data==fixed->coll_data &&
        stage->param==fixed->param && stage->ald_yaku_all==fixed->ald_yaku_all &&
        stage->map_ptcl==fixed->map_ptcl && stage->map_texg==fixed->map_texg &&
        stage->yakumono_param==fixed->yakumono_param && stage->map_plit==fixed->map_plit &&
        stage->x6C8==fixed->x6C8 && stage->quake_model_set==fixed->quake_model_set &&
        stage->on_touch_line==fixed->on_touch_line &&
        stage->on_check_shadow_render==fixed->on_check_shadow_render &&
        stage->on_event==fixed->on_event &&
        (!stage->x90 || stage->x90==fixed->x90) &&
        (!stage->x94 || stage->x94==fixed->x94);
}

static bool geometry(const NativeMatchPool* pool,const NativeMatchPool* fixed)
{
    return pool->lifetime==fixed->lifetime && pool->payload==fixed->payload &&
        pool->slots==fixed->slots && pool->free_order==fixed->free_order &&
        pool->capacity==fixed->capacity && pool->stride==fixed->stride &&
        pool->object_size==fixed->object_size;
}

static bool boundary(void)
{
    return scene.active && nativeGmLoop_SnapshotBoundary() &&
        Stage_80225194()==scene.selected_stage && residents(&stage_info,stage_start_state.started) &&
        geometry(&nativeGObjPool,&scene.objects) && geometry(&nativeGroundPool,&scene.ground) &&
        (!stage_info.param ||
            (!memcmp((u8*)stage_info.param+sizeof(float),(u8*)&scene.parameter+sizeof(float),
                     ParameterColorOffset-sizeof(float)) &&
             !memcmp((u8*)stage_info.param+ParameterColorEnd,(u8*)&scene.parameter+ParameterColorEnd,
                     sizeof(GroundParam)-ParameterColorEnd)));
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
    EXCLUDE(scene); EXCLUDE(stage_info); EXCLUDE(stage_joint_updates);
    EXCLUDE(stage_joint_update_counter); EXCLUDE(stage_spawn_cache);
    EXCLUDE(nativeGObjPool); EXCLUDE(nativeGroundPool);
#undef EXCLUDE
    const NativeMatchPool* pools[]={&scene.objects,&scene.ground};
    for (unsigned i=0;i<2;++i) {
        const NativeMatchPool* pool=pools[i];
        if (overlap(data,bytes,pool->payload,pool->capacity*pool->stride) ||
            overlap(data,bytes,pool->slots,pool->capacity*sizeof(*pool->slots)) ||
            overlap(data,bytes,pool->free_order,pool->capacity*sizeof(*pool->free_order))) return false;
    }
    if (stage_info.param && overlap(data,bytes,stage_info.param,sizeof(GroundParam))) return false;
    for (unsigned i=0;i<GROUND_JOINT_POINT_COUNT;++i)
        if (stage_info.x280[i] && overlap(data,bytes,stage_info.x280[i],sizeof(HSD_JObj))) return false;
    for (unsigned i=0;i<STAGE_CAMERA_COUNT;++i)
        if (stage_info.x694[i] && overlap(data,bytes,stage_info.x694[i],sizeof(HSD_CObj))) return false;
    for (unsigned i=0;i<scene.joint_type_count;++i)
        if (overlap(data,bytes,scene.joint_types[i],scene.joint_types[i]->head.info_size)) return false;
    for (unsigned i=0;i<scene.camera_type_count;++i)
        if (overlap(data,bytes,scene.camera_types[i],scene.camera_types[i]->head.info_size)) return false;
    return true;
}

static bool admit_type(HSD_ClassInfo* type,HSD_ClassInfo** types,unsigned* count,unsigned capacity)
{
    if (!type) return false;
    for (unsigned i=0;i<*count;++i) if (types[i]==type) return true;
    if (*count==capacity) return false;
    types[(*count)++]=type; return true;
}

void nativeStage_BeginScene(void)
{
    if(!scene.active || scene.objects.lifetime!=nativeGObjPool.lifetime) {
        if(scene.epoch==UINT64_MAX)abort();uint64_t epoch=scene.epoch+1;
        memset(&scene,0,sizeof(scene));scene.epoch=epoch;
    }
    scene.active=false;
    memset(scene.joints,0,sizeof(scene.joints));
    memset(scene.cameras,0,sizeof(scene.cameras));
    if (stage_info.grkind<0 || !nativeGObjPool.lifetime || !nativeMatchPool_Validate(&nativeGObjPool) ||
        !nativeMatchPool_Validate(&nativeGroundPool) ||
        nativeGObjPool.lifetime!=nativeGroundPool.lifetime) return;
    scene.objects=nativeGObjPool; scene.ground=nativeGroundPool;
    scene.selected_stage=Stage_80225194();
    scene.resident[stage_start_state.started]=stage_info;
    scene.admitted[stage_start_state.started]=true;
    if (stage_info.param) memcpy(&scene.parameter,stage_info.param,sizeof(scene.parameter));
    for (unsigned i=0;i<GROUND_JOINT_POINT_COUNT;++i) if (stage_info.x280[i]) {
        NativeClassReference backing;
        if (!(backing=nativeClass_Reference(stage_info.x280[i],sizeof(HSD_JObj))).bytes ||
            !admit_type(HSD_CLASS_METHOD(stage_info.x280[i]),scene.joint_types,
                &scene.joint_type_count,GROUND_JOINT_POINT_COUNT) ||
            HSD_CLASS_METHOD(stage_info.x280[i])->head.obj_size<(int)sizeof(HSD_JObj)) return;
        scene.joints[i]=(StageBackingCache){stage_info.x280[i],backing};
    }
    for (unsigned i=0;i<STAGE_CAMERA_COUNT;++i) if (stage_info.x694[i]) {
        NativeClassReference backing;
        if (!(backing=nativeClass_Reference(stage_info.x694[i],sizeof(HSD_CObj))).bytes ||
            !admit_type(HSD_CLASS_METHOD(stage_info.x694[i]),scene.camera_types,
                &scene.camera_type_count,STAGE_CAMERA_COUNT) ||
            HSD_CLASS_METHOD(stage_info.x694[i])->head.obj_size<(int)sizeof(HSD_CObj)) return;
        scene.cameras[i]=(StageBackingCache){stage_info.x694[i],backing};
    }
    scene.active=true;
}

void nativeStage_EndScene(void) { scene.active=false; }

// A joint attachment can change during play. Reuse the O(1) backing claim
// only while both its numeric address and allocator epoch/span still resolve;
// refresh changed attachments using the actual allocator's span lookup.
// This cache proves backing only, never occupied-object/type identity.
static bool capture_backing(void* pointer,size_t bytes,StageBackingCache* cache,
                            NativeClassReference* out)
{
    if (cache->pointer!=pointer || nativeClass_Resolve(cache->backing)!=pointer) {
        NativeClassReference ref;
        if (!(ref=nativeClass_Reference(pointer,bytes)).bytes) return false;
        *cache=(StageBackingCache){pointer,ref};
    }
    *out=cache->backing;return true;
}

static bool reference_shape(const void* pointer,NativeMatchPoolReference reference,
                            const NativeMatchPool* pool,const NativeMatchPool* fixed)
{
    if (!pointer) return !reference.pool && !reference.lifetime && !reference.slot && !reference.generation;
    return reference.pool==pool && reference.lifetime==fixed->lifetime &&
        reference.slot<fixed->capacity && reference.generation &&
        pointer==fixed->payload+reference.slot*fixed->stride;
}

static bool class_shape(const void* pointer,NativeStageClassRoot root,size_t bytes,
                        HSD_ClassInfo* const* types,unsigned count,const void* data,size_t size)
{
    if (!pointer) return !root.type && !root.backing.epoch && !root.backing.block &&
        !root.backing.reserved && !root.backing.offset && !root.backing.bytes;
    bool admitted=false;
    for (unsigned i=0;i<count;++i) if (root.type==types[i]) admitted=true;
    return admitted && root.backing.bytes==bytes &&
        (uintptr_t)pointer%_Alignof(HSD_Class)==0 &&
        nativeClass_Resolve(root.backing)==pointer && !overlap(data,size,pointer,bytes);
}

bool nativeStage_Validate(const void* data,size_t bytes)
{
    if (!boundary() || bytes!=sizeof(NativeStageSnapshot) || !independent(data,bytes)) return false;
    NativeStageSnapshot state; memcpy(&state,data,sizeof(state));
    if (state.epoch!=scene.epoch || state.selected_stage!=scene.selected_stage || !residents(&state.stage,state.started)) return false;
    for (unsigned i=0;i<STAGE_MAP_OBJECT_COUNT;++i) {
        if (state.joint_updates[i]>1 ||
            !reference_shape(state.stage.map_gobjs[i],state.map_objects[i],&nativeGObjPool,&scene.objects)) return false;
        NativeMatchPoolReference ref=state.map_userdata[i];
        // Candidate userdata identity follows its slot; occupied contents are
        // checked after the companion has restored, never read from here.
        if (!state.stage.map_gobjs[i]) {
            if (ref.pool || ref.lifetime || ref.slot || ref.generation) return false;
        } else if (ref.pool!=&nativeGroundPool || ref.lifetime!=scene.ground.lifetime ||
                   ref.slot>=scene.ground.capacity || !ref.generation) return false;
    }
    if (!reference_shape(state.stage.x12C,state.fog,&nativeGObjPool,&scene.objects) ||
        !reference_shape(state.stage.x72C,state.special_object,&nativeGObjPool,&scene.objects)) return false;
    for (unsigned i=0;i<GROUND_JOINT_POINT_COUNT;++i)
        if (!class_shape(state.stage.x280[i],state.joints[i],sizeof(HSD_JObj),
                scene.joint_types,scene.joint_type_count,data,bytes)) return false;
    for (unsigned i=0;i<STAGE_CAMERA_COUNT;++i)
        if (!class_shape(state.stage.x694[i],state.cameras[i],sizeof(HSD_CObj),
                scene.camera_types,scene.camera_type_count,data,bytes)) return false;
    return true;
}

static bool live_roots(const NativeStageSnapshot* state)
{
    for (unsigned i=0;i<STAGE_MAP_OBJECT_COUNT;++i) if (state->stage.map_gobjs[i]) {
        HSD_GObj* object=nativeMatchPool_Resolve(state->map_objects[i]);
        Ground* ground=nativeMatchPool_Resolve(state->map_userdata[i]);
        if (object!=state->stage.map_gobjs[i] || !ground ||
            object->classifier!=HSD_GOBJ_CLASS_STAGE || object->user_data_kind!=3 ||
            object->user_data!=ground || object->user_data_remove_func!=nativeGroundPool_Release ||
            ground->gobj!=object) return false;
    }
    if (state->stage.x12C) {
        HSD_GObj* fog=nativeMatchPool_Resolve(state->fog);
        if (fog!=state->stage.x12C || fog->classifier!=0xA || fog->obj_kind!=HSD_GObj_FogKind) return false;
    }
    if (state->stage.x72C && nativeMatchPool_Resolve(state->special_object)!=state->stage.x72C) return false;
    for (unsigned i=0;i<GROUND_JOINT_POINT_COUNT;++i)
        if (state->stage.x280[i] && HSD_CLASS_METHOD(state->stage.x280[i])!=state->joints[i].type) return false;
    for (unsigned i=0;i<STAGE_CAMERA_COUNT;++i)
        if (state->stage.x694[i] && HSD_CLASS_METHOD(state->stage.x694[i])!=state->cameras[i].type) return false;
    return true;
}

bool nativeStage_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeStageSnapshot);
    if (!boundary() || capacity<bytes || !independent(data,bytes) ||
        !independent(written,sizeof(*written)) || overlap(data,bytes,written,sizeof(*written))) return false;
    NativeStageSnapshot state={0}; state.epoch=scene.epoch; state.selected_stage=scene.selected_stage;
    state.started=stage_start_state.started;
    memcpy(&state.stage,&stage_info,sizeof(stage_info));
    memcpy(state.joint_updates,stage_joint_updates,sizeof(stage_joint_updates));
    state.joint_update_counter=stage_joint_update_counter;
    memcpy(state.spawn_cache,stage_spawn_cache,sizeof(stage_spawn_cache));
    state.ground_scale=stage_info.param ? stage_info.param->y : 1.0f;
    // Ground_801C052C..05EC write these through a local GroundParam alias;
    // Corneria and Venom change them during play. They are not resident data.
    if (stage_info.param) memcpy(state.ground_colors,&stage_info.param->xB8,sizeof(state.ground_colors));
    for (unsigned i=0;i<STAGE_MAP_OBJECT_COUNT;++i) if (state.stage.map_gobjs[i]) {
        if (!nativeMatchPool_Reference(&nativeGObjPool,state.stage.map_gobjs[i],&state.map_objects[i])) return false;
        HSD_GObj* object=state.stage.map_gobjs[i];
        if (!nativeMatchPool_Reference(&nativeGroundPool,object->user_data,&state.map_userdata[i])) return false;
    }
    if (state.stage.x12C && !nativeMatchPool_Reference(&nativeGObjPool,state.stage.x12C,&state.fog)) return false;
    if (state.stage.x72C && !nativeMatchPool_Reference(&nativeGObjPool,state.stage.x72C,&state.special_object)) return false;
    for (unsigned i=0;i<GROUND_JOINT_POINT_COUNT;++i) if (state.stage.x280[i]) {
        if (!capture_backing(state.stage.x280[i],sizeof(HSD_JObj),&scene.joints[i],&state.joints[i].backing)) return false;
        state.joints[i].type=HSD_CLASS_METHOD(state.stage.x280[i]);
    }
    for (unsigned i=0;i<STAGE_CAMERA_COUNT;++i) if (state.stage.x694[i]) {
        if (!capture_backing(state.stage.x694[i],sizeof(HSD_CObj),&scene.cameras[i],&state.cameras[i].backing)) return false;
        state.cameras[i].type=HSD_CLASS_METHOD(state.stage.x694[i]);
    }
    if (!nativeStage_Validate(&state,bytes) || !live_roots(&state)) return false;
    memcpy(data,&state,bytes); *written=bytes; return true;
}

bool nativeStage_Load(const void* data,size_t bytes)
{
    if (!nativeStage_Validate(data,bytes)) return false;
    NativeStageSnapshot state; memcpy(&state,data,sizeof(state));
    // GObj, Ground and class payload companions restore first. Aggregate
    // validation must prove their candidate agreement before any owner writes.
    if (!live_roots(&state)) return false;
    memcpy(&stage_info,&state.stage,sizeof(stage_info));
    stage_start_state.started=state.started!=0;
    memcpy(stage_joint_updates,state.joint_updates,sizeof(stage_joint_updates));
    stage_joint_update_counter=state.joint_update_counter;
    memcpy(stage_spawn_cache,state.spawn_cache,sizeof(stage_spawn_cache));
    if (stage_info.param) {
        stage_info.param->y=state.ground_scale;
        memcpy(&stage_info.param->xB8,state.ground_colors,sizeof(state.ground_colors));
    }
    return true;
}

#ifdef MELEE_NATIVE
bool nativeStage_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(stage_info);
    ROOT(stage_joint_updates);
    ROOT(stage_joint_update_counter);
    ROOT(stage_spawn_cache);
    if(stage_info.param){
        ROOT(stage_info.param->y);
        if(!nativeMatchArena_AddRoot(arena,&stage_info.param->xB8,sizeof(((NativeStageSnapshot*)0)->ground_colors)))return false;
    }
#undef ROOT
    return true;
}
#endif
