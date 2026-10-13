// Unexercised graph/render endpoints terminate rather than silently faking
// camera gameplay. Actual state methods and serializers run in camera_cases.h.
#include <melee/cm/camera.h>
#include <melee/gr/grlib.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdlib.h>
f32 HSD_CObjGetFov(HSD_CObj* x) { (void)x;abort(); }
f32 HSD_CObjGetAspect(HSD_CObj* x) { (void)x;abort(); }
void HSD_CObjGetEyePosition(HSD_CObj* x,Vec3* y) { (void)x;(void)y;abort(); }
int HSD_CObjGetEyeVector(HSD_CObj* x,Vec3* y) { (void)x;(void)y;abort(); }
void HSD_CObjGetInterest(HSD_CObj* x,Vec3* y) { (void)x;(void)y;abort(); }
void HSD_CObjGetScissor(HSD_CObj* x,Scissor* y) { (void)x;(void)y;abort(); }
void HSD_CObjSetMtxDirty(HSD_CObj* x) { (void)x;abort(); }
void HSD_CObjSetupViewingMtx(HSD_CObj* x) { (void)x;abort(); }
Vec3* lbVector_WorldToScreen(HSD_CObj* x,const Vec3* y,Vec3* z,int n)
{ (void)x;(void)y;(void)z;(void)n;abort(); }
float lbVector_8000E838(Vec3* a,Vec3* b,Vec3* c,Vec3* d)
{ (void)a;(void)b;(void)c;(void)d;abort(); }
Vec3* lbVector_Diff(Vec3* a,Vec3* b,Vec3* c) { (void)a;(void)b;(void)c;abort(); }
float lbVector_Normalize(Vec3* a) { (void)a;abort(); }
Vec3* lbVector_Add(Vec3* a,Vec3* b) { (void)a;(void)b;abort(); }
HSD_GObj* grLib_801C9CEC(CmQuakeKind kind) { (void)kind;abort(); }
u32 Camera_80029124(Vec3* x,s32 n) { (void)x;(void)n;abort(); }
void HSD_GObj_80390ED0(HSD_GObj* x,u32 n) { (void)x;(void)n;abort(); }
