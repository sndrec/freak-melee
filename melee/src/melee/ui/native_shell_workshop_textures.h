#pragma once
#include "native_shell_workshop_materials.h"
struct NativeCostumeDat;
std::string nativeWorkshopTextures_Controls(const NativeFighterModelEditor&,
                                            unsigned material);
bool nativeWorkshopTextures_Apply(Rml::ElementDocument*,
                                  const NativeFighterModelEditor&,
                                  unsigned material, NativeCostumeDat&,
                                  std::string* error);
