// Only finish / release paths are admitted in this cohort. Other state-entry
// leaves abort so unrelated behavior cannot silently satisfy a comparison.
#include <melee/ft/ft_081B.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_LandingAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/ft/kinds/ftCommon/ftCo_CliffJump.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageFall.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>

void ftCo_Fall_Enter(Fighter_GObj* g)
{step(24,g);((Fighter*)g->user_data)->motion_id=ftCo_MS_Fall;}
void ft_8008A2BC(Fighter_GObj* g)
{step(25,g);((Fighter*)g->user_data)->motion_id=ftCo_MS_Wait;}
s32 ftGetParasolStatus(Fighter_GObj* g){step(26,g);return -1;}
void ftCo_800CF4DC(Fighter_GObj* g){(void)g;abort();}
void ftCommon_ClampAirDrift(Fighter* fp){step(27,fp->gobj);fp->self_vel.x=0.5f;}
void ftCommon_8007EBAC(Fighter* fp,u32 a,u32 b)
{CHECK(a==(shield_break_case?24:8)&&b==0);step(28,fp->gobj);}

GroundOrAir ft_80081D0C(Fighter_GObj* g){(void)g;abort();}
bool ft_80082708(Fighter_GObj* g){(void)g;abort();}
bool ft_800827A0(Fighter_GObj* g){(void)g;abort();}
bool ft_80082888(Fighter_GObj* g,ftCollisionBox* b){(void)g;(void)b;abort();}
bool ft_80082978(Fighter_GObj* g,ftCollisionBox* b){(void)g;(void)b;abort();}
bool ft_800824A0(Fighter_GObj* g,ftCollisionBox* b){(void)g;(void)b;abort();}
void ftCo_8009B390(Fighter_GObj* g,float f){(void)g;(void)f;abort();}
void ft_8008A348(Fighter_GObj* g,float f){(void)g;(void)f;abort();}
void ftCo_FallAerial_Enter(Fighter_GObj* g){(void)g;abort();}
void ftCo_80096900(Fighter_GObj* g,int a,int b,bool c,float d,float e)
{(void)g;(void)a;(void)b;(void)c;(void)d;(void)e;abort();}
void ftCo_800969D8(Fighter_GObj* g,int a,int b,int c,float d,float e,float f)
{(void)g;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;abort();}
void ftCo_Landing_Enter(Fighter_GObj* g,FtMotionId a,bool b,MotionFlags c,float d,float e)
{(void)g;(void)a;(void)b;(void)c;(void)d;(void)e;abort();}
void ftCo_Landing_Enter_Basic(Fighter_GObj* g){(void)g;abort();}
void ftCo_LandingFallSpecial_Enter(Fighter_GObj* g,bool b,float f){(void)g;(void)b;(void)f;abort();}
void ftCo_LandingAir_EnterWithMsidLag(Fighter_GObj* g,FtMotionId m,float f)
{(void)g;(void)m;(void)f;abort();}
