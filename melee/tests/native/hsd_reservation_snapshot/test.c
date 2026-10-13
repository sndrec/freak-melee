#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/memory.h>
#include <dolphin/os/OSHeapSnapshot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static unsigned checks;
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"%s:%u: %s\n",__FILE__,__LINE__,#c); exit(1); } } while (0)
static unsigned char heap[1024*1024] ATTRIBUTE_ALIGN(64);
static unsigned char backing[65536] ATTRIBUTE_ALIGN(64);
static unsigned char second_backing[4096] ATTRIBUTE_ALIGN(64);
static NativeHsdReservation owner, second;
static HSD_ObjAllocData satellite;

void __assert(char* file, u32 line, char* condition)
{ fprintf(stderr,"assert %s:%lu: %s\n",file,line,condition); exit(2); }
void HSD_Panic(char* file, u32 line, char* condition)
{ __assert(file,line,condition); }
void OSReport(char* format, ...)
{ va_list args; va_start(args,format); vfprintf(stderr,format,args); va_end(args); }

#include "snapshot_cases.h"
#include "objalloc_cases.h"
#include "class_cases.h"
#include "class_arena_cases.h"
#include "camera_cases.h"
#include "stage_cases.h"
#include "scene_history_cases.h"
#include "hazard_cases.h"
#include "bubble_cases.h"
#include "moving_cases.h"
#include "grid_cases.h"
#include "car_wind_cases.h"
#include "pura_cases.h"
#include "big_blue_cases.h"
#include "generator_cases.h"
#include "spawn_cases.h"
#include "rainbow_cases.h"
#include "fountain_cases.h"
#include "effect_control_cases.h"
#include "particle_control_cases.h"
#include "light_control_cases.h"
#include "versus_control_cases.h"
#include "material_texture_cases.h"
#include "objalloc_heap_cases.h"
#include "id_cases.h"
#include "os_heap_cases.h"

int main(int argc,char** argv)
{
    if(argc==2) {
        void* initial=OSInitAlloc(heap,heap+sizeof(heap),1);
        int target=OSCreateHeap(initial,heap+sizeof(heap));CHECK(target>=0);
        void* allocated=OSAllocFromHeap(target,32);CHECK(allocated);
        CHECK(OSLockHeapForHistory(target));
        if(!strcmp(argv[1],"frozen-allocation"))OSAllocFromHeap(target,32);
        else if(!strcmp(argv[1],"frozen-free"))OSFreeToHeap(target,allocated);
        else if(!strcmp(argv[1],"frozen-geometry"))OSDestroyHeap(target);
        return 0;
    }

    void* start = OSInitAlloc(heap,heap+sizeof(heap),1); CHECK(start);
    int handle = OSCreateHeap(start,heap+sizeof(heap)); CHECK(handle >= 0);
    HSD_SetHeap(handle); HSD_ObjAllocInit(&satellite,24,16);
    // Retain an ordinary free slot in the same allocator. Explicit reserved
    // restore must not touch its freelist or heap allocation accounting.
    void* ordinary = HSD_ObjAlloc(&satellite); CHECK(ordinary);
    HSD_ObjFree(&satellite,ordinary);
    test_snapshots();
    CHECK(HSD_ObjAlloc(&satellite) == ordinary);
    HSD_ObjFree(&satellite,ordinary);
    test_descriptors();
    test_class_storage();
    test_class_arena_history();
    test_heap_objects();
    test_id_roots();
    test_camera_state();
    test_stage_state();
    test_hazard_state();
    test_bubble_state();
    test_moving_state();
    test_grid_state();
    test_car_wind_state();
    test_pura_state();
    test_big_blue_state();
    test_rainbow_state();
    test_fountain_state();
    test_effect_controls();
    test_particle_controls();
    test_light_controls();
    test_versus_controls();
    test_material_textures();
    test_generator_state();
    test_spawn_state();
    test_scene_history_renewal();
    test_os_headers();
    printf("{\"checks\":%u,\"snapshot_checks\":%u,\"rejected_candidates\":%u,\"snapshot_bytes\":%zu,\"reserved_backing_bytes\":%zu,\"owners\":2,\"bins\":4,\"occupied_records\":4,\"descriptor_checks\":%u,\"descriptor_rejections\":%u,\"descriptor_bytes\":%zu,\"class_checks\":%u,\"class_rejections\":%u,\"class_bytes\":%zu,\"heap_object_checks\":%u,\"heap_object_rejections\":%u,\"heap_object_bytes\":%zu,\"os_checks\":%u,\"os_rejections\":%u,\"os_bytes\":%lu,\"id_checks\":%u,\"id_rejections\":%u,\"camera_checks\":%u,\"camera_rejections\":%u,\"camera_bytes\":%zu,\"stage_checks\":%u,\"stage_rejections\":%u,\"stage_bytes\":%zu,\"hazard_checks\":%u,\"hazard_rejections\":%u,\"hazard_bytes\":%zu,\"bubble_checks\":%u,\"bubble_rejections\":%u,\"moving_checks\":%u,\"moving_rejections\":%u,\"grid_checks\":%u,\"grid_rejections\":%u,\"grid_bytes\":%zu,\"car_wind_checks\":%u,\"car_wind_rejections\":%u,\"pura_checks\":%u,\"pura_rejections\":%u,\"big_blue_checks\":%u,\"big_blue_rejections\":%u,\"generator_checks\":%u,\"generator_rejections\":%u,\"generator_bytes\":%zu,\"spawn_checks\":%u,\"spawn_refusals\":%u,\"spawn_bytes\":%zu,\"rainbow_checks\":%u,\"rainbow_refusals\":%u,\"fountain_checks\":%u,\"fountain_refusals\":%u,\"effect_control_checks\":%u,\"effect_control_refusals\":%u,\"effect_control_bytes\":%zu,\"particle_control_checks\":%u,\"particle_control_refusals\":%u,\"particle_control_bytes\":%zu,\"appsrt_bytes\":%zu}\n",checks,snapshot_checks,snapshot_rejections,snapshot_packed_bytes,sizeof(backing)+sizeof(second_backing),descriptor_checks,descriptor_rejections,descriptor_bytes,class_checks,class_rejections,class_bytes,heap_object_checks,heap_object_rejections,heap_object_bytes,os_checks,os_rejections,(unsigned long)os_bytes,id_checks,id_rejections,camera_checks,camera_rejections,sizeof(NativeCameraSnapshot),stage_checks,stage_rejections,sizeof(NativeStageSnapshot),hazard_checks,hazard_rejections,sizeof(NativeStageHazardSnapshot),bubble_checks,bubble_rejections,moving_checks,moving_rejections,grid_checks,grid_rejections,grid_snapshot_bytes,car_wind_checks,car_wind_rejections,pura_checks,pura_rejections,big_blue_checks,big_blue_rejections,generator_checks,generator_rejections,sizeof(NativeStageGeneratorSnapshot),spawn_checks,spawn_refusals,spawn_bytes,rainbow_checks,rainbow_refusals,fountain_checks,fountain_refusals,effect_control_checks,effect_control_refusals,sizeof(NativeEffectControlSnapshot),particle_control_checks,particle_control_refusals,sizeof(NativeParticleControlSnapshot),sizeof(HSD_psAppSRT));
    return 0;
}
