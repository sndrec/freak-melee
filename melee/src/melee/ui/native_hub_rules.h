#pragma once
#include <melee/gm/forward.h>
#include "native_hub_internal.h"
void nativeHubRules_Open();
bool nativeHubRules_Page(NativeHubPage page);
bool nativeHubRules_Activate(HubAction action, int value);
void nativeHubRules_Render(NativeHubPage page);
GameRules* nativeHubRules_Data();
struct GamePrefs;
GamePrefs* nativeHubRules_Preferences();
void nativeHubRules_Inspect();
