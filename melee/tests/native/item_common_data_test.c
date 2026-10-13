#include <stdint.h>

#include <melee/it/itcommon_data.h>
#include <melee/it/itCommonItems.h>
#include <melee/it/kinds/itkinoko.h>
#include <melee/lb/types.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

static u32 require_local_reference(const HSD_Dat* dat, u32 slot,
                                   const char* message)
{
    u32 target;
    const char* external;
    require(HSD_DatResolve(dat, slot, &target, &external) == HSD_DAT_REF_LOCAL,
            message);
    return target;
}

static u32 verify_item_attribute_bits(const HSD_Dat* dat, u32 table_source,
                                      Article* const* articles, u32 count)
{
    u32 verified = 0;
    for (u32 i = 0; i < count; ++i) {
        Article* article = articles[i];
        if (article == NULL) {
            continue;
        }

        u32 article_source = require_local_reference(
            dat, table_source + i * 4, "resolve item article descriptor\n");
        u32 attributes_source;
        const char* external;
        HSD_DatReferenceKind kind = HSD_DatResolve(
            dat, article_source, &attributes_source, &external);
        if (kind == HSD_DAT_REF_NULL) {
            require(article->x0_common_attr == NULL,
                    "preserve null item common attributes\n");
            continue;
        }
        require(kind == HSD_DAT_REF_LOCAL && article->x0_common_attr != NULL,
                "resolve item common attributes\n");
        const u8* source = HSD_DatBytes(dat, attributes_source, 4);
        ItemAttr* attributes = article->x0_common_attr;
        require(source != NULL &&
                    attributes->x0_is_heavy == ((source[0] >> 7) & 1) &&
                    attributes->use_kind == ((source[0] >> 3) & 0xF) &&
                    attributes->x0_hold_kind == (source[0] & 7) &&
                    attributes->x1_1 == ((source[1] >> 6) & 3) &&
                    attributes->x1_3 == ((source[1] >> 5) & 1) &&
                    attributes->x1_4 == ((source[1] >> 4) & 1) &&
                    attributes->x1_5 == ((source[1] >> 3) & 1) &&
                    attributes->x1_67_cam_kind == ((source[1] >> 1) & 3) &&
                    attributes->x1_8 == (source[1] & 1) &&
                    attributes->x3 == source[3],
                "decoded item common attribute bitfields\n");
        ++verified;
    }
    return verified;
}

int main(int argc, char** argv)
{
    DVDFileInfo file;
    ItCommonAssets assets;
    void* arena;
    void* bytes;
    u32 size;

    require(argc == 2, "usage: native_item_common_data_test <iso>\n");
    require(aurora_dvd_open(argv[1]), "open ISO\n");
    require(DVDOpen("/ItCo.usd", &file), "open ItCo.usd\n");
    size = file.length;
    bytes = _aligned_malloc((size + 31) & ~(size_t) 31, 32);
    arena = _aligned_malloc(IT_COMMON_IMPORT_ARENA_SIZE, 32);
    require(bytes != NULL && arena != NULL, "allocate item bank\n");
    require(DVDReadPrio(&file, bytes, (size + 31) & ~(size_t) 31, 0, 2) >=
                (s32) size && DVDClose(&file),
            "read item bank\n");
    if (!itCommonAssets_Import(bytes, size, arena,
                               IT_COMMON_IMPORT_ARENA_SIZE,
                               IT_COMMON_IMPORT_RECORD_CAPACITY, &assets))
    {
        fprintf(stderr,
                "import item common data: status=%d offset=0x%lX records=%lu "
                "arena=%zu articles=%lu states=%lu scripts=%lu external=%s\n",
                assets.import.status, assets.import.error_offset,
                assets.import.record_count, assets.import.arena_used,
                assets.article_stats.article_count,
                assets.article_stats.state_count,
                assets.article_stats.script_count,
                assets.import.external_symbol != NULL
                    ? assets.import.external_symbol
                    : "none");
        return 1;
    }
    require(assets.public_data.x0 == &assets.common_data &&
                assets.public_data.x4 == assets.common_articles &&
                assets.public_data.x8 == assets.character_articles &&
                assets.public_data.xC == assets.pokemon_articles &&
                assets.public_data.x14 == assets.color_entries &&
                assets.common_data.x0 == 40 &&
                assets.common_data.x30_lifetime == 1400 &&
                assets.common_data.x48_byte == 0x44 &&
                assets.common_data.x54_float == 0.3F &&
                assets.common_data.x68_float == 15.0F &&
                assets.zako_data.x0 == 5 && assets.zako_data.x4 == 5.0F &&
                assets.article_stats.article_count == 98 &&
                assets.article_stats.state_count == 262 &&
                assets.article_stats.script_count == 162 &&
                assets.command_data.entry_count == 162 &&
                assets.command_data.instruction_count == 757 &&
                assets.color_entries[0].unk == NULL &&
                assets.color_entries[1].unk != NULL &&
                assets.color_entries[1].unk4 == 30 &&
                assets.color_entries[1].unk5 == 0 &&
                assets.color_command_count == 77 &&
                assets.import.unresolved_external_count == 9,
            "decoded item common values\n");

    u32 public_root;
    require(HSD_DatFindRoot(&assets.dat, "itPublicData", &public_root),
            "find item public root\n");
    u32 common_table = require_local_reference(
        &assets.dat, public_root + 4, "resolve common item table\n");
    u32 character_table = require_local_reference(
        &assets.dat, public_root + 8, "resolve character item table\n");
    u32 pokemon_table = require_local_reference(
        &assets.dat, public_root + 12, "resolve Pokemon item table\n");
    u32 attribute_count = verify_item_attribute_bits(
        &assets.dat, common_table, assets.common_articles,
        IT_COMMON_ARTICLE_COUNT);
    attribute_count += verify_item_attribute_bits(
        &assets.dat, character_table, assets.character_articles,
        IT_CHARACTER_ARTICLE_COUNT);
    attribute_count += verify_item_attribute_bits(
        &assets.dat, pokemon_table, assets.pokemon_articles,
        IT_POKEMON_ARTICLE_COUNT);
    require(attribute_count == 97,
            "decoded complete item common attribute registry\n");
    require(assets.common_articles[It_Kind_MSBomb]->x0_common_attr->use_kind ==
                0 &&
                assets.common_articles[It_Kind_Bat]->x0_common_attr->use_kind ==
                    2 &&
                assets.common_articles[It_Kind_Sword]->x0_common_attr->use_kind ==
                    2 &&
                assets.common_articles[It_Kind_Parasol]
                        ->x0_common_attr->use_kind == 2 &&
                assets.common_articles[It_Kind_L_Gun]->x0_common_attr->use_kind ==
                    3 &&
                assets.common_articles[It_Kind_S_Scope]
                        ->x0_common_attr->use_kind == 3 &&
                assets.common_articles[It_Kind_F_Flower]
                        ->x0_common_attr->use_kind == 3,
            "decoded throwable, swingable, and shootable item actions\n");

    require(assets.character_articles[It_PKind_Random -
                                      It_Kind_Kuriboh] != NULL,
            "random stage-item article\n");
    u32 character_article_count = 0;
    for (u32 i = 0; i < IT_CHARACTER_ARTICLE_COUNT; ++i) {
        bool expected =
            i <= 4 || i >= It_Kind_Unk4 - It_Kind_Kuriboh;
        require((assets.character_articles[i] != NULL) == expected,
                "decoded complete character article registry\n");
        character_article_count += assets.character_articles[i] != NULL;
    }
    require(character_article_count == 8,
            "character article registry coverage\n");

    it_2E5A_Attrs* it_2e5a_attributes =
        assets.character_articles[It_Kind_Unk4 - It_Kind_Kuriboh]
            ->x4_specialAttributes;
    require(it_2e5a_attributes->x0 == 280.0F &&
                it_2e5a_attributes->tiers[0].xD84_value == 149 &&
                it_2e5a_attributes->tiers[0].threshold == 1 &&
                it_2e5a_attributes->tiers[0].scale == 1.7F &&
                it_2e5a_attributes->tiers[1].xD84_value == 148 &&
                it_2e5a_attributes->tiers[1].threshold == 5 &&
                it_2e5a_attributes->tiers[2].xD84_value == 146 &&
                it_2e5a_attributes->tiers[2].threshold == 10,
            "decoded it_2E5A tier attributes\n");
    for (u32 tier = 0; tier < ARRAY_SIZE(it_2e5a_attributes->tiers); ++tier) {
        require(it_2e5a_attributes->tiers[tier].joint != NULL &&
                    it_2e5a_attributes->tiers[tier].anim_joint != NULL &&
                    it_2e5a_attributes->tiers[tier].matanim_joint != NULL &&
                    it_2e5a_attributes->tiers[tier].shape_anim_joint == NULL,
                "decoded it_2E5A tier graphs\n");
    }

    itOldkuriAttributes* goomba_attributes =
        assets.character_articles[It_Kind_Kuriboh - It_Kind_Kuriboh]
            ->x4_specialAttributes;
    itLeadeadAttributes* redead_attributes =
        assets.character_articles[It_Kind_Leadead - It_Kind_Kuriboh]
            ->x4_specialAttributes;
    itOctarockAttributes* octorok_attributes =
        assets.character_articles[It_Kind_Octarock - It_Kind_Kuriboh]
            ->x4_specialAttributes;
    itOldottoseaAttributes* ottosea_attributes =
        assets.character_articles[It_Kind_Ottosea - It_Kind_Kuriboh]
            ->x4_specialAttributes;
    require(goomba_attributes->common->damage_threshold == 2 &&
                goomba_attributes->common->movement_speed == 0.15F &&
                redead_attributes->common->damage_threshold == 25 &&
                redead_attributes->common->movement_speed == 0.08F &&
                octorok_attributes->common->damage_threshold == 10 &&
                octorok_attributes->common->movement_speed == 0.0F &&
                ottosea_attributes->common->damage_threshold == 10 &&
                ottosea_attributes->common->movement_speed == 0.3F &&
                redead_attributes->x18 == 60 && redead_attributes->x1A == 10 &&
                redead_attributes->x1C == 120 && redead_attributes->x1E == 3 &&
                octorok_attributes->x1C == 180 &&
                ottosea_attributes->x10 == 4 &&
                ottosea_attributes->x28 == 4,
            "decoded monster common attributes\n");

    it_2728_DatAttrs* star_attributes =
        assets.common_articles[It_Kind_Star - It_Common_Start]
            ->x4_specialAttributes;
    require(star_attributes != NULL &&
                star_attributes->invincibility_duration == 600,
            "decoded Starman invincibility duration\n");

    itFoodsAttributes* food_attributes =
        assets.common_articles[It_Kind_Foods - It_Common_Start]
            ->x4_specialAttributes;
    require(food_attributes != NULL && food_attributes->variant_count == 28,
            "decoded food variant count\n");
    for (s32 i = 0; i < food_attributes->variant_count; ++i) {
        itFoodVariant* variant = &food_attributes->variants[i];
        require(variant->joint != NULL && variant->heal_amount > 0 &&
                    isfinite(variant->offset_x) && isfinite(variant->offset_y),
                "decoded food variant\n");
    }

    for (ItemKind kind = It_Kind_Kinoko; kind <= It_Kind_DKinoko; ++kind) {
        KinokoAttrs* attributes =
            assets.common_articles[kind - It_Common_Start]
                ->x4_specialAttributes;
        require(attributes != NULL && isfinite(attributes->ground_speed) &&
                    attributes->ground_speed > 0.0F &&
                    isfinite(attributes->deceleration) &&
                    attributes->deceleration > 0.0F &&
                    attributes->animation_joints[0] != NULL &&
                    attributes->animation_joints[1] != NULL,
                "decoded mushroom attributes\n");
    }

    itWstarAttributes* warp_star_attributes =
        assets.common_articles[It_Kind_WStar - It_Common_Start]
            ->x4_specialAttributes;
    require(warp_star_attributes != NULL &&
                warp_star_attributes->entry_count == 7,
            "decoded Warp Star animation count\n");
    for (s32 i = 0; i < warp_star_attributes->entry_count; ++i) {
        require(warp_star_attributes->entries[i].anim_joint != NULL,
                "decoded Warp Star animation\n");
    }

    const ItemKind unown_kinds[] = { It_PKind_Unknown,
                                     It_Kind_Unknown_Swarm };
    for (u32 kind_index = 0; kind_index < ARRAY_SIZE(unown_kinds);
         ++kind_index)
    {
        ItemKind kind = unown_kinds[kind_index];
        itUnknownAttributes* attributes =
            assets.pokemon_articles[kind - It_PKind_Start]
                ->x4_specialAttributes;
        require(attributes != NULL, "decoded Unown attributes\n");
        for (u32 joint = 0; joint < ARRAY_SIZE(attributes->x24); ++joint) {
            require(attributes->x24[joint] != NULL,
                    "decoded Unown model variant\n");
        }
    }

    Article** tables[] = { assets.common_articles,
                           assets.character_articles,
                           assets.pokemon_articles };
    const u32 counts[] = { IT_COMMON_ARTICLE_COUNT,
                           IT_CHARACTER_ARTICLE_COUNT,
                           IT_POKEMON_ARTICLE_COUNT };
    u32 state_count = 0;
    u32 script_count = 0;
    u32 flexible_bone_count = 0;
    u32 collision_bone_count = 0;
    for (u32 table = 0; table < 3; ++table) {
        for (u32 i = 0; i < counts[table]; ++i) {
            Article* article = tables[table][i];
            if (article == NULL) {
                continue;
            }
            bool random_stage_item =
                table == 1 &&
                i == It_PKind_Random - It_Kind_Kuriboh;
            require((random_stage_item && article->x0_common_attr == NULL) ||
                        (article->x0_common_attr != NULL &&
                         isfinite(article->x0_common_attr->x60_scale)),
                    "decoded item article\n");
            if (article->x14_dynamics != NULL) {
                ItemDynamics* dynamics = article->x14_dynamics;
                require(dynamics->flexible_bone_count >= 0 &&
                            dynamics->flexible_bone_count <= 32 &&
                            dynamics->collision_bone_count >= 0 &&
                            dynamics->collision_bone_count <= 2 &&
                            (dynamics->flexible_bone_count == 0 ||
                             dynamics->flexible_bones != NULL) &&
                            (dynamics->collision_bone_count == 0 ||
                             dynamics->collision_bones != NULL),
                        "decoded item dynamics header\n");
                for (s32 bone = 0; bone < dynamics->flexible_bone_count;
                     ++bone)
                {
                    BoneDynamicsDesc* desc = &dynamics->flexible_bones[bone];
                    require(desc->bone_id >= 0 &&
                                (u32) desc->bone_id <
                                    article->x10_modelDesc->x4_bone_count &&
                                desc->dyn_desc.count <= 256 &&
                                (desc->dyn_desc.count == 0 ||
                                 desc->dyn_desc.data != NULL) &&
                                isfinite(desc->dyn_desc.pos.x) &&
                                isfinite(desc->dyn_desc.pos.y) &&
                                isfinite(desc->dyn_desc.pos.z),
                            "decoded flexible item bone\n");
                }
                for (s32 bone = 0; bone < dynamics->collision_bone_count;
                     ++bone)
                {
                    ItemCollisionDynamicsDesc* desc =
                        &dynamics->collision_bones[bone];
                    require(desc->bone_id >= 0 &&
                                (u32) desc->bone_id <
                                    article->x10_modelDesc->x4_bone_count &&
                                isfinite(desc->offset.x) &&
                                isfinite(desc->offset.y) &&
                                isfinite(desc->offset.z) &&
                                isfinite(desc->size) && desc->size >= 0.0F,
                            "decoded collision item bone\n");
                }
                flexible_bone_count += dynamics->flexible_bone_count;
                collision_bone_count += dynamics->collision_bone_count;
            }
            if (article->xC_itemStates == NULL) {
                continue;
            }
            require(article->xC_itemStates->count > 0,
                    "decoded item state table\n");
            for (u32 state = 0; state < article->xC_itemStates->count;
                 ++state)
            {
                ItemStateDesc* desc =
                    &article->xC_itemStates->x0_itemStateDesc[state];
                if ((desc->x0_anim_joint != NULL &&
                     (uintptr_t) desc->x0_anim_joint < 4096) ||
                    (desc->x4_matanim_joint != NULL &&
                     (uintptr_t) desc->x4_matanim_joint < 4096) ||
                    (desc->x8_parameters != NULL &&
                     (uintptr_t) desc->x8_parameters < 4096))
                {
                    fprintf(stderr,
                            "invalid state graph table=%lu article=%lu "
                            "state=%lu anim=%p mat=%p shape=%p\n",
                            table, i, state, (void*) desc->x0_anim_joint,
                            (void*) desc->x4_matanim_joint,
                            (void*) desc->x8_parameters);
                    return 1;
                }
                if (article->xC_itemStates->x0_itemStateDesc[state]
                        .xC_script != NULL)
                    ++script_count;
            }
            state_count += article->xC_itemStates->count;
        }
    }
    require(state_count == assets.article_stats.state_count,
            "decoded item state coverage\n");
    require(script_count == 162,
            "converted common item script pointers\n");
    require(flexible_bone_count == 4 && collision_bone_count == 1,
            "decoded complete item dynamics coverage\n");
    // These offsets are reachable instructions in the retail ItCo.usd graph.
    // Check fields consumed by the native item handlers, not just graph size.
    union CmdUnion* gfx = &assets.command_data.commands[
        (0xACC0 - assets.command_data.source_begin) / 4];
    require(gfx->it_spawn_gfx_0.opcode == 10 &&
                gfx->it_spawn_gfx_0.arg == 0 &&
                ((u16*) (gfx + 1))[0] == 0x040F,
            "item GFX command fields and halfword payload\n");
    union CmdUnion* hitbox = &assets.command_data.commands[
        (0xAD6C - assets.command_data.source_begin) / 4];
    u32 source_fourth;
    require(HSD_DatReadU32(&assets.dat, 0xAD7C, &source_fourth) &&
                source_fourth == 0x0C840023 &&
                hitbox->it_create_hitbox_0.opcode == 11 &&
                hitbox->it_create_hitbox_0.damage == 1 &&
                (hitbox + 4)->it_create_hitbox_4.base_knockback ==
                    ((source_fourth >> 23) & 0x1FF) &&
                (hitbox + 4)->it_create_hitbox_4.element ==
                    ((source_fourth >> 18) & 0x1F) &&
                (hitbox + 4)->it_create_hitbox_4.shield_damage ==
                    (int8_t) ((source_fourth >> 9) & 0xFF) &&
                memcmp(hitbox + 5, HSD_DatBytes(&assets.dat, 0xAD80, 4),
                       4) == 0,
            "item hitbox fields and byte payload\n");
    union CmdUnion* branch = &assets.command_data.commands[
        (0xA7B8 - assets.command_data.source_begin) / 4];
    s32 relative;
    memcpy(&relative, branch + 1, sizeof(relative));
    require(branch->Command_09.id == 5 &&
                relative == (s32) (0xA6FC - 0xA7BC),
            "item subroutine relocation\n");
    union CmdUnion* extended = &assets.command_data.commands[
        (0xAA90 - assets.command_data.source_begin) / 4];
    require(extended->unk0.opcode == 16 &&
                (((u16*) extended)[0] >> 6 & 0xFF) == 0 &&
                memcmp(extended + 2,
                       HSD_DatBytes(&assets.dat, 0xAA98, 4), 4) == 0,
            "item extended command header and byte payload\n");
    printf("articles=%lu states=%lu scripts=%lu flexible_bones=%lu "
           "collision_bones=%lu records=%lu arena=%zu externals=%lu\n",
           assets.article_stats.article_count,
           assets.article_stats.state_count,
           assets.article_stats.script_count, flexible_bone_count,
           collision_bone_count,
           assets.import.record_count, assets.import.arena_used,
           assets.import.unresolved_external_count);

    // A truncated file must never yield a partially usable graph.
    require(!itCommonAssets_Import(bytes, size - 1, arena,
                                   IT_COMMON_IMPORT_ARENA_SIZE,
                                   IT_COMMON_IMPORT_RECORD_CAPACITY, &assets) &&
                assets.public_data.x0 == NULL,
            "reject truncated item bank\n");
    _aligned_free(arena);
    _aligned_free(bytes);
    puts("native item common core passed");
    return 0;
}
