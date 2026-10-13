// This cohort selects only source Item state transitions. Other spawn families
// reject; native operations outside the exercised kernel abort when reached.
#include <melee/it/native_script_item_spawn.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftdat.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/lb/lb_00B0.h>
#include <stdlib.h>
FighterPartsTable** ftPartsTable;
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind k){(void)k;return NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object){(void)object;return NativeScriptVM_Error;}
Fighter_Part ftParts_GetBoneIndex(Fighter* f,Fighter_Part p){(void)f;(void)p;abort();}
void lb_8000B1CC(HSD_JObj* o,Vec3* offset,Vec3* out){(void)o;(void)offset;(void)out;abort();}
void it_8026B724(Item_GObj* o){(void)o;abort();}
void it_8026B73C(Item_GObj* o){(void)o;abort();}
void it_80274574(Item_GObj* o){(void)o;abort();}
bool it_8026B894(Item_GObj* o,HSD_GObj* other){(void)o;(void)other;abort();}
#define TRANSITION_UNUSED(x) (void)(x)
NativeScriptVMStatus nativeScriptItemSpawn_HeldAtCenter(NativeScriptRuntimeContext* c,uint32_t n,const Vec3* p,uint32_t bone,float f,Item_GObj** o,NativeScriptDiagnostic* d)
{TRANSITION_UNUSED(c);TRANSITION_UNUSED(n);TRANSITION_UNUSED(p);TRANSITION_UNUSED(bone);TRANSITION_UNUSED(f);TRANSITION_UNUSED(o);TRANSITION_UNUSED(d);return NativeScriptVM_Error;}
NativeScriptVMStatus nativeScriptItemSpawn_At(NativeScriptRuntimeContext* c,uint32_t n,const Vec3* p,const Vec3* v,float f,bool owner,uint32_t g,float value,bool collision,bool gameplay_owned,Item_GObj** o,NativeScriptDiagnostic* d)
{TRANSITION_UNUSED(c);TRANSITION_UNUSED(n);TRANSITION_UNUSED(p);TRANSITION_UNUSED(v);TRANSITION_UNUSED(f);TRANSITION_UNUSED(owner);TRANSITION_UNUSED(g);TRANSITION_UNUSED(value);TRANSITION_UNUSED(collision);TRANSITION_UNUSED(gameplay_owned);TRANSITION_UNUSED(o);TRANSITION_UNUSED(d);return NativeScriptVM_Error;}
NativeScriptVMStatus nativeScriptItemSpawn_Child(NativeScriptRuntimeContext* c,uint32_t n,const Vec3* p,float f,uint32_t damage,uint32_t g,float value,uint32_t back,uint32_t parent_slot,Item_GObj** o,NativeScriptDiagnostic* d)
{TRANSITION_UNUSED(c);TRANSITION_UNUSED(n);TRANSITION_UNUSED(p);TRANSITION_UNUSED(f);TRANSITION_UNUSED(damage);TRANSITION_UNUSED(g);TRANSITION_UNUSED(value);TRANSITION_UNUSED(back);TRANSITION_UNUSED(parent_slot);TRANSITION_UNUSED(o);TRANSITION_UNUSED(d);return NativeScriptVM_Error;}
NativeScriptVMStatus nativeScriptItemSpawn_Projectile(NativeScriptRuntimeContext* c,uint32_t n,const Vec3* p,float f,int32_t v,const NativeScriptItemLaunchData* launch,Item_GObj** o,NativeScriptDiagnostic* d)
{TRANSITION_UNUSED(c);TRANSITION_UNUSED(n);TRANSITION_UNUSED(p);TRANSITION_UNUSED(f);TRANSITION_UNUSED(v);TRANSITION_UNUSED(launch);TRANSITION_UNUSED(o);TRANSITION_UNUSED(d);return NativeScriptVM_Error;}
NativeScriptVMStatus nativeScriptItemSpawn_Attached(NativeScriptRuntimeContext* c,uint32_t n,uint32_t part,Item_GObj** o,NativeScriptDiagnostic* d)
{TRANSITION_UNUSED(c);TRANSITION_UNUSED(n);TRANSITION_UNUSED(part);TRANSITION_UNUSED(o);TRANSITION_UNUSED(d);return NativeScriptVM_Error;}
NativeScriptVMStatus nativeScriptItemSpawn_AttachedPresentation(NativeScriptRuntimeContext* c,uint32_t n,uint32_t part,int32_t v,int32_t appearance,uint32_t initial,uint32_t attached,float lifetime,Item_GObj** o,NativeScriptDiagnostic* d)
{TRANSITION_UNUSED(c);TRANSITION_UNUSED(n);TRANSITION_UNUSED(part);TRANSITION_UNUSED(v);TRANSITION_UNUSED(appearance);TRANSITION_UNUSED(initial);TRANSITION_UNUSED(attached);TRANSITION_UNUSED(lifetime);TRANSITION_UNUSED(o);TRANSITION_UNUSED(d);return NativeScriptVM_Error;}
NativeScriptVMStatus nativeScriptItemSpawn_PullHoldable(NativeScriptRuntimeContext* c,uint32_t n,Item_GObj** o,NativeScriptDiagnostic* d)
{TRANSITION_UNUSED(c);TRANSITION_UNUSED(n);TRANSITION_UNUSED(o);TRANSITION_UNUSED(d);return NativeScriptVM_Error;}
NativeScriptVMStatus nativeScriptItemSpawn_Food(NativeScriptRuntimeContext* c,Item_GObj** o,NativeScriptDiagnostic* d)
{TRANSITION_UNUSED(c);TRANSITION_UNUSED(o);TRANSITION_UNUSED(d);return NativeScriptVM_Error;}
