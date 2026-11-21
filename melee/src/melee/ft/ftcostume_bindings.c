#include "ftcostume_bindings.h"
#include "ftcharacter_assets.h"
#include "ftcostume_assets.h"
#include "native_fighter_pool.h"
#include <melee/it/native_article_animation_texture.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <string.h>

static const FtCostumeBindings stock[][8] = {
#include <melee/ft/stock_costume_bindings.inc>
};
_Static_assert(ARRAY_SIZE(stock) == Ft_Kind_StockCount, "Stock outfit table must cover all fighters");
_Static_assert(ARRAY_SIZE(((FtCostumeBindings*)0)->textures) ==
               ARRAY_SIZE(((CostumeTObjList*)0)->costume_tobjs), "Match runtime texture slots");

bool ftCostumeBindings_Stock(FighterKind kind, unsigned variant, FtCostumeBindings* out)
{
    if ((unsigned)kind >= ARRAY_SIZE(stock) || variant >= ARRAY_SIZE(stock[0]) ||
        !stock[kind][variant].joints) return false;
    *out = stock[kind][variant];
    return true;
}

static void count_required_draws(FtCostumeBindings* out, const FtPartsVisLookup* lookup,
                                 unsigned groups)
{
    if (!lookup) return;
    for (unsigned group = 0; group < groups; ++group) {
        for (int i = 0; i < lookup[group].x0; ++i) {
            const TempS* variant = &lookup[group].x4[i];
            for (int j = 0; j < variant->x0; ++j) {
                unsigned needed = 1u + variant->x4[j];
                if (needed > out->required_draws) out->required_draws = needed;
            }
        }
    }
}

bool ftCostumeBindings_FromCharacter(const FtCharacterAssets* character, unsigned joints,
    unsigned visibility_mask, unsigned variant, FtCostumeBindings* out)
{
    const struct ftData_x8* model = &character->model_data;
    FtCostumeBindings value = { .joints = joints };
    unsigned v = variant < model->x0.costume_count ? variant : 0;
    for (unsigned group = 0; group < 4; ++group) {
        if (group == 2) continue; // Metal uses a separate model and draw list.
        const FtPartsVisLookup* lookup = model->x0.vis_table[v][group];
        if (!lookup) lookup = model->x0.vis_table[0][group];
        count_required_draws(&value, lookup, model->x0.model_num);
    }
    for (unsigned i = 0; i < FT_CHARACTER_ITEM_CAPACITY; ++i)
        if (visibility_mask & (1u << i))
            count_required_draws(&value, character->visibility_assets[i], model->x0.model_num);
    if (model->x8.x8 > ARRAY_SIZE(value.textures)) return false;
    value.texture_count = model->x8.x8;
    if (value.texture_count) {
        v = variant < model->x8.costume_count ? variant : 0;
        const u16* textures = model->x8.xC[v] ? model->x8.xC[v] : model->x8.xC[0];
        if (!textures) return false;
        memcpy(value.textures, textures, value.texture_count * sizeof(*textures));
    }
    *out = value;
    return true;
}

typedef struct CostumeWalk {
    const FtCostumeBindings* bindings;
    unsigned draws, textures, animated;
} CostumeWalk;

// Follow the live fighter's joint/draw order. Material animations are paired
// by joint and mesh position, then by the GX texture ID assigned to the layer.
static bool walk_model(CostumeWalk* walk, const HSD_Joint* joint, const HSD_MatAnimJoint* materials,
                       bool at_root)
{
    for (; joint; joint = joint->next, materials = materials && !at_root ? materials->next : NULL) {
        const HSD_MatAnim* anim = materials ? materials->matanim : NULL;
        for (const HSD_DObjDesc* mesh = union_type_dobj(joint) ? joint->u.dobjdesc : NULL;
             mesh; mesh = mesh->next, anim = anim ? anim->next : NULL) {
            if (++walk->draws > NATIVE_FIGHTER_PRIMARY_DOBJS) return false;
            if (!walk->bindings->texture_count || !mesh->mobjdesc) continue;
            NativeArticleAnimationTextureID layers[GX_MAX_TEXMAP];
            uint32_t count = 0;
            if (!nativeArticleAnimation_TextureIDs(mesh->mobjdesc, layers, ARRAY_SIZE(layers), &count))
                return false;
            for (unsigned slot = 0; slot < walk->bindings->texture_count; ++slot) {
                unsigned layer = walk->bindings->textures[slot] - walk->textures;
                if (layer >= count) continue;
                const HSD_TexAnim* track = anim ? anim->texanim : NULL;
                while (track && track->id != layers[layer].id) track = track->next;
                if (!track || !track->aobjdesc) return false;
                walk->animated |= 1u << slot;
            }
            walk->textures += count;
        }
        if (!(joint->flags & JOBJ_INSTANCE) &&
            !walk_model(walk, joint->child, materials ? materials->child : NULL, false)) return false;
    }
    return true;
}

bool ftCostumeBindings_Fit(const FtCostumeBindings* bindings, const FtCostumeAssets* costume)
{
    if (costume->joint_count != bindings->joints || bindings->texture_count > ARRAY_SIZE(bindings->textures))
        return false;
    CostumeWalk walk = { .bindings = bindings };
    return walk_model(&walk, costume->joint, costume->matanim_joint, true) &&
        walk.draws >= bindings->required_draws && walk.animated == (1u << bindings->texture_count) - 1;
}
