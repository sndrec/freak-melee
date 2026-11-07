#pragma once

// Modal notices use the hub's native menu canvas and consume dismissal input.
void nativeHubNotice_Show(const char* title, const char* message);
bool nativeHubNotice_BlocksInput();
void nativeHubNotice_Dismiss();
void nativeHubNotice_Draw();
void nativeHubNotice_Inspect();
void nativeHubNotice_Clear();
