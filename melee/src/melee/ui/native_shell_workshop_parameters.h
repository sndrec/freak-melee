#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_PARAMETERS_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_PARAMETERS_H

#ifdef MELEE_NATIVE

#include <string>

namespace Rml { class ElementDocument; }
struct NativeFighterProject;

void nativeShellWorkshopParameters_Open(Rml::ElementDocument* document,
                                       NativeFighterProject* project);
bool nativeShellWorkshopParameters_Save(Rml::ElementDocument* document,
                                       NativeFighterProject* project,
                                       const char* project_path);
bool nativeShellWorkshopParameters_Dirty(Rml::ElementDocument* document,
                                        const NativeFighterProject* project);
bool nativeShellWorkshopParameters_Import(Rml::ElementDocument* document,
                                         const NativeFighterProject* project,
                                         std::string* state_source);

#endif
#endif
