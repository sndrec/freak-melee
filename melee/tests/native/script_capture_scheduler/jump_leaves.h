#include <melee/ft/native_script_binding_jump.h>
#include <melee/ft/kinds/ftCommon/ftCo_KneeBend.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0C8C.h>
#include <melee/ft/ftwalljump.h>
struct Fighter_804D6524_t* Fighter_804D6524;
float ftCo_800D0EC8(Fighter* fp){(void)fp;abort();}
static bool jump_case;
static bool multi_jump_case;
void ft_800881D8(Fighter* fp,int sound,u8 volume,u8 pan)
{CHECK(jump_case&&sound==0x456&&volume==0x7f&&pan==0x40);step(31,fp->gobj);}
void ft_PlaySFX(Fighter* fp,enum_t sound,u8 volume,u8 pan)
{if(shield_break_case){CHECK(sound==130&&volume==127&&pan==64);step(41,fp->gobj);return;}
 if(multi_jump_case){CHECK(sound==0x96&&volume==0x7f&&pan==0x40);step(34,fp->gobj);return;}
 CHECK(jump_case&&sound==0x11a&&volume==0x7f&&pan==0x40);step(32,fp->gobj);}
void ft_80084DB0(Fighter_GObj* actor)
{CHECK(jump_case||damage_tick_case);step(30,actor);Fighter* fp=actor->user_data;fp->self_vel.y-=0.125f;fp->x74_self_accel.x=0.25f;}
// Unrelated aerial-jump paths must never satisfy this ground-jump cohort.
void ftCo_JumpAerial_Begin(Fighter_GObj* g,FtMotionId motion,Vec3* velocity,bool reset,bool animated,bool sound)
{(void)g;(void)motion;(void)velocity;(void)reset;(void)animated;(void)sound;abort();}
void ftCo_JumpAerial_Anim(Fighter_GObj* g){(void)g;abort();}
void ftCo_JumpAerial_IASA(Fighter_GObj* g){(void)g;abort();}
bool ftWallJump_8008169C(Fighter_GObj* g){(void)g;abort();}
