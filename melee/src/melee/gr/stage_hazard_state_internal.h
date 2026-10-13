#ifndef MELEE_GR_STAGE_HAZARD_STATE_INTERNAL_H
#define MELEE_GR_STAGE_HAZARD_STATE_INTERNAL_H
#include "types.h"

struct grCorneria_YakumonoParam;
struct grVenom_YakumonoParam;
struct grGreens_YakumonoParam;
struct grKraid_YakumonoParam;
struct grZe_YakumonoParam;
struct grKongo_YakumonoParam;
struct GrIceMtYakumonoParam;
struct grCastle_YakumonoParam;
struct grMc_YakumonoParam;
struct grBattle_YakumonoParam;
struct grBb_YakumonoParam;
struct grRCruise_YakumonoParam;
struct grIzumi_YakumonoParam;
struct lb_80011A50_t;

typedef struct KongoBankJoints {
    HSD_JObj* unk0;
    HSD_JObj* unk4;
} KongoBankJoints;
typedef struct KongoMotionLink {
    s16 unk0, unk2;
    HSD_JObj* unk4;
    f32 unk8, unkC, unk10, unk14;
} KongoMotionLink;
typedef struct KongoState {
    struct grKongo_YakumonoParam* parameters;
    KongoBankJoints banks;
    KongoMotionLink links[15];
} KongoState;
typedef struct IceMtRowData {
    int gobj_id;
    f32 x4, x8;
} IceMtRowData;
typedef struct IceMountainState {
    struct GrIceMtYakumonoParam* parameters;
    HSD_GObj* extra_rows[3];
    IceMtRowData rows[6];
} IceMountainState;

enum { BRINSTAR_BUBBLE_COUNT=20 };
typedef struct grZe_BubbleEntry {
    u8 x00_active, pad_01;
    s16 x02_timer;
    HSD_JObj* x04;
    f32 x08_x, x0C_y, x10, x14, x18_size, x1C;
    HSD_GObj* x20_gobj;
} grZe_BubbleEntry;
typedef struct grZe_BubbleState {
    Vec3 positions[4];
    grZe_BubbleEntry bubbles[BRINSTAR_BUBBLE_COUNT];
} grZe_BubbleState;
typedef struct BrinstarState {
    struct grZe_YakumonoParam* parameters;
    s32 bubble_cooldown;
    s16 debris_count;
    grZe_BubbleState bubbles;
} BrinstarState;

typedef struct CorneriaState {
    struct grCorneria_YakumonoParam* parameters;
    u32 current_slot;
    int spawn_timer;
    bool updates_disabled;
    int multi_arwing_mode;
    HSD_GObj* arwings[3];
    int types[3], groups[3];
    Vec3 cannon_positions[5];
} CorneriaState;
typedef struct VenomState {
    struct grVenom_YakumonoParam* parameters;
    u32 current_slot;
    s32 spawn_timer, updates_disabled, multi_arwing_mode;
    HSD_GObj* arwings[3];
    int types[3], groups[3];
} VenomState;
typedef struct GreensState {
    struct grGreens_YakumonoParam* parameters;
    u8 initializing, block_update_flag, block_states[32];
} GreensState;
typedef struct KraidState {
    struct grKraid_YakumonoParam* parameters;
    int debug_animation;
} KraidState;


enum { MUTE_CITY_CAR_COUNT=30 };
typedef struct grMc_CarEntry {
    f32 x0,x4,x8,xC,x10,x14,x18,x1C;
    u16 x20;
    struct { u8 b0:1,b1:1; } x22_flags;
    u8 x23;
    Item_GObj* material;
    s32 effect_active;
} grMc_CarEntry;
typedef struct MuteCityState {
    struct grMc_YakumonoParam* parameters;
    grMc_CarEntry cars[MUTE_CITY_CAR_COUNT];
    s32 order[MUTE_CITY_CAR_COUNT];
    s32 collision_disabled;
} MuteCityState;
typedef struct CastleState {
    struct grCastle_YakumonoParam* parameters;
    struct lb_80011A50_t* wind;
    f32 wind_angle_step;
    u8 strong_wind;
} CastleState;

enum { PURA_COLLISION_MAP_COUNT=44, PURA_UPDATED_MAP_COUNT=42 };
struct GrPuVtxMapEntry {
    GrJoint joint;
    s16 x06;
    HSD_JObj* jobj;
};
typedef struct PuraState {
    void* parameters;
    struct GrPuVtxMapEntry maps[PURA_COLLISION_MAP_COUNT];
} PuraState;
typedef struct BattlefieldState {
    struct grBattle_YakumonoParam* parameters;
    GXBool is_demo;
} BattlefieldState;
enum { BIG_BLUE_TRACK_COUNT=12 };
typedef struct grBb_TrackEntry {
    s16 jobj_index,start_index,end_index,pad;
    Vec3 delta;
} grBb_TrackEntry;
typedef struct BigBlueState {
    struct grBb_YakumonoParam* parameters;
    grBb_TrackEntry tracks[BIG_BLUE_TRACK_COUNT];
} BigBlueState;
extern BigBlueState big_blue_state;
enum { RAINBOW_CRUISE_ENTRIES=17, RAINBOW_CRUISE_VANISH=20 };
typedef struct RainbowCruiseState {
    struct grRCruise_YakumonoParam* parameters;
    HSD_GObj* owner;
    struct grRCruise_Entry entries[RAINBOW_CRUISE_ENTRIES];
    struct Map_VanishEntry vanish[RAINBOW_CRUISE_VANISH];
} RainbowCruiseState;
extern RainbowCruiseState rainbow_cruise_state;
typedef struct IzumiReflection {
    Mtx texture_matrix;
    HSD_ImageDesc* image;
} IzumiReflection;
typedef struct FountainState {
    struct grIzumi_YakumonoParam* parameters;
    HSD_GObj* owner;
    IzumiReflection reflection;
} FountainState;
extern FountainState fountain_state;
void nativeFountain_ReleaseReflection(void* data);
extern PuraState pura_state;
extern BattlefieldState battlefield_state;
extern CastleState castle_state;
extern MuteCityState mute_city_state;
extern CorneriaState corneria_state;
extern VenomState venom_state;
extern GreensState greens_state;
extern KraidState kraid_state;
extern BrinstarState brinstar_state;
extern KongoState kongo_state;
extern IceMountainState ice_mountain_state;
extern int arwing_conversation_selection;
#endif
