#pragma once
#include "native_hub_internal.h"

void nativeHubNews_Tick();
void nativeHubNews_Input();
void nativeHubNews_Ticker();
void nativeHubNews_OpenArticle(const char* id);
void nativeHubNews_Render();
bool nativeHubNews_Activate(HubAction action, int value);
void nativeHubNews_Scroll(int rows);
void nativeHubNews_Inspect();
void nativeHubNews_Shutdown();
