#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_BONES_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_BONES_H

#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_animation_editor.h>
namespace Rml { class ElementDocument; }
struct NativeFighterProject;

void nativeShellWorkshopBones_Init(Rml::ElementDocument* document);
void nativeShellWorkshopBones_Reset();
void nativeShellWorkshopBones_Update(const NativeFighterProject* project,
    NativeFighterAnimationClip clip, int selected_joint, bool action_visible,
    bool animation_visible);
bool nativeShellWorkshopBones_Click(const char* id, unsigned* joint);
#endif

#endif
