#pragma once
union SDL_Event;
struct HSD_PadStatus;
void nativeHubPortrait_Begin();
void nativeHubPortrait_Tick();
void nativeHubPortrait_Render();
void nativeHubPortrait_Reset();
void nativeHubPortrait_Save();
bool nativeHubPortrait_Busy();
bool nativeHubPortrait_Controller(const HSD_PadStatus& pad);
bool nativeHubPortrait_Event(const SDL_Event* event);
void nativeHubPortrait_Inspect();
void nativeHubPortrait_Open(const char* draft, unsigned member, bool return_to_editor);
bool nativeHubPortrait_Editing();
char* nativeHubPortrait_TileName();
enum class HubAction;
bool nativeHubPortrait_Activate(HubAction action, int value);
bool nativeHubPortrait_StoreCamera(const char* draft);
