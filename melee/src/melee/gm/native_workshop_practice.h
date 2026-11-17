#ifndef MELEE_GM_NATIVE_WORKSHOP_PRACTICE_H
#define MELEE_GM_NATIVE_WORKSHOP_PRACTICE_H
#include "native_content_store.h"
#include <dolphin/pad.h>
#include <melee/mn/forward.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NativeWorkshopPractice_Ports = 4,
    NativeWorkshopPractice_Fighters = 10,
    NativeWorkshopPractice_MaxRun = 600,
    NativeWorkshopPractice_TraceFrames = 1200,
    NativeWorkshopPractice_TraceHits = 16,
    NativeWorkshopPractice_TraceItems = 16,
    NativeWorkshopPractice_TraceFaults = 4,
};

typedef struct NativeWorkshopPracticeSeat {
    NativeContentId package;
    int character, spawn, facing;
    unsigned damage, cpu_level;
    bool present;
} NativeWorkshopPracticeSeat;

typedef struct NativeWorkshopPracticeConfig {
    uint32_t seed;
    char fighter_revision[65];
    NativeWorkshopPracticeSeat seats[NativeWorkshopPractice_Ports];
    unsigned sandbag_damage;
    bool sandbag;
} NativeWorkshopPracticeConfig;

typedef enum NativeWorkshopPracticeStatus {
    NativeWorkshopPractice_Closed,
    NativeWorkshopPractice_Opening,
    NativeWorkshopPractice_Ready,
    NativeWorkshopPractice_Running,
    NativeWorkshopPractice_CheckpointPending,
    NativeWorkshopPractice_Failed,
} NativeWorkshopPracticeStatus;

typedef enum NativeWorkshopPracticeRequest {
    NativeWorkshopPractice_Save,
    NativeWorkshopPractice_Restore,
    NativeWorkshopPractice_Reset,
} NativeWorkshopPracticeRequest;

typedef struct NativeWorkshopPracticeState {
    NativeWorkshopPracticeStatus status;
    uint32_t id, revision, frame, target_frame, oldest_frame, checkpoint_frame;
    uint32_t seed;
    bool has_checkpoint;
    NativeContentId package;
    char fighter_revision[65];
    char error[192];
} NativeWorkshopPracticeState;

typedef struct NativeWorkshopPracticeFighter {
    bool present, airborne;
    int kind, action, hitboxes;
    float animation_frame, damage, x, y, vx, vy, facing, hitlag;
    uint64_t script_state;
} NativeWorkshopPracticeFighter;

typedef struct NativeWorkshopPracticeHit {
    int item_kind, victim, attacker;
    float damage;
} NativeWorkshopPracticeHit;
typedef struct NativeWorkshopPracticeItem {
    int kind, action, owner;
    float x, y, vx, vy;
} NativeWorkshopPracticeItem;
typedef struct NativeWorkshopPracticeFault {
    unsigned generation, line, column;
    const char* source_kind;
    char message[192];
} NativeWorkshopPracticeFault;

typedef struct NativeWorkshopPracticeTrace {
    uint32_t frame, rng, items, projectiles;
    NativeWorkshopPracticeFighter fighters[NativeWorkshopPractice_Fighters];
    unsigned hit_count, fault_count;
    NativeWorkshopPracticeHit hits[NativeWorkshopPractice_TraceHits];
    NativeWorkshopPracticeItem item_rows[NativeWorkshopPractice_TraceItems];
    NativeWorkshopPracticeFault faults[NativeWorkshopPractice_TraceFaults];
} NativeWorkshopPracticeTrace;

struct Item;
struct Fighter;
void nativeWorkshopPractice_ItemHit(const struct Item*, const struct Fighter*, float damage);
void nativeWorkshopPractice_ScriptFault(const char* source_kind, unsigned generation,
    unsigned line, unsigned column, const char* message);
bool nativeWorkshopPractice_Arm(const NativeWorkshopPracticeConfig*);
int nativeWorkshopPractice_StockCharacter(unsigned fighter_id);
void nativeWorkshopPractice_CancelStart(const char* error);
bool nativeWorkshopPractice_Packages(NativeContentId packages[4]);
void nativeWorkshopPractice_Configure(StartMeleeData*, const uint8_t kinds[4]);
void nativeWorkshopPractice_Dummy(StartMeleeData*);
void nativeWorkshopPractice_EnterScene(void);
void nativeWorkshopPractice_BeginLoop(void);
void nativeWorkshopPractice_Boundary(void);
void nativeWorkshopPractice_EndFrame(void);
void nativeWorkshopPractice_EndLoop(void);
bool nativeWorkshopPractice_Controlled(void);
bool nativeWorkshopPractice_Stepping(void);
const NativeWorkshopPracticeState* nativeWorkshopPractice_State(void);
const NativeWorkshopPracticeTrace* nativeWorkshopPractice_Trace(uint32_t frame);
bool nativeWorkshopPractice_Run(const PADStatus (*pads)[4], unsigned frames);
bool nativeWorkshopPractice_Checkpoint(NativeWorkshopPracticeRequest);

#ifdef __cplusplus
}
#endif
#endif
