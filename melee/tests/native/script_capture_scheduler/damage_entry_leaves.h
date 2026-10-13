// Native reaction arithmetic is extracted unchanged. These leaves control
// player credit, presentation, camera, rig effects and common initialization.
#include <Runtime/platform.h>
#include <dolphin/mtx.h>
#include <placeholder.h>
#include <math.h>
#include <melee/cm/camera.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageIce.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageSong.h>
#include <melee/sfx/crowdsfx.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/mtx.h>
HSD_GObjEvent ftData_OnKnockbackEnter[Ft_Kind_RuntimeCapacity];
void Fighter_8006CDA4(Fighter* fp,s32 damage)
{(void)damage;step(16,fp->gobj);}
void pl_80040270(int player,int secondary,float knockback)
{(void)player;(void)secondary;(void)knockback;}
bool ftColl_8007AC68(u32 angle){(void)angle;return false;}
static bool ftCo_8008DA4C(Fighter_GObj* actor,HitElement element,enum_t level)
{(void)element;(void)level;step(17,actor);return false;}
void ftCo_8008DB10(Fighter_GObj* actor,HitElement element,float knockback)
{(void)element;(void)knockback;step(18,actor);}
float un_803222EC(float magnitude,float angle){(void)angle;return magnitude;}
void un_8032233C(u32 source,u32 generation){(void)source;(void)generation;}
bool gm_8016B014(void){return false;}
void ftColl_8007B6A0(Fighter_GObj* actor){(void)actor;abort();}
float lbVector_Angle(Vec3* normal,Vec3* vector)
{CHECK(normal->x==0&&normal->y==1&&normal->z==0&&vector->z==0);return atan2f(fabsf(vector->x),vector->y);}
static void inlineA0(Fighter_GObj* actor,Fighter* fp,float* angle)
{CHECK(fp==actor->user_data&&angle);step(19,actor);}
static void inlineA1(Fighter_GObj* actor){step(20,actor);}
void Camera_RequestQuake(CmQuakeKind kind,Vec3* position)
{CHECK(kind>=QuakeKind_Small&&kind<=QuakeKind_Large&&position);}
void ftCo_DamageIce_Init(Fighter_GObj* actor){step(21,actor);}
void ftCo_800C318C(Fighter_GObj* actor,bool flower)
{step(22,actor);((Fighter*)actor->user_data)->motion_id=flower?301:300;}
void ftCo_Damage_OnEveryHitlag(Fighter_GObj* actor){(void)actor;abort();}
void ftCo_Damage_OnExitHitlag(Fighter_GObj* actor){(void)actor;abort();}
