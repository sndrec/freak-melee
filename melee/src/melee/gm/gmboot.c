#include "gmboot.h"

#include "gm_unsplit.h"
#include "gmmain_lib.h"
#include "types.h"
#include <melee/lb/lbcardgame.h>
#include <melee/lb/lbcardnew.h>
#include <melee/lb/lblanguage.h>
#include <melee/ty/toy.h>

/* 1BF948 */ static void bootOnLoad(GameModeState*);
/* 1BF9A8 */ static void bootOnLeave(GameModeState*);
/* 1BFA3C */ static void memcardOnLoad(GameModeState*);

/// @todo Move to toy header
enum {
    TROPHY_PIKMIN = 275,
};

struct loadData {
    u32 x0;
    u8 x4;
    u8 mode_id; ///< Copied to ::leaveData::mode_id to set next mode
};

struct leaveData {
    u32 x0;
    u8 mode_id;
};

static struct loadData load_data;
static struct leaveData leave_data;

GameModeState gm_Mode_Boot_States[] = {
    {
        0,
        1,
        0,
        bootOnLoad,
        bootOnLeave,
        GS_MEMCARD,
        &load_data,
        &leave_data,
    },
    {
        -1,
    },
};

void bootOnLoad(GameModeState* scene)
{
    struct loadData* scene_data = gm_GetGameModeStateEnterData(scene);
    scene_data->x4 = 0;
    scene_data->x0 = 0;
    scene_data->mode_id = GM_MENU;
}

void bootOnLeave(GameModeState* data)
{
    struct leaveData* scene_data = gm_GetGameModeStateExitData(data);

    if (!Toy_803048C0(TROPHY_PIKMIN)) {
        if (!lb_8001C2D8(0, "01",
                         lbLang_GetLanguageSetting() == LANG_JP ? "GPIJ"
                                                                : "GPIE",
                         "Pikmin dataFile"))
        {
            Toy_803124BC();
            Toy_SetUnlockState(TROPHY_PIKMIN, true);
        }
    }

#ifndef MELEE_NATIVE
    gm_SetGameModeOverride(lbCardGame_DecideGameMode);
#endif

    // Enter mode
    gm_ChangeGameModeAfterCurrentScene(scene_data->mode_id);
}

GameModeState gm_Mode_MemCard_States[] = {
    {
        0,
        3,
        0,
        memcardOnLoad,
        NULL,
        {
            GS_MEMCARD,
            &load_data,
            &leave_data,
        },
    },
    { -1 },
};

void memcardOnLoad(GameModeState* scene)
{
    struct loadData* temp_r3 = gm_GetGameModeStateEnterData(scene);
    temp_r3->x4 = 0;
    temp_r3->x0 = 1;
}
