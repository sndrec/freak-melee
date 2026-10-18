#include <melee/gm/native_home.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/quatlib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static void check(bool ok) { ++checks; if (!ok) { fprintf(stderr,"Bone pose check %u failed\n",checks); exit(1); } }
static struct { bool preview_live_test; } home;
static HSD_GObj actor;
static Fighter fighter;
static FighterBone part;
static HSD_JObj bone;
static bool present = true;
unsigned nativeHome_WorkshopBoneCount(void) { return present ? 1 : 0; }
static Fighter_GObj* workshop_preview_actor(bool dormant) { (void)dormant; return &actor; }
void HSD_JObjClearFlags(HSD_JObj* joint, u32 flags) { joint->flags &= ~flags; }
void(HSD_JObjSetMtxDirty)(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }

#include "bodies.inc"

static void euler_matrix(const float v[3], double m[3][3])
{
    const double sx=sin(v[0]),cx=cos(v[0]),sy=sin(v[1]),cy=cos(v[1]),sz=sin(v[2]),cz=cos(v[2]);
    const double result[3][3]={{cy*cz,sx*sy*cz-cx*sz,cx*sy*cz+sx*sz},
        {cy*sz,sx*sy*sz+cx*cz,cx*sy*sz-sx*cz},{-sy,sx*cy,cx*cy}};
    memcpy(m,result,sizeof(result));
}

int main(void)
{
    actor.user_data=&fighter; fighter.parts=&part; part.joint=&bone;
    for(unsigned axis=0;axis<3;++axis) for(int step=-6;step<=6;++step) {
        memset(&bone,0,sizeof(bone));
        const float angle=step*.5f, s=sinf(angle*.5f);
        bone.rotate.w=cosf(angle*.5f); ((float*)&bone.rotate)[axis]=s;
        bone.flags=JOBJ_USE_QUATERNION|JOBJ_HIDDEN;
        bone.scale=(Vec3){1,2,3}; bone.translate=(Vec3){4,5,6};
        Mtx original; C_MTXQuat(original,&bone.rotate);
        HSD_JObj before=bone;
        NativeHomeWorkshopBoneLocal local;
        check(nativeHome_WorkshopBoneLocal(0,&local));
        check(!memcmp(&before,&bone,sizeof(bone))); // Reading preserves animation state.
        double converted[3][3]; euler_matrix(local.rotation,converted);
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)check(fabs(converted[r][c]-original[r][c])<.00001);
        check(local.scale[1]==2&&local.position[2]==6);
        // Identical Euler edits must produce the same rotation regardless of
        // the quaternion W retained by the previous animation/scene.
        local.rotation[0]=.31f;local.rotation[1]=-.84f;local.rotation[2]=1.2f;
        for(unsigned w=0;w<3;++w) {
            bone.flags|=JOBJ_USE_QUATERNION; bone.rotate.w=.13f+w*.31f;
            check(nativeHome_WorkshopSetBoneLocal(0,&local));
            check(!(bone.flags&JOBJ_USE_QUATERNION));
            check(bone.flags&JOBJ_HIDDEN);check(bone.flags&JOBJ_MTX_DIRTY);
            check(!memcmp(&bone.rotate,local.rotation,3*sizeof(float)));
            check(bone.rotate.w==0);
        }
        NativeHomeWorkshopBoneLocal read;
        check(nativeHome_WorkshopBoneLocal(0,&read));check(!memcmp(&read,&local,sizeof(local)));
    }
    NativeHomeWorkshopBoneLocal value={0};
    check(!nativeHome_WorkshopBoneLocal(1,&value));check(!nativeHome_WorkshopBoneLocal(0,NULL));
    check(!nativeHome_WorkshopSetBoneLocal(1,&value));
    home.preview_live_test=true;check(!nativeHome_WorkshopSetBoneLocal(0,&value));home.preview_live_test=false;
    HSD_JObj before=bone;value.rotation[0]=NAN;
    check(!nativeHome_WorkshopSetBoneLocal(0,&value));check(!memcmp(&before,&bone,sizeof(bone)));
    bone.flags|=JOBJ_USE_QUATERNION;bone.rotate=(Quaternion){0,0,0,0};
    check(!nativeHome_WorkshopBoneLocal(0,&value));bone.rotate.w=INFINITY;
    check(!nativeHome_WorkshopBoneLocal(0,&value));present=false;
    check(!nativeHome_WorkshopBoneLocal(0,&value));
    printf("PASS: %u actual Workshop bone pose representation checks\n",checks);
    return 0;
}
