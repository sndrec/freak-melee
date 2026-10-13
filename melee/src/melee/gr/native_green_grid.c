#include "native_green_grid.h"
#include "native_ground_pool.h"
#include <string.h>

// Actual optional storage. Only occupied records are serialized. Unoccupied
// reservation bytes are never part of the explicit snapshot payload.
NativeGreenGrid nativeGreenGrids[GREEN_GRID_CAPACITY];
uint64_t nativeGreenGridOccupied[GREEN_GRID_WORDS];
uint64_t nativeGreenGridLifetime;
uint32_t nativeGreenGridInitializedCount;

bool nativeGreenGrid_Bind(HSD_GObj* object)
{
    NativeMatchPoolReference actor,ground;
    if (!nativeGreenGridLifetime || nativeGroundPool.lifetime!=nativeGreenGridLifetime ||
        !nativeMatchPool_Reference(&nativeGObjPool,object,&actor) ||
        object->classifier!=HSD_GOBJ_CLASS_STAGE || object->user_data_kind!=3 ||
        !nativeMatchPool_Reference(&nativeGroundPool,object->user_data,&ground) ||
        object->user_data_remove_func!=nativeGroundPool_Release) return false;
    Ground* owner=object->user_data;
    uint32_t slot=ground.slot;
    if (slot>=GREEN_GRID_CAPACITY || owner->gobj!=object || owner->map_id!=6 ||
        (nativeGreenGridOccupied[slot/64] & (UINT64_C(1)<<(slot%64)))) return false;
    NativeGreenGrid* grid=&nativeGreenGrids[slot];memset(grid,0,sizeof(*grid));
    grid->actor=actor;grid->ground=ground;
    owner->u.greens.block_positions=grid->positions;
    owner->u.greens.blocks=grid->blocks;
    if (nativeGreenGridInitializedCount<slot+1) nativeGreenGridInitializedCount=slot+1;
    nativeGreenGridOccupied[slot/64]|=UINT64_C(1)<<(slot%64);
    return true;
}

bool nativeGreenGrid_DropOwner(NativeMatchPoolReference ground)
{
    if (ground.pool!=&nativeGroundPool || ground.slot>=GREEN_GRID_CAPACITY ||
        ground.lifetime!=nativeGreenGridLifetime || !ground.generation) return false;
    uint64_t bit=UINT64_C(1)<<(ground.slot%64);
    if (!(nativeGreenGridOccupied[ground.slot/64]&bit)) return true;
    NativeMatchPoolReference actual=nativeGreenGrids[ground.slot].ground;
    if (actual.lifetime!=ground.lifetime || actual.generation!=ground.generation) return false;
    nativeGreenGridOccupied[ground.slot/64]&=~bit;return true;
}
