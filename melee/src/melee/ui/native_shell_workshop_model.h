#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_MODEL_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_MODEL_H

#ifdef MELEE_NATIVE

#include <melee/ft/native_fighter_action_editor.h>

namespace Rml { class ElementDocument; }

void nativeShellWorkshopModel_Init(Rml::ElementDocument* document);
bool nativeShellWorkshopModel_Open(unsigned member, NativeFighterProject* project,
                                   const NativeFighterActionInfo* actions,
                                   const bool* has_clip,
                                   const float* clip_frames,
                                   size_t action_count);
bool nativeShellWorkshopModel_Select(unsigned member);
void nativeShellWorkshopModel_Close();
void nativeShellWorkshopModel_Show(bool visible);
bool nativeShellWorkshopModel_Click(const char* id, bool shift);
void nativeShellWorkshopModel_Update();
bool nativeShellWorkshopModel_HasUnsavedEdits(unsigned member);
bool nativeShellWorkshopModel_Save(const char* path, unsigned member);
bool nativeShellWorkshopModel_Undo(bool redo);
int nativeShellWorkshopModel_SelectedClip();

#endif
#endif
