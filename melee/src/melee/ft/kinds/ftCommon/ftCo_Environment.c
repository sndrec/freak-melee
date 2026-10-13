#include "ftCo_Bury.h"
#include "forward.h"
#include <melee/ft/fighter.h>
#include <melee/ft/types.h>
#include <melee/gr/forward.h>

bool ftCo_800C0A28(Fighter_GObj* gobj, UNK_T arg1, ftCommon_BuryType arg2)
{
    Fighter* fp = GET_FIGHTER(gobj);
    switch (arg2) {
    case BuryType_Unk2: {
        if (fp->bury_timer_2 != 0 || fp->motion_id == ftCo_MS_BarrelWait ||
            fp->x1A6A & 0x20 || fp->x2224_b2)
        {
            return false;
        }
        break;
    }
    case BuryType_Unk3: {
        if (fp->bury_timer_2 != 0) {
            return false;
        }
        break;
    }
    default:
        break;
    }
    return true;
}

void ftCo_800C0A98(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    if (fp->bury_timer_1 != 0) {
        --fp->bury_timer_1;
    }
    switch (fp->bury_grkind) {
    case Gr_Kind_OldKongo:
    case Gr_Kind_Kongo:
        if (fp->bury_timer_2 != 0) {
            --fp->bury_timer_2;
            return;
        }
        break;
    case Gr_Kind_ShrineRoute:
    case Gr_Kind_Pushon:
    case Gr_Kind_Zebes:
        if (fp->bury_timer_2 != 0) {
            --fp->bury_timer_2;
            return;
        }
        break;
    default:
        break;
    }
}
