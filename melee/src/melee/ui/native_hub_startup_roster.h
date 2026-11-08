#pragma once
#include "native_menu_portrait_capture.h"
void nativeHubStartupRoster_Begin();
void nativeHubStartupRoster_Select(int kind);
void nativeHubStartupRoster_Move(int direction);
void nativeHubStartupRoster_Render();
void nativeHubStartupRoster_Unload();
void nativeHubStartupRoster_Inspect();
void nativeHubStartupRoster_Capture();
unsigned nativeHubStartupRoster_Subjects(NativePortraitSubject subjects[2]);
void nativeHubStartupRoster_Confirm();
