#ifndef MELEE_GM_NATIVE_LOBBY_SELECTION_H
#define MELEE_GM_NATIVE_LOBBY_SELECTION_H

#ifdef MELEE_NATIVE

#include "native_content_store.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_LOBBY_SELECTION_MAX_PLAYERS = 4,
    NATIVE_LOBBY_SELECTION_MAX_WIRE_BYTES = 335,
};

typedef struct NativeLobbyPlayerSelection {
    uint64_t character_id;
    uint64_t costume_id;
    NativeContentId character_package_id;
    NativeContentId costume_package_id;
    uint8_t team;
    bool selected;
} NativeLobbyPlayerSelection;

typedef struct NativeLobbySelection {
    NativeLobbyPlayerSelection players[NATIVE_LOBBY_SELECTION_MAX_PLAYERS];
    uint8_t player_count;
    bool confirmed;
    bool start_requested;
} NativeLobbySelection;

#ifdef __cplusplus
extern "C" {
#endif

bool nativeLobbySelection_Validate(const NativeLobbySelection* selection);
size_t nativeLobbySelection_Encode(const NativeLobbySelection* selection,
                                   uint8_t* output,
                                   size_t output_capacity);
bool nativeLobbySelection_Decode(NativeLobbySelection* selection,
                                 const uint8_t* bytes, size_t size);

#ifdef __cplusplus
}
#endif

#endif
#endif
