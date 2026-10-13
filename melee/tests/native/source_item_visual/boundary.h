#ifndef SOURCE_ITEM_VISUAL_BOUNDARY_H
#define SOURCE_ITEM_VISUAL_BOUNDARY_H
#include <melee/it/native_script_item_actor.h>
#include <melee/ft/native_script_fighter.h>
#include <dolphin/mtx.h>
extern bool fixture_idle,fixture_camera;
extern uint32_t fixture_references;
extern const NativeScriptCompileBank* fixture_selected,*fixture_retained;
extern const NativeScriptItemProgram* fixture_spec;
typedef struct VisualTrace {
    unsigned quads,positions,uvs,models,pass_tail,invalidations;
    const void* pixels;
    uint16_t width,height;
    Vec3 points[4];float uv[4][2];
} VisualTrace;
extern VisualTrace visual_trace;
void visualFixture_Reset(void);
#endif
