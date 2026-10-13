#ifndef MELEE_GM_SCORE_STATE_INTERNAL_H
#define MELEE_GM_SCORE_STATE_INTERNAL_H
#include <Runtime/platform.h>
typedef struct GmScoreState {
    struct {
        u8 initialized,track_leaders;
        struct {u8 zero_score,leader;} players[6];
    } flags;
    u8 scores[6];
    u8 predicates_disabled;
    s32 top_score;
} GmScoreState;
extern GmScoreState gm_score_state;
#endif
