#pragma once
#include "native_hub_internal.h"

void nativeHubQuickStage_Render();
void nativeHubQuickStage_Tick();
void nativeHubQuickStage_DrawCursor(const Mtx canvas);
bool nativeHubQuickStage_Activate(HubAction action, int value);
bool nativeHubQuickStage_OpenRound();
