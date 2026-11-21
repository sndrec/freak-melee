#include <melee/gm/native_match_control_history.h>
#include "native_particle_control_snapshot.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include "cobj.h"
#include "list.h"
#include <stdlib.h>
#include <string.h>

// Host lease; bank payloads/code remain resident for the scene/history lifetime.
static struct {uint64_t epoch;NativeParticleControlState definitions;bool active;} scene;
static void capture(NativeParticleControlState* s)
{
    memset(s,0,sizeof(*s));s->float_bits=particle_float_bits;s->appsrt_peak=hsd_804D78D8;
    s->work=particle_work_this_tick;
    s->display_frame=particle_display_frame;
    memcpy(s->sort_frames,particle_sort_frames,sizeof(s->sort_frames));
    memcpy(s->non_edge_heads,particle_non_edge_heads,sizeof(s->non_edge_heads));
    s->generator_peak=hsd_804D78DA;s->particle_peak=particle_peak_count;s->appsrt_count=hsd_804D78DE;
    s->generator_count=hsd_804D78E0;s->particle_count=particle_count;s->id_cursor=generator_id_cursor;
    memcpy(s->appsrt_config,appsrt_configuration,sizeof(s->appsrt_config));
    s->callbacks=particle_callbacks;s->matrix_hook=hsd_804D78E8;s->generator_hook=hsd_804D78EC;s->appsrt_hook=hsd_804D7900;
    s->camera=psCamera;s->deferred=hsd_804D78F4;s->pending=pending_generators;s->generators=hsd_804D78FC;
    memcpy(s->joints,particle_attachment_joints,sizeof(s->joints));memcpy(s->particles,hsd_804D0908,sizeof(s->particles));
    memcpy(s->references,hsd_804D0948,sizeof(s->references));memcpy(s->forms,psFormGroupArray,sizeof(s->forms));
    memcpy(s->textures,psTexGroupArray,sizeof(s->textures));memcpy(s->form_lists,psNumCmdList,sizeof(s->form_lists));
    memcpy(s->command_counts,psCmdListArray,sizeof(s->command_counts));memcpy(s->commands,ptclref_804D0E5C,sizeof(s->commands));
}
void nativeParticleControl_BeginScene(void)
{
    if(scene.epoch==UINT64_MAX)abort();++scene.epoch;scene.active=false;
    capture(&scene.definitions);scene.active=true;
}
void nativeParticleControl_EndScene(void) {scene.active=false;}
static bool boundary(void)
{return scene.active && nativeGmLoop_SnapshotBoundary() && nativeScriptRuntime_Idle();}
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);}
static bool independent(const void* data,size_t bytes)
{
    if(!data || !bytes || bytes>UINTPTR_MAX-(uintptr_t)data || overlap(data,bytes,&scene,sizeof(scene)))return false;
#define EXCLUDE(v) if(overlap(data,bytes,&(v),sizeof(v)))return false
    EXCLUDE(particle_display_frame);EXCLUDE(particle_sort_frames);EXCLUDE(particle_non_edge_heads);
    EXCLUDE(particle_work_this_tick);
    EXCLUDE(particle_float_bits);EXCLUDE(particle_callbacks);EXCLUDE(particle_peak_count);EXCLUDE(particle_count);
    EXCLUDE(hsd_804D78D8);EXCLUDE(hsd_804D78DA);EXCLUDE(hsd_804D78DE);EXCLUDE(hsd_804D78E0);
    EXCLUDE(hsd_804D78E8);EXCLUDE(hsd_804D78EC);EXCLUDE(hsd_804D7900);EXCLUDE(psCamera);EXCLUDE(hsd_804D78F4);
    EXCLUDE(pending_generators);EXCLUDE(hsd_804D78FC);EXCLUDE(generator_id_cursor);EXCLUDE(appsrt_configuration);
    EXCLUDE(particle_attachment_joints);EXCLUDE(hsd_804D0908);EXCLUDE(hsd_804D0948);EXCLUDE(psFormGroupArray);
    EXCLUDE(psTexGroupArray);EXCLUDE(psNumCmdList);EXCLUDE(psCmdListArray);EXCLUDE(ptclref_804D0E5C);
#undef EXCLUDE
    if(psCamera && overlap(data,bytes,psCamera,sizeof(HSD_CObj)))return false;
    for(unsigned i=0;i<8;++i)if(particle_attachment_joints[i] && overlap(data,bytes,particle_attachment_joints[i],sizeof(HSD_JObj)))return false;
    return nativeObjAlloc_BufferIndependent(data,bytes);
}
static bool definitions(const NativeParticleControlState* s)
{
    const NativeParticleControlState* d=&scene.definitions;
    return s->callbacks==d->callbacks && s->matrix_hook==d->matrix_hook && s->generator_hook==d->generator_hook && s->appsrt_hook==d->appsrt_hook &&
        !memcmp(s->appsrt_config,d->appsrt_config,sizeof(s->appsrt_config)) &&
        !memcmp(s->references,d->references,sizeof(s->references)) && !memcmp(s->forms,d->forms,sizeof(s->forms)) &&
        !memcmp(s->textures,d->textures,sizeof(s->textures)) && !memcmp(s->form_lists,d->form_lists,sizeof(s->form_lists)) &&
        !memcmp(s->command_counts,d->command_counts,sizeof(s->command_counts)) && !memcmp(s->commands,d->commands,sizeof(s->commands));
}
static bool slot(const void* object,NativeObjAllocReference r,HSD_ObjAllocData* allocator,size_t bytes)
{
    static const NativeObjAllocReference zero={0};
    return object ? r.allocator==allocator && r.bytes==bytes && nativeObjAlloc_Resolve(r)==object : !memcmp(&r,&zero,sizeof(r));
}
static bool model(const void* object,NativeClassReference r,HSD_ClassInfo* type,size_t bytes,const void* data,size_t length)
{
    static const NativeClassReference zero={0};
    return object ? r.bytes==bytes && nativeClass_Resolve(r)==object && nativeClass_TypeKnown(type,bytes) &&
        !overlap(data,length,object,bytes) && !overlap(data,length,type,type->head.info_size) : !type && !memcmp(&r,&zero,sizeof(r));
}
bool nativeParticleControl_Validate(const void* data,size_t bytes)
{
    if(!boundary() || bytes!=sizeof(NativeParticleControlSnapshot) || !independent(data,bytes))return false;
    NativeParticleControlSnapshot s;memcpy(&s,data,sizeof(s));
    if(s.epoch!=scene.epoch || !definitions(&s.state) || s.state.id_cursor<256)return false;
    if(s.state.work.generators>HSD_GENERATORS_PER_TICK || s.state.work.emissions>HSD_PARTICLES_PER_TICK ||
       s.state.work.particles>HSD_PARTICLES_PER_TICK || s.state.generator_count>HSD_GENERATORS_LIVE ||
       s.state.particle_count>HSD_PARTICLES_LIVE)return false;
    for(unsigned i=0;i<16;++i)if(!slot(s.state.particles[i],s.particles[i],&hsd_804D0F60.alloc_data,sizeof(HSD_Particle)))return false;
    for(unsigned i=0;i<17;++i)if(!slot(s.state.non_edge_heads[i],s.non_edge_heads[i],&hsd_804D0F60.alloc_data,sizeof(HSD_Particle)))return false;
    if(!slot(s.state.pending,s.generators[0],&hsd_804D0F90.alloc_data,sizeof(HSD_Generator)) ||
        !slot(s.state.generators,s.generators[1],&hsd_804D0F90.alloc_data,sizeof(HSD_Generator)) ||
        !slot(s.state.deferred,s.deferred,HSD_SListGetAllocData(),sizeof(HSD_SList)) ||
        !model(s.state.camera,s.camera,s.camera_type,sizeof(HSD_CObj),data,bytes))return false;
    for(unsigned i=0;i<8;++i)if(!model(s.state.joints[i],s.joints[i],s.joint_types[i],sizeof(HSD_JObj),data,bytes))return false;
    return true;
}
static bool live(const NativeParticleControlSnapshot* s)
{
    if(s->state.camera && HSD_CLASS_METHOD(s->state.camera)!=s->camera_type)return false;
    for(unsigned i=0;i<8;++i)if(s->state.joints[i] && HSD_CLASS_METHOD(s->state.joints[i])!=s->joint_types[i])return false;
    return true;
}
bool nativeParticleControl_Save(void* data,size_t capacity,size_t* written)
{
    size_t bytes=sizeof(NativeParticleControlSnapshot);
    if(!boundary() || capacity<bytes || !independent(data,bytes) || !independent(written,sizeof(*written)) || overlap(data,bytes,written,sizeof(*written)))return false;
    NativeParticleControlSnapshot s;memset(&s,0,sizeof(s));s.epoch=scene.epoch;capture(&s.state);
    for(unsigned i=0;i<16;++i)if(s.state.particles[i] && !(s.particles[i]=nativeObjAlloc_Reference(&hsd_804D0F60.alloc_data,s.state.particles[i],sizeof(HSD_Particle))).epoch)return false;
    for(unsigned i=0;i<17;++i)if(s.state.non_edge_heads[i] && !(s.non_edge_heads[i]=nativeObjAlloc_Reference(&hsd_804D0F60.alloc_data,s.state.non_edge_heads[i],sizeof(HSD_Particle))).epoch)return false;
    HSD_Generator* generators[2]={s.state.pending,s.state.generators};
    for(unsigned i=0;i<2;++i)if(generators[i] && !(s.generators[i]=nativeObjAlloc_Reference(&hsd_804D0F90.alloc_data,generators[i],sizeof(HSD_Generator))).epoch)return false;
    if(s.state.deferred && !(s.deferred=nativeObjAlloc_Reference(HSD_SListGetAllocData(),s.state.deferred,sizeof(HSD_SList))).epoch)return false;
    if(s.state.camera) {
        if(!(s.camera=nativeClass_Reference(s.state.camera,sizeof(HSD_CObj))).bytes)return false;s.camera_type=HSD_CLASS_METHOD(s.state.camera);
    }
    for(unsigned i=0;i<8;++i)if(s.state.joints[i]) {
        if(!(s.joints[i]=nativeClass_Reference(s.state.joints[i],sizeof(HSD_JObj))).bytes)return false;s.joint_types[i]=HSD_CLASS_METHOD(s.state.joints[i]);
    }
    if(!nativeParticleControl_Validate(&s,bytes) || !live(&s))return false;
    memcpy(data,&s,bytes);*written=bytes;return true;
}
bool nativeParticleControl_Load(const void* data,size_t bytes)
{
    if(!nativeParticleControl_Validate(data,bytes))return false;
    NativeParticleControlState current;capture(&current);
    if(!definitions(&current))return false;
    NativeParticleControlSnapshot s;memcpy(&s,data,sizeof(s));if(!live(&s))return false;
    // Payload/list/allocator candidates must agree before aggregate writes.
    // This control owner restores no allocation records and invokes no callbacks.
    particle_float_bits=s.state.float_bits;hsd_804D78D8=s.state.appsrt_peak;hsd_804D78DA=s.state.generator_peak;
    particle_work_this_tick=s.state.work;
    particle_peak_count=s.state.particle_peak;hsd_804D78DE=s.state.appsrt_count;hsd_804D78E0=s.state.generator_count;
    particle_count=s.state.particle_count;generator_id_cursor=s.state.id_cursor;
    particle_display_frame=s.state.display_frame;
    memcpy(particle_sort_frames,s.state.sort_frames,sizeof(s.state.sort_frames));
    memcpy(particle_non_edge_heads,s.state.non_edge_heads,sizeof(s.state.non_edge_heads));
    psCamera=s.state.camera;hsd_804D78F4=s.state.deferred;pending_generators=s.state.pending;hsd_804D78FC=s.state.generators;
    memcpy(particle_attachment_joints,s.state.joints,sizeof(s.state.joints));memcpy(hsd_804D0908,s.state.particles,sizeof(s.state.particles));
    return true;
}

#ifdef MELEE_NATIVE
bool nativeParticleControl_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(particle_float_bits);
    ROOT(particle_work_this_tick);
    ROOT(hsd_804D78D8);
    ROOT(hsd_804D78DA);
    ROOT(particle_peak_count);
    ROOT(hsd_804D78DE);
    ROOT(hsd_804D78E0);
    ROOT(particle_count);
    ROOT(generator_id_cursor);
    ROOT(particle_display_frame);
    ROOT(particle_sort_frames);
    ROOT(particle_non_edge_heads);
    ROOT(psCamera);
    ROOT(hsd_804D78F4);
    ROOT(pending_generators);
    ROOT(hsd_804D78FC);
    ROOT(particle_attachment_joints);
    ROOT(hsd_804D0908);
#undef ROOT
    return true;
}
#endif
