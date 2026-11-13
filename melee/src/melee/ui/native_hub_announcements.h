#pragma once
struct NativeCommunityView;
void nativeHubAnnouncements_Init(const char* root);
void nativeHubAnnouncements_Tick(const NativeCommunityView& community);
void nativeHubAnnouncements_Inspect();
void nativeHubAnnouncements_Shutdown();
