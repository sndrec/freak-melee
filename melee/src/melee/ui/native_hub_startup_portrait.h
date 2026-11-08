#pragma once
union SDL_Event;
struct HSD_PadStatus;
void nativeHubStartupPortrait_Begin();
void nativeHubStartupPortrait_Tick();
void nativeHubStartupPortrait_Render();
void nativeHubStartupPortrait_Reset();
void nativeHubStartupPortrait_Save();
bool nativeHubStartupPortrait_Busy();
bool nativeHubStartupPortrait_Controller(const HSD_PadStatus& pad);
bool nativeHubStartupPortrait_Event(const SDL_Event* event);
void nativeHubStartupPortrait_Inspect();
