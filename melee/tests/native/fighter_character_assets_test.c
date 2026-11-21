#include <melee/it/article_model_layers.h>
#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/ft/ftaction.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftYoshi/types.h>
#include <melee/ft/kinds/ftGameWatch/types.h>
#include <melee/it/itCharItems.h>
#include <melee/it/itYoyo.h>
#include <melee/it/itanimlist.h>
#include <melee/it/types.h>
#include <melee/lb/lbcommand.h>
#include <sysdolphin/baselib/gobj.h>

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

static void validate_timeline_catchup(void)
{
    // Seek across many ordinary one-frame waits. All four playback paths
    // must preserve the first unprocessed command and finish on the next call.
    static union CmdUnion commands[LB_COMMAND_UPDATE_LIMIT + 2];
    for (unsigned i = 0; i + 1 < ARRAY_SIZE(commands); ++i) {
        commands[i].Command_00 = (struct Command_00) { .code = 1, .value = 1 };
    }
    static Fighter fighter;
    Fighter_GObj fighter_object = { .user_data = &fighter };
    void (*updates[])(Fighter_GObj*) = {
        ftAction_80073240, ftAction_80073354, ftAction_8007349C
    };
    CommandInfo start = { .u = commands, .timer = -(float) ARRAY_SIZE(commands) };
    for (unsigned i = 0; i < ARRAY_SIZE(updates); ++i) {
        fighter.x3E4_fighterCmdScript = start;
        updates[i](&fighter_object);
        require(fighter.x3E4_fighterCmdScript.u == &commands[LB_COMMAND_UPDATE_LIMIT],
                "fighter timeline resumes at the next command\n");
        updates[i](&fighter_object);
        require(fighter.x3E4_fighterCmdScript.u == NULL, "fighter timeline finished\n");
    }
    static Item item;
    Item_GObj item_object = { .user_data = &item };
    item.x524_cmd = start;
    it_802799E4(&item_object);
    require(item.x524_cmd.u == &commands[LB_COMMAND_UPDATE_LIMIT],
            "item timeline resumes at the next command\n");
    it_802799E4(&item_object);
    require(item.x524_cmd.u == NULL, "item timeline finished\n");
}

static void validate_asset_layouts(void)
{
    require(!ftCharacterAssetLayout_IsValid(NULL), "reject missing asset layout\n");
    for (unsigned kind = 0; kind < Ft_Kind_StockCount; ++kind)
        require(ftCharacterAssetLayout_IsValid(&ftCharacterStockLayouts[kind]),
                "all explicit stock layouts validate\n");
    const FtCharacterAssetLayout* yoshi = &ftCharacterStockLayouts[Ft_Kind_Yoshi];
    require(yoshi->article_mask == 7 && yoshi->joint_graph_mask == 8 &&
                yoshi->item_format == FtCharacterItems_Plain &&
                yoshi->byte_range_count == 1 && yoshi->byte_ranges[0].offset == 0x12C &&
                yoshi->byte_ranges[0].size == 12,
            "Yoshi layout owns articles, egg graph and raw frame bytes\n");
    const FtCharacterAssetLayout* paper = &ftCharacterStockLayouts[Ft_Kind_GameWatch];
    require(paper->byte_range_count == 1 && paper->byte_ranges[0].offset == 4 &&
                paper->byte_ranges[0].size == 20,
            "Game & Watch layout preserves four colors and outline RGBA bytes\n");
    require(ftCharacterStockLayouts[Ft_Kind_Mewtwo].article_mask == 3 &&
                ftCharacterStockLayouts[Ft_Kind_Mewtwo].joint_graph_mask == 0 &&
                ftCharacterStockLayouts[Ft_Kind_Mewtwo].item_format == FtCharacterItems_Plain,
            "Mewtwo plain article layout has no inserted graph slots\n");
    static const u16 graph_masks[Ft_Kind_StockCount] = {
        [Ft_Kind_Kirby] = 0x10, [Ft_Kind_Link] = 0x40,
        [Ft_Kind_CLink] = 0x40, [Ft_Kind_Seak] = 0x30,
        [Ft_Kind_Yoshi] = 8,
    };
    static const u8 item_formats[Ft_Kind_StockCount] = {
        [Ft_Kind_Popo] = FtCharacterItems_ClimberString,
        [Ft_Kind_Nana] = FtCharacterItems_ClimberString,
        [Ft_Kind_Link] = FtCharacterItems_LinkWeapons,
        [Ft_Kind_CLink] = FtCharacterItems_LinkWeapons,
        [Ft_Kind_Seak] = FtCharacterItems_Chain,
        [Ft_Kind_Ness] = FtCharacterItems_Yoyo,
        [Ft_Kind_Samus] = FtCharacterItems_Grapple,
        [Ft_Kind_Purin] = FtCharacterItems_ModelParts,
        [Ft_Kind_GameWatch] = FtCharacterItems_PaperArticles,
    };
    for (unsigned kind = 0; kind < Ft_Kind_StockCount; ++kind)
        require(ftCharacterStockLayouts[kind].joint_graph_mask == graph_masks[kind] &&
                    ftCharacterStockLayouts[kind].item_format == item_formats[kind],
                "explicit stock graph/typed layouts match original importer cases\n");
    FtCharacterAssetLayout layout = {0};
    require(ftCharacterAssetLayout_IsValid(&layout), "empty plain layout is valid\n");
    layout.item_format = FtCharacterItems_FormatCount;
    require(!ftCharacterAssetLayout_IsValid(&layout), "reject unknown item format\n");
    layout = (FtCharacterAssetLayout){.article_mask=1,.joint_graph_mask=1};
    require(!ftCharacterAssetLayout_IsValid(&layout), "reject article and graph slot alias\n");
    layout = (FtCharacterAssetLayout){.byte_range_count=FT_CHARACTER_BYTE_RANGE_LIMIT+1};
    require(!ftCharacterAssetLayout_IsValid(&layout), "reject excessive raw spans\n");
    layout = (FtCharacterAssetLayout){.byte_range_count=1,.byte_ranges={{4,0}}};
    require(!ftCharacterAssetLayout_IsValid(&layout), "reject empty raw span\n");
    layout.byte_ranges[0] = (FtCharacterByteRange){FT_CHARACTER_EXT_ATTR_CAPACITY+1,1};
    require(!ftCharacterAssetLayout_IsValid(&layout), "reject raw offset beyond storage\n");
    layout.byte_ranges[0] = (FtCharacterByteRange){FT_CHARACTER_EXT_ATTR_CAPACITY-1,2};
    require(!ftCharacterAssetLayout_IsValid(&layout), "reject raw span beyond storage\n");
    layout.byte_ranges[0] = (FtCharacterByteRange){FT_CHARACTER_EXT_ATTR_CAPACITY-1,1};
    require(ftCharacterAssetLayout_IsValid(&layout), "last storage byte is valid\n");
    layout = (FtCharacterAssetLayout){.byte_range_count=2,.byte_ranges={{1,3},{4,5}}};
    require(ftCharacterAssetLayout_IsValid(&layout), "adjacent unaligned raw spans are valid\n");
    layout.byte_ranges[1].offset = 3;
    require(!ftCharacterAssetLayout_IsValid(&layout), "reject partial raw overlap\n");
    layout.byte_ranges[1] = layout.byte_ranges[0];
    require(!ftCharacterAssetLayout_IsValid(&layout), "reject duplicate raw span\n");
    static const u16 typed_articles[] = {0,4,14,8,0x400,8,0,0x3FF};
    for (unsigned format=1; format<FtCharacterItems_FormatCount; ++format) {
        layout = (FtCharacterAssetLayout){.item_format=format,.article_mask=typed_articles[format]};
        require(ftCharacterAssetLayout_IsValid(&layout), "required typed article slots validate\n");
        for (unsigned bit=0; bit<FT_CHARACTER_ITEM_CAPACITY; ++bit)
            if (typed_articles[format] & (1u<<bit)) {
                layout.article_mask = typed_articles[format] & ~(1u<<bit);
                require(!ftCharacterAssetLayout_IsValid(&layout), "reject missing typed article slot\n");
            }
    }
    layout = (FtCharacterAssetLayout){.item_format=FtCharacterItems_Grapple,.article_mask=8,.joint_graph_mask=16};
    require(!ftCharacterAssetLayout_IsValid(&layout), "graph must not overwrite typed grapple model slot\n");
    layout.joint_graph_mask = 0; layout.article_mask |= 16;
    require(!ftCharacterAssetLayout_IsValid(&layout), "article must not overwrite typed grapple model slot\n");
    layout = (FtCharacterAssetLayout){.item_format=FtCharacterItems_ModelParts,.article_mask=2};
    require(!ftCharacterAssetLayout_IsValid(&layout), "article must not alias typed model-parts slot\n");
    layout.article_mask = 0; layout.joint_graph_mask = 2;
    require(!ftCharacterAssetLayout_IsValid(&layout), "graph must not overwrite typed model-parts slot\n");
    layout = (FtCharacterAssetLayout){.item_format=FtCharacterItems_PaperArticles,.article_mask=0x7FF};
    require(!ftCharacterAssetLayout_IsValid(&layout), "article must not alias typed paper model-parts slot\n");
    layout.article_mask = 0x3FF; layout.joint_graph_mask = 0x400;
    require(!ftCharacterAssetLayout_IsValid(&layout), "graph must not overwrite typed paper model-parts slot\n");
}

#include "accessory_asset_cases.h"

static void validate_samus_named_animations(FtCharacterAssets* assets)
{
    Article* article = assets->fighter_items[3];
    itSamusGrappleAttributes* attrs = article->x4_specialAttributes;
    // These seven channels are bound to NULL by retail archive initialization;
    // the disc has no public graph definitions for their external names.
    require(!attrs->x7C && !attrs->x88 && !attrs->x94 && !attrs->x9C &&
            !attrs->xA0 && !attrs->xA8 && !attrs->xAC,
            "Samus external channels retain retail NULL bindings\n");
    const NativeFighterArticleModelAnimation recipes[] = {
        { 1, { {116, 0}, {120, 0}, {UINT32_MAX, UINT32_MAX} } },
        { 2, { {128, 0}, {132, 0}, {UINT32_MAX, UINT32_MAX} } },
        { 3, { {140, 0}, {144, 0}, {UINT32_MAX, UINT32_MAX} } },
        { 4, { {152, 0}, {UINT32_MAX, UINT32_MAX}, {UINT32_MAX, UINT32_MAX} } },
        { 5, { {164, 0}, {UINT32_MAX, UINT32_MAX}, {UINT32_MAX, UINT32_MAX} } }
    };
    HSD_AnimJoint* joints[] = { *attrs->x74, *attrs->x80, *attrs->x8C, *attrs->x98, *attrs->xA4 };
    HSD_MatAnimJoint* materials[] = { *attrs->x78, *attrs->x84, *attrs->x90, NULL, NULL };
    u32 records = assets->import.record_count;
    size_t used = assets->import.arena_used;
    require(itArticle_ImportModelAnimations(&assets->import, article, recipes, ARRAY_SIZE(recipes)),
            "Samus named arrays import through actual generic graph importer\n");
    require(article->native_model_animation_count == 5,
            "Samus five named animation recipes publish together\n");
    for (unsigned i = 0; i < 5; ++i)
        require(article->native_model_animations[i].anim == joints[i] &&
                article->native_model_animations[i].matanim == materials[i] &&
                article->native_model_animations[i].shapeanim == NULL,
                "Samus indexed recipes match original typed graph identities\n");
    require(assets->import.record_count == records && assets->import.arena_used == used,
            "Samus named animation import reuses graphs in own arena\n");
    const NativeFighterArticleModelRoot roots[] = { {11,100}, {12,104}, {13,108}, {14,112} };
    HSD_Joint* models[] = { attrs->x64, attrs->x68, attrs->x6C, attrs->x70 };
    require(itArticle_ImportModelRoots(&assets->import, article, roots, ARRAY_SIZE(roots)),
            "Samus four named resource models import\n");
    for (unsigned i = 0; i < 4; ++i)
        require(article->native_model_roots[i].root == models[i] && models[i]->child,
                "Samus named resource models retain actual first-child attachment roots\n");
    printf("Samus: 8 indexed graphs and 4 resource roots match typed stock import; 7 audited channels remain NULL\n");
}

static void validate_wait_choices(const WaitStruct* choices, int action_count)
{
    if (choices == NULL) {
        return;
    }
    int total_weight = 0;
    for (u32 i = 0; i < 256; ++i) {
        if (choices[i].animation_id == -1) {
            require(total_weight == 0 || total_weight == 100,
                    "wait animation weights cover the random range\n");
            return;
        }
        require(choices[i].animation_id >= 0 &&
                    choices[i].animation_id < action_count &&
                    choices[i].weight > 0 && choices[i].weight <= 100 &&
                    total_weight + choices[i].weight <= 100,
                "wait animation choice is valid\n");
        total_weight += choices[i].weight;
    }
    require(false, "wait animation choices terminate\n");
}

static u32 action_command_width(u32 opcode)
{
    static const u8 widths[49] = {
        5, 5, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 7, 4, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 1, 4,
    };
    if (opcode < 10) {
        return opcode == 5 || opcode == 7 ? 2 : 1;
    }
    return opcode <= 58 ? widths[opcode - 10] : 0;
}

static void require_hitbox_matches_source(const FtCharacterAssets* assets,
                                          u32 word)
{
    const union CmdUnion* command = &assets->action_stats.commands[word];
    u32 raw[5];
    for (u32 i = 0; i < ARRAY_SIZE(raw); ++i) {
        require(HSD_DatReadU32(&assets->dat,
                               assets->action_stats.source_begin +
                                   (word + i) * 4,
                               &raw[i]),
                "read fighter hitbox source words\n");
    }
    require(command[0].create_hitbox_0.opcode == raw[0] >> 26 &&
                command[0].create_hitbox_0.id == ((raw[0] >> 23) & 7) &&
                command[0].create_hitbox_0.hit_group ==
                    ((raw[0] >> 20) & 7) &&
                command[0].create_hitbox_0.only_hit_grabbed ==
                    ((raw[0] >> 19) & 1) &&
                command[0].create_hitbox_0.bone ==
                    ((raw[0] >> 11) & 0xFF) &&
                command[0].create_hitbox_0.use_common_bone_ids ==
                    ((raw[0] >> 10) & 1) &&
                command[0].create_hitbox_0.damage == (raw[0] & 0x3FF) &&
                command[1].create_hitbox_1.size == (raw[1] >> 16) &&
                command[1].create_hitbox_1.z_offset == (s16) raw[1] &&
                command[2].create_hitbox_2.y_offset == (s16) (raw[2] >> 16) &&
                command[2].create_hitbox_2.x_offset == (s16) raw[2] &&
                command[3].create_hitbox_3.angle == ((raw[3] >> 23) & 0x1FF) &&
                command[3].create_hitbox_3.knockback_growth ==
                    ((raw[3] >> 14) & 0x1FF) &&
                command[3].create_hitbox_3.weight_set_knockback ==
                    ((raw[3] >> 5) & 0x1FF) &&
                command[3].create_hitbox_3.item_hit_interaction ==
                    ((raw[3] >> 4) & 1) &&
                command[3].create_hitbox_3.ignore_thrown_fighters ==
                    ((raw[3] >> 3) & 1) &&
                command[3].create_hitbox_3.ignore_fighter_scale ==
                    ((raw[3] >> 2) & 1) &&
                command[3].create_hitbox_3.clank == ((raw[3] >> 1) & 1) &&
                command[3].create_hitbox_3.rebound == (raw[3] & 1) &&
                command[4].create_hitbox_4.base_knockback ==
                    ((raw[4] >> 23) & 0x1FF) &&
                command[4].create_hitbox_4.element ==
                    ((raw[4] >> 18) & 0x1F) &&
                command[4].create_hitbox_4.shield_damage ==
                    (s8) (raw[4] >> 10) &&
                command[4].create_hitbox_4.hit_sfx_severity ==
                    ((raw[4] >> 7) & 7) &&
                command[4].create_hitbox_4.hit_sfx_kind ==
                    ((raw[4] >> 2) & 0x1F) &&
                command[4].create_hitbox_4.hit_grounded ==
                    ((raw[4] >> 1) & 1) &&
                command[4].create_hitbox_4.hit_aerial == (raw[4] & 1),
            "fighter hitbox command matches retail fields\n");
}

static u32 validate_action_commands(const FtCharacterAssets* assets,
                                    u32 action_count)
{
    const u32 word_count = assets->action_stats.word_count;
    u8* visited = calloc(word_count, 1);
    u32* pending = malloc((word_count + action_count) * sizeof(*pending));
    require(visited != NULL && pending != NULL,
            "allocate fighter action graph audit\n");
    u32 pending_count = 0;
    for (u32 i = 0; i < action_count; ++i) {
        const union CmdUnion* command = assets->data.xC[i].xC;
        if (command != NULL) {
            pending[pending_count++] =
                (u32) (command - assets->action_stats.commands);
        }
    }
    u32 hitbox_count = 0;
    while (pending_count != 0) {
        u32 word = pending[--pending_count];
        while (word < word_count && !visited[word]) {
            const union CmdUnion* command =
                &assets->action_stats.commands[word];
            u32 opcode = command->Command_09.id;
            u32 width = action_command_width(opcode);
            require(width != 0 && width <= word_count - word,
                    "fighter action instruction width\n");
            visited[word] = 1;
            if (opcode == 11) {
                require_hitbox_matches_source(assets, word);
                ++hitbox_count;
            }
            if (opcode == 17) {
                u32 source = assets->action_stats.source_begin + word * 4;
                u32 head, id, tail;
                require(HSD_DatReadU32(&assets->dat, source, &head) &&
                        HSD_DatReadU32(&assets->dat, source + 4, &id) &&
                        HSD_DatReadU32(&assets->dat, source + 8, &tail),
                        "read fighter sound source words\n");
                require(command[0].sound_effect_0.behavior == ((head >> 18) & 0xFF) &&
                        command[1].sound_effect_1.sfx_id == id &&
                        command[2].sound_effect_2.volume == ((tail >> 8) & 0xFF) &&
                        command[2].sound_effect_2.panning == (tail & 0xFF),
                        "fighter sound channel and settings match retail fields\n");
            }
            if (opcode == 5 || opcode == 7) {
                s32 relative;
                memcpy(&relative, &command[1], sizeof(relative));
                if (relative != INT32_MIN) {
                    s64 target_bytes =
                        (s64) (word + 1) * 4 + (s64) relative;
                    require(target_bytes >= 0 && (target_bytes & 3) == 0 &&
                                target_bytes / 4 < word_count,
                            "fighter action branch target\n");
                    pending[pending_count++] = (u32) target_bytes / 4;
                }
            }
            if (opcode == 0 || opcode == 6 || opcode == 7) {
                break;
            }
            word += width;
        }
    }
    free(pending);
    free(visited);
    return hitbox_count;
}

static bool special_attributes_have_references(const HSD_Dat* dat,
                                               u32 article_source)
{
    u32 source;
    const char* external;
    if (HSD_DatResolve(dat, article_source + 4, &source, &external) !=
        HSD_DAT_REF_LOCAL)
    {
        return false;
    }
    u32 end = HSD_DatNextTarget(dat, source);
    for (u32 slot = source; slot + 4 <= end; slot += 4) {
        u32 target;
        HSD_DatReferenceKind kind =
            HSD_DatResolve(dat, slot, &target, &external);
        if (kind == HSD_DAT_REF_LOCAL || kind == HSD_DAT_REF_EXTERNAL) {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv)
{
    validate_asset_layouts();
    if (argc == 2 && strcmp(argv[1], "--layout-only") == 0) {
        puts("explicit fighter asset layouts validated");
        return 0;
    }
    static const u32 expected_article_masks[Ft_Kind_StockCount] = {
        [Ft_Kind_Mario] = 0x005,
        [Ft_Kind_Fox] = 0x007,
        [Ft_Kind_Kirby] = 0x00F,
        [Ft_Kind_Koopa] = 0x001,
        [Ft_Kind_Link] = 0x01F,
        [Ft_Kind_Seak] = 0x00F,
        [Ft_Kind_Ness] = 0x7FF,
        [Ft_Kind_Peach] = 0x01F,
        [Ft_Kind_Popo] = 0x007,
        [Ft_Kind_Nana] = 0x007,
        [Ft_Kind_Pikachu] = 0x007,
        [Ft_Kind_Samus] = 0x00F,
        [Ft_Kind_Yoshi] = 0x007,
        [Ft_Kind_Mewtwo] = 0x003,
        [Ft_Kind_Luigi] = 0x001,
        [Ft_Kind_Zelda] = 0x003,
        [Ft_Kind_CLink] = 0x03F,
        [Ft_Kind_DrMario] = 0x00A,
        [Ft_Kind_Falco] = 0x00B,
        [Ft_Kind_Pichu] = 0x007,
        [Ft_Kind_GameWatch] = 0x3FF,
        [Ft_Kind_MasterH] = 0x007,
        [Ft_Kind_CrezyH] = 0x007,
        [Ft_Kind_GKoops] = 0x001,
    };
    char path[64];
    u32 article_count = 0;
    u32 item_script_count = 0;
    u32 item_instruction_count = 0;
    u32 unresolved_external_count = 0;
    u32 action_entry_count = 0;
    u32 action_instruction_count = 0;
    u32 action_hitbox_count = 0;
    u32 reference_attribute_count = 0;

    require(argc == 2, "usage: native_fighter_character_assets_test <iso>|--layout-only\n");
    validate_timeline_catchup();
    require(aurora_dvd_open(argv[1]), "open ISO\n");

    for (FighterKind kind = Ft_Kind_Mario; kind < Ft_Kind_StockCount; ++kind) {
        require(ftCharacterStockLayouts[kind].article_mask == expected_article_masks[kind],
                "explicit stock article layout matches original importer mask\n");
        DVDFileInfo file;
        const FtDataArchiveName* archive = &ftData_803C1F40[kind];
        snprintf(path, sizeof(path), "/%s", archive->filename);
        require(DVDOpen(path, &file), "open fighter archive\n");

        size_t size = file.length;
        size_t padded = (size + 31) & ~(size_t) 31;
        void* bytes = _aligned_malloc(padded, 32);
        void* arena = _aligned_malloc(FT_CHARACTER_IMPORT_ARENA_SIZE,
                                      sizeof(void*));
        require(bytes != NULL && arena != NULL &&
                    DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                    DVDClose(&file),
                "read fighter archive\n");

        FtCharacterAssets assets;
        if (!ftCharacterAssets_Import(
                bytes, size, &ftCharacterStockLayouts[kind], archive->root_name,
                (u32) ftData_Table_Unk0[kind].count,
                (u32) ftData_UnkIntPairs[kind].count,
                CostumeListsForeachCharacter[kind].numCostumes, arena,
                FT_CHARACTER_IMPORT_ARENA_SIZE,
                FT_CHARACTER_IMPORT_RECORD_CAPACITY, &assets))
        {
            HSD_Dat probe;
            u32 probe_root = 0;
            HSD_DatResult probe_status = HSD_DatOpen(&probe, bytes, size);
            fprintf(stderr,
                    "%s (%s): status=%d offset=%lx records=%lu external=%s "
                    "open=%d root=%d wait=%d costumes=%u\n",
                    archive->filename, archive->root_name,
                    assets.import.status, assets.import.error_offset,
                    assets.import.record_count,
                    assets.import.external_symbol != NULL
                        ? assets.import.external_symbol
                        : "-",
                    probe_status,
                    probe_status == HSD_DAT_OK &&
                        HSD_DatFindRoot(&probe, archive->root_name,
                                        &probe_root),
                    ftData_Table_Unk0[kind].count,
                    (unsigned) CostumeListsForeachCharacter[kind].numCostumes);
            require(false, "import fighter archive\n");
        }
        require(assets.model_data.x0.costume_count >= CostumeListsForeachCharacter[kind].numCostumes &&
                assets.model_data.x8.costume_count >= CostumeListsForeachCharacter[kind].numCostumes,
                "model tables retain all costume variant rows\n");
        if (kind == Ft_Kind_Luigi || kind == Ft_Kind_Falco) {
            require(assets.sfx.x1C == &assets.light_damage_sfx &&
                        assets.light_damage_sfx.num > 0 &&
                        assets.light_damage_sfx.sfx_ids != NULL,
                    "light damage sound list uses imported native pointers\n");
        }
        if (kind == Ft_Kind_Ganon || kind == Ft_Kind_Zelda ||
            kind == Ft_Kind_Popo || kind == Ft_Kind_Nana ||
            kind == Ft_Kind_GameWatch)
        {
            require(assets.data.x14 != NULL && assets.data.x18 != NULL &&
                        assets.data.x14[0].x0 != NULL,
                    "results demo motion table imported\n");
        }
        if (kind == Ft_Kind_GameWatch) {
            u32 paper_root, paper_attributes;
            const char* external = NULL;
            require(HSD_DatFindRoot(&assets.dat, archive->root_name, &paper_root) &&
                        HSD_DatResolve(&assets.dat, paper_root + 4,
                                       &paper_attributes, &external) == HSD_DAT_REF_LOCAL,
                    "resolve retail Game & Watch attributes\n");
            const u8* colors = HSD_DatBytes(&assets.dat, paper_attributes + 4, 20);
            const ftGameWatchAttributes* attributes = assets.data.ext_attr;
            _Static_assert(offsetof(ftGameWatchAttributes, x4_GAMEWATCH_COLOR) == 4 &&
                               offsetof(ftGameWatchAttributes, x14_GAMEWATCH_OUTLINE) == 20,
                           "Game & Watch palettes are five consecutive RGBA tuples");
            require(colors != NULL &&
                        memcmp(attributes->x4_GAMEWATCH_COLOR, colors, 16) == 0 &&
                        memcmp(&attributes->x14_GAMEWATCH_OUTLINE, colors + 16, 4) == 0,
                    "all imported Game & Watch RGBA channels match original DAT bytes\n");
            float width, loop_frame;
            require(HSD_DatReadF32(&assets.dat, paper_attributes, &width) &&
                        HSD_DatReadF32(&assets.dat, paper_attributes + 24, &loop_frame) &&
                        memcmp(&attributes->x0_GAMEWATCH_WIDTH, &width, 4) == 0 &&
                        memcmp(&attributes->x18_GAMEWATCH_CHEF_LOOPFRAME, &loop_frame, 4) == 0,
                    "numeric attributes around RGBA range retain scalar decoding\n");
            Article* spray = assets.fighter_items[0];
            require(spray != NULL && spray->xC_itemStates != NULL &&
                        spray->xC_itemStates->count == 4,
                    "Game & Watch spray has all four animation states\n");
            FtPartsVisLookup* lookup = assets.fighter_items[10];
            require(assets.model_data.x0.model_num == 11 && lookup != NULL,
                    "Game & Watch visibility lookup covers every model\n");
            for (u32 model = 0; model < assets.model_data.x0.model_num;
                 ++model)
            {
                require(lookup[model].x0 >= 0 &&
                            (lookup[model].x0 == 0 || lookup[model].x4 != NULL),
                        "Game & Watch model visibility variants imported\n");
                for (int variant = 0; variant < lookup[model].x0; ++variant) {
                    TempS* draws = &lookup[model].x4[variant];
                    require(draws->x0 >= 0 &&
                                (draws->x0 == 0 || draws->x4 != NULL),
                            "Game & Watch draw visibility indices imported\n");
                }
            }
        }
        if (kind == Ft_Kind_Falco) {
            union CmdUnion* script = assets.data.xC[ftCo_MS_AttackHi3].xC;
            require(script != NULL && (((u32*) script)[0] & 63) == 7,
                    "Falco up tilt action script entry\n");
            s32 relative = ((s32*) script)[1];
            union CmdUnion* target = (union CmdUnion*)
                ((u8*) &script[1] + relative);
            require(target[1].create_hitbox_0.opcode == 11 &&
                        target[4].create_hitbox_3.ignore_thrown_fighters == 0,
                    "Falco up tilt contains an unconditional hitbox\n");
            u32 source = assets.action_stats.source_begin +
                         (u32) ((u8*) &target[4] -
                                (u8*) assets.action_stats.commands);
            u32 raw;
            require(HSD_DatReadU32(&assets.dat, source, &raw) &&
                        target[4].create_hitbox_3.ignore_thrown_fighters ==
                            ((raw >> 3) & 1),
                    "Falco up tilt skip flag matches retail bit\n");
        }
        if (kind == Ft_Kind_Luigi) {
            union CmdUnion* script = assets.data.xC[ftCo_MS_AttackLw4].xC;
            require(script != NULL && script[22].create_hitbox_0.opcode == 11,
                    "Luigi down smash hitbox command\n");
            u32 source = assets.action_stats.source_begin +
                         (u32) ((u8*) &script[23] -
                                (u8*) assets.action_stats.commands);
            u32 raw_size, raw_offset;
            require(HSD_DatReadU32(&assets.dat, source, &raw_size) &&
                        HSD_DatReadU32(&assets.dat, source + 4,
                                       &raw_offset) &&
                        script[23].create_hitbox_1.size == (raw_size >> 16) &&
                        script[23].create_hitbox_1.z_offset ==
                            (s16) raw_size &&
                        script[24].create_hitbox_2.y_offset ==
                            (s16) (raw_offset >> 16) &&
                        script[24].create_hitbox_2.x_offset ==
                            (s16) raw_offset,
                    "Luigi down smash hitbox geometry matches retail words\n");
        }
        require(assets.data.x0 == &assets.common_attributes &&
                    isfinite(assets.common_attributes.walk_accel_mul) &&
                    assets.common_attributes.weight > 0.0F &&
                    assets.data.xC != NULL &&
                    assets.data.x1C == assets.part_animation_groups &&
                    assets.data.x38 == assets.collision_bubbles &&
                    assets.data.x2C == &assets.dynamics &&
                    assets.dynamics.x4 >= 0 &&
                    assets.dynamics.x4 <= 0xB &&
                    (assets.dynamics.x4 == 0 || assets.dynamics.x8 != NULL) &&
                    assets.hurtboxes.count > 0 && assets.data.x44 != NULL &&
                    assets.data.x54 == assets.x54 &&
                    (kind >= Ft_Kind_MasterH ||
                     (assets.model_data.x0.model_num > 0 &&
                      assets.metal_model != NULL)),
                "fighter archive dimensions\n");
        validate_wait_choices(assets.data.x24,
                              ftData_Table_Unk0[kind].count);
        validate_wait_choices(assets.data.x28,
                              ftData_Table_Unk0[kind].count);
        for (u32 i = 0; i < 5; ++i) {
            require(assets.x54[i] >= 0 && assets.x54[i] < MAX_FT_PARTS,
                    "fighter effect bone table contains valid joints\n");
        }
        require(!assets.import.allow_unresolved_externals,
                "fighter archive external references\n");
        u32 root_source;
        u32 wait_source;
        const char* external;
        require(HSD_DatFindRoot(&assets.dat, archive->root_name,
                                &root_source) &&
                    HSD_DatResolve(&assets.dat, root_source + 12,
                                   &wait_source, &external) ==
                        HSD_DAT_REF_LOCAL &&
                    assets.action_stats.commands != NULL &&
                    assets.action_stats.source_begin < wait_source &&
                    assets.action_stats.word_count ==
                        (wait_source - assets.action_stats.source_begin) / 4,
                "fighter action command arena\n");
        u32 live_entries = 0;
        for (u32 i = 0; i < (u32) ftData_Table_Unk0[kind].count; ++i) {
            u32 target;
            HSD_DatReferenceKind reference = HSD_DatResolve(
                &assets.dat, wait_source + i * 24 + 12,
                &target, &external);
            union CmdUnion* command = assets.data.xC[i].xC;
            if (reference == HSD_DAT_REF_NULL) {
                require(command == NULL, "empty fighter action\n");
                continue;
            }
            u32 source_head;
            require(reference == HSD_DAT_REF_LOCAL &&
                        HSD_DatReadU32(&assets.dat, target,
                                       &source_head) &&
                        target >= assets.action_stats.source_begin &&
                        target < wait_source &&
                        command == &assets.action_stats.commands[
                            (target - assets.action_stats.source_begin) / 4] &&
                        command->Command_09.id == source_head >> 26,
                    "fighter action entry mapping and opcode\n");
            ++live_entries;
        }
        require(live_entries == assets.action_stats.entry_count &&
                    assets.action_stats.instruction_count > 0,
                "fighter action graph coverage\n");
        action_hitbox_count += validate_action_commands(
            &assets, (u32) ftData_Table_Unk0[kind].count);
        u32 article_mask = 0;
        u32 article_table = 0;
        if (expected_article_masks[kind] != 0) {
            require(HSD_DatResolve(&assets.dat, root_source + 72,
                                   &article_table, &external) ==
                        HSD_DAT_REF_LOCAL,
                    "fighter article source table\n");
        }
        for (u32 i = 0; i < ARRAY_SIZE(assets.fighter_items); ++i) {
            Article* article = assets.fighter_items[i];
            if ((expected_article_masks[kind] & (1U << i)) != 0) {
                u32 article_source;
                require(article != NULL, "fighter article present\n");
                article_mask |= 1U << i;
                require(article->x0_common_attr != NULL &&
                            isfinite(article->x0_common_attr->x60_scale) &&
                            HSD_DatResolve(&assets.dat,
                                           article_table + i * 4,
                                           &article_source, &external) ==
                                HSD_DAT_REF_LOCAL,
                        "fighter article attributes\n");
                if (special_attributes_have_references(&assets.dat,
                                                       article_source))
                {
                    ++reference_attribute_count;
                }
            }
        }
        require(article_mask == expected_article_masks[kind],
                "fighter article table\n");
        switch (kind) {
        case Ft_Kind_Popo:
        case Ft_Kind_Nana: {
            itClimbersStringAttributes* attrs =
                ((Article*) assets.fighter_items[2])->x4_specialAttributes;
            require(attrs != NULL && attrs->x0_count >= 2 &&
                        (kind == Ft_Kind_Nana ||
                         (attrs->x24_joint != NULL &&
                          attrs->x28_joint != NULL)),
                    "Ice Climbers string model attributes\n");
            break;
        }
        case Ft_Kind_Kirby:
            require(assets.fighter_items[4] != NULL,
                    "Kirby inhale model\n");
            break;
        case Ft_Kind_Link:
        case Ft_Kind_CLink: {
            static const u32 expected_state_counts[] = { 4, 3, 0, 1, 6 };
            for (u32 i = 0; i < ARRAY_SIZE(expected_state_counts); ++i) {
                Article* article = assets.fighter_items[i];
                require(article != NULL &&
                            (expected_state_counts[i] == 0
                                 ? article->xC_itemStates == NULL
                                 : article->xC_itemStates != NULL &&
                                       article->xC_itemStates->count ==
                                           expected_state_counts[i]),
                        "Link article state table coverage\n");
            }
            itLinkBoomerangAttributes* boomerang =
                ((Article*) assets.fighter_items[1])->x4_specialAttributes;
            itLinkHookshotAttributes* hookshot =
                ((Article*) assets.fighter_items[2])->x4_specialAttributes;
            itLinkArrowAttributes* arrow =
                ((Article*) assets.fighter_items[3])->x4_specialAttributes;
            require(boomerang != NULL && boomerang->x44 != NULL &&
                        boomerang->x48 != NULL &&
                        boomerang->x4C_anim.anim != NULL &&
                        boomerang->x58_anim.anim != NULL,
                    "Link boomerang model attributes\n");
            require(hookshot != NULL && hookshot->x54 != NULL &&
                        hookshot->x58 != NULL && hookshot->x5C != NULL,
                    "Link hookshot model attributes\n");
            require(arrow != NULL && arrow->x24 != NULL &&
                        arrow->x28 != NULL,
                    "Link arrow model attributes\n");
            require(assets.fighter_items[6] != NULL,
                    "Link equipment model\n");
            HSD_Joint sword = *assets.joint_graphs[6];
            sword.child = sword.next = NULL;
            require(ftCostumeAssets_ValidateModel(&sword, NULL, NULL, 124, NULL) == FT_MODEL_VALID,
                    "The stock sword can be installed in the reserved joint by itself\n");
            break;
        }
        case Ft_Kind_Seak:
            {
                itSeakChain_Attrs* attrs =
                    ((Article*) assets.fighter_items[3])->x4_specialAttributes;
                require(attrs != NULL && attrs->x0 >= 2 && attrs->x0 <= 30 &&
                            attrs->x64_joint != NULL &&
                            attrs->x68_joint != NULL,
                        "Sheik chain model attributes\n");
            }
            require(assets.fighter_items[4] != NULL &&
                        assets.fighter_items[5] != NULL,
                    "Sheik transform models\n");
            break;
        case Ft_Kind_Ness: {
            itYoyoAttributes* attrs =
                ((Article*) assets.fighter_items[10])->x4_specialAttributes;
            require(attrs != NULL && attrs->link_count == 20 &&
                        attrs->up_smash_charge_link_count == 9 &&
                        attrs->down_smash_charge_link_count == 11 &&
                        attrs->link_length == 1.0F &&
                        attrs->minimum_link_length == 0.7F &&
                        attrs->release_velocity_x == 0.7F &&
                        attrs->release_velocity_y == 0.8F &&
                        attrs->gravity == 0.1F &&
                        attrs->terminal_velocity == 1.0F &&
                        attrs->up_smash_rotate_frame == 4 &&
                        attrs->up_smash_retract_frame == 49 &&
                        attrs->down_smash_rotate_frame == 5 &&
                        attrs->down_smash_retract_frame == 60 &&
                        attrs->string_joint != NULL &&
                        attrs->yoyo_joint != NULL &&
                        attrs->yoyo_material_animation != NULL,
                    "Ness yo-yo attributes and model graphs\n");
            break;
        }
        case Ft_Kind_Samus:
            {
                itSamusGrappleAttributes* attrs =
                    ((Article*) assets.fighter_items[3])->x4_specialAttributes;
                require(attrs != NULL && attrs->x64 != NULL &&
                            attrs->x68 != NULL && attrs->x6C != NULL &&
                            attrs->x70 != NULL && attrs->x74 != NULL &&
                            *attrs->x74 != NULL && attrs->x78 != NULL &&
                            *attrs->x78 != NULL && attrs->x80 != NULL &&
                            *attrs->x80 != NULL && attrs->x84 != NULL &&
                            *attrs->x84 != NULL && attrs->x8C != NULL &&
                            *attrs->x8C != NULL && attrs->x90 != NULL &&
                            *attrs->x90 != NULL && attrs->x98 != NULL &&
                            *attrs->x98 != NULL && attrs->xA4 != NULL &&
                            *attrs->xA4 != NULL,
                        "Samus grapple model and animation graphs\n");
                validate_samus_named_animations(&assets);
                validate_accessory_assets(&assets);
            }
            require(assets.fighter_items[4] != NULL,
                    "Samus held grapple model\n");
            break;
        case Ft_Kind_Yoshi:
            {
                u32 attribute_source;
                const ftYoshiAttributes* attrs = assets.data.ext_attr;
                require(HSD_DatResolve(&assets.dat, root_source + 4,
                                       &attribute_source, &external) ==
                            HSD_DAT_REF_LOCAL,
                        "Yoshi extended attribute source\n");
                const u8* frames = HSD_DatBytes(
                    &assets.dat,
                    attribute_source + offsetof(ftYoshiAttributes, x12C),
                    sizeof(attrs->x12C));
                require(frames != NULL &&
                            memcmp(attrs->x12C, frames,
                                   sizeof(attrs->x12C)) == 0,
                        "Yoshi capture frame byte array preserves retail order\n");
                u32 scalar;
                float expected;
                require(HSD_DatReadU32(
                            &assets.dat, attribute_source +
                                             offsetof(ftYoshiAttributes, x124),
                            &scalar),
                        "Yoshi capture scalar source\n");
                memcpy(&expected, &scalar, sizeof(expected));
                require(attrs->x124 == expected,
                        "Yoshi capture scalar preserves retail word decoding\n");
            }
            require(assets.fighter_items[3] != NULL,
                    "Yoshi egg model\n");
            for (u32 costume = 0;
                 costume < CostumeListsForeachCharacter[kind].numCostumes;
                 ++costume)
            {
                for (u32 group = 0; group < 2; ++group) {
                    FtPartsVisLookup* visibility =
                        assets.model_data.x0.vis_table[costume][group];
                    if (visibility == NULL) {
                        visibility =
                            assets.model_data.x0.vis_table[0][group];
                    }
                    require(visibility != NULL && visibility[0].x0 > 1 &&
                                visibility[0].x4 != NULL &&
                                visibility[0].x4[1].x0 > 0 &&
                                visibility[0].x4[1].x0 <= 32 &&
                                visibility[0].x4[1].x4 != NULL,
                            "Yoshi shield material visibility variant\n");
                }
            }
            break;
        case Ft_Kind_Purin:
            require(assets.fighter_items[1] != NULL,
                    "Jigglypuff costume parts\n");
            break;
        case Ft_Kind_GameWatch:
            for (u32 i = 0; i < 10; ++i) {
                Article* article = assets.fighter_items[i];
                const ArticleModelLayers* layers = article->model_layers;
                require(article->x4_specialAttributes != NULL && layers != NULL &&
                            layers->normal_count > 0 &&
                            layers->normal_indices != NULL &&
                            layers->outline_count > 0 &&
                            layers->outline_indices != NULL,
                        "Game & Watch article model attributes\n");
            }
            {
                itGamewatchchefAttributes* chef =
                    ((Article*) assets.fighter_items[8])->x4_specialAttributes;
                require(chef->wall_bounce_velocity_multiplier == 0.5F &&
                            chef->active_lifetime == 80.0F &&
                            chef->inactive_lifetime == 30.0F,
                        "Game & Watch Chef shared physics attributes\n");
                for (u32 i = 0; i < IT_GAMEWATCH_CHEF_VARIANT_COUNT; ++i) {
                    require(isfinite(chef->entries[i].horizontal_velocity) &&
                                isfinite(chef->entries[i].vertical_velocity) &&
                                chef->entries[i].gravity > 0.0F &&
                                chef->entries[i].terminal_velocity > 0.0F &&
                                isfinite(chef->entries[i].angular_velocity),
                            "Game & Watch Chef variant physics attributes\n");
                }
            }
            require(assets.fighter_items[10] != NULL,
                    "Game & Watch visibility data\n");
            break;
        default:
            break;
        }
        unresolved_external_count +=
            assets.import.unresolved_external_count;
        article_count += assets.article_stats.article_count;
        item_script_count += assets.article_stats.script_count;
        item_instruction_count += assets.item_command_data.instruction_count;
        require(assets.article_stats.script_count ==
                    assets.item_command_data.entry_count,
                "fighter item script entry coverage\n");
        action_entry_count += assets.action_stats.entry_count;
        action_instruction_count += assets.action_stats.instruction_count;

        printf("%-10s records=%lu arena=%zu externals=%lu articles=%lu scripts=%lu\n",
               archive->filename, assets.import.record_count,
               assets.import.arena_used,
               assets.import.unresolved_external_count,
               assets.article_stats.article_count,
               assets.article_stats.script_count);
        if (kind == Ft_Kind_Yoshi || kind == Ft_Kind_Mewtwo) {
            FtCharacterAssetLayout owned_layout = ftCharacterStockLayouts[kind];
            FtCharacterAssets owned_assets;
            require(ftCharacterAssets_Import(
                        bytes, size, &owned_layout, archive->root_name,
                        (u32) ftData_Table_Unk0[kind].count,
                        (u32) ftData_UnkIntPairs[kind].count,
                        CostumeListsForeachCharacter[kind].numCostumes, arena,
                        FT_CHARACTER_IMPORT_ARENA_SIZE,
                        FT_CHARACTER_IMPORT_RECORD_CAPACITY, &owned_assets),
                    "import independent copied Yoshi/Mewtwo layout facts\n");
            require(memcmp(assets.extended_attributes, owned_assets.extended_attributes,
                           sizeof(assets.extended_attributes)) == 0 &&
                        assets.article_stats.article_count == owned_assets.article_stats.article_count &&
                        assets.article_stats.script_count == owned_assets.article_stats.script_count &&
                        assets.import.record_count == owned_assets.import.record_count,
                    "copied layout preserves decoded attributes and complete import counts\n");
            for (unsigned slot=0; slot<FT_CHARACTER_ITEM_CAPACITY; ++slot)
                if ((owned_layout.article_mask | owned_layout.joint_graph_mask) & (1u<<slot))
                    require(owned_assets.fighter_items[slot] != NULL,
                            "owned layout imports every declared article and joint graph\n");
            owned_layout.byte_range_count = 1;
            owned_layout.byte_ranges[0] = (FtCharacterByteRange){FT_CHARACTER_EXT_ATTR_CAPACITY-4,4};
            require(ftCharacterAssetLayout_IsValid(&owned_layout),
                    "storage-valid span still requires real serialized attribute coverage\n");
            require(!ftCharacterAssets_Import(
                        bytes, size, &owned_layout, archive->root_name,
                        (u32) ftData_Table_Unk0[kind].count,
                        (u32) ftData_UnkIntPairs[kind].count,
                        CostumeListsForeachCharacter[kind].numCostumes, arena,
                        FT_CHARACTER_IMPORT_ARENA_SIZE,
                        FT_CHARACTER_IMPORT_RECORD_CAPACITY, &owned_assets),
                    "reject declared byte span beyond actual Yoshi/Mewtwo serialized attributes\n");
        }
        _aligned_free(arena);
        _aligned_free(bytes);
    }

    require(unresolved_external_count == 18,
            "fighter archive external reference count\n");
    require(article_count == 87 && item_script_count == 119 &&
                item_instruction_count == 306,
            "fighter article coverage\n");
    require(action_entry_count == 9994 &&
                action_instruction_count == 64174,
            "all fighter action graphs converted\n");
    require(action_hitbox_count == 6655,
            "all fighter hitbox commands match retail fields\n");
    require(reference_attribute_count == 21,
            "all reference-bearing fighter article attributes covered\n");
    union CmdUnion next = { 0 };
    next.spawn_gfx_0.opcode = 10;
    next.spawn_gfx_0.boneId = 2;
    require(ftActionData_NextHitboxFlag(&next),
            "next-command flag in GFX bone field\n");
    next.spawn_gfx_0.boneId = 1;
    require(!ftActionData_NextHitboxFlag(&next),
            "next-command adjacent GFX bit\n");
    next.Command_09.id = 11;
    next.create_hitbox_0.only_hit_grabbed = 1;
    require(ftActionData_NextHitboxFlag(&next),
            "next-command flag in hitbox field\n");
    puts("all native fighter character archives imported");
    return 0;
}
