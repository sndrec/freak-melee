#include <melee/ft/ftaccessory_assets.h>
#include <melee/lb/lb_00B0.h>
#include <melee/gm/native_match_arena.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <stdlib.h>

// This fixture exercises sound/rotation/matrix leaves, not accessory asset
// construction or match-arena admission. An accidental call must fail loudly.
u8 HSD_GObj_JObjKind;
bool ftAccessoryAssets_Validate(const HSD_DatImport* import,const FtAccessoryAsset* asset)
{(void)import;(void)asset;abort();}
void lb_8000C2F8(HSD_JObj* object,HSD_JObj* constraint)
{(void)object;(void)constraint;abort();}
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* joint){(void)joint;abort();}
void HSD_JObjRemoveAll(HSD_JObj* object){(void)object;abort();}
void HSD_JObjAddAnimAll(HSD_JObj* object,HSD_AnimJoint* anim,HSD_MatAnimJoint* material,HSD_ShapeAnimJoint* shape)
{(void)object;(void)anim;(void)material;(void)shape;abort();}
void HSD_AObjSetRate(HSD_AObj* object,f32 rate){(void)object;(void)rate;abort();}
void HSD_ForeachAnim(void* object,HSD_Type type,HSD_TypeMask mask,void* func,AObj_Arg_Type argument,...)
{(void)object;(void)type;(void)mask;(void)func;(void)argument;abort();}
void HSD_JObjReqAnimAll(HSD_JObj* object,f32 frame){(void)object;(void)frame;abort();}
void HSD_JObjAnimAll(HSD_JObj* object){(void)object;abort();}
bool nativeMatchArena_AddRoot(NativeMatchArena* arena,void* base,size_t size)
{(void)arena;(void)base;(void)size;abort();}
