#include <melee/gm/native_match_control_history.h>
#include "native_light_control_snapshot.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <string.h>

static bool boundary(void) {return nativeGmLoop_SnapshotBoundary() && nativeScriptRuntime_Idle();}
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool independent(const void* data,size_t bytes)
{
    if(!data || !bytes || bytes>UINTPTR_MAX-(uintptr_t)data)return false;
#define EXCLUDE(v) if(overlap(data,bytes,&(v),sizeof(v)))return false
    EXCLUDE(light_default_class);EXCLUDE(light_current_list);EXCLUDE(light_active);EXCLUDE(light_active_count);
    EXCLUDE(light_diffuse_mask);EXCLUDE(light_specular_mask);EXCLUDE(light_attenuation_mask);EXCLUDE(light_alpha_mask);
#undef EXCLUDE
    for(unsigned i=0;i<MAX_GXLIGHT;++i)
        if(light_active[i] && overlap(data,bytes,light_active[i],sizeof(HSD_LObj)))return false;
    return nativeObjAlloc_BufferIndependent(data,bytes);
}
static void capture(NativeLightControlState* state)
{
    memset(state,0,sizeof(*state));state->definition=light_default_class;state->current=light_current_list;
    memcpy(state->active,light_active,sizeof(state->active));state->count=light_active_count;
    state->diffuse=light_diffuse_mask;state->specular=light_specular_mask;
    state->attenuation=light_attenuation_mask;state->alpha=light_alpha_mask;
}
bool nativeLightControl_Validate(const void* data,size_t bytes)
{
    if(!boundary() || bytes!=sizeof(NativeLightControlSnapshot) || !independent(data,bytes))return false;
    NativeLightControlSnapshot s;memcpy(&s,data,sizeof(s));
    if(s.state.definition!=light_default_class || s.state.count<0 || s.state.count>=MAX_GXLIGHT)return false;
    static const NativeObjAllocReference zero_list={0};
    static const NativeClassReference zero_class={0};
    if(s.state.current) {
        if(s.list.allocator!=HSD_SListGetAllocData() || s.list.bytes!=sizeof(HSD_SList) || nativeObjAlloc_Resolve(s.list)!=s.state.current)return false;
    } else if(memcmp(&s.list,&zero_list,sizeof(s.list)))return false;
    for(unsigned i=0;i<MAX_GXLIGHT;++i) {
        if(!s.state.active[i]) {
            if(s.types[i] || memcmp(&s.lights[i],&zero_class,sizeof(s.lights[i])))return false;
        } else if(s.lights[i].bytes!=sizeof(HSD_LObj) || nativeClass_Resolve(s.lights[i])!=s.state.active[i] ||
                  !nativeClass_TypeKnown(s.types[i],sizeof(HSD_LObj)) ||
                  overlap(data,bytes,s.state.active[i],sizeof(HSD_LObj)) ||
                  overlap(data,bytes,s.types[i],s.types[i]->head.info_size))return false;
    }
    return true;
}
bool nativeLightControl_Save(void* data,size_t capacity,size_t* written)
{
    const size_t bytes=sizeof(NativeLightControlSnapshot);
    if(!boundary() || capacity<bytes || !independent(data,bytes) || !independent(written,sizeof(*written)) || overlap(data,bytes,written,sizeof(*written)))return false;
    NativeLightControlSnapshot s;memset(&s,0,sizeof(s));capture(&s.state);
    if(s.state.current && !(s.list=nativeObjAlloc_Reference(HSD_SListGetAllocData(),s.state.current,sizeof(HSD_SList))).epoch)return false;
    for(unsigned i=0;i<MAX_GXLIGHT;++i)if(s.state.active[i]) {
        s.lights[i]=nativeClass_Reference(s.state.active[i],sizeof(HSD_LObj));
        if(!s.lights[i].bytes)return false;s.types[i]=HSD_CLASS_METHOD(s.state.active[i]);
    }
    if(!nativeLightControl_Validate(&s,bytes))return false;
    memcpy(data,&s,bytes);*written=bytes;return true;
}
bool nativeLightControl_Load(const void* data,size_t bytes)
{
    if(!nativeLightControl_Validate(data,bytes))return false;
    NativeLightControlSnapshot s;memcpy(&s,data,sizeof(s));
    for(unsigned i=0;i<MAX_GXLIGHT;++i)
        if(s.state.active[i] && HSD_CLASS_METHOD(s.state.active[i])!=s.types[i])return false;
    // The ObjAlloc/class companions already restored list nodes, light payloads
    // and reference counts. Restore heads directly, without add/delete/ref calls.
    light_current_list=s.state.current;memcpy(light_active,s.state.active,sizeof(s.state.active));
    light_active_count=s.state.count;light_diffuse_mask=s.state.diffuse;light_specular_mask=s.state.specular;
    light_attenuation_mask=s.state.attenuation;light_alpha_mask=s.state.alpha;return true;
}

#ifdef MELEE_NATIVE
bool nativeLightControl_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(light_current_list);
    ROOT(light_active);
    ROOT(light_active_count);
    ROOT(light_diffuse_mask);
    ROOT(light_specular_mask);
    ROOT(light_attenuation_mask);
    ROOT(light_alpha_mask);
#undef ROOT
    return true;
}
#endif
