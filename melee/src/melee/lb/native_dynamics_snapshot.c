#include <melee/gm/native_match_control_history.h>
#include "native_dynamics_snapshot.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <stdlib.h>
#include <string.h>

// Host lifetime, not simulation state. Reinitialization invalidates history.
static struct { uint64_t epoch;bool active; } scene;
void nativeDynamics_BeginScene(void)
{
    if(scene.epoch==UINT64_MAX)abort();
    ++scene.epoch;scene.active=true;
}
void nativeDynamics_EndScene(void) { scene.active=false; }
static bool boundary(void)
{ return scene.active && nativeGmLoop_SnapshotBoundary() && nativeScriptRuntime_Idle(); }
static bool overlap(const void* first,size_t an,const void* second,size_t bn)
{
    uintptr_t a=(uintptr_t)first,b=(uintptr_t)second;
    return an>UINTPTR_MAX-a || bn>UINTPTR_MAX-b || (a<b+bn && b<a+an);
}
static bool independent(const void* data,size_t bytes)
{
    return data && bytes && bytes<=UINTPTR_MAX-(uintptr_t)data &&
        !overlap(data,bytes,&scene,sizeof(scene)) &&
        !overlap(data,bytes,&dynamics_state,sizeof(dynamics_state));
}
// Addresses remain the actual engine addresses, never offsets into the copied
// candidate. Numeric lookup precedes every candidate link traversal.
static int index_of(const void* pointer,const void* base,size_t stride,unsigned count)
{
    if(!pointer)return -1;
    uintptr_t at=(uintptr_t)pointer,start=(uintptr_t)base;
    if(at<start || (at-start)%stride || (at-start)/stride>=count)return -2;
    return (int)((at-start)/stride);
}
static int node_index(const struct DynamicsData* pointer)
{ return index_of(pointer,dynamics_state.nodes,sizeof(*pointer),DYNAMICS_NODE_COUNT); }
static int wind_index(const struct lb_80011A50_t* pointer)
{ return index_of(pointer,dynamics_state.wind,sizeof(*pointer),WIND_FIELD_COUNT); }
bool nativeDynamics_WindReference(const struct lb_80011A50_t* pointer,NativeWindReference* out)
{
    int slot=wind_index(pointer);
    if(!scene.active || slot<0 || !independent(out,sizeof(*out)))return false;
    *out=(NativeWindReference){scene.epoch,(uint32_t)slot,0};return true;
}
struct lb_80011A50_t* nativeDynamics_ResolveWind(NativeWindReference ref)
{
    if(!scene.active || ref.epoch!=scene.epoch || ref.reserved || ref.slot>=WIND_FIELD_COUNT)return NULL;
    return &dynamics_state.wind[ref.slot];
}
static bool shape(const NativeDynamicsState* state)
{
    u8 free_nodes[DYNAMICS_NODE_COUNT]={0},color[DYNAMICS_NODE_COUNT]={0};
    u8 incoming[DYNAMICS_NODE_COUNT]={0};
    int next[DYNAMICS_NODE_COUNT];
    for(unsigned i=0;i<DYNAMICS_NODE_COUNT;++i) {
        next[i]=node_index(state->nodes[i].next);
        if(next[i]==-2)return false;
    }
    int current=node_index(state->free_nodes);
    if(current==-2)return false;
    while(current>=0) {
        if(free_nodes[current])return false;
        free_nodes[current]=1;current=next[current];
    }
    // Allocated dynamics form disjoint forward chains. Free nodes deliberately
    // keep old joint/pose bytes after release; do not treat them as live roots.
    for(unsigned i=0;i<DYNAMICS_NODE_COUNT;++i)if(!free_nodes[i] && next[i]>=0) {
        if(free_nodes[next[i]] || ++incoming[next[i]]>1)return false;
    }
    for(unsigned i=0;i<DYNAMICS_NODE_COUNT;++i)if(!free_nodes[i] && !color[i]) {
        current=(int)i;
        while(current>=0 && !color[current]) { color[current]=1;current=next[current]; }
        if(current>=0 && color[current]==1)return false;
        current=(int)i;
        while(current>=0 && color[current]==1) { color[current]=2;current=next[current]; }
    }
    u8 seen[WIND_FIELD_COUNT]={0};
    const struct lb_80011A50_t* heads[]={state->free_wind,state->active_wind};
    unsigned visited=0;
    for(unsigned list=0;list<2;++list) {
        current=wind_index(heads[list]);
        if(current==-2)return false;
        while(current>=0) {
            if(seen[current])return false;
            seen[current]=1;++visited;
            current=wind_index(state->wind[current].next);
            if(current==-2)return false;
        }
    }
    return visited==WIND_FIELD_COUNT && state->wind_status>=-1 && state->wind_status<=2;
}
bool nativeDynamics_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeDynamicsSnapshot);
    if(!boundary() || capacity<bytes || !independent(data,bytes) ||
        !independent(written,sizeof(*written)) || overlap(data,bytes,written,sizeof(*written)) ||
        !shape(&dynamics_state))return false;
    memcpy(data,&scene.epoch,sizeof(scene.epoch));
    memcpy((u8*)data+offsetof(NativeDynamicsSnapshot,state),&dynamics_state,sizeof(dynamics_state));
    *written=bytes;return true;
}
bool nativeDynamics_Validate(const void* data,size_t bytes)
{
    if(!boundary() || bytes!=sizeof(NativeDynamicsSnapshot) || !independent(data,bytes))return false;
    NativeDynamicsSnapshot state;memcpy(&state,data,sizeof(state));
    return state.epoch==scene.epoch && shape(&state.state);
}
bool nativeDynamics_Load(const void* data,size_t bytes)
{
    if(!nativeDynamics_Validate(data,bytes))return false;
    // External DynamicsDesc heads/counts, model joints and caller-held wind
    // pointers belong to companion owners. Aggregate validation must agree on
    // this same candidate before writes; this component never follows them.
    memcpy(&dynamics_state,(const u8*)data+offsetof(NativeDynamicsSnapshot,state),sizeof(dynamics_state));
    return true;
}

#ifdef MELEE_NATIVE
bool nativeDynamics_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(dynamics_state);
#undef ROOT
    return true;
}
#endif
