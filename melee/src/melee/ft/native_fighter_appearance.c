#include "native_fighter_appearance.h"
#include "fighter.h"
#include "native_fighter_runtime.h"
#include "native_costume_runtime.h"
#include <melee/gm/native_presentation.h>
#include <melee/gm/native_home.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/state.h>
#include <dolphin/gx.h>
#include <math.h>

bool nativeFighterAppearance_IsFiltered(const Fighter* fighter)
{
    if (nativePresentation_CustomModels() || nativeHome_WorkshopCameraActive()) return false;
    return nativeFighterRuntime_SourcePackage(fighter->kind)!=NULL ||
        nativeCostumeRuntime_ForFighter(fighter)!=NULL;
}

static Vec3 add(Vec3 a,Vec3 b) { return (Vec3){a.x+b.x,a.y+b.y,a.z+b.z}; }
static Vec3 scale(Vec3 a,float s) { return (Vec3){a.x*s,a.y*s,a.z*s}; }
static Vec3 cross(Vec3 a,Vec3 b) { return (Vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
static void vertex(Vec3 p)
{
    GXPosition3f32(p.x,p.y,p.z); GXColor4u8(192,224,244,255);
}

static Vec3 surface(Vec3 center,Vec3 direction,Vec3 radial,float radius,float sine,float cosine)
{
    return add(center,add(scale(direction,radius*sine),scale(radial,radius*cosine)));
}

static void capsule(const HurtCapsule* hurt,Mtx view)
{
    if (!hurt->bone || hurt->scale<=0) return;
    Vec3 a=hurt->a_offset,b=hurt->b_offset;
    Vec3 d=add(b,scale(a,-1));
    float length=sqrtf(d.x*d.x+d.y*d.y+d.z*d.z);
    d=length>.0001f?scale(d,1/length):(Vec3){0,1,0};
    Vec3 u=cross(d,fabsf(d.y)<.9f?(Vec3){0,1,0}:(Vec3){1,0,0});
    u=scale(u,1/sqrtf(u.x*u.x+u.y*u.y+u.z*u.z));
    Vec3 v=cross(d,u);
    Mtx transform; C_MTXConcat(view,HSD_JObjGetMtxPtr(hurt->bone),transform);
    GXLoadPosMtxImm(transform,GX_PNMTX0);
    static const float circle[12][2]={
        {1,0},{.866025404f,.5f},{.5f,.866025404f},{0,1},
        {-.5f,.866025404f},{-.866025404f,.5f},{-1,0},{-.866025404f,-.5f},
        {-.5f,-.866025404f},{0,-1},{.5f,-.866025404f},{.866025404f,-.5f}};
    static const float sine[5]={0,.382683432f,.707106781f,.923879533f,1};
    static const float cosine[5]={1,.923879533f,.707106781f,.382683432f,0};
    Vec3 radial[12];
    for (unsigned i=0;i<12;++i) radial[i]=add(scale(u,circle[i][0]),scale(v,circle[i][1]));
    // Rings and longitudinal arcs form the current hurtbox's capsule. Drawing
    // reads the posed bone; it leaves collision caches and hurt states intact.
    GXBegin(GX_LINES,GX_VTXFMT0,4*12*2+6*2+6*2*4*2);
    for (unsigned end=0;end<2;++end) for (unsigned ring=0;ring<2;++ring) {
        float sign=end?1:-1;
        Vec3 center=end?b:a;
        for (unsigned segment=0;segment<12;++segment) {
            vertex(surface(center,d,radial[segment],hurt->scale,sign*sine[ring*2],cosine[ring*2]));
            vertex(surface(center,d,radial[(segment+1)%12],hurt->scale,sign*sine[ring*2],cosine[ring*2]));
        }
    }
    for (unsigned meridian=0;meridian<6;++meridian) {
        Vec3 normal=radial[meridian*2];
        vertex(surface(a,d,normal,hurt->scale,0,1)); vertex(surface(b,d,normal,hurt->scale,0,1));
        for (unsigned end=0;end<2;++end) for (unsigned segment=0;segment<4;++segment) {
            float sign=end?1:-1; Vec3 center=end?b:a;
            vertex(surface(center,d,normal,hurt->scale,sign*sine[segment],cosine[segment]));
            vertex(surface(center,d,normal,hurt->scale,sign*sine[segment+1],cosine[segment+1]));
        }
    }
    GXEnd();
}

void nativeFighterAppearance_Draw(Fighter_GObj* actor,int pass)
{
    Fighter* fighter=GET_FIGHTER(actor);
    if (pass!=0 || !nativeFighterAppearance_IsFiltered(fighter) || fighter->x221F_b3 ||
        !fighter->x21FC_flag.b7 || fighter->invisible || fighter->x221E_b5) return;
    HSD_CObj* camera=HSD_CObjGetCurrent();
    if (!camera) return;
    HSD_StateInvalidate(HSD_STATE_ALL); HSD_ClearVtxDesc(); GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS,GX_DIRECT); GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
    GXSetCurrentMtx(GX_PNMTX0); GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0,GX_FALSE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
    GXSetNumTexGens(0); GXSetNumTevStages(1); GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0); GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR); GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_TRUE,GX_LEQUAL,GX_TRUE); GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_COPY);
    GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0); GXSetColorUpdate(GX_TRUE);
    GXSetLineWidth(6,GX_TO_ZERO);
    for (unsigned i=0;i<(unsigned)fighter->hurt_capsules_len;++i)
        capsule(&fighter->hurt_capsules[i].capsule,HSD_CObjGetViewingMtxPtrDirect(camera));
    HSD_StateInvalidate(HSD_STATE_ALL);
}
