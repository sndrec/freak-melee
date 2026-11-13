#pragma once

// Modal notices use the hub's native menu canvas and consume dismissal input.
struct NativeHubNoticeOptions {
    const char* id = nullptr;
    const char* action = nullptr;
    void (*dismissed)(void* context, bool action_chosen) = nullptr;
    void* context = nullptr;
};
bool nativeHubNotice_Show(const char* title, const char* message, NativeHubNoticeOptions options = {});
bool nativeHubNotice_Contains(const char* id);
bool nativeHubNotice_Update(const char* id, const char* title, const char* message, const char* action = nullptr);
bool nativeHubNotice_Cancel(const char* id);
bool nativeHubNotice_BlocksInput();
void nativeHubNotice_Dismiss(bool choose_action = false);
void nativeHubNotice_Click(float x, float y);
void nativeHubNotice_Draw();
void nativeHubNotice_Inspect();
void nativeHubNotice_Clear();
