#include "engine_fixture.h"
#include <melee/gr/stage.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/it/it_3F14.h>
#include <melee/lb/lbvector.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/fighter.h>
#include <placeholder.h>
#include <math.h>
#include <stdlib.h>

static ItemCommonData common;ItemCommonData* it_804D6D28=&common;
const Vec3 it_803B8570={0},it_803B857C={0};
int script_item_sound[16][4];unsigned script_item_sound_count;
void scriptItemFixture_Reset(void){common.x68_float=37.5f;common.x4C_float=0.375f;script_item_sound_count=0;}
static void sound(int operation,int id,int first,int second)
{if(script_item_sound_count==16)abort();int* row=script_item_sound[script_item_sound_count++];row[0]=operation;row[1]=id;row[2]=first;row[3]=second;}
int lbAudioAx_800237A8(enum_t id,int first,int second){sound(0,id,first,second);return 91;}
int lbAudioAx_800233EC(int id){return id+7;}
int lbAudioAx_800236B8(int handle){sound(1,handle,0,0);return 0;}
// The scene geometry search and moving-platform observation are controlled;
// native item integration, response, bounce and vector arithmetic remain real.
bool mpColl_800477E0(CollData* coll)
{coll->cur_pos.x+=0.125f;coll->cur_pos.y-=0.0625f;coll->floor.index=5;return (coll->env_flags&Collide_FloorMask)!=0;}
void it_80276278(Item_GObj* object){(void)object;}
static bool platform(CollData* coll,Vec3* result,unsigned flag)
{if(!(coll->env_flags&flag))return false;*result=(Vec3){0.125f,-0.25f,0};return true;}
bool mpCollGetSpeedLeftWall(CollData* c,Vec3* r){return platform(c,r,Collide_LeftWallMask);}
bool mpCollGetSpeedRightWall(CollData* c,Vec3* r){return platform(c,r,Collide_RightWallMask);}
bool mpCollGetSpeedCeiling(CollData* c,Vec3* r){return platform(c,r,Collide_CeilingMask);}
bool mpCollGetSpeedFloor(CollData* c,Vec3* r){return platform(c,r,Collide_FloorMask);}
#include "script_item_actual.h"

// This older primitive suite does not exercise source-owned contact responses.
// Their connected native/VM comparisons live in script_item_responses; reject
// any accidental use here instead of claiming a substitute kernel as evidence.
u8 HSD_GObj_JObjKind;
HSD_JObjInfo hsdJObj;
struct Fighter_804D6524_t* Fighter_804D6524;
bool hsdObjIsDescendantOf(HSD_Obj* object,HSD_ClassInfo* info)
{(void)object;(void)info;abort();}
void itColl_BounceOffVictim(Item_GObj* object){(void)object;abort();}
void it_80276FC4(Item_GObj* object,s32 mask){(void)object;(void)mask;abort();}
f32 ftCo_CalcYScaledKnockback(f32 damage,f32 scale,f32 factor)
{(void)damage;(void)scale;(void)factor;abort();}
void Item_8026B074(Item* item){(void)item;abort();}
StKind Stage_80225194(void){abort();}
