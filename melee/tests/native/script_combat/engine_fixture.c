#include "engine_fixture.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ft_0C8C.h>
#include <melee/ft/native_fighter_damage.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/inlines.h>
#include <melee/ef/eflib.h>
#include <melee/ef/types.h>
#include <melee/ef/native_model_effect.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/random.h>
#include <string.h>
#include <stdlib.h>

// Assertion is a hard failure; normal admitted bone writes never take it.
void OSReport(char* format,...){(void)format;}
void __assert(char* file,u32 line,char* expression)
{(void)file;(void)line;(void)expression;abort();}

u8 HSD_GObj_JObjKind=1;
u32 it_804D6D14;
// Reaction entry has a dedicated owned-actor/native-body cohort. This older
// status/effect fixture must not pretend to implement the native calculation.
void ftCo_8008DCE0(Fighter_GObj* object,int motion,float facing)
{(void)object;(void)motion;(void)facing;abort();}
void ftCo_8008E908(Fighter_GObj* object,float facing)
{(void)object;(void)facing;abort();}
void ftCommon_8007DB58(Fighter_GObj* object){(void)object;abort();}
void ftCo_8008F744(Fighter_GObj* object){(void)object;abort();}
void ftCo_Damage_Phys(Fighter_GObj* object){(void)object;abort();}
EF_ParamEntry efLib_ParamTable[8];
unsigned script_combat_free_count,script_combat_walk_count,script_combat_color;
int script_combat_hp,script_combat_heal_credit;
float script_combat_damage_credit,script_combat_damage_percent;
static HSD_GObj* retire_source;static unsigned retire_stage;static bool replacement;
void scriptCombatFixture_Reset(void)
{memset(efLib_ParamTable,0,sizeof(efLib_ParamTable));script_combat_free_count=script_combat_walk_count=script_combat_color=0;script_combat_hp=script_combat_heal_credit=0;script_combat_damage_credit=script_combat_damage_percent=0;retire_source=NULL;retire_stage=0;}
void scriptCombatFixture_Retire(HSD_GObj* source,unsigned stage,bool replace)
{retire_source=source;retire_stage=stage;replacement=replace;}
static void retire(unsigned stage)
{
 if(!retire_source||stage!=retire_stage)return;
 if(replacement){++((Fighter*)retire_source->user_data)->x8_spawnNum;return;}
 HSD_GObj** cursor=&HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
 while(*cursor&&*cursor!=retire_source)cursor=&(*cursor)->next;
 if(*cursor==retire_source)*cursor=retire_source->next;
 retire_source->user_data=(void*)1;
}
void Player_SetHPByIndex(s32 player,s32 secondary,s32 amount)
{(void)player;(void)secondary;script_combat_hp=amount;}
void pl_80040B8C(int player,int secondary,int amount)
{(void)player;(void)secondary;script_combat_heal_credit=amount;}
void pl_8003EC9C(int player,int secondary,float percent,float amount)
{(void)player;(void)secondary;script_combat_damage_credit=amount;script_combat_damage_percent=percent;}
bool ftCo_800C8C84(Fighter_GObj* object){(void)object;retire(100);return false;}
bool ftCo_800BFFD0(Fighter* fp,enum_t color,bool flag)
{(void)fp;(void)flag;script_combat_color=(unsigned)color;retire(101);return true;}
void HSD_GObjFree(HSD_GObj* object)
{
 for(unsigned i=0;i<32;++i){HSD_GObj** cursor=&HSD_GObjPLinkHead[i];while(*cursor&&*cursor!=object)cursor=&(*cursor)->next;if(*cursor==object)*cursor=object->next;}
 ++script_combat_free_count;retire(script_combat_free_count);
}
void nativeModelEffect_Cancel(EF_Effect* effect)
{effect->native_cancelled=true;HSD_GObjFree(effect->gobj);}
void hsd_8039D688(HSD_JObj* object,f32** unused,s32 value){(void)object;(void)unused;(void)value;}
void HSD_JObjWalkTree(HSD_JObj* object,HSD_JObjWalkTreeCallback callback,f32** argument)
{(void)object;(void)callback;(void)argument;++script_combat_walk_count;}
#define GET_EFFECT(gobj) ((EF_Effect*)(gobj)->user_data)
#include "script_combat_actual.h"
