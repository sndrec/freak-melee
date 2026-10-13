#ifndef MELEE_GM_NATIVE_SETTINGS_H
#define MELEE_GM_NATIVE_SETTINGS_H

#ifdef MELEE_NATIVE

#include <stdbool.h>

struct GameRules;
struct GamePrefs;

#ifdef __cplusplus
extern "C" {
#endif

bool nativeSettings_Load(struct GameRules* rules, struct GamePrefs* prefs);
bool nativeSettings_Save(const struct GameRules* rules,
                         const struct GamePrefs* prefs);
bool nativeSettings_LoadFile(struct GameRules* rules, struct GamePrefs* prefs,
                             const char* path);
bool nativeSettings_SaveFile(const struct GameRules* rules,
                             const struct GamePrefs* prefs, const char* path);
bool nativeSettings_PreserveForRoomMatch(const struct GameRules* rules,
                                         const struct GamePrefs* prefs);
bool nativeSettings_RestoreAfterRoomMatch(struct GameRules* rules,
                                          struct GamePrefs* prefs);

#ifdef __cplusplus
}
#endif

#endif
#endif
