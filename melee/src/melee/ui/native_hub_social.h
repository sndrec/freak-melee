#pragma once
#include "native_hub_internal.h"
void nativeHubSocial_Tick();
void nativeHubSocial_Render(NativeHubPage page);
bool nativeHubSocial_Activate(HubAction action, int value);
bool nativeHubSocial_Page(NativeHubPage page);
void nativeHubSocial_FighterRecipients();
bool nativeHubSocial_TakeFighter(NativeContentId* id);
void nativeHubSocial_Scroll(int amount);
void nativeHubSocial_Inspect();
void nativeHubSocial_Shutdown();
