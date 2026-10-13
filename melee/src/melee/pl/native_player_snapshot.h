#ifndef MELEE_PL_NATIVE_PLAYER_SNAPSHOT_H
#define MELEE_PL_NATIVE_PLAYER_SNAPSHOT_H

#include "player.h"
#include <melee/gm/native_match_pool.h>

typedef struct NativePlayerRoots {
    NativeMatchPoolReference object[2], fighter[2];
} NativePlayerRoots;

typedef struct NativePlayerSnapshotHeader {
    uint64_t epoch, object_epoch, fighter_epoch;
    pl_804D6470_t* common_data;
    u16 stale_instance, attack_instance;
    u32 reserved;
} NativePlayerSnapshotHeader;

// Actual six player records, including stale queues and extended statistics.
// Pointed fighter/GObj payloads are separate companions restored before Load.
typedef struct NativePlayerSnapshot {
    NativePlayerSnapshotHeader header;
    StaticPlayer players[Gm_Player_NumMax];
    NativePlayerRoots roots[Gm_Player_NumMax];
} NativePlayerSnapshot;

// Freeze the actual family mapping/common-data lifetime at loop entry; no
// constructors or gameplay initialization, and no allocation on save/load.
void nativePlayerSnapshot_BeginScene(void);
void nativePlayerSnapshot_EndScene(void);
bool nativePlayerSnapshot_Save(void*, size_t capacity, size_t* written);
bool nativePlayerSnapshot_Validate(const void*, size_t bytes);
bool nativePlayerSnapshot_Load(const void*, size_t bytes);

#endif
