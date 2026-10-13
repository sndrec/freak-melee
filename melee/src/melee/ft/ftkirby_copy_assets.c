#include "ftkirby_copy_assets.h"

#include <melee/it/forward.h>
#include <sysdolphin/baselib/dat_import_internal.h>

#include <stdint.h>
#include <string.h>

typedef enum KirbyCopyResourceKind {
    COPY_RESOURCE_SCALAR,
    COPY_RESOURCE_ARTICLE,
    COPY_RESOURCE_DYNAMICS,
    COPY_RESOURCE_JOINT,
    COPY_RESOURCE_ANIM_JOINT,
    COPY_RESOURCE_VISIBILITY,
    COPY_RESOURCE_COLORS,
} KirbyCopyResourceKind;

static bool fail(HSD_DatImport* import, HSD_DatResult status, u32 offset)
{
    import->status = status;
    import->error_offset = offset;
    return false;
}

static HSD_DatReferenceKind resolve(const HSD_Dat* dat, u32 slot,
                                    u32* target)
{
    const char* external;
    return HSD_DatResolve(dat, slot, target, &external);
}

static bool local(const HSD_Dat* dat, u32 slot, u32* target)
{
    return resolve(dat, slot, target) == HSD_DAT_REF_LOCAL;
}

static bool read_s32(const HSD_Dat* dat, u32 source, s32* output)
{
    u32 value;
    if (!HSD_DatReadU32(dat, source, &value)) {
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
    for (size_t offset = 0; offset < size; offset += 4) {
        if (!HSD_DatReadF32(dat, source + (u32) offset,
                            (f32*) (destination + offset)))
        {
            return false;
        }
    }
    return true;
}

static FtPartsVisLookup* import_visibility_lookups(HSD_DatImport* import,
                                                   u32 source,
                                                   u32 source_count,
                                                   u32 native_count)
{
    if (source_count > native_count ||
        HSD_DatBytes(import->dat, source, (size_t) source_count * 8) == NULL)
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    FtPartsVisLookup* lookups = HSD_DatImportAlloc(
        import, (size_t) native_count * sizeof(*lookups));
    if (lookups == NULL) {
        import->error_offset = source;
        return NULL;
    }
    for (u32 model = 0; model < source_count; ++model) {
        s32 variant_count;
        u32 variants_source;
        u32 at = source + model * 8;
        if (!read_s32(import->dat, at, &variant_count) ||
            variant_count < 0 || variant_count > 256)
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
        lookups[model].x0 = variant_count;
        if (variant_count == 0) {
            continue;
        }
        if (!local(import->dat, at + 4, &variants_source) ||
            HSD_DatBytes(import->dat, variants_source,
                         (size_t) variant_count * 8) == NULL)
        {
            fail(import, HSD_DAT_RELOCATION, at + 4);
            return NULL;
        }
        TempS* variants = HSD_DatImportAlloc(
            import, (size_t) variant_count * sizeof(*variants));
        if (variants == NULL) {
            import->error_offset = variants_source;
            return NULL;
        }
        lookups[model].x4 = variants;
        for (s32 variant = 0; variant < variant_count; ++variant) {
            s32 draw_count;
            u32 draws_source;
            u32 entry = variants_source + (u32) variant * 8;
            if (!read_s32(import->dat, entry, &draw_count) ||
                draw_count < 0 || draw_count > 256)
            {
                fail(import, HSD_DAT_DESCRIPTOR, entry);
                return NULL;
            }
            variants[variant].x0 = draw_count;
            if (draw_count == 0) {
                continue;
            }
            if (!local(import->dat, entry + 4, &draws_source)) {
                fail(import, HSD_DAT_RELOCATION, entry + 4);
                return NULL;
            }
            const u8* draws = HSD_DatBytes(import->dat, draws_source,
                                           (size_t) draw_count);
            variants[variant].x4 = HSD_DatImportAlloc(import,
                                                       (size_t) draw_count);
            if (draws == NULL || variants[variant].x4 == NULL) {
                import->error_offset = draws_source;
                return NULL;
            }
            memcpy(variants[variant].x4, draws, (size_t) draw_count);
        }
    }
    return lookups;
}

static bool import_visibility(HSD_DatImport* import, u32 source,
                              u32 model_count, u32 costume_count,
                              void* (**result)[4])
{
    void*(*table)[4] = HSD_DatImportAlloc(
        import, (size_t) costume_count * sizeof(*table));
    if (table == NULL) {
        import->error_offset = source;
        return false;
    }
    for (u32 costume = 0; costume < costume_count; ++costume) {
        for (u32 group = 0; group < 4; ++group) {
            u32 lookup_source;
            u32 slot = source + (costume * 4 + group) * 4;
            HSD_DatReferenceKind kind = resolve(import->dat, slot,
                                                &lookup_source);
            if (kind == HSD_DAT_REF_NULL) {
                continue;
            }
            if (kind != HSD_DAT_REF_LOCAL) {
                return fail(import, HSD_DAT_RELOCATION, slot);
            }
            table[costume][group] = import_visibility_lookups(
                import, lookup_source, model_count, model_count);
            if (table[costume][group] == NULL) {
                return false;
            }
        }
    }
    *result = table;
    return true;
}

static bool import_parts(HSD_DatImport* import, u32 count_slot,
                         u32 visibility_slot, u32 costume_count,
                         FtPartsDesc* output)
{
    u32 visibility_source;
    return HSD_DatReadU32(import->dat, count_slot, &output->model_num) &&
           output->model_num > 0 && output->model_num <= 11 &&
           local(import->dat, visibility_slot, &visibility_source) &&
           import_visibility(import, visibility_source, output->model_num,
                             costume_count, &output->vis_table);
}

static bool import_texture_indices(HSD_DatImport* import, u32 source,
                                   u32 texture_count, u32 costume_count,
                                   u16*** result)
{
    u16** table = HSD_DatImportAlloc(
        import, (size_t) costume_count * sizeof(*table));
    if (table == NULL) {
        import->error_offset = source;
        return false;
    }
    for (u32 costume = 0; costume < costume_count; ++costume) {
        u32 indices_source;
        HSD_DatReferenceKind kind = resolve(
            import->dat, source + costume * 4, &indices_source);
        if (kind == HSD_DAT_REF_NULL) {
            continue;
        }
        if (kind != HSD_DAT_REF_LOCAL ||
            HSD_DatBytes(import->dat, indices_source,
                         (size_t) texture_count * 2) == NULL)
        {
            return fail(import, HSD_DAT_RELOCATION, source + costume * 4);
        }
        table[costume] = HSD_DatImportAlloc(
            import, (size_t) texture_count * sizeof(*table[costume]));
        if (table[costume] == NULL) {
            import->error_offset = indices_source;
            return false;
        }
        for (u32 texture = 0; texture < texture_count; ++texture) {
            if (!HSD_DatReadU16(import->dat,
                                indices_source + texture * 2,
                                &table[costume][texture]))
            {
                return fail(import, HSD_DAT_DESCRIPTOR, indices_source);
            }
        }
    }
    *result = table;
    return true;
}

static bool import_dynamic_nodes(HSD_DatImport* import, u32 source,
                                 u32 count, DynamicsDesc* output)
{
    struct lb_00F9_UnkDesc1Inner* nodes = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*nodes));
    if (nodes == NULL ||
        HSD_DatBytes(import->dat, source,
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

static ftDynamics* import_dynamics(HSD_DatImport* import, u32 source)
{
    s32 dynamics_count;
    s32 collision_count;
    u32 bones_source;
    u32 collision_source;
    if (!read_s32(import->dat, source, &dynamics_count) ||
        dynamics_count < 0 || dynamics_count > Ft_Dynamics_NumMax ||
        !read_s32(import->dat, source + 8, &collision_count) ||
        collision_count < 0 || collision_count > 11)
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    ftDynamics* output = HSD_DatImportAlloc(import, sizeof(*output));
    ArticleDynamicBones* bones = HSD_DatImportAlloc(import, sizeof(*bones));
    if (output == NULL || bones == NULL) {
        import->error_offset = source;
        return NULL;
    }
    output->dynamicsNum = dynamics_count;
    output->ftDynamicBones = bones;
    output->x4 = collision_count;
    if (collision_count != 0) {
        if (!local(import->dat, source + 12, &collision_source) ||
            HSD_DatBytes(import->dat, collision_source,
                         (size_t) collision_count * 20) == NULL)
        {
            fail(import, HSD_DAT_RELOCATION, source + 12);
            return NULL;
        }
        output->x8 = HSD_DatImportAlloc(
            import, (size_t) collision_count * sizeof(*output->x8));
        if (output->x8 == NULL) {
            import->error_offset = collision_source;
            return NULL;
        }
        for (s32 i = 0; i < collision_count; ++i) {
            u32 at = collision_source + (u32) i * 20;
            s32 bone_id;
            if (!read_s32(import->dat, at, &bone_id) ||
                !read_f32_words(import->dat, at + 4, &output->x8[i].x4, 16))
            {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                return NULL;
            }
            output->x8[i].x0 = bone_id;
        }
    }
    if (dynamics_count == 0) {
        return output;
    }
    if (!local(import->dat, source + 4, &bones_source) ||
        HSD_DatBytes(import->dat, bones_source,
                     (size_t) dynamics_count * 24) == NULL)
    {
        fail(import, HSD_DAT_RELOCATION, source + 4);
        return NULL;
    }
    for (s32 i = 0; i < dynamics_count; ++i) {
        BoneDynamicsDesc* bone = &bones->array[i];
        u32 at = bones_source + (u32) i * 24;
        u32 node_count;
        u32 nodes_source;
        s32 bone_id;
        if (!read_s32(import->dat, at, &bone_id) ||
            !HSD_DatReadU32(import->dat, at + 8, &node_count) ||
            node_count > 256 ||
            !read_f32_words(import->dat, at + 12, &bone->dyn_desc.pos,
                            sizeof(bone->dyn_desc.pos)))
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
        bone->bone_id = (Fighter_Part) bone_id;
        bone->dyn_desc.count = node_count;
        HSD_DatReferenceKind kind = resolve(import->dat, at + 4,
                                            &nodes_source);
        if (node_count == 0) {
            if (kind != HSD_DAT_REF_NULL) {
                fail(import, HSD_DAT_DESCRIPTOR, at + 4);
                return NULL;
            }
        } else if (kind != HSD_DAT_REF_LOCAL ||
                   !import_dynamic_nodes(import, nodes_source, node_count,
                                         &bone->dyn_desc))
        {
            return NULL;
        }
    }
    return output;
}

static KirbyCopyResourceKind resource_kind(FighterKind kind, u32 index,
                                           s32* item_kind)
{
    *item_kind = -1;
#define ARTICLE(i, value)                                                     \
    if (index == (i)) {                                                       \
        *item_kind = (value);                                                 \
        return COPY_RESOURCE_ARTICLE;                                         \
    }
#define RESOURCE(i, value)                                                    \
    if (index == (i)) {                                                       \
        return (value);                                                       \
    }
    switch (kind) {
    case Ft_Kind_Mario:
        ARTICLE(0, It_Kind_Kirby_MarioFire);
        break;
    case Ft_Kind_Fox:
        ARTICLE(0, It_Kind_Kirby_FoxLaser);
        ARTICLE(1, It_Kind_Kirby_FoxBlaster);
        break;
    case Ft_Kind_Donkey:
        RESOURCE(2, COPY_RESOURCE_JOINT);
        break;
    case Ft_Kind_Koopa:
        ARTICLE(0, It_Kind_Kirby_KoopaFlame);
        RESOURCE(1, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_Link:
        ARTICLE(0, It_Kind_Kirby_LinkArrow);
        ARTICLE(1, It_Kind_Kirby_LinkBow);
        RESOURCE(2, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_Seak:
        ARTICLE(0, It_Kind_Kirby_SeakNeedleThrow);
        ARTICLE(1, It_Kind_Kirby_SeakNeedleHeld);
        RESOURCE(2, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_Ness:
        ARTICLE(0, It_Kind_Kirby_NessPKFlush);
        ARTICLE(1, It_Kind_Kirby_NessPKFlush_Explode);
        break;
    case Ft_Kind_Peach:
        ARTICLE(0, It_Kind_Kirby_PeachToad);
        ARTICLE(1, It_Kind_Kirby_PeachToadSpore);
        break;
    case Ft_Kind_Popo:
        ARTICLE(0, It_Kind_Kirby_IceClimberIce);
        RESOURCE(1, COPY_RESOURCE_JOINT);
        break;
    case Ft_Kind_Pikachu:
        ARTICLE(0, It_Kind_Kirby_PikachuTJolt_Ground);
        ARTICLE(1, It_Kind_Kirby_PikachuTJolt_Air);
        RESOURCE(2, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_Samus:
        ARTICLE(0, It_Kind_Kirby_SamusCharge);
        break;
    case Ft_Kind_Yoshi:
        RESOURCE(0, COPY_RESOURCE_JOINT);
        if (index >= 1 && index <= 4) {
            return COPY_RESOURCE_ANIM_JOINT;
        }
        ARTICLE(5, It_Kind_Kirby_YoshiEggLay);
        break;
    case Ft_Kind_Purin:
        RESOURCE(2, COPY_RESOURCE_JOINT);
        RESOURCE(3, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_Mewtwo:
        RESOURCE(2, COPY_RESOURCE_JOINT);
        ARTICLE(3, It_Kind_Kirby_MewtwoShadowBall);
        RESOURCE(4, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_Luigi:
        ARTICLE(0, It_Kind_Kirby_LuigiFire);
        break;
    case Ft_Kind_Mars:
        RESOURCE(0, COPY_RESOURCE_JOINT);
        RESOURCE(1, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_Zelda:
        RESOURCE(0, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_CLink:
        ARTICLE(0, It_Kind_Kirby_CLinkArrow);
        ARTICLE(1, It_Kind_Kirby_CLinkBow);
        RESOURCE(2, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_DrMario:
        ARTICLE(0, It_Kind_Kirby_DrMarioVitamin);
        break;
    case Ft_Kind_Falco:
        RESOURCE(2, COPY_RESOURCE_JOINT);
        ARTICLE(3, It_Kind_Kirby_FalcoLaser);
        ARTICLE(4, It_Kind_Kirby_FalcoBlaster);
        break;
    case Ft_Kind_Pichu:
        ARTICLE(0, It_Kind_Kirby_PichuTJolt_Ground);
        ARTICLE(1, It_Kind_Kirby_PichuTJolt_Air);
        RESOURCE(2, COPY_RESOURCE_DYNAMICS);
        break;
    case Ft_Kind_GameWatch:
        RESOURCE(3, COPY_RESOURCE_VISIBILITY);
        RESOURCE(4, COPY_RESOURCE_COLORS);
        ARTICLE(5, It_Kind_Kirby_GameWatchChef);
        ARTICLE(6, It_Kind_Kirby_GameWatchChefPan);
        break;
    case Ft_Kind_Emblem:
        RESOURCE(0, COPY_RESOURCE_JOINT);
        RESOURCE(1, COPY_RESOURCE_DYNAMICS);
        break;
    default:
        break;
    }
#undef RESOURCE
#undef ARTICLE
    return COPY_RESOURCE_SCALAR;
}

static bool uses_merged_model(FighterKind kind)
{
    return kind == Ft_Kind_Donkey || kind == Ft_Kind_Purin ||
           kind == Ft_Kind_Mewtwo || kind == Ft_Kind_Falco ||
           kind == Ft_Kind_GameWatch;
}

static bool import_resource(HSD_DatImport* import, FighterKind kind,
                            u32 root, u32 index,
                            const u32* article_sources,
                            u32 article_source_count,
                            ItArticleImportStats* article_stats,
                            ItScriptCollect* scripts, KirbyCopyData* output)
{
    u32 slot = root + 12 + index * 4;
    u32 source;
    s32 item_kind;
    KirbyCopyResourceKind type = resource_kind(kind, index, &item_kind);
    HSD_DatReferenceKind reference = resolve(import->dat, slot, &source);
    if (reference == HSD_DAT_REF_NULL) {
        return true;
    }
    if (reference != HSD_DAT_REF_LOCAL) {
        if (type == COPY_RESOURCE_SCALAR) {
            u32 value;
            if (!HSD_DatReadU32(import->dat, slot, &value)) {
                return fail(import, HSD_DAT_DESCRIPTOR, slot);
            }
            output->resources[index].scalar = value;
            return true;
        }
        return fail(import, HSD_DAT_RELOCATION, slot);
    }
    switch (type) {
    case COPY_RESOURCE_ARTICLE:
        output->resources[index].ptr = itArticle_Import(
            import, source, article_stats, scripts, false, item_kind,
            article_sources, article_source_count);
        break;
    case COPY_RESOURCE_DYNAMICS:
        output->resources[index].dynamics = import_dynamics(import, source);
        break;
    case COPY_RESOURCE_JOINT:
        output->resources[index].joint = HSD_DatImportGraph(import, source,
                                                            DAT_JOINT);
        break;
    case COPY_RESOURCE_ANIM_JOINT:
        output->resources[index].animation = HSD_DatImportGraph(
            import, source, DAT_ANIMJOINT);
        break;
    case COPY_RESOURCE_VISIBILITY:
        // Game & Watch's copy lookup contains one serialized entry but is also
        // installed on Kirby's two-model body visibility state. On GameCube,
        // the following relocated pointer has its high bit set and therefore
        // acts as a negative, empty variant count when read as the second
        // entry. Preserve that effective layout explicitly on 64-bit hosts.
        output->resources[index].ptr = import_visibility_lookups(
            import, source, 1, 2);
        break;
    case COPY_RESOURCE_COLORS: {
        KirbyCopyColorData* colors = HSD_DatImportAlloc(
            import, sizeof(*colors));
        if (colors == NULL ||
            !HSD_DatReadU32(import->dat, source, &colors->material_color) ||
            !HSD_DatReadU32(import->dat, source + 4, &colors->item_color) ||
            !HSD_DatReadU32(import->dat, source + 8,
                            &colors->outline_color))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        output->resources[index].colors = colors;
        break;
    }
    case COPY_RESOURCE_SCALAR:
        return fail(import, HSD_DAT_DESCRIPTOR, slot);
    }
    return output->resources[index].ptr != NULL;
}

bool ftKirbyCopyAssets_Import(const void* bytes, size_t size,
                              FighterKind copied_kind,
                              const char* root_name, void* arena,
                              size_t arena_size, u32 record_capacity,
                              FtKirbyCopyAssets* output)
{
    static const u8 root_sizes[Ft_Kind_RuntimeCapacity] = {
        [Ft_Kind_Mario] = 20,     [Ft_Kind_Fox] = 32,
        [Ft_Kind_Captain] = 32,   [Ft_Kind_Donkey] = 24,
        [Ft_Kind_Koopa] = 32,     [Ft_Kind_Link] = 44,
        [Ft_Kind_Seak] = 48,      [Ft_Kind_Ness] = 40,
        [Ft_Kind_Peach] = 32,     [Ft_Kind_Popo] = 20,
        [Ft_Kind_Pikachu] = 40,   [Ft_Kind_Samus] = 32,
        [Ft_Kind_Yoshi] = 44,     [Ft_Kind_Purin] = 56,
        [Ft_Kind_Mewtwo] = 48,    [Ft_Kind_Luigi] = 24,
        [Ft_Kind_Mars] = 36,      [Ft_Kind_Zelda] = 36,
        [Ft_Kind_CLink] = 44,     [Ft_Kind_DrMario] = 40,
        [Ft_Kind_Falco] = 36,     [Ft_Kind_Pichu] = 52,
        [Ft_Kind_GameWatch] = 44, [Ft_Kind_Ganon] = 36,
        [Ft_Kind_Emblem] = 28,
    };
    ItScriptEntry script_entries[128];
    ItScriptCollect scripts = {
        script_entries, 0, ARRAY_SIZE(script_entries)
    };
    u32 root;
    if (output == NULL || copied_kind >= Ft_Kind_RuntimeCapacity ||
        root_sizes[copied_kind] == 0 || root_name == NULL)
    {
        return false;
    }
    memset(output, 0, sizeof(*output));
    if (HSD_DatOpen(&output->dat, bytes, size) != HSD_DAT_OK ||
        HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&output->dat, root_name, &root) ||
        HSD_DatNextTarget(&output->dat, root) - root !=
            root_sizes[copied_kind])
    {
        return false;
    }

    bool merged = uses_merged_model(copied_kind);
    if (merged) {
        u32 texture_source;
        if (!import_parts(&output->import, root, root + 4, 6,
                          &output->data.parts) ||
            !HSD_DatReadU32(&output->dat, root + 8,
                            &output->data.part_animation.x8) ||
            output->data.part_animation.x8 > 5 ||
            !local(&output->dat, root + 12, &texture_source) ||
            !import_texture_indices(
                &output->import, texture_source,
                output->data.part_animation.x8, 6,
                &output->data.part_animation.xC))
        {
            return false;
        }
        output->data.part_animation.costume_count = 6;
        output->data.resources[0].ptr = output->data.part_animation.xC;
    } else {
        u32 joint_source;
        if (!local(&output->dat, root, &joint_source) ||
            (output->data.hat_joint = HSD_DatImportGraph(
                 &output->import, joint_source, DAT_JOINT)) == NULL ||
            !import_parts(&output->import, root + 4, root + 8, 1,
                          &output->data.parts))
        {
            return false;
        }
    }

    u32 article_sources[8];
    u32 article_source_count = 0;
    u32 resource_count = (root_sizes[copied_kind] - 12) / 4;
    for (u32 index = 0; index < resource_count; ++index) {
        s32 item_kind;
        if (resource_kind(copied_kind, index, &item_kind) ==
            COPY_RESOURCE_ARTICLE)
        {
            u32 article_source;
            if (!local(&output->dat, root + 12 + index * 4,
                       &article_source) ||
                !itArticle_IsDescriptor(&output->dat, article_source, false) ||
                article_source_count == ARRAY_SIZE(article_sources))
            {
                return fail(&output->import, HSD_DAT_DESCRIPTOR,
                            root + 12 + index * 4);
            }
            article_sources[article_source_count++] = article_source;
        }
    }
    for (u32 index = merged ? 1 : 0; index < resource_count; ++index) {
        if (!import_resource(&output->import, copied_kind, root, index,
                             article_sources, article_source_count,
                             &output->article_stats, &scripts,
                             &output->data))
        {
            return false;
        }
    }
    return output->import.status == HSD_DAT_OK &&
           itCommandData_Import(&output->import, &scripts,
                                &output->item_command_data);
}
