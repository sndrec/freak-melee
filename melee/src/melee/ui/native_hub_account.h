#pragma once
#include "native_hub_internal.h"
void nativeHubAccount_Tick();
void nativeHubAccount_Render();
const char* nativeHubAccount_Status();
bool nativeHubAccount_Activate(HubAction action, int value);
void nativeHubAccount_Leave();
void nativeHubAccount_Inspect();
void nativeHubAccount_Shutdown();
