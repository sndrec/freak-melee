#pragma once
#include "native_hub_internal.h"
#include <melee/gm/native_account.h>
const NativeAccountView& nativeHubAccount_View();
void nativeHubAccount_Tick();
void nativeHubAccount_Render();
const char* nativeHubAccount_Status();
bool nativeHubAccount_Activate(HubAction action, int value);
void nativeHubAccount_Leave();
void nativeHubAccount_Inspect();
void nativeHubAccount_Shutdown();
void nativeHubAccount_BeginStartup();
bool nativeHubAccount_BackStartup();
bool nativeHubAccount_StartupContinue();
