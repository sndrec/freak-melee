#include "lbspdisplay.h"
#include <melee/sc/types.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/debug.h>

HSD_LObj* lb_80011AC4(LightList** list)
{
    HSD_LObj* prev;
    HSD_LObj* curr;
    HSD_LObj* first;
    HSD_LightAnim** temp_r4;

    prev = NULL;
    while (*list != NULL) {
        curr = HSD_LObjLoadDesc((*list)->desc);
        temp_r4 = (*list)->anims;
        if (temp_r4 != NULL) {
            HSD_LObjAddAnimAll(curr, temp_r4[0]);
        }
        if (prev != NULL) {
            HSD_LObjSetNext(prev, curr);
        } else {
            first = curr;
        }
        prev = curr;
        list++;
    }
    return first;
}

