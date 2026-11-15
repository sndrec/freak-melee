#pragma once
union SDL_Event;
void nativeCssChat_SetVisible(bool visible);
void nativeCssChat_Focus(bool focused);
void nativeCssChat_Draw();
void nativeCssChat_Send();
bool nativeCssChat_Event(const SDL_Event* event);
bool nativeCssChat_Focused();
void nativeCssChat_Inspect();
