#include "ftCo_800C70D0.h"

#include "fighter.h"
#include "ftCo_800C7070.h"
#include "ftdata.h"
#include "kinds/ftCommon/forward.h"
#include "types.h"

static inline void inlineB0(Fighter_GObj* gobj)
{
    ftCo_800C7070(gobj);
}

static inline void inlineB1(Fighter_GObj* gobj)
{
    inlineB0(gobj);
}

static inline void inlineB2(Fighter_GObj* gobj)
{
    inlineB1(gobj);
}

void ftCo_800C70D0(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCommon_MotionState ms;
#ifdef MELEE_NATIVE
    FighterKind kind = fp->behavior_kind;
#else
    FighterKind kind = fp->kind;
#endif
    if (kind == Ft_Kind_Mario) {
        ms = ftCo_MS_Wait;
    } else if (kind == Ft_Kind_Luigi) {
        ms = ftCo_MS_Wait;
    } else {
        inlineB2(gobj);
        return;
    }
    Fighter_ChangeMotionState(gobj, ms, 0, 0.0F, 1.0F, 0.0F, NULL);
    fp->x2219_b2 = true;
    fp->x2219_b1 = true;
}

void ftCo_800C7158(Fighter_GObj* gobj)
{
    ft_8008521C(gobj);
}
