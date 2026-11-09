#include "native_ucf.h"

#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftZelda/forward.h>
#include <melee/ft/types.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/controller.h>
#include <math.h>

// UCF 0.84 uses the last three game-frame pad samples before HSD clamping.
// Keep that history in the fighter pool so rollback restores it with input.
bool nativeUcf_StickTravel(const Fighter* fp, bool horizontal, bool vertical,
                          int distance)
{
    const S8Vec2* stick = fp->ucf_stick;
    const int dx = horizontal ? stick[0].x - stick[2].x : 0;
    const int dy = vertical ? stick[0].y - stick[2].y : 0;
    return dx * dx + dy * dy > distance * distance;
}

bool nativeUcf_AtStickRim(float x, float y)
{
    // The bias makes Popo and Nana's quantized rim coordinates agree.
    const int ix = (int)(fabsf(x) * 80.0f - 0.0001f) + 2;
    const int iy = (int)(fabsf(y) * 80.0f - 0.0001f) + 2;
    return ix * ix + iy * iy > 80 * 80;
}

static void snap_cardinal(Vec2* stick, int x, int y)
{
    if ((x <= -80 || x >= 80) && y >= -6 && y <= 6) {
        *stick = (Vec2){ x < 0 ? -1.0f : 1.0f, 0.0f };
    } else if ((y <= -80 || y >= 80) && x >= -6 && x <= 6) {
        *stick = (Vec2){ 0.0f, y < 0 ? -1.0f : 1.0f };
    }
}

void nativeUcf_Sample(Fighter* fp)
{
    const PadLibData* pads = &HSD_PadLibData;
    if (pads->queue == NULL || pads->qnum == 0 ||
        fp->x618_player_id >= 4)
    {
        return;
    }
    const int index = pads->qread ? pads->qread - 1 : pads->qnum - 1;
    const PADStatus* pad = &pads->queue[index].stat[fp->x618_player_id];
    fp->ucf_stick[2] = fp->ucf_stick[1];
    fp->ucf_stick[1] = fp->ucf_stick[0];
    fp->ucf_stick[0] = pad->err ? (S8Vec2){ 0, 0 }
                               : (S8Vec2){ pad->stickX, pad->stickY };

    if (pad->err == 0 &&
        !(fp->behavior_kind == Ft_Kind_Zelda &&
          fp->motion_id == ftZd_MS_SpecialHiStart_0))
    {
        snap_cardinal(&fp->input.lstick[0], pad->stickX, pad->stickY);
        snap_cardinal(&fp->input.cstick[0], pad->substickX, pad->substickY);
    }

    const Vec2 stick = fp->input.lstick[0];
    if (stick.y <= -78.0f / 128.0f && nativeUcf_AtStickRim(stick.x, stick.y) &&
        (fp->ucf_shield_drop_frames != 0 ||
         (fp->x671_timer_lstick_tilt_y < 2 &&
          nativeUcf_StickTravel(fp, false, true, 44))))
    {
        if (fp->ucf_shield_drop_frames < 2) {
            ++fp->ucf_shield_drop_frames;
        }
    } else {
        fp->ucf_shield_drop_frames = 0;
    }
}

bool nativeUcf_ShieldDropReady(const Fighter* fp)
{
    if (fp->x221F_b4) {
        const Fighter_GObj* leader = Player_GetEntityAtIndex(fp->player_id, 0);
        if (leader != NULL) {
            fp = GET_FIGHTER(leader);
        }
    }
    return fp->ucf_shield_drop_frames >= 2;
}
