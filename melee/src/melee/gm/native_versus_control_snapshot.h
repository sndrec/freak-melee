#ifndef MELEE_GM_NATIVE_VERSUS_CONTROL_SNAPSHOT_H
#define MELEE_GM_NATIVE_VERSUS_CONTROL_SNAPSHOT_H
#include "types.h"
#include "gm_score_state_internal.h"
#include "native_match_pool.h"
#include <melee/it/types.h>
#include <melee/ft/forward.h>
#include <melee/if/types.h>
#include <melee/if/ifall_state_internal.h>
#include <melee/if/iftime_state_internal.h>
#include <melee/if/ifstock_state_internal.h>
#include <melee/sfx/crowdsfx.h>
#include <sysdolphin/baselib/native_class_snapshot.h>

typedef struct NativeVersusControlSnapshot {
    uint64_t epoch;
    GmScoreState score_state;
    u32 item_serial;
    HSD_ObjAllocUnk item_population;
    int fighter_references[Ft_Kind_RuntimeCapacity];
    Element_803F9628 messages[8];
    NativeMatchPoolReference message_objects[8];
    HSD_GObj* stock_objects[12];
    NativeMatchPoolReference stock_object_refs[12];
    IfTimeState timers;
    NativeMatchPoolReference timer_objects[2];
    IfStockState stock_hud;
    NativeMatchPoolReference stock_hud_objects[IfStock_ObjectCount];
    Vec3 hud_timer_position,hud_player_positions[6],hud_three_positions[3],hud_two_positions[2];
    u8 hud_hidden;
    u8 nametag_hidden,nametag_force_visible[Gm_Player_NumMax];
    HSD_CObj* debug_text_camera;
    NativeClassReference debug_text_camera_backing;
    HSD_ClassInfo* debug_text_camera_type;
    CrowdSFX_UnkStruct crowd;
    VsSceneController versus;
    RandomItemSpawner spawner;
    ItemPickTable picks[2];
    HudIndex hud;
    u8 hud_count;
    s8 rounding;
    u8 kinds[3][UINT8_MAX];
    u16 weights[3][UINT8_MAX];
    NativeMatchPoolReference objects[GM_MAX_PLAYERS][2];
    NativeClassReference joints[GM_MAX_PLAYERS][HUD_PLACE_MAX];
    HSD_ClassInfo* joint_types[GM_MAX_PLAYERS][HUD_PLACE_MAX];
} NativeVersusControlSnapshot;
void nativeVersusControl_BeginScene(void);
void nativeVersusControl_EndScene(void);
bool nativeVersusControl_Save(void*,size_t,size_t*);
bool nativeVersusControl_Validate(const void*,size_t);
bool nativeVersusControl_Load(const void*,size_t);
bool nativeVersusControl_ValidateClass(const void*,size_t,NativeClassSnapshotView);
#endif
