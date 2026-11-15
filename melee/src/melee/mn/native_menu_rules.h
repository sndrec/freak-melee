#pragma once
#include <stdbool.h>
#include <melee/gm/forward.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NativeMenuRuleBounds { float x,y,w,h; } NativeMenuRuleBounds;
struct MnMenuAssets;
void nativeMenuRules_Load(struct MnMenuAssets* assets, GameRules* rules);
typedef enum NativeMenuRulesPage { NativeMenuRules_Match, NativeMenuRules_Additional, NativeMenuRules_Lobby } NativeMenuRulesPage;
void nativeMenuRules_Draw(NativeMenuRulesPage page,unsigned selected,unsigned value);
bool nativeMenuRules_Bounds(unsigned row, NativeMenuRuleBounds* bounds);
void nativeMenuRules_Unload(void);
void nativeMenuRules_Label(unsigned row,const char* text,bool selected);
void nativeMenuRules_Value(unsigned row,const char* text);
#ifdef __cplusplus
}
#endif
