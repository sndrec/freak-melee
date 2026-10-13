#include "engine_fixture.h"
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdlib.h>

unsigned script_setup_count,script_setup_log[16][3];
uint32_t script_setup_callbacks[NativeScriptFighterCallback_Count];
static bool is_heavy;static int hold_kind;static unsigned retire_at;
void scriptSetupFixture_Reset(void){script_setup_count=retire_at=0;is_heavy=false;hold_kind=1;for(unsigned i=0;i<NativeScriptFighterCallback_Count;++i)script_setup_callbacks[i]=UINT32_MAX;}
void scriptSetupFixture_Held(bool heavy,int kind){is_heavy=heavy;hold_kind=kind;script_setup_count=0;}
void scriptSetupFixture_RetireAt(unsigned stage){retire_at=stage;script_setup_count=0;}
static void phase(HSD_GObj* object,unsigned id,unsigned selector,unsigned value)
{
    if(script_setup_count>=16)abort();script_setup_log[script_setup_count][0]=id;script_setup_log[script_setup_count][1]=selector;script_setup_log[script_setup_count][2]=value;++script_setup_count;
    if(script_setup_count==retire_at){HSD_GObj** cursor=&HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];while(*cursor&&*cursor!=object)cursor=&(*cursor)->next;if(*cursor==object)*cursor=object->next;object->user_data=(void*)1;}
}
bool itIsHeavy(HSD_GObj* object){(void)object;return is_heavy;}
int itGetHoldKind(HSD_GObj* object){(void)object;return hold_kind;}
void ftAnim_80070FB4(HSD_GObj* object,s32 selector,s32 value){phase(object,1,(unsigned)selector,(unsigned)value);}
void ftAnim_80070C48(HSD_GObj* object,s32 selector){phase(object,2,(unsigned)selector,0);}
void ftAnim_80070CC4(HSD_GObj* object,int selector){phase(object,3,(unsigned)selector,0);}
void ftAnim_800704F0(HSD_GObj* object,int selector,float value){phase(object,4,(unsigned)selector,(unsigned)value);}
bool nativeScriptFighter_BindCallback(Fighter_GObj* object,uint32_t slot,uint32_t function)
{(void)object;if(slot>=NativeScriptFighterCallback_Count||(function!=UINT32_MAX&&function>=1024))return false;script_setup_callbacks[slot]=function;return true;}
uint32_t nativeScriptFighter_CallbackFunction(Fighter_GObj* object,uint32_t slot)
{(void)object;return slot<NativeScriptFighterCallback_Count?slot==NativeScriptFighterCallback_Death3?66:64+slot:UINT32_MAX;}
// This setup-only fixture has no actual source actor/owned-helper query state.
bool nativeScriptFighter_CallbackMatches(Fighter_GObj* object,uint32_t slot,uint32_t function,bool* result)
{(void)object;(void)slot;(void)function;(void)result;return false;}
// This isolated setup fixture has no actual motion-phase actor scheduler.
bool nativeScriptFighter_BindMotionPhase(Fighter_GObj* object,uint32_t phase,uint32_t function,bool cue)
{(void)object;(void)phase;(void)function;(void)cue;return false;}
// The owned alignment callback has a connected actor/pool fixture of its own.
// This older setup-only cohort must never fabricate its lifetime or execution.
void ftCommon_8007F76C(Fighter_GObj* object)
{(void)object;abort();}
