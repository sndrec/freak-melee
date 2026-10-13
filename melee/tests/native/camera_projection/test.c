#include <melee/cm/camera.h>
#include <melee/cm/types.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/wobj.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Controlled camera accessors; projection and SDK math are actual source.
int HSD_CObjGetProjectionType(HSD_CObj* c) { return c->projection_type; }
void HSD_CObjGetEyePosition(HSD_CObj* c, Vec3* p) { *p=c->eyepos->pos; }
void HSD_CObjGetInterest(HSD_CObj* c, Vec3* p) { *p=c->interest->pos; }
int HSD_CObjGetUpVector(HSD_CObj* c, Vec3* p) { *p=c->u.up; return 0; }
MtxPtr HSD_CObjGetViewingMtxPtr(HSD_CObj* c) { return c->view_mtx; }
void __assert(char* file, u32 line, char* condition)
{
    fprintf(stderr,"%s:%lu: %s\n",file,line,condition); abort();
}

static unsigned checks;
static void close_to(float actual, float expected)
{
    assert(isfinite(actual) && fabsf(actual-expected)<0.002f); ++checks;
}

static void projection_cases(void)
{
    HSD_WObj eye={.pos={0,0,100}}, target={.pos={0,0,0}};
    HSD_CObj c={0};
    c.eyepos=&eye; c.interest=&target; c.u.up.y=1;
    c.viewport.xmin=20; c.viewport.xmax=660;
    c.viewport.ymin=10; c.viewport.ymax=490;
    c.near=1; c.far=1000;
    c.projection_type=PROJ_PERSPECTIVE;
    c.projection_param.perspective.fov=90;
    c.projection_param.perspective.aspect=4.0f/3.0f;
    C_MTXLookAt(c.view_mtx,&eye.pos,&c.u.up,&target.pos);
    const Vec3 points[]={{0,0,0},{100,0,0},{-100,0,0},{0,100,0},{0,-100,0}};
    const float expected[][2]={{340,250},{580,250},{100,250},{340,10},{340,490}};
    for (int view=0;view<2;++view) for (unsigned i=0;i<5;++i) {
        Vec3 result;
        assert(lbVector_WorldToScreen(&c,&points[i],&result,view)==&result);
        close_to(result.x,expected[i][0]); close_to(result.y,expected[i][1]);
        assert(isfinite(result.z)); ++checks;
    }
    c.projection_type=PROJ_ORTHO;
    c.projection_param.ortho.top=100; c.projection_param.ortho.bottom=-100;
    c.projection_param.ortho.left=-100; c.projection_param.ortho.right=100;
    const float ortho[][2]={{340,250},{660,250},{20,250},{340,10},{340,490}};
    for (int view=0;view<2;++view) for (unsigned i=0;i<5;++i) {
        Vec3 result;
        assert(lbVector_WorldToScreen(&c,&points[i],&result,view)==&result);
        close_to(result.x,ortho[i][0]); close_to(result.y,ortho[i][1]);
        assert(isfinite(result.z)); ++checks;
    }
    c.projection_type=0;
    Vec3 result={11,22,33};
    assert(!lbVector_WorldToScreen(&c,&points[0],&result,1));
    close_to(result.x,11); close_to(result.y,22); close_to(result.z,33);
}

#ifndef PROJECTION_NEGATIVE_CONTROL
// Match-bound stage constants; these leaves do not implement gameplay.
static int single_player;
int gm_IsCurrently1PMode_inline(void) { return single_player; }
float Stage_GetCamZoomRate(void) { return 100; }
float Stage_GetCamMaxDepth(void) { return 500; }
extern Camera game_camera;
extern CameraUnkGlobals cm_803BCCA0;
extern HSD_CameraDescPerspective cm_803BCB64;

static void quake_cases(void)
{
    CameraBounds bounds={.z_pos=300};
    CameraTransformState transform={.fov=30};
    cm_803BCB64.aspect=4.0f/3.0f;
    cm_803BCB64.viewport.xmin=0; cm_803BCB64.viewport.xmax=640;
    cm_803BCB64.viewport.ymin=0; cm_803BCB64.viewport.ymax=480;
    cm_803BCCA0.x54=1; cm_803BCCA0.x5C=1;
    cm_803BCCA0.x58=1; cm_803BCCA0.x60=1;
    cm_803BCCA0.xE8=0.5f;
    game_camera.quake_scale=1; game_camera.x2BC=1;
    for (single_player=0;single_player<2;++single_player) {
        game_camera.quake_offset.x=0; game_camera.quake_offset.y=0;
        Camera_ApplyQuake(&bounds,&transform);
        close_to(game_camera.translation.x,0); close_to(game_camera.translation.y,0);
        game_camera.quake_offset.x=2; game_camera.quake_offset.y=-3;
        const float half_height=300*tanf(0.5f*(0.017453292f*30));
        const float factor=single_player?0.5f:1;
        Camera_ApplyQuake(&bounds,&transform);
        close_to(game_camera.translation.x,factor*20*(4.0f/3.0f)*(half_height/320));
        close_to(game_camera.translation.y,factor*-30*(half_height/240));
        close_to(game_camera.quake_offset.x,0); close_to(game_camera.quake_offset.y,0);
    }
}
#endif

int main(void)
{
    projection_cases();
#ifndef PROJECTION_NEGATIVE_CONTROL
    quake_cases();
#endif
    printf("camera projection: %u checks passed\n",checks); return 0;
}
