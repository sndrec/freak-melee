#include <melee/gm/native_match_control_history.h>
#include "native_stage_hazard_snapshot.h"
#include "stage_state_internal.h"
#include "native_ground_pool.h"
#include "grbigblue.static.h"
#include <melee/it/native_item_pool.h>
#include <melee/it/item.h>
#include <melee/gm/native_gm_loop_snapshot.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <stdlib.h>
#include <string.h>

// Host scene lease. The actual per-stage records live in stage_hazard_state.c.
static struct {
    uint64_t epoch;
    GrKind kind;
    NativeMatchPool objects,ground,items;
    const void* parameters;
    HSD_ClassInfo* joint_types[GROUND_JOINT_POINT_COUNT+NATIVE_STAGE_HAZARD_ROOTS];
    unsigned joint_type_count;
    struct { GrJoint joint;s16 x06; } pura_config[PURA_COLLISION_MAP_COUNT];
    struct { s16 joint,start,end,pad; } big_blue_config[BIG_BLUE_TRACK_COUNT];
    s16 rainbow_collision_ids[RAINBOW_CRUISE_ENTRIES];
    struct {
        HSD_JObj* pointer;
        NativeClassReference backing;
    } joint_cache[NATIVE_STAGE_HAZARD_ROOTS];
    HSD_ImageDesc* reflection_image;
    HSD_ImageDesc reflection_description;
    HSD_CObj* reflection_camera;
    StageHazardJointReference reflection_camera_claim;
    bool active;
} scene;

static void* current_record(size_t* size)
{
    switch (scene.kind) {
    case Gr_Kind_Izumi:*size=sizeof(fountain_state);return &fountain_state;
    case Gr_Kind_RCruise:*size=sizeof(rainbow_cruise_state);return &rainbow_cruise_state;
    case Gr_Kind_BigBlue:*size=sizeof(big_blue_state);return &big_blue_state;
    case Gr_Kind_Pura:*size=sizeof(pura_state);return &pura_state;
    case Gr_Kind_Battle:*size=sizeof(battlefield_state);return &battlefield_state;
    case Gr_Kind_MuteCity:*size=sizeof(mute_city_state);return &mute_city_state;
    case Gr_Kind_Castle:*size=sizeof(castle_state);return &castle_state;
    case Gr_Kind_Corneria:*size=sizeof(corneria_state);return &corneria_state;
    case Gr_Kind_Venom:*size=sizeof(venom_state);return &venom_state;
    case Gr_Kind_Greens:*size=sizeof(greens_state);return &greens_state;
    case Gr_Kind_Kraid:*size=sizeof(kraid_state);return &kraid_state;
    case Gr_Kind_Zebes:*size=sizeof(brinstar_state);return &brinstar_state;
    case Gr_Kind_Kongo:*size=sizeof(kongo_state);return &kongo_state;
    case Gr_Kind_Icemt:*size=sizeof(ice_mountain_state);return &ice_mountain_state;
    default:*size=0;return NULL;
    }
}

static const void* parameters(const StageHazardState* state)
{
    switch (scene.kind) {
    case Gr_Kind_Izumi:return state ? state->fountain.parameters : fountain_state.parameters;
    case Gr_Kind_RCruise:return state ? state->rainbow_cruise.parameters : rainbow_cruise_state.parameters;
    case Gr_Kind_BigBlue:return state ? state->big_blue.parameters : big_blue_state.parameters;
    case Gr_Kind_Pura:return state ? state->pura.parameters : pura_state.parameters;
    case Gr_Kind_Battle:return state ? state->battlefield.parameters : battlefield_state.parameters;
    case Gr_Kind_MuteCity:return state ? state->mute_city.parameters : mute_city_state.parameters;
    case Gr_Kind_Castle:return state ? state->castle.parameters : castle_state.parameters;
    case Gr_Kind_Corneria:return state ? state->corneria.parameters : corneria_state.parameters;
    case Gr_Kind_Venom:return state ? state->venom.parameters : venom_state.parameters;
    case Gr_Kind_Greens:return state ? state->greens.parameters : greens_state.parameters;
    case Gr_Kind_Kraid:return state ? state->kraid.parameters : kraid_state.parameters;
    case Gr_Kind_Zebes:return state ? state->brinstar.parameters : brinstar_state.parameters;
    case Gr_Kind_Kongo:return state ? state->kongo.parameters : kongo_state.parameters;
    case Gr_Kind_Icemt:return state ? state->ice_mountain.parameters : ice_mountain_state.parameters;
    default:return NULL;
    }
}

static HSD_GObj* root(const StageHazardState* state,unsigned i)
{
    if (scene.kind==Gr_Kind_Izumi && i==0) return state->fountain.owner;
    if (scene.kind==Gr_Kind_RCruise && i==0) return state->rainbow_cruise.owner;
    if (scene.kind==Gr_Kind_MuteCity && i<MUTE_CITY_CAR_COUNT) return state->mute_city.cars[i].material;
    if (scene.kind==Gr_Kind_Corneria && i<3) return state->corneria.arwings[i];
    if (scene.kind==Gr_Kind_Venom && i<3) return state->venom.arwings[i];
    if (scene.kind==Gr_Kind_Zebes && i<BRINSTAR_BUBBLE_COUNT) return state->brinstar.bubbles.bubbles[i].x20_gobj;
    if (scene.kind==Gr_Kind_Icemt && i<3) return state->ice_mountain.extra_rows[i];
    return NULL;
}

static HSD_JObj* joint(const StageHazardState* state,unsigned i)
{
    if(scene.kind==Gr_Kind_RCruise) {
        if(i<RAINBOW_CRUISE_ENTRIES)return state->rainbow_cruise.entries[i].x14;
        i-=RAINBOW_CRUISE_ENTRIES;
        return i<RAINBOW_CRUISE_VANISH ? state->rainbow_cruise.vanish[i].jobj : NULL;
    }
    if(scene.kind==Gr_Kind_Pura && i<PURA_UPDATED_MAP_COUNT)return state->pura.maps[i].jobj;
    if (scene.kind==Gr_Kind_Kongo) {
        if (i<15) return state->kongo.links[i].unk4;
        if (i==15) return state->kongo.banks.unk0;
        if (i==16) return state->kongo.banks.unk4;
        return NULL;
    }
    // Merging bubbles clears the donor's actor/active fields but deliberately
    // retains its joint pointer. That dormant alias can subsequently dangle.
    // It is preserved as bytes, not followed as a live graph root. New bubble
    // creation resets it before use (grZebes_801DAE70).
    if (scene.kind!=Gr_Kind_Zebes || !root(state,i)) return NULL;
    return state->brinstar.bubbles.bubbles[i].x04;
}

static bool admit_joint(HSD_JObj* object)
{
    if (!object) return true;
    NativeClassReference backing;
    if (!(backing=nativeClass_Reference(object,sizeof(*object))).bytes) return false;
    HSD_ClassInfo* type=HSD_CLASS_METHOD(object);
    if (!nativeClass_TypeKnown(type,sizeof(*object))) return false;
    for (unsigned i=0;i<scene.joint_type_count;++i) if (scene.joint_types[i]==type) return true;
    if (scene.joint_type_count==GROUND_JOINT_POINT_COUNT+NATIVE_STAGE_HAZARD_ROOTS) return false;
    scene.joint_types[scene.joint_type_count++]=type;return true;
}

void nativeStageHazards_BeginScene(void)
{
    if(!scene.active || scene.objects.lifetime!=nativeGObjPool.lifetime) {
        if(scene.epoch==UINT64_MAX)abort();uint64_t epoch=scene.epoch+1;
        memset(&scene,0,sizeof(scene));scene.epoch=epoch;
    }
    scene.active=false;memset(scene.joint_cache,0,sizeof(scene.joint_cache));
    if (!nativeGObjPool.lifetime || !nativeMatchPool_Validate(&nativeGObjPool) ||
        !nativeMatchPool_Validate(&nativeGroundPool) ||
        nativeGObjPool.lifetime!=nativeGroundPool.lifetime) return;
    scene.kind=stage_info.grkind;scene.objects=nativeGObjPool;scene.ground=nativeGroundPool;scene.items=nativeItemPool;
    if(scene.kind==Gr_Kind_MuteCity && nativeItemPool.lifetime!=nativeGroundPool.lifetime)return;
    scene.parameters=parameters(NULL);
    if(scene.kind==Gr_Kind_Izumi) {
        NativeMatchPoolReference reference;
        if(!nativeMatchPool_Reference(&nativeGObjPool,fountain_state.owner,&reference))return;
        HSD_GObj* owner=nativeMatchPool_Resolve(reference);
        if(!owner || owner->classifier!=0x11 || owner->p_link!=0x12 ||
            owner->obj_kind!=HSD_GObj_CameraKind || owner->user_data_kind!=3 ||
            owner->user_data!=&fountain_state.reflection || owner->user_data_remove_func!=nativeFountain_ReleaseReflection ||
            !(scene.reflection_camera_claim.backing=nativeClass_Reference(owner->hsd_obj,sizeof(HSD_CObj))).bytes)return;
        scene.reflection_camera=owner->hsd_obj;
        scene.reflection_camera_claim.type=HSD_CLASS_METHOD(scene.reflection_camera);
        if(!nativeClass_TypeKnown(scene.reflection_camera_claim.type,sizeof(HSD_CObj)) || !fountain_state.reflection.image)return;
        scene.reflection_image=fountain_state.reflection.image;
        scene.reflection_description=*scene.reflection_image;
    }
    if(scene.kind==Gr_Kind_RCruise) {
        if(!rainbow_cruise_state.owner)return;
        for(unsigned i=0;i<RAINBOW_CRUISE_ENTRIES;++i)
            scene.rainbow_collision_ids[i]=rainbow_cruise_state.entries[i].x2;
    }
    if(scene.kind==Gr_Kind_BigBlue)for(unsigned i=0;i<BIG_BLUE_TRACK_COUNT;++i) {
        const grBb_TrackEntry* entry=&big_blue_state.tracks[i];
        scene.big_blue_config[i].joint=entry->jobj_index;scene.big_blue_config[i].start=entry->start_index;
        scene.big_blue_config[i].end=entry->end_index;scene.big_blue_config[i].pad=entry->pad;
    }
    if(scene.kind==Gr_Kind_Pura)for(unsigned i=0;i<PURA_COLLISION_MAP_COUNT;++i) {
        scene.pura_config[i].joint=pura_state.maps[i].joint;scene.pura_config[i].x06=pura_state.maps[i].x06;
    }
    if (scene.kind==Gr_Kind_Zebes || scene.kind==Gr_Kind_Kongo || scene.kind==Gr_Kind_Pura || scene.kind==Gr_Kind_RCruise) {
        for (unsigned i=0;i<GROUND_JOINT_POINT_COUNT;++i)
            if (!admit_joint(stage_info.x280[i])) return;
        StageHazardState state;memset(&state,0,sizeof(state));
        size_t n;void* record=current_record(&n);memcpy(&state,record,n);
        for (unsigned i=0;i<NATIVE_STAGE_HAZARD_ROOTS;++i)
            if (!admit_joint(joint(&state,i))) return;
    }
    scene.active=true;
}

void nativeStageHazards_EndScene(void) { scene.active=false; }

static bool geometry(const NativeMatchPool* pool,const NativeMatchPool* fixed)
{
    return pool->lifetime==fixed->lifetime && pool->payload==fixed->payload &&
        pool->slots==fixed->slots && pool->free_order==fixed->free_order &&
        pool->capacity==fixed->capacity && pool->stride==fixed->stride && pool->object_size==fixed->object_size;
}

static bool boundary(void)
{
    return scene.active && nativeGmLoop_SnapshotBoundary() && stage_info.grkind==scene.kind &&
        parameters(NULL)==scene.parameters &&
        geometry(&nativeGObjPool,&scene.objects) && geometry(&nativeGroundPool,&scene.ground) &&
        (scene.kind!=Gr_Kind_MuteCity || geometry(&nativeItemPool,&scene.items)) &&
        (scene.kind!=Gr_Kind_Izumi || (fountain_state.reflection.image==scene.reflection_image &&
            !memcmp(scene.reflection_image,&scene.reflection_description,sizeof(HSD_ImageDesc))));
}

static bool overlap(const void* first,size_t an,const void* second,size_t bn)
{
    uintptr_t a=(uintptr_t)first,b=(uintptr_t)second;
    return an>UINTPTR_MAX-a || bn>UINTPTR_MAX-b || (a<b+bn && b<a+an);
}

static bool independent(const void* data,size_t bytes)
{
    if (!data || !bytes || bytes>UINTPTR_MAX-(uintptr_t)data) return false;
#define EXCLUDE(value) if (overlap(data,bytes,&(value),sizeof(value))) return false
    EXCLUDE(scene);EXCLUDE(corneria_state);EXCLUDE(venom_state);EXCLUDE(greens_state);
    EXCLUDE(kraid_state);EXCLUDE(arwing_conversation_selection);EXCLUDE(stage_info);
    EXCLUDE(brinstar_state);
    EXCLUDE(kongo_state);EXCLUDE(ice_mountain_state);EXCLUDE(mute_city_state);EXCLUDE(castle_state);
    EXCLUDE(big_blue_state);EXCLUDE(rainbow_cruise_state);EXCLUDE(fountain_state);
    if(scene.kind==Gr_Kind_Izumi && (overlap(data,bytes,scene.reflection_image,sizeof(HSD_ImageDesc)) ||
        overlap(data,bytes,scene.reflection_camera,sizeof(HSD_CObj)) ||
        overlap(data,bytes,scene.reflection_camera_claim.type,scene.reflection_camera_claim.type->head.info_size)))return false;
    if(scene.kind==Gr_Kind_BigBlue && scene.parameters &&
        overlap(data,bytes,scene.parameters,sizeof(grBb_YakumonoParam)))return false;
    EXCLUDE(dynamics_state);EXCLUDE(nativeItemPool);EXCLUDE(pura_state);EXCLUDE(battlefield_state);
    EXCLUDE(nativeGObjPool);EXCLUDE(nativeGroundPool);
#undef EXCLUDE
    const NativeMatchPool* pools[]={&scene.objects,&scene.ground,&scene.items};
    for (unsigned i=0;i<3;++i) {
        const NativeMatchPool* pool=pools[i];
        if (overlap(data,bytes,pool->payload,pool->capacity*pool->stride) ||
            overlap(data,bytes,pool->slots,pool->capacity*sizeof(*pool->slots)) ||
            overlap(data,bytes,pool->free_order,pool->capacity*sizeof(*pool->free_order))) return false;
    }
    if (scene.kind==Gr_Kind_Zebes || scene.kind==Gr_Kind_Kongo || scene.kind==Gr_Kind_Pura || scene.kind==Gr_Kind_RCruise) {
        StageHazardState state;memset(&state,0,sizeof(state));
        size_t n;void* record=current_record(&n);memcpy(&state,record,n);
        for (unsigned i=0;i<NATIVE_STAGE_HAZARD_ROOTS;++i) {
            HSD_JObj* model=joint(&state,i);
            if (model && overlap(data,bytes,model,sizeof(HSD_JObj))) return false;
        }
        for (unsigned i=0;i<scene.joint_type_count;++i)
            if (overlap(data,bytes,scene.joint_types[i],scene.joint_types[i]->head.info_size)) return false;
    }
    return true;
}

static bool zero_reference(NativeMatchPoolReference ref)
{ return !ref.pool && !ref.lifetime && !ref.slot && !ref.generation; }

static bool joint_shape(HSD_JObj* object,StageHazardJointReference ref,const void* data,size_t bytes)
{
    if (!object) return !ref.type && !ref.backing.epoch && !ref.backing.block &&
        !ref.backing.reserved && !ref.backing.offset && !ref.backing.bytes;
    bool admitted=false;
    for (unsigned i=0;i<scene.joint_type_count;++i) if (ref.type==scene.joint_types[i]) admitted=true;
    return admitted && (uintptr_t)object%_Alignof(HSD_JObj)==0 &&
        ref.backing.bytes==sizeof(HSD_JObj) && nativeClass_Resolve(ref.backing)==object &&
        !overlap(data,bytes,object,sizeof(HSD_JObj));
}

bool nativeStageHazards_Validate(const void* data,size_t bytes)
{
    if (!boundary() || bytes!=sizeof(NativeStageHazardSnapshot) || !independent(data,bytes)) return false;
    NativeStageHazardSnapshot state;memcpy(&state,data,sizeof(state));
    if (state.epoch!=scene.epoch || state.kind!=scene.kind || state.reserved || parameters(&state.state)!=scene.parameters) return false;
    size_t record_bytes;current_record(&record_bytes);
    static const u8 zero[sizeof(StageHazardState)]={0};
    // Unused union bytes and roots have a single representation. No other
    // stage's dormant globals or stale handles are read/restored in this match.
    if (memcmp((u8*)&state.state+record_bytes,zero+record_bytes,sizeof(zero)-record_bytes)) return false;
    if (scene.kind==Gr_Kind_Corneria &&
        (state.state.corneria.current_slot>=3 ||
         ((u8*)&state.state.corneria)[offsetof(CorneriaState,updates_disabled)]>1)) return false;
    if (scene.kind==Gr_Kind_Venom && state.state.venom.current_slot>=3) return false;
    if(scene.kind==Gr_Kind_Izumi) {
        if(!state.state.fountain.owner || state.state.fountain.reflection.image!=scene.reflection_image ||
            memcmp(&state.reflection_camera,&scene.reflection_camera_claim,sizeof(state.reflection_camera)) ||
            nativeClass_Resolve(state.reflection_camera.backing)!=scene.reflection_camera)return false;
    } else {
        static const StageHazardJointReference absent={0};
        if(memcmp(&state.reflection_camera,&absent,sizeof(absent)))return false;
    }
    if(scene.kind==Gr_Kind_RCruise) {
        if(!state.state.rainbow_cruise.owner)return false;
        for(unsigned i=0;i<RAINBOW_CRUISE_ENTRIES;++i)
            if(state.state.rainbow_cruise.entries[i].x2!=scene.rainbow_collision_ids[i] ||
                state.state.rainbow_cruise.entries[i].x0>5)return false;
        for(unsigned i=0;i<RAINBOW_CRUISE_VANISH;++i)
            if(state.state.rainbow_cruise.vanish[i].x0<0 || state.state.rainbow_cruise.vanish[i].x0>3)return false;
    }
    if(scene.kind!=Gr_Kind_BigBlue && state.big_blue_mode)return false;
    if(scene.kind==Gr_Kind_BigBlue) {
        if(!scene.parameters && state.big_blue_mode)return false;
        for(unsigned i=0;i<BIG_BLUE_TRACK_COUNT;++i) {
            const grBb_TrackEntry* entry=&state.state.big_blue.tracks[i];
            if(entry->jobj_index!=scene.big_blue_config[i].joint || entry->start_index!=scene.big_blue_config[i].start ||
                entry->end_index!=scene.big_blue_config[i].end || entry->pad!=scene.big_blue_config[i].pad)return false;
        }
    }
    if(scene.kind==Gr_Kind_Pura)for(unsigned i=0;i<PURA_COLLISION_MAP_COUNT;++i) {
        const GrJoint* a=&state.state.pura.maps[i].joint;const GrJoint* b=&scene.pura_config[i].joint;
        if(a->x!=b->x || a->y!=b->y || a->z!=b->z || state.state.pura.maps[i].x06!=scene.pura_config[i].x06)return false;
    }
    if(scene.kind==Gr_Kind_Battle && state.state.battlefield.is_demo>1)return false;
    if(scene.kind==Gr_Kind_MuteCity) {
        uint32_t seen=0;
        for(unsigned i=0;i<MUTE_CITY_CAR_COUNT;++i) {
            int index=state.state.mute_city.order[i];
            if(index<0 || index>=MUTE_CITY_CAR_COUNT || (seen&(UINT32_C(1)<<index)))return false;
            seen|=UINT32_C(1)<<index;
        }
    }
    if(scene.kind==Gr_Kind_Castle && state.state.castle.strong_wind>1)return false;
    struct lb_80011A50_t* wind=scene.kind==Gr_Kind_Castle ? state.state.castle.wind : NULL;
    if(wind) {
        if(nativeDynamics_ResolveWind(state.wind)!=wind)return false;
    } else if(state.wind.epoch || state.wind.slot || state.wind.reserved) {
        if(scene.kind!=Gr_Kind_RCruise || !nativeDynamics_ResolveWind(state.wind))return false;
    }
    const NativeMatchPool* userdata_pool=scene.kind==Gr_Kind_MuteCity ? &nativeItemPool : &nativeGroundPool;
    const NativeMatchPool* userdata_shape=scene.kind==Gr_Kind_MuteCity ? &scene.items : &scene.ground;
    for (unsigned i=0;i<NATIVE_STAGE_HAZARD_ROOTS;++i) {
        if(scene.kind==Gr_Kind_RCruise && i<RAINBOW_CRUISE_ENTRIES+RAINBOW_CRUISE_VANISH &&
            !joint(&state.state,i))return false;
        if (!joint_shape(joint(&state.state,i),state.joints[i],data,bytes)) return false;
        HSD_GObj* object=root(&state.state,i);
        NativeMatchPoolReference obj=state.objects[i],ground=state.userdata[i];
        if (!object) {
            if (!zero_reference(obj) || !zero_reference(ground)) return false;
        } else if(scene.kind==Gr_Kind_Izumi) {
            if(obj.pool!=&nativeGObjPool || obj.lifetime!=scene.objects.lifetime ||
                obj.slot>=scene.objects.capacity || !obj.generation || !zero_reference(ground) ||
                object!=(HSD_GObj*)(scene.objects.payload+obj.slot*scene.objects.stride))return false;
        } else if (obj.pool!=&nativeGObjPool || obj.lifetime!=scene.objects.lifetime ||
            obj.slot>=scene.objects.capacity || !obj.generation ||
            object!=(HSD_GObj*)(scene.objects.payload+obj.slot*scene.objects.stride) ||
            ground.pool!=userdata_pool || ground.lifetime!=userdata_shape->lifetime ||
            ground.slot>=userdata_shape->capacity || !ground.generation) return false;
    }
    return true;
}

static bool live_roots(const NativeStageHazardSnapshot* state)
{
    for (unsigned i=0;i<NATIVE_STAGE_HAZARD_ROOTS;++i) {
        if (root(&state->state,i)) {
            HSD_GObj* object=nativeMatchPool_Resolve(state->objects[i]);
            if(!object || object!=root(&state->state,i))return false;
            if(scene.kind==Gr_Kind_Izumi) {
                if(object->classifier!=0x11 || object->p_link!=0x12 || object->obj_kind!=HSD_GObj_CameraKind ||
                    object->user_data_kind!=3 || object->user_data!=&fountain_state.reflection ||
                    object->user_data_remove_func!=nativeFountain_ReleaseReflection || object->hsd_obj!=scene.reflection_camera ||
                    HSD_CLASS_METHOD(scene.reflection_camera)!=state->reflection_camera.type)return false;
            } else if(scene.kind==Gr_Kind_MuteCity) {
                NativeItemPoolSlot* slot=nativeMatchPool_Resolve(state->userdata[i]);
                if(!slot || nativeItemPool_Slot((Item*)slot)!=slot ||
                    object->classifier!=HSD_GOBJ_CLASS_ITEM || object->user_data_kind!=6 ||
                    object->user_data!=&slot->actor || object->user_data_remove_func!=Item_OnUserDataRemove ||
                    slot->actor.entity!=object)return false;
            } else {
                Ground* ground=nativeMatchPool_Resolve(state->userdata[i]);
                if(!ground || object->classifier!=HSD_GOBJ_CLASS_STAGE || object->user_data_kind!=3 ||
                    object->user_data!=ground || object->user_data_remove_func!=nativeGroundPool_Release ||
                    ground->gobj!=object)return false;
                if(scene.kind==Gr_Kind_RCruise && (ground->map_id!=1 ||
                    ground->u.rcruise.entries!=rainbow_cruise_state.entries ||
                    ground->u.rcruise.vanish!=rainbow_cruise_state.vanish ||
                    nativeDynamics_ResolveWind(state->wind)!=ground->u.rcruise.x4))return false;
            }
        }
        HSD_JObj* model=joint(&state->state,i);
        if (model && HSD_CLASS_METHOD(model)!=state->joints[i].type) return false;
    }
    return true;
}

bool nativeStageHazards_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeStageHazardSnapshot);
    if (!boundary() || capacity<bytes || !independent(data,bytes) ||
        !independent(written,sizeof(*written)) || overlap(data,bytes,written,sizeof(*written))) return false;
    NativeStageHazardSnapshot state;memset(&state,0,sizeof(state));state.epoch=scene.epoch;state.kind=scene.kind;
    state.conversation_selection=arwing_conversation_selection;
    size_t n;void* record=current_record(&n);if (n) memcpy(&state.state,record,n);
    if(scene.kind==Gr_Kind_Izumi)state.reflection_camera=scene.reflection_camera_claim;
    if(scene.kind==Gr_Kind_BigBlue && big_blue_state.parameters)state.big_blue_mode=big_blue_state.parameters->x18;
    if(scene.kind==Gr_Kind_Castle && state.state.castle.wind &&
        !nativeDynamics_WindReference(state.state.castle.wind,&state.wind))return false;
    const NativeMatchPool* userdata_pool=scene.kind==Gr_Kind_MuteCity ? &nativeItemPool : &nativeGroundPool;
    for (unsigned i=0;i<NATIVE_STAGE_HAZARD_ROOTS;++i) {
        if (root(&state.state,i)) {
            HSD_GObj* object=root(&state.state,i);
            if (!nativeMatchPool_Reference(&nativeGObjPool,object,&state.objects[i]))return false;
            if(scene.kind!=Gr_Kind_Izumi &&
                !nativeMatchPool_Reference(userdata_pool,object->user_data,&state.userdata[i]))return false;
            if(scene.kind==Gr_Kind_RCruise) {
                Ground* owner=nativeMatchPool_Resolve(state.userdata[i]);
                if(!owner)return false;
                if(owner->u.rcruise.x4 && !nativeDynamics_WindReference(owner->u.rcruise.x4,&state.wind))return false;
            }
        }
        HSD_JObj* model=joint(&state.state,i);
        if (model) {
            if (scene.joint_cache[i].pointer!=model || nativeClass_Resolve(scene.joint_cache[i].backing)!=model) {
                NativeClassReference ref;
                if (!(ref=nativeClass_Reference(model,sizeof(*model))).bytes) return false;
                scene.joint_cache[i].pointer=model;scene.joint_cache[i].backing=ref;
            }
            state.joints[i].backing=scene.joint_cache[i].backing;
            state.joints[i].type=HSD_CLASS_METHOD(model);
        }
    }
    if (!nativeStageHazards_Validate(&state,bytes) || !live_roots(&state)) return false;
    memcpy(data,&state,bytes);*written=bytes;return true;
}

bool nativeStageHazards_Load(const void* data,size_t bytes)
{
    if (!nativeStageHazards_Validate(data,bytes)) return false;
    NativeStageHazardSnapshot state;memcpy(&state,data,sizeof(state));
    // Actual GObj and Ground/Item payload companions restore first.
    // Castle borrows stable wind backing; Dynamics owns its payload.
    // The aggregate still must prevalidate their candidate graph agreement before any writes.
    if (!live_roots(&state)) return false;
    size_t n;void* record=current_record(&n);if (n) memcpy(record,&state.state,n);
    if(scene.kind==Gr_Kind_BigBlue && big_blue_state.parameters)big_blue_state.parameters->x18=state.big_blue_mode;
    arwing_conversation_selection=state.conversation_selection;return true;
}

#ifdef MELEE_NATIVE
bool nativeStageHazards_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(arwing_conversation_selection);
    size_t bytes;void* record=current_record(&bytes);
    if(bytes && !nativeMatchArena_AddRoot(arena,record,bytes))return false;
    if(scene.kind==Gr_Kind_BigBlue && big_blue_state.parameters)ROOT(big_blue_state.parameters->x18);
#undef ROOT
    return true;
}
#endif
