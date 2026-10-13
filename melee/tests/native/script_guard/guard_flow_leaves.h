// Controlled motion/contact boundaries around the actual shared input kernels.
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/kinds/ftCommon/ftCo_Escape.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftCo_Pass.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_Jump.h>
#include <melee/mp/mpcoll.h>
static bool flow_testing,flow_platform,flow_choices[3];
static float flow_friction=1;
static char flow_trace[64];static unsigned flow_count,flow_mutation_step;
static void flow_event(char event,Fighter_GObj* object)
{
 CHECK(flow_testing&&flow_count<sizeof(flow_trace));flow_trace[flow_count++]=event;
 if(flow_mutation_step&&flow_count==flow_mutation_step){
  flow_mutation_step=0;NativeScriptRuntimeContext c={.source=object};guard_write_probe(&c);
 }
}
void ftCo_80099390(Fighter_GObj* o,FtMotionId m,bool b){(void)o;(void)m;(void)b;abort();}
void ftCo_80099438(Fighter_GObj* o,FtMotionId m,bool b){(void)o;(void)m;(void)b;abort();}
void ftCo_80099954(Fighter_GObj* o){(void)o;abort();}
void ftAnim_8006EBA4(Fighter_GObj* o){flow_event('a',o);}
void ftCommon_8007EBAC(Fighter* f,u32 kind,u32 flags)
{CHECK(kind==23&&flags==0);flow_event('b',f->gobj);}
void ftCo_800957F4(Fighter_GObj* o,FtMotionId m)
{CHECK(m==ftCo_MS_LightThrowDash);flow_event('d',o);}
void ftCo_80095A30(Fighter_GObj* o){flow_event('i',o);}
void ftCo_8009A228(Fighter_GObj* o){flow_event('p',o);}
bool mpColl_IsOnPlatform(CollData* c){(void)c;CHECK(flow_testing);return flow_platform;}
bool ftCo_800D8B9C(Fighter_GObj* o){flow_event('s',o);return flow_choices[0];}
bool ftCo_Catch_CheckInput(Fighter_GObj* o){flow_event('g',o);return flow_choices[1];}
bool ftCo_800CB024(Fighter_GObj* o){flow_event('j',o);return flow_choices[2];}
void ft_800845B4(Fighter_GObj* o){flow_event('c',o);}
void ft_80084104(Fighter_GObj* o){flow_event('h',o);}
float ft_GetGroundFrictionMultiplier(Fighter* f){(void)f;CHECK(flow_testing);return flow_friction;}
static void flow_reference_ftCo_80093850(Fighter_GObj* o)
{((Fighter*)o->user_data)->cur_pos.x=NativeFighterShield_ReflectContinue;}
static void flow_reference_ftCo_80092BE8(Fighter_GObj* o)
{((Fighter*)o->user_data)->cur_pos.x=NativeFighterShield_Release;}
