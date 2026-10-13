#ifndef HSD_NATIVE_PARTICLE_CONTROL_SNAPSHOT_H
#define HSD_NATIVE_PARTICLE_CONTROL_SNAPSHOT_H
#include "particle_state_internal.h"
#include "psdisp_state_internal.h"
#include "native_objalloc_snapshot.h"
#include "native_class_snapshot.h"

typedef struct NativeParticleControlState {
    u32 float_bits;
    u8 display_frame,sort_frames[16];
    HSD_Particle* non_edge_heads[17];
    u16 appsrt_peak,generator_peak,particle_peak,appsrt_count,generator_count,particle_count,id_cursor,appsrt_config[4];
    int (**callbacks)(HSD_Particle*);
    void (*matrix_hook)(HSD_Generator*,Mtx);
    void (*generator_hook)(HSD_Generator*);
    void (*appsrt_hook)(HSD_Generator*);
    HSD_CObj* camera;
    HSD_SList* deferred;
    HSD_Generator* pending;
    HSD_Generator* generators;
    HSD_JObj* joints[8];
    HSD_Particle* particles[16];
    u32* references[65];
    HSD_PSFormGroup** forms[65];
    HSD_PSTexGroup** textures[65];
    HSD_PSFormGroup** form_lists[65];
    int command_counts[65];
    HSD_PSCmdList** commands[65];
} NativeParticleControlState;
typedef struct NativeParticleControlSnapshot {
    uint64_t epoch;
    NativeParticleControlState state;
    NativeObjAllocReference particles[16],non_edge_heads[17],generators[2],deferred;
    NativeClassReference camera,joints[8];
    HSD_ClassInfo* camera_type;
    HSD_ClassInfo* joint_types[8];
} NativeParticleControlSnapshot;
void nativeParticleControl_BeginScene(void);
void nativeParticleControl_EndScene(void);
bool nativeParticleControl_Save(void*,size_t,size_t*);
bool nativeParticleControl_Validate(const void*,size_t);
bool nativeParticleControl_Load(const void*,size_t);
#endif
