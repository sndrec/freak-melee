#pragma once
#include <stdbool.h>
#include <melee/gm/forward.h>
struct GamePrefs;
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NativeMenuRuleBounds { float x,y,w,h; } NativeMenuRuleBounds;
struct MnMenuAssets;
void nativeMenuRules_Load(struct MnMenuAssets* assets, GameRules* rules, struct GamePrefs* prefs);
typedef enum NativeMenuRulesPage { NativeMenuRules_Match, NativeMenuRules_Additional, NativeMenuRules_Lobby, NativeMenuRules_Items } NativeMenuRulesPage;
void nativeMenuRules_Draw(NativeMenuRulesPage page,unsigned selected,unsigned value);
void nativeMenuRules_AnimateAdjustment(int direction);
bool nativeMenuRules_Bounds(unsigned row, NativeMenuRuleBounds* bounds);
void nativeMenuRules_Unload(void);
void nativeMenuRules_Label(unsigned row,const char* text,bool selected);
void nativeMenuRules_AnimateLobbyValue(unsigned row, int direction);
void nativeMenuRules_Value(unsigned row, const char* const* values,
                           unsigned count, unsigned value);
#ifdef __cplusplus
}
#endif
