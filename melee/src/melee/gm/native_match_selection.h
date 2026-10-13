#ifndef MELEE_GM_NATIVE_MATCH_SELECTION_H
#define MELEE_GM_NATIVE_MATCH_SELECTION_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_lobby_match.h"

typedef struct VsModeData VsModeData;

// Builds the gameplay portion of a match proposal. The online session assigns
// its fresh session ID and the room topology assigns peers before validation.
bool nativeMatchSelection_Build(NativeLobbyMatch* match,
                                const VsModeData* vs, uint8_t stage,
                                const NativeContentId costume_packages[4],
                                const NativeContentId fighter_packages[4]);

#endif
#endif
