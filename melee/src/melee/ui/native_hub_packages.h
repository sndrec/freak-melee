#pragma once
#include "native_hub_internal.h"
void nativeHubPackages_Open();
void nativeHubPackages_Tick();
bool nativeHubPackages_Page(NativeHubPage page);
bool nativeHubPackages_Activate(HubAction action,int value);
void nativeHubPackages_Render(NativeHubPage page);
void nativeHubPackages_Inspect();
