#ifndef MELEE_GR_NATIVE_GREEN_GRID_H
#define MELEE_GR_NATIVE_GREEN_GRID_H
#include "types.h"
#include "native_ground_pool.h"
#include <melee/gm/native_match_pool.h>
#include <sysdolphin/baselib/native_class_snapshot.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

enum { GREEN_GRID_ROWS=5, GREEN_GRID_COLUMNS=6,
       GREEN_GRID_CAPACITY=NATIVE_GROUND_CAPACITY, GREEN_GRID_WORDS=(GREEN_GRID_CAPACITY+63)/64 };
typedef struct NativeGreenGrid {
    NativeMatchPoolReference actor, ground;
    Vec3 positions[GREEN_GRID_ROWS*GREEN_GRID_COLUMNS];
    struct grGreens_BlockVars blocks[GREEN_GRID_ROWS][GREEN_GRID_COLUMNS];
} NativeGreenGrid;

// One optional satellite per Ground slot; no limit below the Ground reservation.
extern NativeGreenGrid nativeGreenGrids[GREEN_GRID_CAPACITY];
extern uint64_t nativeGreenGridOccupied[GREEN_GRID_WORDS];
extern uint64_t nativeGreenGridLifetime;
extern uint32_t nativeGreenGridInitializedCount;
bool nativeGreenGrid_CanBeginScene(uint64_t epoch);
bool nativeGreenGrid_BeginScene(uint64_t epoch);
bool nativeGreenGrid_RetireScene(void);
bool nativeGreenGrid_Bind(HSD_GObj* actor);
bool nativeGreenGrid_DropOwner(NativeMatchPoolReference ground);

typedef struct NativeGreenGridSnapshotHeader {
    uint64_t lifetime;
    uint32_t count,initialized_count;
} NativeGreenGridSnapshotHeader;
// Claims for active cells only. Inactive cells retain dormant pointer bits,
// but never retain a live generation/type claim. Companion owners restore first.
typedef struct NativeGreenGridCellReferences {
    NativeMatchPoolReference block_actor, block_ground;
    NativeMatchPoolReference material_actor, material_item;
    NativeClassReference joint;
    HSD_ClassInfo* joint_type;
} NativeGreenGridCellReferences;
typedef struct NativeGreenGridSnapshotEntry {
    uint32_t slot,reserved;
    NativeGreenGrid grid;
    NativeGreenGridCellReferences cells[GREEN_GRID_ROWS][GREEN_GRID_COLUMNS];
} NativeGreenGridSnapshotEntry;
size_t nativeGreenGrid_SnapshotBytes(void);
bool nativeGreenGrid_Save(void*,size_t capacity,size_t* written);
bool nativeGreenGrid_Validate(const void*,size_t bytes);
bool nativeGreenGrid_Load(const void*,size_t bytes);
#include <melee/gm/native_match_arena.h>
bool nativeGreenGrid_AdmitHistory(NativeMatchArena*);
#endif
