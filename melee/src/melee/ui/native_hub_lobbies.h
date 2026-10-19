#pragma once
#include "native_hub_internal.h"
void nativeHubLobbies_Tick();
void nativeHubLobbies_Render(bool create);
bool nativeHubLobbies_Activate(HubAction action, int value);
void nativeHubLobbies_Shutdown();
void nativeHubLobbies_Inspect();
