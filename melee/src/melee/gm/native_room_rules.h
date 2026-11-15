#ifndef MELEE_GM_NATIVE_ROOM_RULES_H
#define MELEE_GM_NATIVE_ROOM_RULES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { NATIVE_ROOM_RULES_BYTES = 25 };
typedef struct NativeRoomRules {
    uint64_t item_mask;
    uint32_t stage_mask;
    uint8_t mode, time, stocks, handicap, damage, stage_policy, stock_time;
    uint8_t friendly_fire, pause, score, sd_penalty, item_frequency, teams;
} NativeRoomRules;

typedef struct NativeRoomLobbyState {
    NativeRoomRules rules;
    uint64_t revision;
    uint16_t remaining_ms;
    uint8_t phase;
    bool has_rules;
} NativeRoomLobbyState;

struct GameRules;
struct GamePrefs;
bool nativeRoomRules_Encode(const NativeRoomRules* rules, uint8_t bytes[NATIVE_ROOM_RULES_BYTES]);
bool nativeRoomRules_Decode(NativeRoomRules* rules, const uint8_t* bytes, size_t size);
void nativeRoomRules_Capture(NativeRoomRules* output, const struct GameRules* rules,
                             const struct GamePrefs* prefs, bool teams);
void nativeRoomRules_Apply(const NativeRoomRules* source, struct GameRules* rules,
                           struct GamePrefs* prefs);

#ifdef __cplusplus
}
#endif
#endif
