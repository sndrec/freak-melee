#ifndef MELEE_GR_NATIVE_STAGE_HAZARD_SNAPSHOT_H
#define MELEE_GR_NATIVE_STAGE_HAZARD_SNAPSHOT_H
#include "stage_hazard_state_internal.h"
#include <melee/gm/native_match_pool.h>
#include <sysdolphin/baselib/native_class_snapshot.h>
#include <melee/lb/native_dynamics_snapshot.h>

enum { NATIVE_STAGE_HAZARD_ROOTS=PURA_COLLISION_MAP_COUNT };
typedef struct StageHazardJointReference {
    NativeClassReference backing;
    HSD_ClassInfo* type;
} StageHazardJointReference;

typedef union StageHazardState {
    FountainState fountain;
    RainbowCruiseState rainbow_cruise;
    BigBlueState big_blue;
    PuraState pura;
    BattlefieldState battlefield;
    CastleState castle;
    MuteCityState mute_city;
    CorneriaState corneria;
    VenomState venom;
    GreensState greens;
    KraidState kraid;
    BrinstarState brinstar;
    KongoState kongo;
    IceMountainState ice_mountain;
} StageHazardState;
typedef struct NativeStageHazardSnapshot {
    uint64_t epoch;
    GrKind kind;
    int conversation_selection;
    StageHazardState state;
    NativeMatchPoolReference objects[NATIVE_STAGE_HAZARD_ROOTS], userdata[NATIVE_STAGE_HAZARD_ROOTS];
    StageHazardJointReference joints[NATIVE_STAGE_HAZARD_ROOTS];
    StageHazardJointReference reflection_camera;
    NativeWindReference wind;
    // Actual mutable loaded parameter; no duplicated live mirror.
    s32 big_blue_mode;
    u32 reserved;
} NativeStageHazardSnapshot;

void nativeStageHazards_BeginScene(void);
void nativeStageHazards_EndScene(void);
bool nativeStageHazards_Save(void*,size_t capacity,size_t* written);
bool nativeStageHazards_Validate(const void*,size_t bytes);
bool nativeStageHazards_Load(const void*,size_t bytes);
#endif
