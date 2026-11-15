#include "native_match_selection.h"

#ifdef MELEE_NATIVE

#include <string.h>

#include "forward.h"
#include "gmmain_lib.h"
#include "gmresult.h"
#include "native_stock_content.h"
#include "types.h"
#include <melee/ft/forward.h>
#include <melee/mn/types.h>

uint8_t nativeMatchSelection_LoserPort(const NativeLobbyMatch* match,
                                      const MatchEnd* result)
{
    if (!match || !result || gm_WasMatchCanceled(result->outcome))
        return UINT8_MAX;
    uint8_t loser = UINT8_MAX, worst = 0;
    for (uint8_t port = 0; port < 4; ++port) {
        if (match->slots[port].kind != NativeLobbyMatchSlot_Human) continue;
        uint8_t team = match->slots[port].team;
        if (match->teams && team >= GM_MAX_TEAMS) return UINT8_MAX;
        uint8_t rank = match->teams
            ? result->team_standings[team].is_big_loser
            : result->player_standings[port].is_big_loser;
        if (rank > worst) { worst = rank; loser = port; }
    }
    return loser;
}

static bool valid_stock_identity(uint8_t character, uint8_t color)
{
    uint8_t decoded_character;
    uint8_t decoded_color;
    return nativeStockContent_CharacterFromId(
               nativeStockContent_CharacterId(character),
               &decoded_character) &&
           decoded_character == character &&
           nativeStockContent_CostumeFromId(
               nativeStockContent_CostumeId(character, color),
               character, &decoded_color) &&
           decoded_color == color;
}

bool nativeMatchSelection_Build(NativeLobbyMatch* match,
                                const VsModeData* vs, uint8_t stage,
                                const NativeContentId costume_packages[4],
                                const NativeContentId fighter_packages[4])
{
    if (match == NULL || vs == NULL) {
        return false;
    }
    uint8_t decoded_stage;
    uint64_t stage_id = nativeStockContent_StageId(stage);
    if (!nativeStockContent_StageFromId(stage_id, &decoded_stage) ||
        decoded_stage != stage)
    {
        return false;
    }

    const GameRules* rules = gmMainLib_GetGameRules();
    const struct GamePrefs* prefs = gmMainLib_GetGamePrefs();
    if (rules->mode > NativeLobbyMatchMode_Coin) {
        return false;
    }
    *match = (NativeLobbyMatch) {
        .stage_id = stage_id,
        .item_mask = prefs->item_freq == UINT8_MAX ? 0 : prefs->item_mask,
        .mode = rules->mode,
        .time_limit_minutes = rules->time_limit,
        .stock_time_limit_minutes = rules->stock_time_limit,
        .stock_count = rules->stock_count,
        .item_frequency = prefs->item_freq == UINT8_MAX
                              ? -1
                              : (int8_t) prefs->item_freq,
        .handicap = rules->handicap,
        .damage_ratio = rules->damage_ratio,
        .teams = vs->start.rules.is_teams,
        .friendly_fire = rules->friendly_fire,
        .pause = rules->pause,
        .score_display = rules->score_display,
        .sd_penalty = rules->unk_xc,
    };
    memcpy(match->content_digest, nativeStockContent_Digest(),
           sizeof(match->content_digest));

    for (uint32_t port = 0; port < 4; ++port) {
        const PlayerInitData* source = &vs->start.players[port];
        NativeLobbyMatchSlot* output = &match->slots[port];
        if (source->slot_type == Gm_PKind_NA) {
            output->kind = NativeLobbyMatchSlot_Empty;
            continue;
        }
        if (source->slot_type == Gm_PKind_Human) {
            output->kind = NativeLobbyMatchSlot_Human;
        } else if (source->slot_type == Gm_PKind_Cpu) {
            output->kind = NativeLobbyMatchSlot_Cpu;
            output->cpu_level = source->cpu_level;
        } else {
            return false;
        }
        const NativeContentId empty = { 0 };
        const NativeContentId* fighter = fighter_packages != NULL
                                             ? &fighter_packages[port]
                                             : &empty;
        bool custom = memcmp(fighter, &empty, sizeof(empty)) != 0;
        if (custom) {
            if (source->ckind < ChKind_CustomFirst || source->color != 0) {
                return false;
            }
            output->character_package_id = *fighter;
        } else {
            if (!valid_stock_identity(source->ckind, source->color)) {
                return false;
            }
            output->character_id =
                nativeStockContent_CharacterId(source->ckind);
            output->costume_id =
                nativeStockContent_CostumeId(source->ckind, source->color);
        }
        if (costume_packages != NULL) {
            output->costume_package_id = costume_packages[port];
        }
        output->team = source->team;
        output->handicap = source->handicap;
    }
    return true;
}

#endif
