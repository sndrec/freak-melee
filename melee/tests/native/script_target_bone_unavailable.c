#include <melee/ft/ftdata.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/types.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/lb/lbcollision.h>
#include <melee/it/it_26B1.h>
#include <melee/gm/gmvs.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/robj.h>
#include <stdlib.h>

// Ahead-search engine leaves are unavailable in cohorts selecting exports or
// scalar targets. The geometry cohort supplies the unchanged native bodies.
bool ftLib_80086FD4(HSD_GObj* owner,HSD_GObj* target){(void)owner;(void)target;abort();}
bool gm_8016B168(void){abort();}
HSD_GObj* it_8026C258(Vec3* position,float facing){(void)position;(void)facing;abort();}
Fighter* ftCo_800A4A40(Fighter* fighter){(void)fighter;abort();}
void it_8026BB88(HSD_GObj* object,Vec3* position){(void)object;(void)position;abort();}

// Older target/recipient cohorts exercise no bone sampling. Keep that engine
// dependency explicitly unavailable; the connected bone cohort uses real math.
struct UnkCostumeList CostumeListsForeachCharacter[Ft_Kind_RuntimeCapacity];
HSD_JObjInfo hsdJObj;
// Explicit unselected geometry/scalar closure for older target cohorts. The
// connected geometry cohort supplies actual native bodies and common data.
__declspec(selectany) ftCommonData* p_ftCommonData;
float ftLib_800869D4(HSD_GObj* object){(void)object;abort();}
bool ftLib_80086A18(HSD_GObj* object){(void)object;abort();}
void ftLib_800866DC(HSD_GObj* object,Vec3* out){(void)object;(void)out;abort();}
float ftCo_80094098(HSD_GObj* object,float* out){(void)object;(void)out;abort();}

bool nativeFighterTarget_BonePosition(uint32_t target, unsigned part, Vec3* out)
{ (void) target; (void) part; (void) out; abort(); }

void HSD_JObjMakeMatrix(HSD_JObj* joint)
{ (void) joint; abort(); }

bool hsdObjIsDescendantOf(HSD_Obj* object, HSD_ClassInfo* parent)
{ (void) object; (void) parent; abort(); }

bool HSD_RObjExpressionUsesNativeDummy(const HSD_RObj* constraint)
{ (void) constraint; abort(); }

// Capsule operations are unavailable in cohorts that do not select geometry.
MtxPtr ftCommon_8007F804(Fighter* fighter){(void)fighter;abort();}
bool lbColl_80008248(HitCapsule* hit,HurtCapsule* hurt,Mtx m,float a,float b,float c)
{(void)hit;(void)hurt;(void)m;(void)a;(void)b;(void)c;abort();}
