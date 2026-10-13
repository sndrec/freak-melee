#include "native_match_arena.h"
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_pool.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/native_item_link_pool.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// Immutable scene admission and backing identity are host state. Only the live
// prefix is game state; the broad migration oracle includes backing separately.
#pragma clang section data = "" bss = ""
NativeMatchArena nativeMatchSceneArena;
static void* backing;
NativeMatchActorStorage nativeMatchActorStorage;
static bool reserve_actors(void)
{
    if(nativeMatchActorStorage.memory)return true;
    size_t sizes[]={sizeof(NativeFighterPoolSlot)*NATIVE_FIGHTER_POOL_CAPACITY,
        sizeof(Ground)*NATIVE_GROUND_CAPACITY,
        sizeof(NativeItemPoolSlot)*NATIVE_ITEM_CAPACITY,
        sizeof(ItemLink)*NATIVE_ITEM_LINK_CAPACITY,
        sizeof(HSD_GObj)*NATIVE_GOBJ_CAPACITY,sizeof(HSD_GObjProc)*NATIVE_GOBJ_PROC_CAPACITY};
    size_t offsets[6],bytes=0;
    for(unsigned i=0;i<6;++i){offsets[i]=bytes;bytes=(bytes+sizes[i]+63)&~(size_t)63;}
    uint8_t* memory=VirtualAlloc(NULL,bytes,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!memory)return false;
    memset(memory,0,bytes);
    nativeMatchActorStorage=(NativeMatchActorStorage){memory,bytes,
        memory+offsets[0],memory+offsets[1],memory+offsets[2],memory+offsets[3],
        memory+offsets[4],memory+offsets[5]};
    return true;
}
static uint64_t history_epoch;
bool nativeMatchScene_Begin(void)
{
    if(nativeMatchSceneArena.lifetime || history_epoch==UINT64_MAX || !reserve_actors())return false;
    if(!backing){
        backing=VirtualAlloc(NULL,NATIVE_MATCH_SCENE_BYTES,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(!backing)return false;
        // Pay first-touch costs before running scene constructors/ticks.
        memset(backing,0,NATIVE_MATCH_SCENE_BYTES);
    }
    return nativeMatchArena_Begin(&nativeMatchSceneArena,backing,NATIVE_MATCH_SCENE_BYTES,++history_epoch);
}
bool nativeMatchScene_ResetHistory(void)
{
    if(!nativeMatchSceneArena.lifetime || history_epoch==UINT64_MAX)return false;
    nativeMatchSceneArena.lifetime=++history_epoch;
    nativeMatchSceneArena.root_count=0;nativeMatchSceneArena.root_bytes=0;
    nativeMatchSceneArena.sealed=false;
    return true;
}
void nativeMatchScene_Retire(void)
{
    nativeMatchSceneArena.lifetime=0;
}
#endif
