#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_SKINNING_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_SKINNING_H
#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/gm/native_home.h>
#include <vector>

struct NativeWorkshopSkinStroke {
    std::vector<NativeImportedSkinVertex> vertices;
    std::vector<NativeHomeWorkshopBoneScreen> screens;
    std::vector<uint8_t> visible;
    std::vector<NativeSkinBrushPoint> centers;
    std::vector<unsigned> indices;
    bool active = false;
};

bool nativeWorkshopSkinning_Begin(NativeWorkshopSkinStroke*,
                                  const NativeFighterProject*, unsigned mesh);
void nativeWorkshopSkinning_Append(NativeWorkshopSkinStroke*, float x, float y,
                                   float width, float height);
void nativeWorkshopSkinning_Clear(NativeWorkshopSkinStroke*);
void nativeWorkshopSkinning_Inspect(const NativeFighterProject*, unsigned mesh);
#endif
