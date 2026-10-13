// These guard slots are shared by Yoshi entry and common guard/input code.
#include <melee/ft/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SAME_SLOT(name)                                                       \
    _Static_assert(offsetof(union ftYoshi_MotionVars, guard.name) ==            \
                       offsetof(union ftCommon_MotionVars, guard.name),       \
                   "Yoshi/common guard slot mismatch: " #name)
SAME_SLOT(x0);
SAME_SLOT(xC);
SAME_SLOT(x10);
SAME_SLOT(x14);
SAME_SLOT(x18);
SAME_SLOT(x1C);
SAME_SLOT(x20);
SAME_SLOT(x24);
_Static_assert(sizeof(((union ftYoshi_MotionVars*) 0)->guard.x1C) == 4,
               "Guard input lock is a console scalar countdown");

int main(void)
{
    static Fighter fp;
    memset(&fp, 0xA5, sizeof(fp));
    fp.mv.co.guard.x1C = 7;
    fp.mv.co.guard.x20 = 19;
    fp.mv.co.guard.x24 = 23;

    // Exact entry writes in 8012BF40/44 and 8012C9DC/E0 must reset both
    // counters consumed by common item-throw and dash-catch input checks.
    fp.mv.ys.guard.x24 = 0;
    fp.mv.ys.guard.x20 = 0;
    if (fp.mv.co.guard.x1C != 7 || fp.mv.co.guard.x20 != 0 ||
        fp.mv.co.guard.x24 != 0)
    {
        fputs("Yoshi guard entry displaced common input counters\n", stderr);
        return 1;
    }
    // The shared powershield input lock stays an independent integer slot.
    fp.mv.co.guard.x1C -= 1;
    if (fp.mv.ys.guard.x1C != 6) {
        fputs("Yoshi guard input lock does not alias common guard\n", stderr);
        return 1;
    }
    puts("Yoshi/common guard scalar layout and entry reset passed");
    return 0;
}
