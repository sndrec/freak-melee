#include "kernels.h"
#include <stdlib.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/kinds/ftCommon/ftCo_LandingAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/mp/mpcoll.h>
#include <melee/ft/ft_0819.h>
#include <melee/ft/ftwalljump.h>
#include <melee/ft/ftcliffcommon.h>
// Every unselected engine leaf aborts if reached.
bool ft_80082708(Fighter_GObj* gobj) { abort(); }
bool ft_800827A0(Fighter_GObj* gobj) { abort(); }
GroundOrAir ft_80081D0C(Fighter_GObj* gobj) { abort(); }
bool ft_80082888(Fighter_GObj*, ftCollisionBox*) { abort(); }
bool ft_800824A0(Fighter_GObj*, ftCollisionBox*) { abort(); }
void ftCo_LandingFallSpecial_Enter(Fighter_GObj*,
                                                bool allow_interrupt,
                                                float landing_lag) { abort(); }
void ftCo_80096900(Fighter_GObj* gobj, int, int,
                                bool allow_interrupt, float mobility,
                                float landing_lag) { abort(); }
void ftCo_Fall_Enter(Fighter_GObj* gobj) { abort(); }
void ftCo_FallAerial_Enter(Fighter_GObj* gobj) { abort(); }
void ftCo_Landing_Enter_Basic(Fighter_GObj* gobj) { abort(); }
void ft_8008A348(Fighter_GObj* gobj, float) { abort(); }
void ftCo_800969D8(Fighter_GObj* gobj, int, int, int, float,
                                float, float) { abort(); }
void ftCo_LandingAir_EnterWithMsidLag(Fighter_GObj* gobj,
                                                   FtMotionId msid, float lag) { abort(); }
void ftCo_Landing_Enter(Fighter_GObj* gobj, FtMotionId msid,
                                     bool allow_interrupt, MotionFlags flags,
                                     float anim_start, float anim_speed) { abort(); }
void ftCo_800DEAE8(Fighter_GObj* gobj, FtMotionId msid0,
                                FtMotionId msid1) { abort(); }
void ft_80084104(Fighter_GObj* gobj) { abort(); }
void ft_80084280(Fighter_GObj*) { abort(); }
void mpCopyCollData(CollData* src, CollData* dst, int) { abort(); }
void ft_80083B68(Fighter_GObj* gobj) { abort(); }
void mpCollSetFacingDir(CollData*, int facing_dir) { abort(); }
bool mpColl_80047AC8(CollData*, bool (*)(Fighter_GObj*, enum_t),
                                  Fighter_GObj*) { abort(); }
bool mpColl_80047E14(CollData*, bool (*)(Fighter_GObj*, enum_t),
                                  Fighter_GObj*) { abort(); }
bool ftCo_80096CC8(Fighter_GObj* gobj, int line_id) { abort(); }
bool ft_80081A00(Fighter_GObj* gobj) { abort(); }
bool ftWallJump_8008169C(HSD_GObj* gobj) { abort(); }
bool ftCliffCommon_80081298(Fighter_GObj* gobj) { abort(); }
