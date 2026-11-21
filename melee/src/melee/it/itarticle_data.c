#include "itarticle_data.h"
#include "article_model_layers.h"

#include <stddef.h>
#include <string.h>

#include "itCharItems.h"
#include "itCommonItems.h"
#include "kinds/itkinoko.h"
#include <melee/lb/types.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#ifdef MELEE_NATIVE
#include <melee/ft/native_fighter_runtime.h>
#endif

static bool fail(HSD_DatImport* import, HSD_DatResult status, u32 offset)
{
    import->status = status;
    import->error_offset = offset;
    return false;
}

static HSD_DatReferenceKind resolve(const HSD_Dat* dat, u32 slot, u32* target)
{
    const char* external;
    return HSD_DatResolve(dat, slot, target, &external);
}

#ifdef MELEE_NATIVE
bool itArticle_ImportModelRoot(HSD_DatImport* import, const Article* article,
                              u32 offset, HSD_Joint** output)
{
    if (!import || !article || !output || import->status != HSD_DAT_OK)
        return false;
    u32 source = article->native_special_attribute_source;
    if (source == UINT32_MAX || HSD_DatBytes(import->dat, source, 4) == NULL)
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    u32 end = HSD_DatImportNextTarget(import, source);
    if ((offset & 3) || end < source || end - source < 4 ||
        offset > end - source - 4)
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    u32 target;
    HSD_DatReferenceKind kind = resolve(import->dat, source + offset, &target);
    if (kind == HSD_DAT_REF_NULL) { *output = NULL; return true; }
    if (kind != HSD_DAT_REF_LOCAL)
        return fail(import, kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                       : HSD_DAT_RELOCATION,
                    source + offset);
    HSD_Joint* root = HSD_DatImportGraph(import, target, DAT_JOINT);
    if (!root) return false;
    *output = root;
    return true;
}

bool itArticle_ImportModelRoots(HSD_DatImport* import, Article* article,
                                const NativeFighterArticleModelRoot* roots,
                                unsigned count)
{
    if (import == NULL || article == NULL || import->status != HSD_DAT_OK)
        return false;
    if (count > sizeof(article->native_model_roots) /
                    sizeof(article->native_model_roots[0]) ||
        (count != 0 && roots == NULL))
        return fail(import, HSD_DAT_DESCRIPTOR, article->native_special_attribute_source);
    if (count == 0) return true;
    u32 source = article->native_special_attribute_source;
    HSD_Joint* imported[NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT] = { 0 };
    for (unsigned i = 0; i < count; ++i) {
        if (roots[i].name_hash == 0)
            return fail(import, HSD_DAT_DESCRIPTOR, source);
        for (unsigned j = 0; j < i; ++j)
            if (roots[i].name_hash == roots[j].name_hash)
                return fail(import, HSD_DAT_DESCRIPTOR, source + roots[i].attribute_offset);
        if (!itArticle_ImportModelRoot(import, article, roots[i].attribute_offset,
                                       &imported[i])) return false;
    }
    // Publish only a complete declared table; failed graph import is discarded
    // with its owning character arena before any gameplay begins.
    for (unsigned i = 0; i < count; ++i) {
        article->native_model_roots[i].name_hash = roots[i].name_hash;
        article->native_model_roots[i].root = imported[i];
    }
    article->native_model_root_count = (u16) count;
    return true;
}

bool itArticle_ImportModelAnimation(HSD_DatImport* import, const Article* article,
    const NativeFighterArticleAnimationComponent components[3],
    ItArticleModelAnimationBundle* output)
{
    if (!import || !article || !components || !output || import->status != HSD_DAT_OK)
        return false;
    u32 source = article->native_special_attribute_source;
    if (source == UINT32_MAX || HSD_DatBytes(import->dat, source, 4) == NULL)
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    u32 end = HSD_DatImportNextTarget(import, source);
    if (end < source || end - source < 4)
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    void* imported[3] = { 0 };
    static const HSD_DatImportType types[3] = {
        DAT_ANIMJOINT, DAT_MATANIM_JOINT, DAT_SHAPE_JOINT
    };
    for (unsigned channel = 0; channel < 3; ++channel) {
        const NativeFighterArticleAnimationComponent* component = &components[channel];
        u32 offset = component->attribute_offset;
        if (component->array_index != UINT32_MAX && component->array_index >= UINT32_MAX / 4)
            return fail(import, HSD_DAT_DESCRIPTOR, source);
        if (offset == UINT32_MAX) {
            if (component->array_index != UINT32_MAX)
                return fail(import, HSD_DAT_DESCRIPTOR, source);
            continue;
        }
        if ((offset & 3) != 0 || offset > end - source - 4)
            return fail(import, HSD_DAT_DESCRIPTOR, source);
        u32 target, slot = source + offset;
        HSD_DatReferenceKind kind = resolve(import->dat, slot, &target);
        if (kind == HSD_DAT_REF_NULL) continue;
        if (kind != HSD_DAT_REF_LOCAL)
            return fail(import, kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL :
                        HSD_DAT_RELOCATION, slot);
        if (component->array_index != UINT32_MAX) {
            u32 array_end = HSD_DatImportNextTarget(import, target);
            if (array_end < target || array_end - target < 4 ||
                component->array_index > (array_end - target - 4) / 4)
                return fail(import, HSD_DAT_DESCRIPTOR, slot);
            slot = target + 4 * component->array_index;
            kind = resolve(import->dat, slot, &target);
            if (kind == HSD_DAT_REF_NULL) continue;
            if (kind != HSD_DAT_REF_LOCAL)
                return fail(import, kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL :
                            HSD_DAT_RELOCATION, slot);
        }
        imported[channel] = HSD_DatImportGraph(import, target, types[channel]);
        if (!imported[channel]) return false;
    }
    *output = (ItArticleModelAnimationBundle){ imported[0], imported[1], imported[2] };
    return true;
}

bool itArticle_ImportModelAnimations(HSD_DatImport* import, Article* article,
    const NativeFighterArticleModelAnimation* animations, unsigned count)
{
    if (!import || !article || import->status != HSD_DAT_OK) return false;
    u32 source = article->native_special_attribute_source;
    if (count > NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT ||
        (count && !animations)) return fail(import, HSD_DAT_DESCRIPTOR, source);
    if (!count) return true;
    ItArticleModelAnimationBundle imported[NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT] = { 0 };
    for (unsigned i = 0; i < count; ++i) {
        if (!animations[i].name_hash) return fail(import, HSD_DAT_DESCRIPTOR, source);
        for (unsigned j = 0; j < i; ++j)
            if (animations[i].name_hash == animations[j].name_hash)
                return fail(import, HSD_DAT_DESCRIPTOR, source);
        if (!itArticle_ImportModelAnimation(import, article, animations[i].components, &imported[i]))
            return false;
    }
    // Legacy direct callers retain their complete table publication contract;
    // exact source-owned bundles use the typed importer without this hash table.
    for (unsigned i = 0; i < count; ++i) {
        article->native_model_animations[i].name_hash = animations[i].name_hash;
        article->native_model_animations[i].anim = imported[i].anim;
        article->native_model_animations[i].matanim = imported[i].matanim;
        article->native_model_animations[i].shapeanim = imported[i].shapeanim;
    }
    article->native_model_animation_count = (u16)count;
    return true;
}

#endif

static bool read_s32(const HSD_Dat* dat, u32 offset, s32* output)
{
    u32 value;
    if (!HSD_DatReadU32(dat, offset, &value)) {
        return false;
    }
    *output = (s32) value;
    return true;
}

static bool read_f32_words(const HSD_Dat* dat, u32 source, void* output,
                           size_t size)
{
    u8* destination = output;
    if ((size & 3) != 0 || HSD_DatBytes(dat, source, size) == NULL) {
        return false;
    }
    for (size_t i = 0; i < size; i += 4) {
        if (!HSD_DatReadF32(dat, source + (u32) i, (f32*) (destination + i))) {
            return false;
        }
    }
    return true;
}

static bool import_graph_reference(HSD_DatImport* import, u32 slot,
                                   HSD_DatImportType type, void** output);


static itGamewatchchefAttributes*
import_gamewatch_chef_attributes(HSD_DatImport* import, u32 slot, Article* article)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x74)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itGamewatchchefAttributes* output =
        HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->model_layers = articleModelLayers_Import(import, source,
        article->x10_modelDesc != NULL ? article->x10_modelDesc->x0_joint : NULL);
    if (output->model_layers == NULL ||
        !read_f32_words(
            import->dat, source + 4, &output->wall_bounce_velocity_multiplier,
            sizeof(*output) - offsetof(itGamewatchchefAttributes,
                                       wall_bounce_velocity_multiplier)))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return NULL;
    }
    article->model_layers = output->model_layers;
    return output;
}

static itGamewatchArticleAttributes*
import_gamewatch_article_attributes(HSD_DatImport* import, u32 slot, Article* article)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 4)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itGamewatchArticleAttributes* output =
        HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->model_layers = articleModelLayers_Import(import, source,
        article->x10_modelDesc != NULL ? article->x10_modelDesc->x0_joint : NULL);
    article->model_layers = output->model_layers;
    return output->model_layers != NULL ? output : NULL;
}

static void* import_words(HSD_DatImport* import, u32 source, size_t size)
{
    u8* destination;
    if ((size & 3) != 0 || HSD_DatBytes(import->dat, source, size) == NULL ||
        (destination = HSD_DatImportAlloc(import, size)) == NULL)
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    for (size_t i = 0; i < size; i += 4) {
        u32 value;
        if (!HSD_DatReadU32(import->dat, source + (u32) i, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, source + (u32) i);
            return NULL;
        }
        memcpy(destination + i, &value, sizeof(value));
    }
    return destination;
}

static void* import_optional_words(HSD_DatImport* import, u32 slot,
                                   size_t maximum)
{
    u32 source;
    HSD_DatReferenceKind kind = resolve(import->dat, slot, &source);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        fail(import,
             kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                          : HSD_DAT_RELOCATION,
             slot);
        return NULL;
    }
    u32 end = HSD_DatImportNextTarget(import, source);
    size_t size = end - source;
    if (size > maximum) {
        size = maximum;
    }
    size &= ~(size_t) 3;
    return size != 0 ? import_words(import, source, size) : NULL;
}

static itLinkArrowAttributes*
import_link_arrow_attributes(HSD_DatImport* import, u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x2C)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itLinkArrowAttributes* output =
        HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!read_f32_words(import->dat, source, output, 0x24) ||
        !import_graph_reference(import, source + 0x24, DAT_JOINT,
                                (void**) &output->x24) ||
        !import_graph_reference(import, source + 0x28, DAT_JOINT,
                                (void**) &output->x28))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return NULL;
    }
    return output;
}

static bool import_graph_reference(HSD_DatImport* import, u32 slot,
                                   HSD_DatImportType type, void** output)
{
    u32 source;
    const char* external;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &source, &external);
    if (kind == HSD_DAT_REF_NULL) {
        *output = NULL;
        return true;
    }
    if (kind == HSD_DAT_REF_EXTERNAL) {
        import->unresolved_external_count++;
        import->external_symbol = external;
        *output = NULL;
        return true;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        return fail(import, HSD_DAT_RELOCATION, slot);
    }
    bool allow_unresolved = import->allow_unresolved_externals;
    import->allow_unresolved_externals = true;
    *output = HSD_DatImportGraph(import, source, type);
    import->allow_unresolved_externals = allow_unresolved;
    return *output != NULL;
}

static itFoodsAttributes* import_food_attributes(HSD_DatImport* import,
                                                 u32 slot)
{
    u32 source;
    s32 count;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        !read_s32(import->dat, source, &count) || count <= 0 || count > 64 ||
        HSD_DatImportNextTarget(import, source) - source !=
            4 + (u32) count * 16)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    size_t size = offsetof(itFoodsAttributes, variants) +
                  (size_t) count * sizeof(itFoodVariant);
    itFoodsAttributes* output = HSD_DatImportAlloc(import, size);
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->variant_count = count;
    for (s32 i = 0; i < count; ++i) {
        u32 at = source + 4 + (u32) i * 16;
        itFoodVariant* variant = &output->variants[i];
        if (!import_graph_reference(import, at, DAT_JOINT,
                                    (void**) &variant->joint) ||
            !read_s32(import->dat, at + 4, &variant->heal_amount) ||
            !HSD_DatReadF32(import->dat, at + 8, &variant->offset_x) ||
            !HSD_DatReadF32(import->dat, at + 12, &variant->offset_y))
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
    }
    return output;
}

static KinokoAttrs* import_mushroom_attributes(HSD_DatImport* import, u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 16)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    KinokoAttrs* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!HSD_DatReadF32(import->dat, source, &output->ground_speed) ||
        !HSD_DatReadF32(import->dat, source + 4, &output->deceleration) ||
        !import_graph_reference(import, source + 8, DAT_ANIMJOINT,
                                (void**) &output->animation_joints[0]) ||
        !import_graph_reference(import, source + 12, DAT_ANIMJOINT,
                                (void**) &output->animation_joints[1]))
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    return output;
}

static itWstarAttributes* import_warp_star_attributes(HSD_DatImport* import,
                                                      u32 slot)
{
    u32 source;
    s32 count;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        !read_s32(import->dat, source + 0x24, &count) || count <= 0 ||
        count > 7 ||
        HSD_DatImportNextTarget(import, source) - source !=
            0x28 + (u32) count * 8)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    size_t size = offsetof(itWstarAttributes, entries) +
                  (size_t) count * sizeof(itWstarAttrEntry);
    itWstarAttributes* output = HSD_DatImportAlloc(import, size);
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!read_f32_words(import->dat, source, output, 0x24)) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    output->entry_count = count;
    for (s32 i = 0; i < count; ++i) {
        u32 at = source + 0x28 + (u32) i * 8;
        s32 sfx_id;
        if (!import_graph_reference(import, at, DAT_ANIMJOINT,
                                    (void**) &output->entries[i].anim_joint) ||
            !read_s32(import->dat, at + 4, &sfx_id))
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
        output->entries[i].sfx_id = sfx_id;
    }
    return output;
}

static bool special_attributes_have_references(const HSD_DatImport* import, u32 slot)
{
    const HSD_Dat* dat = import->dat;
    u32 source;
    if (resolve(dat, slot, &source) != HSD_DAT_REF_LOCAL) {
        return false;
    }
    return HSD_DatContainsReference(dat, source, HSD_DatImportNextTarget(import, source));
}

static ItemAttr* import_item_attributes(HSD_DatImport* import, u32 slot);

static itStageItemCommonAttributes*
import_stage_item_common_attributes(HSD_DatImport* import, u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 20)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itStageItemCommonAttributes* output =
        HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!read_s32(import->dat, source, &output->damage_threshold) ||
        !HSD_DatReadF32(import->dat, source + 4, &output->movement_speed) ||
        !HSD_DatReadF32(import->dat, source + 8, &output->x8) ||
        !HSD_DatReadF32(import->dat, source + 12, &output->xC) ||
        !read_s32(import->dat, source + 16, &output->x10))
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    return output;
}

static itHeihoAttributes* import_heiho_attributes(HSD_DatImport* import,
                                                  u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x1C)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itHeihoAttributes* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->common = import_stage_item_common_attributes(import, source);
    if (output->common == NULL ||
        !read_f32_words(import->dat, source + 4, output->movement_speeds,
                        6 * sizeof(f32)))
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    return output;
}

static itKyasarinAttributes* import_kyasarin_attributes(HSD_DatImport* import,
                                                        u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x4C)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itKyasarinAttributes* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->common = import_stage_item_common_attributes(import, source);
    if (output->common == NULL ||
        !read_f32_words(import->dat, source + 4, &output->x4, 0x3C) ||
        !read_s32(import->dat, source + 0x40, &output->x40) ||
        !read_s32(import->dat, source + 0x44, &output->x44) ||
        !read_s32(import->dat, source + 0x48, &output->x48))
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    return output;
}

static itKyasarinEggAttributes*
import_kyasarin_egg_attributes(HSD_DatImport* import, u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x14)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itKyasarinEggAttributes* output =
        HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->common = import_stage_item_common_attributes(import, source);
    if (output->common == NULL ||
        !HSD_DatReadF32(import->dat, source + 4, &output->x4) ||
        !HSD_DatReadF32(import->dat, source + 8, &output->x8) ||
        !read_s32(import->dat, source + 0xC, &output->xC) ||
        !read_s32(import->dat, source + 0x10, &output->x10))
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    return output;
}

static itWhiteBeaAttributes* import_whitebea_attributes(HSD_DatImport* import,
                                                        u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x18)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itWhiteBeaAttributes* output = HSD_DatImportAlloc(import, sizeof(*output));
    u16 x8, xA, xC, xE, x14;
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->common = import_stage_item_common_attributes(import, source);
    if (output->common == NULL ||
        !HSD_DatReadF32(import->dat, source + 4, &output->x4) ||
        !HSD_DatReadU16(import->dat, source + 8, &x8) ||
        !HSD_DatReadU16(import->dat, source + 0xA, &xA) ||
        !HSD_DatReadU16(import->dat, source + 0xC, &xC) ||
        !HSD_DatReadU16(import->dat, source + 0xE, &xE) ||
        !HSD_DatReadF32(import->dat, source + 0x10, &output->x10) ||
        !HSD_DatReadU16(import->dat, source + 0x14, &x14))
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    output->x8 = (s16) x8;
    output->xA = (s16) xA;
    output->xC = (s16) xC;
    output->xE = (s16) xE;
    output->x14 = (s16) x14;
    return output;
}

static itKlapAttributes* import_klap_attributes(HSD_DatImport* import,
                                                u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 4)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itKlapAttributes* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->common = import_stage_item_common_attributes(import, source);
    return output->common != NULL ? output : NULL;
}

static itWhispyAppleAttributes*
import_whispy_apple_attributes(HSD_DatImport* import, u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x1C)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itWhispyAppleAttributes* output =
        HSD_DatImportAlloc(import, sizeof(*output));
    const u8* xC = HSD_DatBytes(import->dat, source + 0xC, 8);
    if (output == NULL || xC == NULL) {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    output->common = import_stage_item_common_attributes(import, source);
    if (output->common == NULL ||
        !read_s32(import->dat, source + 4, &output->x4) ||
        !read_s32(import->dat, source + 8, &output->x8) ||
        !HSD_DatReadF32(import->dat, source + 0x14, &output->x14) ||
        !HSD_DatReadF32(import->dat, source + 0x18, &output->x18))
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    memcpy(output->xC, xC, sizeof(output->xC));
    return output;
}

static itTincleAttributes* import_tincle_attributes(HSD_DatImport* import,
                                                    u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x58)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itTincleAttributes* output = HSD_DatImportAlloc(import, sizeof(*output));
    const u8* tail = HSD_DatBytes(import->dat, source + 0x54, 4);
    if (output == NULL || tail == NULL) {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    memset(output, 0, sizeof(*output));
    output->common = import_stage_item_common_attributes(import, source);
    if (output->common == NULL ||
        !read_s32(import->dat, source + 4, &output->x4) ||
        !read_s32(import->dat, source + 8, &output->x8) ||
        !read_f32_words(import->dat, source + 0xC, &output->xC, 7 * 4) ||
        !read_s32(import->dat, source + 0x28, &output->x28) ||
        !read_s32(import->dat, source + 0x2C, &output->x2C) ||
        !read_s32(import->dat, source + 0x30, &output->x30) ||
        !read_f32_words(import->dat, source + 0x34, &output->x34, 2 * 4) ||
        !read_s32(import->dat, source + 0x3C, &output->x3C) ||
        !read_s32(import->dat, source + 0x40, &output->x40) ||
        !read_f32_words(import->dat, source + 0x44, &output->x44, 4 * 4))
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    output->x54 = (s8) tail[0];
    output->x55 = (s8) tail[1];
    return output;
}

static itArwingLaser_Attrs*
import_arwing_laser_attributes(HSD_DatImport* import, u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 12)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itArwingLaser_Attrs* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->common_attrs = import_item_attributes(import, source);
    if (output->common_attrs == NULL ||
        !HSD_DatReadF32(import->dat, source + 4, &output->aimed_speed_scale) ||
        !HSD_DatReadF32(import->dat, source + 8,
                        &output->beam_thickness_scale))
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    return output;
}

static void* import_stage_common_prefixed_attributes(HSD_DatImport* import,
                                                     u32 slot,
                                                     size_t source_size,
                                                     size_t native_size,
                                                     u32* source_out)
{
    u32 source;
    if ((source_size & 3) != 0 ||
        native_size < sizeof(void*) + source_size - 4 ||
        resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != source_size)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    u8* output = HSD_DatImportAlloc(import, native_size);
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    memset(output, 0, native_size);
    itStageItemCommonAttributes* common =
        import_stage_item_common_attributes(import, source);
    if (common == NULL) {
        return NULL;
    }
    memcpy(output, &common, sizeof(common));
    for (size_t offset = 4; offset < source_size; offset += 4) {
        u32 value;
        if (!HSD_DatReadU32(import->dat, source + (u32) offset, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, source + (u32) offset);
            return NULL;
        }
        memcpy(output + sizeof(void*) + offset - 4, &value, sizeof(value));
    }
    if (source_out != NULL) {
        *source_out = source;
    }
    return output;
}

static itLeadeadAttributes* import_leadead_attributes(HSD_DatImport* import,
                                                      u32 slot)
{
    u32 source;
    itLeadeadAttributes* output = import_stage_common_prefixed_attributes(
        import, slot, 0x20, sizeof(*output), &source);
    u16 x18, x1A, x1C;
    const u8* x1E;
    if (output == NULL) {
        return NULL;
    }
    x1E = HSD_DatBytes(import->dat, source + 0x1E, 1);
    if (!HSD_DatReadU16(import->dat, source + 0x18, &x18) ||
        !HSD_DatReadU16(import->dat, source + 0x1A, &x1A) ||
        !HSD_DatReadU16(import->dat, source + 0x1C, &x1C) || x1E == NULL)
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    output->x18 = (s16) x18;
    output->x1A = (s16) x1A;
    output->x1C = (s16) x1C;
    output->x1E = (s8) *x1E;
    return output;
}

static itOctarockAttributes* import_octarock_attributes(HSD_DatImport* import,
                                                        u32 slot)
{
    u32 source;
    itOctarockAttributes* output = import_stage_common_prefixed_attributes(
        import, slot, 0x20, sizeof(*output), &source);
    u16 x1C;
    if (output == NULL) {
        return NULL;
    }
    if (!HSD_DatReadU16(import->dat, source + 0x1C, &x1C)) {
        fail(import, HSD_DAT_DESCRIPTOR, source + 0x1C);
        return NULL;
    }
    output->x1C = (s16) x1C;
    return output;
}

static itOldottoseaAttributes* import_ottosea_attributes(HSD_DatImport* import,
                                                         u32 slot)
{
    u32 source;
    itOldottoseaAttributes* output = import_stage_common_prefixed_attributes(
        import, slot, 0x2C, sizeof(*output), &source);
    if (output == NULL) {
        return NULL;
    }
    const u8* x10 = HSD_DatBytes(import->dat, source + 0x10, 1);
    const u8* x28 = HSD_DatBytes(import->dat, source + 0x28, 1);
    if (x10 == NULL || x28 == NULL) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    output->x10 = (s8) *x10;
    output->x28 = (s8) *x28;
    return output;
}

static it_2E5A_Attrs* import_it_2e5a_attributes(HSD_DatImport* import,
                                                u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0xC0)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    it_2E5A_Attrs* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    memset(output, 0, sizeof(*output));
    if (!read_f32_words(import->dat, source, output, 0x3C)) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    for (u32 i = 0; i < ARRAY_SIZE(output->tiers); ++i) {
        u32 at = source + 0x3C + i * 0x2C;
        it_2E5A_TierEntry* tier = &output->tiers[i];
        if (!import_graph_reference(import, at, DAT_JOINT,
                                    (void**) &tier->joint) ||
            !import_graph_reference(import, at + 4, DAT_ANIMJOINT,
                                    (void**) &tier->anim_joint) ||
            !import_graph_reference(import, at + 8, DAT_MATANIM_JOINT,
                                    (void**) &tier->matanim_joint) ||
            !import_graph_reference(import, at + 0xC, DAT_SHAPE_JOINT,
                                    (void**) &tier->shape_anim_joint) ||
            !read_s32(import->dat, at + 0x10, &tier->xD84_value) ||
            !read_s32(import->dat, at + 0x14, &tier->threshold) ||
            !HSD_DatReadF32(import->dat, at + 0x18, &tier->scale) ||
            !read_f32_words(import->dat, at + 0x1C, &tier->ecb,
                            sizeof(tier->ecb)))
        {
            fail(import,
                 import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                              : import->status,
                 at);
            return NULL;
        }
    }
    return output;
}

static itUnknownAttributes* import_unown_attributes(HSD_DatImport* import,
                                                    u32 slot)
{
    u32 source;
    if (resolve(import->dat, slot, &source) != HSD_DAT_REF_LOCAL ||
        HSD_DatImportNextTarget(import, source) - source != 0x8C)
    {
        fail(import, HSD_DAT_DESCRIPTOR, slot);
        return NULL;
    }
    itUnknownAttributes* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    memset(output, 0, sizeof(*output));
    for (u32 offset = 0; offset < 0x24; offset += 4) {
        u32 value;
        if (!HSD_DatReadU32(import->dat, source + offset, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, source + offset);
            return NULL;
        }
        memcpy((u8*) output + offset, &value, sizeof(value));
    }
    for (u32 i = 0; i < ARRAY_SIZE(output->x24); ++i) {
        if (!import_graph_reference(import, source + 0x24 + i * 4, DAT_JOINT,
                                    (void**) &output->x24[i]))
        {
            return NULL;
        }
    }
    return output;
}

static void* import_special_attributes(HSD_DatImport* import, u32 slot,
                                       s32 item_kind, Article* article)
{
    switch (item_kind) {
    case It_Kind_Kirby_LinkArrow:
    case It_Kind_Kirby_CLinkArrow:
        return import_link_arrow_attributes(import, slot);
    case It_Kind_Kirby_GameWatchChef:
        return import_gamewatch_chef_attributes(import, slot, article);
    case It_Kind_Kirby_GameWatchChefPan:
        return import_gamewatch_article_attributes(import, slot, article);
    case It_Kind_Kuriboh:
        return import_stage_common_prefixed_attributes(
            import, slot, 0x14, sizeof(itOldkuriAttributes), NULL);
    case It_Kind_Leadead:
        return import_leadead_attributes(import, slot);
    case It_Kind_Octarock:
        return import_octarock_attributes(import, slot);
    case It_Kind_Ottosea:
        return import_ottosea_attributes(import, slot);
    case It_Kind_Unk4:
        return import_it_2e5a_attributes(import, slot);
    case It_Kind_Arwing_Laser:
        return import_arwing_laser_attributes(import, slot);
    case It_Kind_Heiho:
        return import_heiho_attributes(import, slot);
    case It_Kind_Klap:
        return import_klap_attributes(import, slot);
    case It_Kind_Kyasarin:
        return import_kyasarin_attributes(import, slot);
    case It_Kind_Kyasarin_Egg:
        return import_kyasarin_egg_attributes(import, slot);
    case It_Kind_Tincle:
        return import_tincle_attributes(import, slot);
    case It_Kind_WhispyApple:
    case It_Kind_WhispyHealApple:
        return import_whispy_apple_attributes(import, slot);
    case It_Kind_Whitebea:
        return import_whitebea_attributes(import, slot);
    case It_Kind_Foods:
        return import_food_attributes(import, slot);
    case It_Kind_Kinoko:
    case It_Kind_DKinoko:
        return import_mushroom_attributes(import, slot);
    case It_Kind_WStar:
        return import_warp_star_attributes(import, slot);
    case It_PKind_Unknown:
    case It_Kind_Unknown_Swarm:
        return import_unown_attributes(import, slot);
    default:
        if (item_kind >= 0 &&
            special_attributes_have_references(import, slot))
        {
            fail(import, HSD_DAT_DESCRIPTOR, slot);
            return NULL;
        }
        return import_optional_words(import, slot, 64 * 1024);
    }
}

static ItemAttr* import_item_attributes(HSD_DatImport* import, u32 slot)
{
    u32 source;
    HSD_DatReferenceKind kind = resolve(import->dat, slot, &source);
    if (kind != HSD_DAT_REF_LOCAL) {
        fail(import, HSD_DAT_RELOCATION, slot);
        return NULL;
    }
    ItemAttr* output = HSD_DatImportAlloc(import, sizeof(*output));
    const u8* prefix = HSD_DatBytes(import->dat, source, 4);
    if (output == NULL || prefix == NULL ||
        !read_f32_words(import->dat, source + 4, (u8*) output + 4,
                        sizeof(*output) - 4))
    {
        fail(import,
             import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                          : import->status,
             source);
        return NULL;
    }
    memcpy(output, prefix, 4);
    // ItemAttr's first two bytes are PowerPC bitfields. The retail compiler
    // assigns fields from the most-significant bit downward, while MSVC
    // assigns them from the least-significant bit upward. Decode each field
    // explicitly so native gameplay sees the same values as the disc build.
    output->x0_is_heavy = (prefix[0] >> 7) & 1;
    output->use_kind = (prefix[0] >> 3) & 0xF;
    output->x0_hold_kind = prefix[0] & 7;
    output->x1_1 = (prefix[1] >> 6) & 3;
    output->x1_3 = (prefix[1] >> 5) & 1;
    output->x1_4 = (prefix[1] >> 4) & 1;
    output->x1_5 = (prefix[1] >> 3) & 1;
    output->x1_67_cam_kind = (prefix[1] >> 1) & 3;
    output->x1_8 = prefix[1] & 1;
    return output;
}

static ItHurtBoneList* import_hurtbones(HSD_DatImport* import, u32 slot)
{
    u32 source;
    HSD_DatReferenceKind kind = resolve(import->dat, slot, &source);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        fail(import, HSD_DAT_RELOCATION, slot);
        return NULL;
    }
    s32 count;
    u32 descs_source;
    if (!read_s32(import->dat, source, &count) || count < 0 || count > 128 ||
        (count != 0 &&
         resolve(import->dat, source + 4, &descs_source) != HSD_DAT_REF_LOCAL))
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    ItHurtBoneList* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->count = count;
    if (count == 0) {
        return output;
    }
    output->descs =
        HSD_DatImportAlloc(import, (size_t) count * sizeof(*output->descs));
    if (output->descs == NULL) {
        import->error_offset = descs_source;
        return NULL;
    }
    for (s32 i = 0; i < count; ++i) {
        ItHurtBoneDesc* desc = &output->descs[i];
        u32 at = descs_source + (u32) i * 32;
        s32 bone_id;
        if (!read_s32(import->dat, at, &bone_id) ||
            !read_f32_words(import->dat, at + 4, &desc->a_offset, 28))
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
        desc->bone_id = bone_id;
    }
    return output;
}

static ItemStateArray* import_item_states(HSD_DatImport* import, u32 slot,
                                          ItArticleImportStats* stats,
                                          ItScriptCollect* scripts,
                                          const Article* article,
                                          bool exact_target_boundary,
                                          const u32* sibling_articles,
                                          u32 sibling_article_count)
{
    u32 source;
    HSD_DatReferenceKind kind = resolve(import->dat, slot, &source);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        fail(import, HSD_DAT_RELOCATION, slot);
        return NULL;
    }
    ItemStateArray* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    u32 state_count;
    if (exact_target_boundary) {
        u32 end = HSD_DatImportNextTarget(import, source);
        u32 byte_count = end - source;
        if (byte_count == 0 || byte_count % 16 != 0 || byte_count > 256 * 16) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
            return NULL;
        }
        state_count = byte_count / 16;
    } else {
        state_count = 0;
        while (state_count < 256) {
            u32 at = source + state_count * 16;
            if (state_count != 0) {
                // Fighter article tables supply their actual descriptor
                // addresses. Using those boundaries avoids mistaking a valid
                // animation state for another article merely because its
                // first six words happen to resemble descriptor references.
                bool found_sibling = false;
                for (u32 i = 0; i < sibling_article_count; ++i) {
                    if (at == sibling_articles[i]) {
                        found_sibling = true;
                        break;
                    }
                }
                if (found_sibling ||
                    (sibling_articles == NULL &&
                     itArticle_IsDescriptor(import->dat, at, false)))
                {
                    break;
                }
            }
            bool valid = true;
            for (u32 field = 0; field < 4; ++field) {
                u32 ignored;
                if (resolve(import->dat, at + field * 4, &ignored) ==
                    HSD_DAT_REF_INVALID)
                {
                    valid = false;
                    break;
                }
            }
            if (!valid) {
                break;
            }
            ++state_count;
        }
        if (state_count == 256) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
            return NULL;
        }
    }
    if (state_count == 0) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    for (u32 i = 0; i < state_count; ++i) {
        u32 at = source + i * 16;
        for (u32 field = 0; field < 4; ++field) {
            u32 ignored;
            if (resolve(import->dat, at + field * 4, &ignored) ==
                HSD_DAT_REF_INVALID)
            {
                fail(import, HSD_DAT_DESCRIPTOR, at + field * 4);
                return NULL;
            }
        }
    }
    output->count = state_count;
    stats->state_count += state_count;
    output->x0_itemStateDesc = HSD_DatImportAlloc(
        import, (size_t) state_count * sizeof(*output->x0_itemStateDesc));
    if (output->x0_itemStateDesc == NULL) {
        import->error_offset = source;
        return NULL;
    }
    for (u32 i = 0; i < state_count; ++i) {
        ItemStateDesc* state = &output->x0_itemStateDesc[i];
        u32 at = source + i * 16;
        if (!import_graph_reference(import, at, DAT_ANIMJOINT,
                                    (void**) &state->x0_anim_joint) ||
            !import_graph_reference(import, at + 4, DAT_MATANIM_JOINT,
                                    (void**) &state->x4_matanim_joint) ||
            !import_graph_reference(import, at + 8, DAT_SHAPE_JOINT,
                                    (void**) &state->x8_parameters))
        {
            return NULL;
        }
        u32 script_source;
        HSD_DatReferenceKind script_kind =
            resolve(import->dat, at + 12, &script_source);
        if (script_kind == HSD_DAT_REF_LOCAL) {
            if (scripts->count >= scripts->capacity) {
                fail(import, HSD_DAT_CAPACITY, at + 12);
                return NULL;
            }
            scripts->entries[scripts->count++] =
                (ItScriptEntry){ .destination=&state->xC_script, .source=script_source, .article=article };
            stats->script_count++;
        } else if (script_kind == HSD_DAT_REF_EXTERNAL) {
            fail(import, HSD_DAT_EXTERNAL, at + 12);
            return NULL;
        } else if (script_kind != HSD_DAT_REF_NULL) {
            fail(import, HSD_DAT_RELOCATION, at + 12);
            return NULL;
        }
    }
    return output;
}

static ItemModelDesc* import_model(HSD_DatImport* import, u32 slot)
{
    u32 source;
    HSD_DatReferenceKind kind = resolve(import->dat, slot, &source);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        fail(import, HSD_DAT_RELOCATION, slot);
        return NULL;
    }
    ItemModelDesc* output = HSD_DatImportAlloc(import, sizeof(*output));
    u32 bone_count;
    s32 attach_id;
    const u8* flags = HSD_DatBytes(import->dat, source + 12, 1);
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!import_graph_reference(import, source, DAT_JOINT,
                                (void**) &output->x0_joint))
    {
        return NULL;
    }
    if (!HSD_DatReadU32(import->dat, source + 4, &bone_count) ||
        !read_s32(import->dat, source + 8, &attach_id) || flags == NULL)
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    output->x4_bone_count = bone_count;
    output->x8_bone_attach_id = attach_id;
    output->xC_bit_field = *flags;
    return output;
}

static bool import_dynamic_nodes(HSD_DatImport* import, u32 source, u32 count,
                                 DynamicsDesc* output)
{
    struct lb_00F9_UnkDesc1Inner* nodes =
        HSD_DatImportAlloc(import, (size_t) count * sizeof(*nodes));
    if (nodes == NULL || HSD_DatBytes(import->dat, source,
                                      (size_t) count * sizeof(*nodes)) == NULL)
    {
        import->error_offset = source;
        return false;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 at = source + i * (u32) sizeof(*nodes);
        if (!read_f32_words(import->dat, at, &nodes[i], sizeof(nodes[i]))) {
            return fail(import, HSD_DAT_DESCRIPTOR, at);
        }
    }
    output->data = (struct DynamicsData*) nodes;
    return true;
}

static ItemDynamics* import_dynamics(HSD_DatImport* import, u32 slot)
{
    u32 source;
    HSD_DatReferenceKind kind = resolve(import->dat, slot, &source);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        fail(import, HSD_DAT_RELOCATION, slot);
        return NULL;
    }
    s32 flexible_bone_count;
    s32 collision_bone_count;
    u32 flexible_bones_source;
    u32 collision_bones_source;
    if (!read_s32(import->dat, source, &flexible_bone_count) ||
        flexible_bone_count < 0 || flexible_bone_count > 32 ||
        !read_s32(import->dat, source + 8, &collision_bone_count) ||
        collision_bone_count < 0 || collision_bone_count > 2 ||
        (flexible_bone_count != 0 &&
         resolve(import->dat, source + 4, &flexible_bones_source) !=
             HSD_DAT_REF_LOCAL) ||
        (collision_bone_count != 0 &&
         resolve(import->dat, source + 12, &collision_bones_source) !=
             HSD_DAT_REF_LOCAL))
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    ItemDynamics* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    memset(output, 0, sizeof(*output));
    output->flexible_bone_count = flexible_bone_count;
    output->collision_bone_count = collision_bone_count;
    if (flexible_bone_count != 0) {
        output->flexible_bones =
            HSD_DatImportAlloc(import, (size_t) flexible_bone_count *
                                           sizeof(*output->flexible_bones));
        if (output->flexible_bones == NULL) {
            import->error_offset = flexible_bones_source;
            return NULL;
        }
    }
    for (s32 i = 0; i < flexible_bone_count; ++i) {
        BoneDynamicsDesc* desc = &output->flexible_bones[i];
        u32 at = flexible_bones_source + (u32) i * 24;
        u32 node_count;
        u32 nodes_source;
        s32 bone_id;
        if (!read_s32(import->dat, at, &bone_id) ||
            !HSD_DatReadU32(import->dat, at + 8, &node_count) ||
            node_count > 256 ||
            !read_f32_words(import->dat, at + 12, &desc->dyn_desc.pos, 12))
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
        desc->bone_id = bone_id;
        desc->dyn_desc.count = node_count;
        kind = resolve(import->dat, at + 4, &nodes_source);
        if (node_count == 0 && kind != HSD_DAT_REF_NULL) {
            fail(import, HSD_DAT_DESCRIPTOR, at + 4);
            return NULL;
        }
        if (node_count != 0 && kind != HSD_DAT_REF_LOCAL) {
            fail(import, HSD_DAT_RELOCATION, at + 4);
            return NULL;
        }
        if (node_count != 0 &&
            !import_dynamic_nodes(import, nodes_source, node_count,
                                  &desc->dyn_desc))
        {
            return NULL;
        }
    }
    if (collision_bone_count != 0) {
        output->collision_bones =
            HSD_DatImportAlloc(import, (size_t) collision_bone_count *
                                           sizeof(*output->collision_bones));
        if (output->collision_bones == NULL) {
            import->error_offset = collision_bones_source;
            return NULL;
        }
    }
    for (s32 i = 0; i < collision_bone_count; ++i) {
        ItemCollisionDynamicsDesc* desc = &output->collision_bones[i];
        u32 at = collision_bones_source + (u32) i * 20;
        if (!read_s32(import->dat, at, &desc->bone_id) ||
            !read_f32_words(import->dat, at + 4, &desc->offset,
                            sizeof(desc->offset)) ||
            !HSD_DatReadF32(import->dat, at + 16, &desc->size))
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
    }
    return output;
}

bool itArticle_IsDescriptor(const HSD_Dat* dat, u32 source,
                            bool allow_null_attributes)
{
    u32 fields[6];
    HSD_DatReferenceKind kinds[6];
    if (HSD_DatBytes(dat, source, 24) == NULL) {
        return false;
    }
    for (u32 i = 0; i < 6; ++i) {
        kinds[i] = resolve(dat, source + i * 4, &fields[i]);
        if (kinds[i] == HSD_DAT_REF_INVALID ||
            (i == 0 && kinds[i] != HSD_DAT_REF_LOCAL &&
             !(allow_null_attributes && kinds[i] == HSD_DAT_REF_NULL)))
        {
            return false;
        }
    }
    if (kinds[3] == HSD_DAT_REF_LOCAL) {
        for (u32 i = 0; i < 4; ++i) {
            u32 ignored;
            if (resolve(dat, fields[3] + i * 4, &ignored) ==
                HSD_DAT_REF_INVALID)
            {
                return false;
            }
        }
    }
    if (kinds[4] == HSD_DAT_REF_LOCAL) {
        u32 ignored;
        HSD_DatReferenceKind joint_kind = resolve(dat, fields[4], &ignored);
        if (joint_kind == HSD_DAT_REF_INVALID) {
            return false;
        }
        for (u32 i = 1; i < 4; ++i) {
            HSD_DatReferenceKind kind =
                resolve(dat, fields[4] + i * 4, &ignored);
            if (kind == HSD_DAT_REF_LOCAL || kind == HSD_DAT_REF_EXTERNAL) {
                return false;
            }
        }
    }
    if (kinds[5] == HSD_DAT_REF_LOCAL) {
        u32 count;
        u32 ignored;
        HSD_DatReferenceKind desc_kind = resolve(dat, fields[5] + 4, &ignored);
        if (!HSD_DatReadU32(dat, fields[5], &count) || count > 32 ||
            (count == 0 && desc_kind != HSD_DAT_REF_NULL) ||
            (count != 0 && desc_kind != HSD_DAT_REF_LOCAL))
        {
            return false;
        }
    }
    return true;
}

Article* itArticle_Import(HSD_DatImport* import, u32 source,
                          ItArticleImportStats* stats,
                          ItScriptCollect* scripts, bool allow_null_attributes,
                          s32 item_kind, const u32* sibling_articles,
                          u32 sibling_article_count)
{
    if (!itArticle_IsDescriptor(import->dat, source, allow_null_attributes)) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    Article* output = HSD_DatImportAlloc(import, sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
#ifdef MELEE_NATIVE
    output->native_special_attribute_source = UINT32_MAX;
    u32 attributes_source;
    if (resolve(import->dat, source + 4, &attributes_source) == HSD_DAT_REF_LOCAL)
        output->native_special_attribute_source = attributes_source;
#endif
    u32 ignored;
    if (resolve(import->dat, source, &ignored) == HSD_DAT_REF_LOCAL) {
        output->x0_common_attr = import_item_attributes(import, source);
        if (output->x0_common_attr == NULL) {
            return NULL;
        }
    }
    output->x10_modelDesc = import_model(import, source + 16);
    if (import->status != HSD_DAT_OK) {
        return NULL;
    }
    output->x4_specialAttributes =
        import_special_attributes(import, source + 4, item_kind, output);
    if (import->status != HSD_DAT_OK) {
        return NULL;
    }
    output->x8_hurtbones = import_hurtbones(import, source + 8);
    if (import->status != HSD_DAT_OK) {
        return NULL;
    }
    // ItCo.usd tables end at the next relocation target. Fighter archives can
    // point into their state arrays and may pad the following object, so those
    // retain structural scanning instead of treating that target as an end.
    output->xC_itemStates =
        import_item_states(import, source + 12, stats, scripts, output, item_kind >= 0,
                           sibling_articles, sibling_article_count);
    if (import->status != HSD_DAT_OK) {
        return NULL;
    }
    output->x14_dynamics = import_dynamics(import, source + 20);
    if (import->status != HSD_DAT_OK) {
        return NULL;
    }
    stats->article_count++;
    return output;
}

bool itArticle_ImportTable(HSD_DatImport* import, u32 source, Article** output,
                           u32 count, ItArticleImportStats* stats,
                           ItScriptCollect* scripts,
                           s32 nullable_attribute_index, s32 first_item_kind)
{
    memset(output, 0, (size_t) count * sizeof(*output));
    for (u32 i = 0; i < count; ++i) {
        u32 article_source;
        HSD_DatReferenceKind kind =
            resolve(import->dat, source + i * 4, &article_source);
        if (kind == HSD_DAT_REF_NULL) {
            continue;
        }
        bool allow_null_attributes = (s32) i == nullable_attribute_index;
        if (kind != HSD_DAT_REF_LOCAL ||
            !itArticle_IsDescriptor(import->dat, article_source,
                                    allow_null_attributes))
        {
            return fail(import,
                        kind == HSD_DAT_REF_LOCAL ? HSD_DAT_DESCRIPTOR
                                                  : HSD_DAT_RELOCATION,
                        source + i * 4);
        }
        output[i] = itArticle_Import(import, article_source, stats, scripts,
                                     allow_null_attributes,
                                     first_item_kind + (s32) i, NULL, 0);
        if (output[i] == NULL) {
            return false;
        }
    }
    return true;
}
