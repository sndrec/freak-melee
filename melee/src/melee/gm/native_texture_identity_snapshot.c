#include <melee/gm/native_match_control_history.h>
#include "native_texture_identity_snapshot.h"
#include "native_gm_loop_snapshot.h"
#include <melee/ft/native_script_runtime.h>
#include <dolphin/gx/GXTexture.h>
#include <string.h>

static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool regions(GXTextureStateRegion state[2])
{
    if(!nativeGmLoop_SnapshotBoundary() || !nativeScriptRuntime_Idle() ||
       GXGetTextureStateRegions(state,2)!=2)return false;
    for(unsigned i=0;i<2;++i)
        if(!state[i].memory || state[i].size!=sizeof(uint32_t) ||
           state[i].size>UINTPTR_MAX-(uintptr_t)state[i].memory)return false;
    return !overlap(state[0].memory,state[0].size,state[1].memory,state[1].size);
}
static bool independent(const GXTextureStateRegion state[2],const void* data,size_t bytes)
{
    return data && bytes && bytes<=UINTPTR_MAX-(uintptr_t)data &&
        !overlap(data,bytes,state[0].memory,state[0].size) &&
        !overlap(data,bytes,state[1].memory,state[1].size);
}
bool nativeTextureIdentity_Save(void* data,size_t capacity,size_t* written)
{
    GXTextureStateRegion state[2];const size_t bytes=sizeof(NativeTextureIdentitySnapshot);
    if(!regions(state) || capacity<bytes || !independent(state,data,bytes) ||
       !independent(state,written,sizeof(*written)) || overlap(data,bytes,written,sizeof(*written)))return false;
    NativeTextureIdentitySnapshot snapshot;
    memcpy(&snapshot.texture,state[0].memory,sizeof(snapshot.texture));
    memcpy(&snapshot.palette,state[1].memory,sizeof(snapshot.palette));
    memcpy(data,&snapshot,bytes);*written=bytes;return true;
}
bool nativeTextureIdentity_Validate(const void* data,size_t bytes)
{
    GXTextureStateRegion state[2];
    // All counter bit patterns are valid captured state. Zero preserves the
    // original exhaustion condition: the next source allocation reports overflow.
    return bytes==sizeof(NativeTextureIdentitySnapshot) && regions(state) &&
        independent(state,data,bytes);
}
bool nativeTextureIdentity_Load(const void* data,size_t bytes)
{
    GXTextureStateRegion state[2];
    if(bytes!=sizeof(NativeTextureIdentitySnapshot) || !regions(state) ||
       !independent(state,data,bytes))return false;
    NativeTextureIdentitySnapshot snapshot;memcpy(&snapshot,data,bytes);
    memcpy(state[0].memory,&snapshot.texture,sizeof(snapshot.texture));
    memcpy(state[1].memory,&snapshot.palette,sizeof(snapshot.palette));return true;
}

#ifdef MELEE_NATIVE
bool nativeTextureIdentity_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    GXTextureStateRegion state[2];if(!regions(state))return false;
    for(unsigned i=0;i<2;++i)if(!nativeMatchArena_AddRoot(arena,state[i].memory,state[i].size))return false;
#undef ROOT
    return true;
}
#endif
