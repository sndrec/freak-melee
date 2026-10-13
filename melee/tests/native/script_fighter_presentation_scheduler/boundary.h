#ifndef SCRIPT_FIGHTER_PRESENTATION_BOUNDARY_H
#define SCRIPT_FIGHTER_PRESENTATION_BOUNDARY_H
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/types.h>
#include <melee/ft/ftwaitanim.h>
#include <sysdolphin/baselib/gobj.h>
extern NativeScriptCompileBank presentation_bank;
extern unsigned presentation_checks,presentation_bookkeeping,presentation_common_motion,presentation_stock_calls;
extern unsigned presentation_reset_calls,presentation_overlay_calls,presentation_loop_calls,presentation_demo_tail;
extern unsigned presentation_retire_mode,presentation_retire_at,presentation_callback_writes;
extern int presentation_overlay_branch;
extern bool presentation_source_package;
extern bool policy_frames,policy_jump_sound;
extern unsigned policy_restarts,policy_wait_changes,policy_rng_calls,policy_jumps;
extern bool policy_multi;
extern unsigned policy_jump_tail;
s32 reference_ft_80089914(HSD_GObj*,int);
void reference_ftCo_8008A7A8(Fighter_GObj*,WaitStruct*);
void reference_ft_800D2E7C(Fighter_GObj*,Vec3*);
void presentation_check(bool,const char*,unsigned);
#define CHECK(v) presentation_check((v),#v,__LINE__)
void presentation_reset(void);
void presentation_restore(void);
void presentation_destroy(void*);
HSD_GObj* presentation_create(FighterKind,bool attach);
NativeScriptValue presentation_state(HSD_GObj*,const char*,const NativeScriptValue*);
#endif
