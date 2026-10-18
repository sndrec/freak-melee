#include <sysdolphin/baselib/native_particle_control_snapshot.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/psappsrt.h>
#include <sysdolphin/baselib/list.h>
#include <sysdolphin/baselib/psdisp.h>
#include <sysdolphin/baselib/particle.h>
#include "reference_benchmark.h"
static unsigned particle_control_checks,particle_control_refusals;
static unsigned char particle_payloads_saved[65536];
static void test_particle_controls(void)
{
    unsigned first=checks;camera_idle=true;
    HSD_ObjAllocInit(&hsd_804D0F60.alloc_data,sizeof(HSD_Particle),8);
    HSD_ObjAllocInit(&hsd_804D0F90.alloc_data,sizeof(HSD_Generator),8);HSD_ListInitAllocData();
    psInitAppSRT(0,sizeof(HSD_psAppSRT));
    CHECK(sizeof(HSD_psAppSRT)>0xA4 && appsrt_configuration[0]==sizeof(HSD_psAppSRT));
    HSD_psAppSRT* srt=psAddGeneratorAppSRT(1,256);CHECK(srt);
    CHECK(HSD_PSAppSrt_804D10B0.size>=sizeof(*srt) && srt->usedCount==1 && srt->scale.x==1);
    NativeObjAllocReference srt_ref;CHECK((srt_ref=nativeObjAlloc_Reference(&HSD_PSAppSrt_804D10B0,srt,sizeof(*srt))).epoch);
    HSD_Particle* particles[2];HSD_Generator* generators[2];
    for(unsigned i=0;i<2;++i) {
        particles[i]=HSD_ObjAlloc(&hsd_804D0F60.alloc_data);generators[i]=HSD_ObjAlloc(&hsd_804D0F90.alloc_data);CHECK(particles[i] && generators[i]);
        memset(particles[i],0,sizeof(*particles[i]));memset(generators[i],0,sizeof(*generators[i]));
    }
    HSD_JObj* joint=hsdNew(&stage_joint_class);HSD_CObj* camera=hsdNew(&camera_class);CHECK(joint && camera);
    HSD_SList* list=HSD_SListAllocAndAppend(NULL,generators[0]);CHECK(list);
    memset(hsd_804D0908,0,sizeof(hsd_804D0908));memset(particle_attachment_joints,0,sizeof(particle_attachment_joints));
    hsd_804D0908[0]=particles[0];hsd_804D0908[15]=particles[1];particle_attachment_joints[7]=joint;
    pending_generators=generators[0];hsd_804D78FC=generators[1];hsd_804D78F4=list;psCamera=camera;
    generator_id_cursor=65535;CHECK(hsd_8039D1EC()==256);CHECK(hsd_8039D1EC()==257);
    particle_count=2;hsd_804D78E0=2;particle_peak_count=7;particle_float_bits=0x12345678;
    memset(particle_sort_frames,0,sizeof(particle_sort_frames));
    memset(particle_non_edge_heads,0,sizeof(particle_non_edge_heads));
    particle_display_frame=254;particle_non_edge_heads[15]=particles[1];
    particles[0]->kind=0;particles[0]->next=particles[1];particles[1]->kind=8;
    nativeParticleControl_BeginScene();
    NativeParticleControlSnapshot saved,bad,after,end,repeat;size_t bytes,ab,cb;
    CHECK(nativeParticleControl_Save(&saved,sizeof(saved),&bytes));
#ifdef NATIVE_SNAPSHOT_BENCHMARK
    benchmark_references(joint,particles[0],&saved);
#endif
    CHECK(nativeObjAlloc_Save(particle_payloads_saved,sizeof(particle_payloads_saved),&ab));
    CHECK(nativeClass_Save(stage_graph_saved,sizeof(stage_graph_saved),&cb));
    // The actual source sorter relinks particles and caches the edge split.
    // Replay it after restoring controls AND their owning allocation payloads.
    for(unsigned pass=0;pass<2;++pass) {
        CHECK(nativeObjAlloc_Load(particle_payloads_saved,ab));
        CHECK(nativeParticleControl_Load(&saved,sizeof(saved)));
        HSD_Particle *sorted,*non_edge;
        CHECK(particleSort(0,particle_display_frame,&sorted,&non_edge)==particles[1]);
        CHECK(sorted==particles[1] && non_edge==particles[0]);
        CHECK(particles[1]->next==particles[0] && particles[0]->next==NULL);
        CHECK(particle_non_edge_heads[0]==particles[0] && particle_sort_frames[0]==254);
        // Reusing this serial takes the source cache path without relinking.
        CHECK(particleSort(0,particle_display_frame,&sorted,&non_edge)==particles[1]);
        CHECK(non_edge==particles[0]);
        for(unsigned frame=0;frame<16;++frame) {
            particle_display_frame=particle_display_frame<255 ? particle_display_frame+1 : 1;
            (void)particleSort(0,particle_display_frame,&sorted,&non_edge);
            CHECK(hsd_8039D1EC()>=256);particle_float_bits+=frame;particle_peak_count+=frame;hsd_804D78D8+=frame;
            pending_generators=generators[frame%2];hsd_804D0908[0]=particles[frame%2];
        }
        CHECK(nativeParticleControl_Save(pass ? &repeat : &end,sizeof(end),&bytes));
    }
    CHECK(!memcmp(&end,&repeat,sizeof(end)));CHECK(nativeParticleControl_Load(&saved,sizeof(saved)));
#define REJECT(change) do {bad=saved;change;CHECK(!nativeParticleControl_Validate(&bad,sizeof(bad))); \
    CHECK(!nativeParticleControl_Load(&bad,sizeof(bad)));CHECK(nativeParticleControl_Save(&after,sizeof(after),&bytes)); \
    CHECK(!memcmp(&after,&saved,sizeof(saved)));++particle_control_refusals;}while(0)
    REJECT(++bad.epoch);REJECT(bad.state.id_cursor=1);REJECT(bad.state.commands[0]=(void*)1);
    REJECT(bad.state.callbacks=(void*)1);REJECT(bad.state.appsrt_config[0]=1);
    REJECT(bad.state.non_edge_heads[15]=(void*)1);REJECT(++bad.non_edge_heads[15].slot);
    REJECT(bad.non_edge_heads[15].allocator=&hsd_804D0F90.alloc_data);
    REJECT(bad.non_edge_heads[14]=bad.non_edge_heads[15]);
    REJECT(bad.state.particles[15]=(void*)1);REJECT(++bad.particles[15].slot);
    REJECT(bad.particles[15].allocator=&hsd_804D0F90.alloc_data);REJECT(bad.generators[1].bytes=1);
    REJECT(bad.deferred.allocator=&hsd_804D0F60.alloc_data);REJECT(++bad.camera.offset);
    REJECT(bad.camera_type=&test_class);REJECT(++bad.joints[7].offset);
#undef REJECT
    // Restoring controls does not duplicate allocation payloads.
    particles[1]->pos.x=777;CHECK(nativeParticleControl_Load(&saved,sizeof(saved)));CHECK(particles[1]->pos.x==777);
    CHECK(nativeObjAlloc_Load(particle_payloads_saved,ab));CHECK(particles[1]->pos.x==0);
    CHECK(nativeParticleControl_Load(&saved,sizeof(saved)));
    psCmdListArray[0]++;CHECK(!nativeParticleControl_Load(&saved,sizeof(saved)));psCmdListArray[0]--;
    HSD_CLASS_METHOD(joint)=&test_class;CHECK(!nativeParticleControl_Load(&saved,sizeof(saved)));
    CHECK(nativeClass_Load(stage_graph_saved,cb));CHECK(nativeParticleControl_Load(&saved,sizeof(saved)));
    CHECK(!nativeParticleControl_Save(particle_non_edge_heads,sizeof(saved),&bytes));
    CHECK(!nativeParticleControl_Save(particle_sort_frames,sizeof(saved),&bytes));
    CHECK(!nativeParticleControl_Save(&after,sizeof(after),(void*)&particle_display_frame));
    CHECK(!nativeParticleControl_Save(hsd_804D0908,sizeof(saved),&bytes));
    CHECK(!nativeParticleControl_Save(joint,sizeof(saved),&bytes));
    CHECK(!nativeParticleControl_Save(&after,sizeof(after),(void*)&particle_count));
    camera_idle=false;CHECK(!nativeParticleControl_Load(&saved,sizeof(saved)));camera_idle=true;
    nativeParticleControl_EndScene();CHECK(!nativeParticleControl_Load(&saved,sizeof(saved)));
    hsdFreeMemPiece(joint,sizeof(*joint));hsdFreeMemPiece(camera,sizeof(*camera));
    // Scene reset destroys the particle allocator, including the cached split
    // left by its last draw. A new scene must not retain those old pointers.
    particle_non_edge_heads[0]=particles[0];
    particle_non_edge_heads[16]=particles[1];
    memset(particle_sort_frames,particle_display_frame,sizeof(particle_sort_frames));
    hsd_80398A08(0);
    for(unsigned i=0;i<17;++i)CHECK(!particle_non_edge_heads[i]);
    for(unsigned i=0;i<16;++i)CHECK(particle_sort_frames[i]!=particle_display_frame);
    memset(hsd_804D0908,0,sizeof(hsd_804D0908));memset(particle_attachment_joints,0,sizeof(particle_attachment_joints));
    memset(particle_non_edge_heads,0,sizeof(particle_non_edge_heads));
    pending_generators=hsd_804D78FC=NULL;hsd_804D78F4=NULL;psCamera=NULL;
    camera_idle=false;particle_control_checks=checks-first;
}
