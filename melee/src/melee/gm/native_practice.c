#include "native_practice.h"

#include "gm_1601.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ft_0D31.h>
#include <melee/ft/forward.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/gr/forward.h>
#include <melee/gr/grpractice.h>
#include <melee/gr/stage.h>
#include <melee/mn/types.h>
#include <melee/pl/forward.h>
#include <melee/pl/player.h>
#include <melee/pl/plstale.h>
#include <sysdolphin/baselib/debug.h>
#include <stdio.h>

void nativePractice_InspectOpponent(void)
{
    const int slot = NATIVE_PRACTICE_OPPONENT_SLOT;
    fprintf(stderr, "[practice-opponent] spawned=%u character=%u damage=%d cpu=%d\n",
        Player_GetEntity(slot) != NULL, (unsigned) Player_GetPlayerCharacter(slot),
        Player_GetDamage(slot), Player_GetCpuType(slot));
}

static void reset_escaped_sandbag(void)
{
    Fighter_GObj* gobj = Player_GetEntity(NATIVE_PRACTICE_OPPONENT_SLOT);
    if (gobj == NULL) {
        return;
    }
    Fighter* fp = GET_FIGHTER(gobj);
    // Sandbag deliberately bypasses the stock blast-zone death checks. Keep
    // that behavior; the practice mode alone returns an escaped dummy home.
    bool escaped = fp->cur_pos.x < Stage_GetBlastZoneLeftOffset() ||
                   fp->cur_pos.x > Stage_GetBlastZoneRightOffset() ||
                   fp->cur_pos.y < Stage_GetBlastZoneBottomOffset() ||
                   fp->cur_pos.y > Stage_GetBlastZoneTopOffset();
    if (!escaped) {
        return;
    }

    // Use the fighter lifecycle cleanup before resetting its position/state:
    // this releases grabs and attached effects rather than teleporting them.
    ftCo_800D331C(gobj);
    Vec3 spawn;
    Stage_80224E64(4, &spawn);
    spawn.y += 20;
    // Player coordinates are updated during play, not an immutable spawn.
    // Respawn above the center and let ordinary collision land the dummy.
    Player_80032768(NATIVE_PRACTICE_OPPONENT_SLOT, &spawn);
    Player_SetHUDDamage(NATIVE_PRACTICE_OPPONENT_SLOT, 0);
    plStale_ResetStaleMoveTableForPlayer(NATIVE_PRACTICE_OPPONENT_SLOT);
    if (!Fighter_UnkProcessDeath_80068354(gobj)) return;
    ftCo_Fall_Enter(gobj);
    OSReport("[practice] Sandbag reset at center floor.\n");
}

void nativePractice_Configure(StartMeleeData* start, int editor_port,
                              unsigned stage, unsigned opponent)
{
    const bool editing = editor_port >= 0 && editor_port < 4;
    const bool sandbag = opponent == ChKind_Sandbag;
    grPractice_SetEditorPreview(editing);
    // Practice has no winner or time limit. Normal time-match rebirth keeps
    // local fighters available after a KO without consuming their stocks.
    start->rules.stkind = stage;
    start->rules.match_kind = MatchKind_Time;
    start->rules.is_stock = false;
    start->rules.timer_enabled = false;
    start->rules.time_limit = 0;
    start->rules.item_freq = -1;
    start->rules.x20 = 0;
    start->rules.disable_pausing = false;
    start->rules.is_teams = false;
    start->rules.x0_3 = 4; // Only the four local players have a versus HUD.
    start->rules.on_frame_end = sandbag ? reset_escaped_sandbag : NULL;

    if (editing) {
        start->rules.on_frame_end = NULL;
        for (unsigned port = 0; port < 6; ++port) {
            if (port != (unsigned) editor_port)
                gm_SetupPlayerDefaults(&start->players[port]);
        }
        start->players[editor_port].spawn_pos = 4;
        start->players[editor_port].xC_b1 = false;
        return;
    }

    PlayerInitData* cpu = &start->players[NATIVE_PRACTICE_OPPONENT_SLOT];
    gm_SetupPlayerDefaults(cpu);
    cpu->ckind = opponent;
    cpu->slot_type = Gm_PKind_Cpu;
    if (sandbag) cpu->cpu_kind = 0xF; // Home-Run Contest dummy behavior.
    cpu->cpu_level = sandbag ? 0 : 3;
    cpu->stocks = 1;
    cpu->slot = 1; // Valid controller and color index for the CPU.
    cpu->spawn_pos = 4;
    cpu->xC_b1 = false;
    cpu->xD_b3 = true;
    gm_SetupPlayerDefaults(&start->players[5]);
}
