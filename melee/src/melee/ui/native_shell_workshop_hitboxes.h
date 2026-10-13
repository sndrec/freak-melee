#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_HITBOXES_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_HITBOXES_H

#ifdef MELEE_NATIVE

namespace Rml { class ElementDocument; }
struct NativeFighterHitbox;

void nativeShellWorkshopHitboxes_Init(Rml::ElementDocument* document);
void nativeShellWorkshopHitboxes_Update(bool visible, int selected_hitbox,
                                       const NativeFighterHitbox* hitboxes,
                                       unsigned active_mask);

#endif
#endif
