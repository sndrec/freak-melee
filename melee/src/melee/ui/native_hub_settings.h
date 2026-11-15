#pragma once
#include "native_hub_internal.h"
void nativeHubSettings_Load();
bool nativeHubSettings_Page(NativeHubPage page);
void nativeHubSettings_Render(NativeHubPage page);
bool nativeHubSettings_Activate(HubAction action, int value);
void nativeHubSettings_Inspect();
