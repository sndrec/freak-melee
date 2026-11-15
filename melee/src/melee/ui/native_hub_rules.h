#pragma once
#include "native_hub_internal.h"
void nativeHubRules_Open();
bool nativeHubRules_Page(NativeHubPage page);
bool nativeHubRules_Activate(HubAction action, int value);
void nativeHubRules_Render(NativeHubPage page);
bool nativeHubRules_OwnsField(const char* field);
void nativeHubRules_CommitField();
void nativeHubRules_Inspect();
