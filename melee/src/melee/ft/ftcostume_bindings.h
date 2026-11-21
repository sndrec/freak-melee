#ifndef MELEE_FT_FTCOSTUME_BINDINGS_H
#define MELEE_FT_FTCOSTUME_BINDINGS_H

#include <melee/ft/forward.h>
#include <stdbool.h>
#include <stdint.h>

struct FtCharacterAssets;
struct FtCostumeAssets;
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FtCostumeBindings {
    uint16_t joints, required_draws;
    uint8_t texture_count;
    uint16_t textures[5];
} FtCostumeBindings;

bool ftCostumeBindings_Stock(FighterKind kind, unsigned variant, FtCostumeBindings* out);
bool ftCostumeBindings_FromCharacter(const struct FtCharacterAssets* character, unsigned joints,
    unsigned visibility_mask, unsigned variant, FtCostumeBindings* out);
bool ftCostumeBindings_Fit(const FtCostumeBindings* bindings, const struct FtCostumeAssets* costume);

#ifdef __cplusplus
}
#endif
#endif
