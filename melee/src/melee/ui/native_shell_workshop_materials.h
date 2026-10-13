#ifndef NATIVE_SHELL_WORKSHOP_MATERIALS_H
#define NATIVE_SHELL_WORKSHOP_MATERIALS_H
#include <string>

#include <melee/ft/native_fighter_model_editor.h>
namespace Rml {
    class ElementDocument;
}
std::string
nativeWorkshopMaterials_Controls(const NativeFighterModelEditor& model,
                                 unsigned material);
bool nativeWorkshopMaterials_Apply(Rml::ElementDocument* document,
                                   NativeFighterModelEditor& model,
                                   unsigned material, std::string* error);
#endif
