#include "native_snapshot_class_graph.h"
#include "native_versus_control_snapshot.h"
#include <melee/cm/native_camera_snapshot.h>
#include <melee/gr/native_stage_snapshot.h>
#include <melee/gr/native_stage_hazard_snapshot.h>
#include <melee/gr/native_green_grid.h>
#include <melee/ef/native_effect_control_snapshot.h>
#include <sysdolphin/baselib/native_particle_control_snapshot.h>
#include <sysdolphin/baselib/native_light_control_snapshot.h>
#include <sysdolphin/baselib/native_material_texture_snapshot.h>
#include <sysdolphin/baselib/cobj.h>
#include <string.h>

// MELEE_ROLLBACK_HOST_STATE: one fixed index for this preflight invocation.
// The prepared view never escapes; no hidden allocator-owner cache is used.
static const uint8_t* payload_index[NATIVE_CLASS_SNAPSHOT_BLOCKS];
static bool typed(NativeClassSnapshotView view,NativeClassReference reference,
                  const HSD_ClassInfo* type,size_t bytes)
{
    if(!reference.bytes)return type==NULL;
    return reference.bytes==bytes && nativeClass_SnapshotObjectType(view,reference,type);
}
bool nativeSnapshotClassGraph_Validate(const NativeMatchSnapshotPart* parts,NativeSnapshotClassGraphReport* report)
{
    if(report)*report=(NativeSnapshotClassGraphReport){0};
    NativeClassSnapshotView view=nativeClass_ViewSnapshot(parts[7].data,parts[7].bytes,
                                                         payload_index,NATIVE_CLASS_SNAPSHOT_BLOCKS);
    if(!view.types || !nativeCamera_ValidateClassSnapshot(parts[25].data,parts[25].bytes,view))return false;
    if(!nativeVersusControl_ValidateClass(parts[34].data,parts[34].bytes,view))return false;
    if(!nativeMaterialTexture_ValidateClass(parts[38].data,parts[38].bytes,view))return false;
    NativeStageSnapshot stage;memcpy(&stage,parts[26].data,sizeof(stage));
    for(unsigned i=0;i<GROUND_JOINT_POINT_COUNT;++i)
        if(!typed(view,stage.joints[i].backing,stage.joints[i].type,sizeof(HSD_JObj)))return false;
    for(unsigned i=0;i<STAGE_CAMERA_COUNT;++i)
        if(!typed(view,stage.cameras[i].backing,stage.cameras[i].type,sizeof(HSD_CObj)))return false;
    NativeStageHazardSnapshot hazard;memcpy(&hazard,parts[27].data,sizeof(hazard));
    for(unsigned i=0;i<NATIVE_STAGE_HAZARD_ROOTS;++i)
        if(!typed(view,hazard.joints[i].backing,hazard.joints[i].type,sizeof(HSD_JObj)))return false;
    if(!typed(view,hazard.reflection_camera.backing,hazard.reflection_camera.type,sizeof(HSD_CObj)))return false;
    NativeGreenGridSnapshotHeader grid;memcpy(&grid,parts[28].data,sizeof(grid));
    const uint8_t* cursor=(const uint8_t*)parts[28].data+sizeof(grid);
    for(unsigned i=0;i<grid.count;++i) {
        NativeGreenGridSnapshotEntry entry;memcpy(&entry,cursor,sizeof(entry));cursor+=sizeof(entry);
        for(unsigned row=0;row<GREEN_GRID_ROWS;++row)for(unsigned column=0;column<GREEN_GRID_COLUMNS;++column) {
            NativeGreenGridCellReferences* cell=&entry.cells[row][column];
            if(!typed(view,cell->joint,cell->joint_type,sizeof(HSD_JObj)))return false;
        }
    }
    NativeEffectControlSnapshot effect;memcpy(&effect,parts[32].data,sizeof(effect));
    for(unsigned i=0;i<(unsigned)effect.animation_count;++i)
        if(nativeClass_ResolveSnapshot(view,effect.joints[i])!=effect.animations[i] ||
           !typed(view,effect.joints[i],effect.joint_types[i],sizeof(HSD_JObj)))return false;
    NativeParticleControlSnapshot particles;memcpy(&particles,parts[33].data,sizeof(particles));
    if(!typed(view,particles.camera,particles.camera_type,sizeof(HSD_CObj)))return false;
    if(particles.state.camera && nativeClass_ResolveSnapshot(view,particles.camera)!=particles.state.camera)return false;
    for(unsigned i=0;i<8;++i)
        if(!typed(view,particles.joints[i],particles.joint_types[i],sizeof(HSD_JObj)) ||
           (particles.state.joints[i] && nativeClass_ResolveSnapshot(view,particles.joints[i])!=particles.state.joints[i]))return false;
    NativeLightControlSnapshot lights;memcpy(&lights,parts[36].data,sizeof(lights));
    for(unsigned i=0;i<MAX_GXLIGHT;++i)
        if(!typed(view,lights.lights[i],lights.types[i],sizeof(HSD_LObj)) ||
           (lights.state.active[i] && nativeClass_ResolveSnapshot(view,lights.lights[i])!=lights.state.active[i]))return false;
    if(report)*report=(NativeSnapshotClassGraphReport){view.header.blocks,view.header.types};
    return true;
}
