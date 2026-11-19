#include <melee/ft/native_fighter_skin_edit.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far

#include <stdint.h>
#include <aurora/dvd.h>
#include <melee/ft/forward.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_template.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_family_project.h>
#include <melee/ft/native_fighter_draft_access.h>
#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/ft/native_fighter_model_editor.h>
#include <melee/ft/native_fighter_action_editor.h>
#include <melee/ft/native_fighter_animation_editor.h>
#include <melee/ft/native_fighter_pose_editor.h>
#include <melee/ft/native_fighter_move_copy.h>
#include "native_source_content_tool.h"
#include "native_workshop_authoring_tool.h"
#include "native_agent_tool.h"
#include "native_source_resource_tool.h"
#include "native_fighter_parameter_tool.h"
#include "native_fighter_cpu_tool.h"
#include <melee/ft/native_costume_package.h>
#include <melee/ft/native_costume_import.h>
#include <melee/gm/gm_1601.h>
#include <melee/gm/native_content_store.h>
#include <melee/lb/native_disc_archive.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobj.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <process.h>
#include <string.h>

static const char* stock_names[CKind_Playable_Count] = {
    "Captain Falcon", "Donkey Kong", "Fox", "Mr. Game & Watch", "Kirby",
    "Bowser", "Link", "Luigi", "Mario", "Marth", "Mewtwo", "Ness",
    "Peach", "Pikachu", "Ice Climbers", "Jigglypuff", "Samus", "Yoshi",
    "Zelda", "Sheik", "Falco", "Young Link", "Dr. Mario", "Roy",
    "Pichu", "Ganondorf",
};

static void usage(void)
{
    fputs(
        "Usage: native_content_tool [--root DIRECTORY] COMMAND\n"
        "  agent list\n"
        "  agent call TOOL [JSON_ARGUMENTS]\n"
        "  mcp\n"
        "  import-costume DAT CHARACTER BASE_COLOR TITLE\n"
        "  import-custom-costume DAT_OR_MEMBER_DIRECTORY FIGHTER_ID TITLE [MODEL_VARIANT]\n"
        "  cache PACKAGE.fmc\n"
        "  accept PACKAGE_ID\n"
        "  install-project PROJECT_DIRECTORY PACKAGE_ID\n"
        "  uninstall PACKAGE_ID\n"
        "  export PACKAGE_ID OUTPUT.fmc\n"
        "  extract-stock ISO CHARACTER BASE_COLOR OUTPUT.dat\n"
        "  clone-stock ISO Mario|DrMario|Ganon|\"Captain Falcon\"|Luigi|Jigglypuff|Fox|Falco|Marth|Roy|\"Donkey Kong\"|Bowser|Pikachu|Pichu|Samus|Link|\"Young Link\"|Mewtwo|Yoshi|Ness|Peach|\"Game & Watch\"|Zelda|Sheik|Popo|Nana PROJECT_DIRECTORY TITLE\n"
        "  clone-installed PACKAGE_ID PROJECT_DIRECTORY TITLE\n"
        "  inspect-project PROJECT_DIRECTORY\n"
        "  project-stock-template PROJECT_DIRECTORY\n"
        "  project-assets-undo PROJECT_DIRECTORY\n"
        "  project-model-scan SOURCE.glb\n"
        "  project-model-sources PROJECT_DIRECTORY\n"
        "  project-reimport-plan PROJECT_DIRECTORY MESH SOURCE.glb one|whole\n"
        "  project-reimport PROJECT_DIRECTORY MESH SOURCE.glb one|whole keep|transfer|source|auto|rigid keep-materials|replace-materials\n"
        "  project-import-meshes PROJECT_DIRECTORY SOURCE.glb JOINT SCALE auto|rigid|source\n"
        "  project-label PROJECT_DIRECTORY bone|mesh|material|action KEY NAME\n"
        "  project-hide-action PROJECT_DIRECTORY ACTION_INDEX 0|1\n"
        "  project-clip-create PROJECT_DIRECTORY ACTION_INDEX NAME END_FRAME\n"
        "  project-clip-copy PROJECT_DIRECTORY action|demo SOURCE_INDEX TARGET_ACTION NAME\n"
        "  project-texture-sequences PROJECT_DIRECTORY MATERIAL\n"
        "  project-texture-image PROJECT_DIRECTORY MATERIAL SEQUENCE_OFFSET IMAGE_INDEX IMAGE_FILE\n"
        "  project-model-import PROJECT_DIRECTORY SOURCE.glb PRIMITIVE JOINT "
        "ADD_OR_MESH_INDEX SCALE rigid|auto|transfer|source|keep\n"
        "  project-model-weights PROJECT_DIRECTORY\n"
        "  project-model-vertices PROJECT_DIRECTORY MESH\n"
        "  project-model-joints PROJECT_DIRECTORY\n"
        "  project-model-rest PROJECT_DIRECTORY JOINT position|rotation|scale "
        "AXIS VALUE\n"
        "  project-model-paint PROJECT_DIRECTORY MESH JOINT X Y Z RADIUS "
        "STRENGTH add|subtract\n"
        "  project-model-texture PROJECT_DIRECTORY MATERIAL IMAGE_FILE\n"
        "  project-attribute PROJECT_DIRECTORY PROPERTY [VALUE]\n"
        "  project-actions PROJECT_DIRECTORY [ACTION_INDEX]\n"
        "  project-insert-at-frame PROJECT_DIRECTORY ACTION_INDEX FRAME TEMPLATE_ACTION TEMPLATE_OFFSET\n"
        "  project-move-event PROJECT_DIRECTORY ACTION_INDEX SOURCE_OFFSET FRAME_DELTA\n"
        "  project-hitbox-intervals PROJECT_DIRECTORY [ACTION_INDEX]\n"
        "  project-insert-command PROJECT_DIRECTORY ACTION_INDEX AFTER_OFFSET "
        "TEMPLATE_ACTION TEMPLATE_OFFSET\n"
        "  project-delete-command PROJECT_DIRECTORY ACTION_INDEX "
        "SOURCE_OFFSET\n"
        "  project-localize-command PROJECT_DIRECTORY ACTION_INDEX "
        "SOURCE_OFFSET\n"
        "  project-action-flags PROJECT_DIRECTORY ACTION_INDEX [FLAGS]\n"
        "  project-animation PROJECT_DIRECTORY ACTION_INDEX [TRACK_INDEX "
        "[KEY_INDEX VALUE]]\n"
        "  project-animation-tangent PROJECT_DIRECTORY ACTION_INDEX "
        "TRACK_INDEX KEY_INDEX VALUE\n"
        "  project-animation-move PROJECT_DIRECTORY ACTION_INDEX TRACK_INDEX "
        "KEY_INDEX FRAME\n"
        "  project-animation-insert PROJECT_DIRECTORY ACTION_INDEX "
        "TRACK_INDEX FRAME VALUE\n"
        "  project-animation-key-bone PROJECT_DIRECTORY ACTION_INDEX JOINT "
        "CHANNEL FRAME VALUE DEFAULT_VALUE\n"
        "  project-animation-delete PROJECT_DIRECTORY ACTION_INDEX "
        "TRACK_INDEX KEY_INDEX\n"
        "  project-animation-frames PROJECT_DIRECTORY ACTION_INDEX END_FRAME\n"
        "  project-pose-paste PROJECT_DIRECTORY SOURCE_ACTION SOURCE_FRAME "
        "DEST_ACTION DEST_FRAME\n"
        "  project-animation-sample PROJECT_DIRECTORY ACTION_INDEX "
        "TRACK_INDEX FRAME\n"
        "  project-animation-undo PROJECT_DIRECTORY\n"
        "  project-event PROJECT_DIRECTORY ACTION_INDEX OFFSET VALUE\n"
        "  project-copy-normal SOURCE_PROJECT DEST_PROJECT ACTION_INDEX\n"
        "  project-copy-normal-group SOURCE_PROJECT DEST_PROJECT "
        "jab|forward-tilt|forward-smash|neutral-air|forward-air|back-air|up-"
        "air|down-air\n"
        "  project-hitbox-power PROJECT_DIRECTORY DAMAGE GROWTH BASE\n"
        "  project-hitbox PROJECT_DIRECTORY ACTION_INDEX OFFSET [FIELD "
        "VALUE]\n"
        "  project-sound PROJECT_DIRECTORY ACTION_INDEX OFFSET [SFX_ID VOLUME "
        "PAN]\n"
        "  project-effect PROJECT_DIRECTORY ACTION_INDEX OFFSET [FIELD "
        "VALUE]\n"
        "  project-undo PROJECT_DIRECTORY\n"
        "  module-probe DLL_FILE\n"
        "  source-check PROJECT_DIRECTORY SCRIPT_FILE\n"
        "  source-audit PROJECT_DIRECTORY SCRIPT_FILE\n"
        "  script-build PROJECT_DIRECTORY SCRIPT_FILE\n"
        "  export-fighter-facts TEMPLATE OUTPUT.fma\n"
        "  import-cpu PROJECT_DIRECTORY ISO STOCK_FIGHTER\n"
        "  import-parameters PROJECT_DIRECTORY SCRIPT_FILE OUTPUT_SCRIPT\n"
        "  package-project PROJECT_DIRECTORY PORTRAIT_PNG\n"
        "  verify-package PACKAGE_ID\n"
        "  inspect-fighter DAT ROOT ACTIONS DEMOS COSTUMES [ANIMATION_DAT]\n"
        "  inspect-costume DAT JOINT_SYMBOL MATANIM_SYMBOL\n"
        "  list\n"
        "\n"
        "CHARACTER is a stock fighter name (quote names with spaces);\n"
        "BASE_COLOR is the original costume index whose symbols the DAT "
        "uses.\n"
        "Cache alone never makes a package selectable. Accept installs it.\n"
        "Fighter selection additionally needs client roster support.\n",
        stderr);
}

static bool parse_count(const char* text, unsigned maximum, u32* value)
{
    char* end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (*text == '\0' || *end != '\0' || parsed == 0 ||
        parsed > maximum) {
        return false;
    }
    *value = (u32) parsed;
    return true;
}

static bool parse_number(const char* text, u32 maximum, u32* value)
{
    char* end = NULL;
    if (text == NULL || *text == '\0' || *text == '-') return false;
    errno = 0;
    unsigned long parsed = strtoul(text, &end, 0);
    if (errno != 0 || *end != '\0' || parsed > maximum) {
        return false;
    }
    *value = (u32) parsed;
    return true;
}

static int character_from_name(const char* name)
{
    for (int i = 0; i < CKind_Playable_Count; ++i) {
        if (_stricmp(name, stock_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static bool parse_color(const char* text, long* color)
{
    char* end = NULL;
    *color = strtol(text, &end, 10);
    return *text != '\0' && *end == '\0' && *color >= 0 && *color <= 255;
}

static bool read_file(const char* path, uint8_t** bytes, size_t* size)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) {
            fclose(file);
        }
        return false;
    }
    long length = ftell(file);
    if (length < 0 || length > NATIVE_CONTENT_MAX_PACKAGE_BYTES ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    uint8_t* data = malloc((size_t) length + 1);
    bool ok = data != NULL &&
              fread(data, 1, (size_t) length, file) == (size_t) length;
    ok = fclose(file) == 0 && ok;
    if (!ok) {
        free(data);
        return false;
    }
    *bytes = data;
    *size = (size_t) length;
    return true;
}

static bool write_file(const char* path, const void* bytes, size_t size)
{
    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    bool ok = fwrite(bytes, 1, size, file) == size && fflush(file) == 0;
    ok = fclose(file) == 0 && ok;
    if (!ok) {
        DeleteFileA(path);
    }
    return ok;
}

static int inspect_fighter(const char* path, const char* root_name,
                           const char* actions_text, const char* demos_text,
                           const char* costumes_text,
                           const char* animation_path)
{
    u32 actions, demos, costumes;
    if (!parse_count(actions_text, 1024, &actions) ||
        !parse_count(demos_text, 128, &demos) ||
        !parse_count(costumes_text, 8, &costumes)) {
        fputs("Invalid fighter action, demo, or costume count.\n", stderr);
        return 1;
    }
    uint8_t* bytes = NULL;
    size_t size = 0;
    if (!read_file(path, &bytes, &size)) {
        fprintf(stderr, "Could not read fighter DAT: %s\n", path);
        return 1;
    }
    void* arena = malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);
    FtCharacterAssets* assets = calloc(1, sizeof(*assets));
    bool valid = arena != NULL && assets != NULL &&
                 ftCharacterAssets_Import(
                     bytes, size, &(FtCharacterAssetLayout){ 0 }, root_name, actions, demos,
                     costumes, arena, FT_CHARACTER_IMPORT_ARENA_SIZE,
                     FT_CHARACTER_IMPORT_RECORD_CAPACITY, assets);
    if (!valid) {
        if (arena != NULL && assets != NULL) {
            fprintf(stderr,
                    "Fighter DAT import failed: status=%d offset=0x%lx "
                    "records=%lu external=%s\n",
                    assets->import.status,
                    (unsigned long) assets->import.error_offset,
                    (unsigned long) assets->import.record_count,
                    assets->import.external_symbol != NULL
                        ? assets->import.external_symbol : "-");
        } else {
            fputs("Could not reserve fighter import memory.\n", stderr);
        }
    } else {
        printf("Fighter DAT imported: %lu actions, %lu demos, %lu "
               "costumes, %lu records, %lu action instructions\n",
               (unsigned long) actions, (unsigned long) demos,
               (unsigned long) costumes,
               (unsigned long) assets->import.record_count,
               (unsigned long) assets->action_stats.instruction_count);
        printf("Fighter model scaling: %.5f\n",
               assets->common_attributes.model_scaling);
    }
    if (valid && animation_path != NULL) {
        uint8_t* animation_bytes = NULL;
        size_t animation_size = 0;
        void* animation_arena = malloc(FT_ANIMATION_IMPORT_ARENA_SIZE);
        FtAnimationBankStats stats = { 0 };
        bool readable = read_file(animation_path, &animation_bytes,
                                  &animation_size);
        bool imported = readable && animation_arena != NULL &&
                ftAnimationBank_Import(
                    animation_bytes, animation_size,
                    (const FtAnimationBankTable[]){
                        { assets->data.xC, actions, false },
                        { assets->data.x14, demos, true } }, 2, animation_arena,
                    FT_ANIMATION_IMPORT_ARENA_SIZE, &stats);
        valid = imported;
        if (valid) {
            printf("Animation bank imported: %lu clips, %lu maximum "
                   "tracks, %zu arena bytes\n",
                   (unsigned long) stats.clip_count,
                   (unsigned long) stats.max_tracks, stats.arena_used);
        } else if (!readable) {
            fprintf(stderr, "Could not read animation DAT: %s\n",
                    animation_path);
        } else if (animation_arena == NULL) {
            fputs("Could not reserve animation import memory.\n", stderr);
        } else {
            const struct Fighter_WaitAnimData* entry =
                stats.failed_index < actions
                    ? &assets->data.xC[stats.failed_index] : NULL;
            fprintf(stderr,
                    "Animation bank import failed: clip=%lu status=%d "
                    "offset=%ld size=%ld name=%s\n",
                    (unsigned long) stats.failed_index, stats.status,
                    entry != NULL ? (long) entry->x4 : -1L,
                    entry != NULL ? (long) entry->x8 : -1L,
                    entry != NULL && entry->x0 != NULL
                        ? entry->x0 : "-");
        }
        free(animation_arena);
        free(animation_bytes);
    }
    free(assets);
    free(arena);
    free(bytes);
    return valid ? 0 : 1;
}

static int inspect_costume(const char* path, const char* joint_name,
                           const char* matanim_name)
{
    uint8_t* bytes = NULL;
    size_t size = 0;
    if (!read_file(path, &bytes, &size)) {
        fprintf(stderr, "Could not read costume DAT: %s\n", path);
        return 1;
    }
    FtCostumeAssets* assets = calloc(1, sizeof(*assets));
    bool valid = assets != NULL && ftCostumeAssets_Load(
        bytes, size, joint_name, matanim_name, assets);
    if (valid) {
        printf("Costume DAT imported: %lu records, %zu arena bytes, "
               "%u joints, root scale %.4f %.4f %.4f\n",
               (unsigned long) assets->import.record_count,
               assets->import.arena_used,
               (unsigned) assets->joint_count,
               assets->joint->scale.x, assets->joint->scale.y,
               assets->joint->scale.z);
    } else if (assets != NULL) {
        fprintf(stderr,
                "Costume DAT import failed: status=%d offset=0x%lx "
                "records=%lu external=%s\n",
                assets->import.status,
                (unsigned long) assets->import.error_offset,
                (unsigned long) assets->import.record_count,
                assets->import.external_symbol != NULL
                    ? assets->import.external_symbol : "-");
    } else {
        fputs("Could not reserve costume import memory.\n", stderr);
    }
    if (assets) free(assets->arena);
    free(assets);
    free(bytes);
    return valid ? 0 : 1;
}



static bool valid_package(const char* root, const uint8_t* package, size_t size,
                          NativeCostumeValidation* validation)
{
    NativeCostumePackage costume;
    NativeCostumePackageData costume_data;
    if (nativeContentStore_ParseCostume(package, size, &costume, &costume_data)) {
        return nativeCostumePackage_Validate(&costume, &costume_data, root,
                                             validation);
    }
    NativeFighterPackage fighter;
    NativeFighterPackageData fighter_data;
    return nativeContentStore_ParseFighter(package, size, &fighter,
                                           &fighter_data);
}

static int import_costume(const char* root, const char* path,
                          const char* character_name, const char* color_text,
                          const char* title)
{
    int character = character_from_name(character_name);
    long color;
    if (character < 0 || !parse_color(color_text, &color) ||
        strlen(title) >= NATIVE_CONTENT_TITLE_BYTES) {
        fputs("Invalid character, base costume, or title.\n", stderr);
        return 1;
    }
    uint8_t* dat = NULL;
    size_t dat_size = 0;
    if (!read_file(path, &dat, &dat_size)) {
        fprintf(stderr, "Could not read costume DAT: %s\n", path);
        return 1;
    }
    NativeCostumePackage metadata = {
        .member_count = 1,
        .character = (uint8_t) character,
        .base_costume = (uint8_t) color,
    };
    strcpy(metadata.title, title);
    NativeCostumeValidation validation = { 0 };
    NativeContentId id;
    bool ok = nativeCostumeImport_Install(root, &metadata,
        &(NativeCostumePackageData){.members={{dat,dat_size}}},
                                          &id, &validation);
    if (!ok) {
        fprintf(stderr,
                "Could not import or install costume for %s color %ld "
                "(status=%u offset=0x%zX records=%zu).\n",
                character_name, color, validation.import_status,
                validation.error_offset, validation.imported_records);
        free(dat);
        return 1;
    }
    free(dat);
    char hex[65];
    nativeContentStore_IdHex(&id, hex);
    printf("Installed %s for %s (base color %ld)\nID: %s\n",
           title, character_name, color, hex);
    return 0;
}

static int import_custom_costume(const char* root, const char* path,
                                 const char* fighter_hex, const char* title,
                                 const char* variant)
{
    NativeCostumePackage metadata = { 0 };
    if (variant != NULL) {
        char* end = NULL;
        unsigned long value = strtoul(variant, &end, 10);
        if (end == variant || *end != '\0' || value > UINT8_MAX) {
            fputs("Model variant must be an integer from 0 to 255.\n", stderr);
            return 1;
        }
        metadata.model_variant = (uint8_t) value;
    }
    if (!nativeContentStore_IdFromHex(fighter_hex, &metadata.fighter_id) ||
        !nativeContentStore_IsInstalled(root, &metadata.fighter_id) ||
        strlen(title) == 0 || strlen(title) >= NATIVE_CONTENT_TITLE_BYTES) {
        fputs("An installed fighter ID and nonempty title are required.\n",
              stderr);
        return 1;
    }
    uint8_t* parent = NULL;
    size_t parent_size = 0;
    NativeFighterPackage family;
    NativeFighterPackageData family_data;
    if (!nativeContentStore_Read(root, &metadata.fighter_id, &parent, &parent_size) ||
        !nativeContentStore_ParseFighter(parent, parent_size, &family, &family_data)) {
        free(parent);
        fputs("Could not read the parent fighter family.\n", stderr);
        return 1;
    }
    metadata.member_count = family.member_count;
    free(parent);
    NativeCostumePackageData data = {0};
    uint8_t* owned[NATIVE_FIGHTER_FAMILY_MEMBERS] = {0};
    bool ok = true;
    DWORD attributes = GetFileAttributesA(path);
    bool directory = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
    if (!directory && metadata.member_count != 1) {
        fputs("A paired outfit needs a directory containing MEMBER_KEY.dat for every family member.\n", stderr);
        return 1;
    }
    for (unsigned m = 0; ok && m < metadata.member_count; ++m) {
        memcpy(metadata.member_keys[m], family.members[m].key, NATIVE_FIGHTER_CODE_KEY_BYTES);
        char filename[MAX_PATH];
        int length = directory ? snprintf(filename, sizeof(filename), "%s/%s.dat", path, family.members[m].key)
                               : snprintf(filename, sizeof(filename), "%s", path);
        ok = length > 0 && length < sizeof(filename) &&
             read_file(filename, &owned[m], &data.members[m].size);
        data.members[m].dat = owned[m];
        if (!ok) fprintf(stderr, "Could not read outfit model for member %s.\n", family.members[m].key);
    }
    strcpy(metadata.title, title);
    NativeCostumeValidation validation = {0};
    NativeContentId id;
    ok = ok && nativeCostumeImport_Install(root, &metadata, &data, &id, &validation);
    for (unsigned m = 0; m < metadata.member_count; ++m) free(owned[m]);
    if (!ok) {
        fprintf(stderr,
                "Costume does not match that fighter or installation failed "
                "(status=%u offset=0x%zX records=%zu).\n",
                validation.import_status, validation.error_offset,
                validation.imported_records);
        return 1;
    }
    char hex[65];
    nativeContentStore_IdHex(&id, hex);
    printf("Installed %s for fighter %s\nID: %s\n",
           title, fighter_hex, hex);
    return 0;
}

static int clone_stock(const char* iso, const char* base, const char* path,
                       const char* title)
{
    FighterKind kind = Ft_Kind_None;
    for (FighterKind candidate = Ft_Kind_Mario;
         candidate < Ft_Kind_StockCount; ++candidate) {
        const char* name = nativeFighterProject_StockBaseName(candidate);
        if (name != NULL && _stricmp(name, base) == 0) {
            kind = candidate;
            break;
        }
    }
    if (kind == Ft_Kind_None) {
        fprintf(stderr, "Unsupported stock clone base: %s\n", base);
        return 1;
    }
    if (!aurora_dvd_open(iso)) {
        fputs("Could not open the Melee disc.\n", stderr);
        return 1;
    }
    bool ok = nativeFighterProject_CreateStockFromDisc(path, title, kind);
    aurora_dvd_close();
    if (!ok) {
        fputs("Could not create the independent stock draft.\n", stderr);
        return 1;
    }
    printf("Copied %s asset draft: %s. Source validation is required before packaging.\n", base, path);
    return 0;
}

static int clone_installed(const char* root, const char* hex,
                           const char* path, const char* title)
{
    NativeContentId id;
    uint8_t* package = NULL;
    size_t size = 0;
    if (!nativeContentStore_IdFromHex(hex, &id) ||
        !nativeContentStore_IsInstalled(root, &id) ||
        !nativeContentStore_Read(root, &id, &package, &size)) {
        fputs("Only an installed fighter can be cloned to a draft.\n", stderr);
        return 1;
    }
    bool ok = nativeFighterFamilyProject_CreateFromPackage(path, title, package,
                                                           size);
    free(package);
    if (!ok) {
        fputs("Could not create a draft from this fighter.\n",
              stderr);
        return 1;
    }
    printf("Created editable fighter draft: %s\n", path);
    return 0;
}

static int inspect_project(const char* path)
{
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    printf("Draft: %s\nBase: %s\nFighter DAT: %zu bytes\n"
           "Animation DAT: %zu bytes\nCostume DAT: %zu bytes\n",
           project.title, project.base_kind == Ft_Kind_None ? "Rook" :
               nativeFighterProject_StockBaseName(project.base_kind),
           project.fighter_size, project.animation_size,
           project.costume_size);
    nativeFighterProject_Close(&project);
    return 0;
}

static int project_attribute(const char* path, const char* property_name,
                             const char* requested_value)
{
    size_t property = NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT;
    for (size_t i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i)
        if (strcmp(property_name, nativeFighterCommonAttributes[i].name) == 0) { property = i; break; }
    if (property == NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT) {
        fputs("Unknown common attribute. Use the schema name shown in the Workshop.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    double value = 0.0;
    bool ok = nativeFighterProject_GetAttribute(&project, property, &value);
    if (ok && requested_value != NULL) {
        char* end;
        double next = strtod(requested_value, &end);
        ok = *requested_value != '\0' && *end == '\0' &&
             nativeFighterProject_SetAttribute(&project, property, next) &&
             nativeFighterProject_Save(path, &project) &&
             nativeFighterProject_GetAttribute(&project, property, &value);
    }
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not read or save this fighter property.\n", stderr);
        return 1;
    }
    printf("%s: %.6g\n", property_name, value);
    return 0;
}

static int project_actions(const char* path, const char* action_text)
{
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    NativeFighterActionInfo actions[NATIVE_FIGHTER_ACTION_CAPACITY];
    size_t count;
    bool ok = nativeFighterAction_List(&project, actions,
                                       NATIVE_FIGHTER_ACTION_CAPACITY,
                                       &count);
    u32 action = 0;
    if (ok && action_text != NULL) {
        ok = parse_number(action_text, (u32) count - 1, &action);
    }
    if (ok && action_text == NULL) {
        for (size_t i = 0; i < count; ++i) {
            if (actions[i].has_script) {
                printf("%u %s\n", actions[i].index, actions[i].name);
            }
        }
    } else if (ok) {
        NativeFighterActionEvent events[256];
        size_t event_count;
        ok = nativeFighterAction_Events(&project, action, events, 256,
                                        &event_count);
        if (ok) {
            printf("%lu %s\n", action, actions[action].name);
            for (size_t i = 0; i < event_count; ++i) {
                const NativeFighterActionEvent* event = &events[i];
                const char* kind =
                    event->kind == NativeFighterActionEvent_SyncTimer
                        ? "wait"
                    : event->kind == NativeFighterActionEvent_AsyncTimer
                        ? "at-frame"
                    : event->kind == NativeFighterActionEvent_CreateHitbox
                        ? "hitbox"
                    : event->kind == NativeFighterActionEvent_SetHitboxDamage
                        ? "hitbox-damage"
                    : event->kind == NativeFighterActionEvent_SetHitboxScale
                        ? "hitbox-scale"
                    : event->kind == NativeFighterActionEvent_SetHitboxFlag
                        ? "hitbox-flag"
                    : event->kind == NativeFighterActionEvent_SetCommandVar
                        ? "command-var"
                    : event->kind == NativeFighterActionEvent_SetBodyCollisionState
                        ? "body-collision-state"
                    : event->kind == NativeFighterActionEvent_SetGlobalHurtState
                        ? "hurt-state"
                    : event->kind == NativeFighterActionEvent_SetBoneHurtState
                        ? "bone-hurt-state"
                    : event->kind == NativeFighterActionEvent_SoundEffect
                        ? "sound-effect"
                    : event->kind == NativeFighterActionEvent_SpawnEffect
                        ? "spawn-effect"
                    : event->kind == NativeFighterActionEvent_ThrowHitbox
                        ? "throw-hitbox"
                    : event->kind == NativeFighterActionEvent_ReleaseThrow
                        ? "release-throw"
                    : event->kind == NativeFighterActionEvent_ReverseFacing
                        ? "reverse-facing"
                    : event->kind == NativeFighterActionEvent_SetAirborneState
                        ? "ground-air-state"
                    : event->kind == NativeFighterActionEvent_AllowInterrupt
                        ? "allow-interrupt"
                    : event->kind == NativeFighterActionEvent_NeutralCue ? "neutral-cue" : "command";
                printf("0x%X %s %u", event->source_offset, kind,
                       event->value);
                if (event->kind == NativeFighterActionEvent_CreateHitbox ||
                    event->kind == NativeFighterActionEvent_SetHitboxDamage ||
                    event->kind == NativeFighterActionEvent_SetHitboxScale ||
                    event->kind == NativeFighterActionEvent_SetHitboxFlag ||
                    event->kind == NativeFighterActionEvent_ThrowHitbox) {
                    printf(" id=%u", event->target_id);
                }
                if (event->kind == NativeFighterActionEvent_Command) printf(" opcode=%u", event->opcode);
                if (event->kind == NativeFighterActionEvent_SetCommandVar)
                    printf(" var=%u", event->target_id);
                if (event->kind == NativeFighterActionEvent_SetBoneHurtState)
                    printf(" bone=%u", event->target_id);
                if (event->kind == NativeFighterActionEvent_SoundEffect)
                    printf(" behavior=%u", event->target_id);
                if (event->kind == NativeFighterActionEvent_SpawnEffect)
                    printf(" bone=%u", event->target_id);
                if (event->kind == NativeFighterActionEvent_SetHitboxFlag)
                    printf(" type=%u enabled=%u", event->value >> 1,
                           event->value & 1);
                putchar('\n');
            }
        }
    }
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not inspect this action's commands.\n", stderr);
    }
    return ok ? 0 : 1;
}

static int project_event(const char* path, const char* action_text,
                         const char* offset_text, const char* value_text)
{
    u32 action, offset, value;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(offset_text, UINT32_MAX, &offset) ||
        !parse_number(value_text, UINT32_MAX, &value)) {
        fputs("Invalid action, source offset, or value.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    bool ok = nativeFighterAction_SetValue(&project, action, offset, value) &&
              nativeFighterProject_Save(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not save that action command.\n", stderr);
    }
    return ok ? 0 : 1;
}

static int project_move_event(const char* path, const char* action_text,
                              const char* source_text, const char* delta_text)
{
    u32 action, source;
    char* end;
    const long delta = strtol(delta_text, &end, 10);
    if (!parse_number(action_text, NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(source_text, UINT32_MAX, &source) || !*delta_text || *end ||
        delta < -1000 || delta > 1000) return 1;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return 1;
    uint32_t moved;
    const bool ok = nativeFighterAction_EditEventTiming(&project, action, source, (int) delta, 0, &moved) &&
                    nativeFighterProject_Save(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not move this event within its structured timeline.\n", stderr);
        return 1;
    }
    printf("Moved event to source 0x%X.\n", (unsigned) moved);
    return 0;
}

static int project_hitbox_intervals(const char* path, const char* action_text)
{
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return 1;
    unsigned converted = 0;
    bool ok = true;
    if (action_text) {
        u32 action;
        ok = parse_number(action_text, project.action_count - 1, &action) &&
            nativeFighterAction_ConvertHitboxIntervals(&project, action, &converted);
    } else nativeFighterAction_ConvertAllHitboxIntervals(&project, &converted);
    if (ok && converted) ok = nativeFighterProject_Save(path, &project);
    nativeFighterProject_Close(&project);
    printf("Converted %u static hitbox intervals; unresolved actions retain their events.\n", converted);
    return ok ? 0 : 1;
}

static int project_insert_command(const char* path,
                                  const char* action_text,
                                  const char* after_text,
                                  const char* template_action_text,
                                  const char* template_offset_text, bool at_frame)
{
    u32 action, after, template_action, template_offset;
    uint32_t inserted;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(after_text, UINT32_MAX, &after) ||
        !parse_number(template_action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1,
                      &template_action) ||
        !parse_number(template_offset_text, UINT32_MAX,
                      &template_offset)) {
        fputs("Invalid action or source offset.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    const bool ok = (at_frame ? nativeFighterAction_InsertAtFrame(&project, action, after, template_action, template_offset, &inserted)
                              : nativeFighterAction_InsertCommand(&project, action, after, template_action, template_offset, &inserted)) &&
                    nativeFighterProject_Save(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not insert this command or rebuild its source graph.\n",
              stderr);
        return 1;
    }
    printf("Inserted action %u command at source 0x%X.\n",
           (unsigned) action, (unsigned) inserted);
    return 0;
}

static int project_delete_command(const char* path,
                                  const char* action_text,
                                  const char* offset_text)
{
    u32 action, offset;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(offset_text, UINT32_MAX, &offset)) {
        fputs("Invalid action or source offset.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    const bool ok = nativeFighterAction_DeleteCommand(&project, action,
                                                       offset) &&
                    nativeFighterProject_Save(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not delete this command or rebuild its source graph.\n",
              stderr);
        return 1;
    }
    printf("Deleted action %u command at source 0x%X.\n",
           (unsigned) action, (unsigned) offset);
    return 0;
}

static int project_localize_command(const char* path,
                                    const char* action_text,
                                    const char* offset_text)
{
    u32 action, offset;
    uint32_t local;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(offset_text, UINT32_MAX, &offset)) {
        fputs("Invalid action or source offset.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    const bool ok = nativeFighterAction_LocalizeCommand(
                        &project, action, offset, &local) &&
                    nativeFighterProject_Save(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not localize this action's source graph.\n", stderr);
        return 1;
    }
    printf("Localized action %u command at source 0x%X.\n",
           (unsigned) action, (unsigned) local);
    return 0;
}

static int project_copy_normal(const char* source_path,
                               const char* destination_path,
                               const char* choice, bool group)
{
    u32 action = 0;
    static const char* const group_names[] = {
        "jab", "forward-tilt", "forward-smash", "neutral-air",
        "forward-air", "back-air", "up-air", "down-air"
    };
    NativeFighterMoveGroup move_group = NativeFighterMoveGroup_Jab;
    bool selected = !group && parse_number(
        choice, NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action);
    if (group) {
        for (unsigned i = 0; i < sizeof(group_names) / sizeof(group_names[0]);
             ++i) {
            if (strcmp(choice, group_names[i]) == 0) {
                move_group = (NativeFighterMoveGroup) i;
                selected = true;
                break;
            }
        }
    }
    char source_absolute[MAX_PATH], destination_absolute[MAX_PATH];
    DWORD source_length = GetFullPathNameA(source_path, MAX_PATH,
                                           source_absolute, NULL);
    DWORD destination_length = GetFullPathNameA(destination_path, MAX_PATH,
                                                destination_absolute, NULL);
    if (!selected ||
        source_length == 0 || source_length >= MAX_PATH ||
        destination_length == 0 || destination_length >= MAX_PATH ||
        _stricmp(source_absolute, destination_absolute) == 0) {
        fputs("Invalid source, destination, or action.\n", stderr);
        return 1;
    }
    NativeFighterProject source, destination;
    if (!nativeFighterProject_Open(source_path, &source)) {
        fputs("Could not open the source draft.\n", stderr);
        return 1;
    }
    if (!nativeFighterProject_Open(destination_path, &destination)) {
        nativeFighterProject_Close(&source);
        fputs("Could not open the destination draft.\n", stderr);
        return 1;
    }
    bool ok = group
        ? nativeFighterMove_CopyNormalGroup(&source, &destination, move_group)
        : nativeFighterMove_CopyNormalAction(&source, &destination, action);
    bool animation_saved = ok &&
        nativeFighterProject_SaveAnimation(destination_path, &destination);
    bool fighter_saved = animation_saved &&
        nativeFighterProject_Save(destination_path, &destination);
    nativeFighterProject_Close(&destination);
    nativeFighterProject_Close(&source);
    if (!animation_saved) {
        fputs("Could not copy this independent, stock-shaped normal action.\n",
              stderr);
        return 1;
    }
    if (group)
        printf("Copied %s's original action variants and animations.\n",
               choice);
    else
        printf("Copied original action %lu, its command graph and animation.\n",
               action);
    return 0;
}

static int project_action_flags(const char* path, const char* action_text,
                                const char* flags_text)
{
    u32 action, flags;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        (flags_text != NULL && !parse_number(flags_text, UINT32_MAX, &flags))) {
        fputs("Invalid action index or flags word.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    NativeFighterActionMetadata metadata;
    bool ok = nativeFighterAction_GetMetadata(&project, action, &metadata);
    if (ok && flags_text != NULL) {
        ok = nativeFighterAction_SetAnimationFlags(&project, action, flags) &&
             nativeFighterProject_Save(path, &project) &&
             nativeFighterAction_GetMetadata(&project, action, &metadata);
    }
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not read or save that action's flags.\n", stderr);
        return 1;
    }
    printf("action %lu animation offset=%d size=%d flags=0x%08X\n",
           (unsigned long) action, metadata.animation_offset,
           metadata.animation_size, metadata.animation_flags);
    return 0;
}

static bool hitbox_field(NativeFighterHitbox* h, const char* name,
                         const char* text)
{
    char* end = NULL;
    if (text == NULL || *text == '\0') return false;
    errno = 0;
    long value = strtol(text, &end, 0);
    if (errno != 0 || *end != '\0') return false;
#define HITBOX_U(field, maximum) \
    if (strcmp(name, #field) == 0) { \
        if (value < 0 || value > (maximum)) return false; \
        h->field = value; return true; \
    }
#define HITBOX_S(field, minimum, maximum) \
    if (strcmp(name, #field) == 0) { \
        if (value < (minimum) || value > (maximum)) return false; \
        h->field = value; return true; \
    }
    HITBOX_U(hit_group, 7)
    HITBOX_U(only_hit_grabbed, 1)
    HITBOX_U(bone, 255)
    HITBOX_U(use_common_bone_ids, 1)
    HITBOX_U(damage, 1023)
    HITBOX_U(radius, 65535)
    HITBOX_S(offset_x, -32768, 32767)
    HITBOX_S(offset_y, -32768, 32767)
    HITBOX_S(offset_z, -32768, 32767)
    HITBOX_U(angle, 511)
    HITBOX_U(knockback_growth, 511)
    HITBOX_U(weight_set_knockback, 511)
    HITBOX_U(item_hit_interaction, 1)
    HITBOX_U(ignore_thrown_fighters, 1)
    HITBOX_U(ignore_fighter_scale, 1)
    HITBOX_U(clank, 1)
    HITBOX_U(rebound, 1)
    HITBOX_U(base_knockback, 511)
    HITBOX_U(element, 31)
    HITBOX_S(shield_damage, -128, 127)
    HITBOX_U(hit_sfx_severity, 7)
    HITBOX_U(hit_sfx_kind, 31)
    HITBOX_U(hit_grounded, 1)
    HITBOX_U(hit_aerial, 1)
    HITBOX_U(duration256, 256000)
    HITBOX_U(refresh_hit_eligibility, 1)
#undef HITBOX_U
#undef HITBOX_S
    return false;
}

static int project_hitbox(const char* path, const char* action_text,
                          const char* offset_text, const char* field,
                          const char* value)
{
    u32 action, offset;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(offset_text, UINT32_MAX, &offset)) {
        fputs("Invalid action or source offset.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    NativeFighterHitbox h;
    bool ok = nativeFighterAction_GetHitbox(&project, action, offset, &h);
    if (ok && field != NULL) {
        ok = hitbox_field(&h, field, value) &&
             nativeFighterAction_SetHitbox(&project, action, offset, &h, NULL) &&
             nativeFighterProject_Save(path, &project);
    }
    if (ok) {
        printf("id=%u group=%u bone=%u common_bone=%u only_grabbed=%u "
               "damage=%g radius_raw=%u offset_raw=(%d,%d,%d)\n"
               "angle=%u kbg=%u weight_set_kb=%u base_kb=%u "
               "element=%u shield_damage=%d ground=%u air=%u\n"
               "item=%u ignore_thrown=%u ignore_scale=%u clank=%u "
               "rebound=%u sfx_severity=%u sfx_kind=%u\n",
               h.id, h.hit_group, h.bone, h.use_common_bone_ids,
               h.only_hit_grabbed, h.damage, h.radius,
               h.offset_x, h.offset_y, h.offset_z,
               h.angle, h.knockback_growth, h.weight_set_knockback,
               h.base_knockback, h.element, h.shield_damage,
               h.hit_grounded, h.hit_aerial, h.item_hit_interaction,
               h.ignore_thrown_fighters, h.ignore_fighter_scale,
               h.clank, h.rebound, h.hit_sfx_severity, h.hit_sfx_kind);
        if (h.timed) printf("duration=%g duration256=%u refresh_hit_eligibility=%u\n",
                            h.duration256 / 256.0, h.duration256, h.refresh_hit_eligibility);
    }
    nativeFighterProject_Close(&project);
    if (!ok) fputs("Could not read or save that hitbox.\n", stderr);
    return ok ? 0 : 1;
}

static int project_sound(const char* path, const char* action_text,
                         const char* offset_text, const char* id_text,
                         const char* volume_text, const char* pan_text)
{
    u32 action, offset, id, volume, pan;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(offset_text, UINT32_MAX, &offset) ||
        (id_text != NULL &&
         (!parse_number(id_text, UINT32_MAX, &id) ||
          !parse_number(volume_text, 255, &volume) ||
          !parse_number(pan_text, 255, &pan)))) {
        fputs("Invalid action, sound offset, ID, volume, or pan.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    NativeFighterSoundEffect sound;
    bool ok = nativeFighterAction_GetSoundEffect(&project, action, offset,
                                                 &sound);
    if (ok && id_text != NULL) {
        sound.sfx_id = id;
        sound.volume = (u8) volume;
        sound.panning = (u8) pan;
        ok = nativeFighterAction_SetSoundEffect(&project, action, offset,
                                                &sound) &&
             nativeFighterProject_Save(path, &project);
    }
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not read or save that sound command.\n", stderr);
        return 1;
    }
    printf("behavior=%u id=%u volume=%u pan=%u\n", sound.behavior,
           sound.sfx_id, sound.volume, sound.panning);
    return 0;
}

static bool effect_field(NativeFighterSpawnEffect* effect, const char* name,
                         const char* text)
{
    if (text == NULL || *text == '\0') return false;
    char* end = NULL;
    errno = 0;
    long value = strtol(text, &end, 0);
    if (errno != 0 || *end != '\0') return false;
#define EFFECT_U(field, maximum) \
    if (strcmp(name, #field) == 0) { \
        if (value < 0 || value > (maximum)) return false; \
        effect->field = value; return true; \
    }
#define EFFECT_S(field) \
    if (strcmp(name, #field) == 0) { \
        if (value < -32768 || value > 32767) return false; \
        effect->field = value; return true; \
    }
    EFFECT_U(bone, 255)
    EFFECT_U(use_common_bone_ids, 1)
    EFFECT_U(destroy_on_state_change, 1)
    EFFECT_U(use_unknown_bone, 1)
    EFFECT_U(gfx_id, 65535)
    EFFECT_S(offset_z)
    EFFECT_S(offset_y)
    EFFECT_S(offset_x)
    EFFECT_U(range_z, 65535)
    EFFECT_U(range_y, 65535)
    EFFECT_U(range_x, 65535)
#undef EFFECT_U
#undef EFFECT_S
    return false;
}

static int project_effect(const char* path, const char* action_text,
                          const char* offset_text, const char* field,
                          const char* value)
{
    u32 action, offset;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(offset_text, UINT32_MAX, &offset)) {
        fputs("Invalid action or effect source offset.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    NativeFighterSpawnEffect effect;
    bool ok = nativeFighterAction_GetSpawnEffect(&project, action, offset,
                                                 &effect);
    if (ok && field != NULL) {
        ok = effect_field(&effect, field, value) &&
             nativeFighterAction_SetSpawnEffect(&project, action, offset,
                                                &effect) &&
             nativeFighterProject_Save(path, &project);
    }
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not read or save that effect command.\n", stderr);
        return 1;
    }
    printf("gfx_id=%u bone=%u common_bone=%u unknown_bone=%u "
           "destroy_on_state_change=%u offset_raw=(%d,%d,%d) "
           "range_raw=(%u,%u,%u)\n",
           effect.gfx_id, effect.bone, effect.use_common_bone_ids,
           effect.use_unknown_bone, effect.destroy_on_state_change,
           effect.offset_z, effect.offset_y, effect.offset_x,
           effect.range_z, effect.range_y, effect.range_x);
    return 0;
}

static int project_undo(const char* path)
{
    if (!nativeFighterProject_UndoLastSave(path)) {
        fputs("No valid previous fighter DAT could be restored.\n", stderr);
        return 1;
    }
    puts("Restored the previous fighter DAT.");
    return 0;
}

static int project_animation(const char* path, const char* action_text,
                             const char* track_text, const char* key_text,
                             const char* value_text, bool tangent)
{
    u32 action, track = 0, key = 0;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        (track_text != NULL && !parse_number(track_text, 4095, &track)) ||
        (key_text != NULL && !parse_number(key_text, 65535, &key))) {
        fputs("Invalid animation action, track, or key index.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    NativeFighterAnimationInfo info;
    bool ok = nativeFighterAnimation_Info(&project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, &info);
    if (ok && value_text != NULL) {
        char* end;
        errno = 0;
        float value = strtof(value_text, &end);
        ok = *value_text != '\0' && *end == '\0' && errno == 0;
        if (ok) ok = tangent
                         ? nativeFighterAnimation_SetTangent(
                               &project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, track, key, value)
                         : nativeFighterAnimation_SetKey(
                               &project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, track, key, value);
        ok = ok && nativeFighterProject_SaveAnimation(path, &project);
    }
    if (ok) {
        printf("action %lu frames=%.6g joints=%u tracks=%u clip=%u+%u\n",
               (unsigned long) action, info.frames, info.joints, info.tracks,
               info.clip_offset, info.clip_size);
        if (track_text == NULL) {
            NativeFighterAnimationTrack* tracks = calloc(
                info.tracks, sizeof(*tracks));
            size_t count;
            ok = tracks != NULL && nativeFighterAnimation_Tracks(
                &project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, tracks, info.tracks, &count);
            if (ok) for (size_t i = 0; i < count; ++i)
                printf("%zu joint=%u channel=%u start=%d keys=%u bytes=%u\n",
                       i, tracks[i].joint, tracks[i].channel,
                       tracks[i].start_frame, tracks[i].keys,
                       tracks[i].encoded_bytes);
            free(tracks);
        } else {
            size_t count;
            ok = nativeFighterAnimation_Keys(&project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, track,
                                              NULL, 0, &count);
            NativeFighterAnimationKey* keys =
                ok ? calloc(count, sizeof(*keys)) : NULL;
            ok = ok && keys != NULL && nativeFighterAnimation_Keys(
                &project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, track, keys, count, &count);
            if (ok) {
                size_t first = key_text != NULL ? key : 0;
                size_t end = key_text != NULL ? (size_t) key + 1 : count;
                ok = end <= count;
                for (size_t i = first; ok && i < end; ++i)
                    printf("%zu frame=%d op=%u value=%.9g%s tangent=%.9g%s\n", i,
                           keys[i].frame, keys[i].interpolation,
                           keys[i].value,
                           keys[i].editable ? "" : " read-only",
                           keys[i].tangent,
                           keys[i].tangent_editable ? "" : " read-only");
            }
            free(keys);
        }
    }
    nativeFighterProject_Close(&project);
    if (!ok) fputs("Could not inspect or save this animation.\n", stderr);
    return ok ? 0 : 1;
}

static int project_animation_sample(const char* path,
                                    const char* action_text,
                                    const char* track_text,
                                    const char* frame_text)
{
    u32 action, track;
    char* end;
    errno = 0;
    float frame = strtof(frame_text, &end);
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(track_text, 4095, &track) ||
        *frame_text == '\0' || *end != '\0' || errno != 0) {
        fputs("Invalid animation sample request.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    float value;
    bool ok = nativeFighterAnimation_Sample(&project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, track,
                                             frame, &value);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not evaluate that animation channel.\n", stderr);
        return 1;
    }
    printf("action %lu track %lu frame %.6g value %.9g\n",
           (unsigned long) action, (unsigned long) track, frame, value);
    return 0;
}

static int project_animation_move(const char* path,
                                  const char* action_text,
                                  const char* track_text,
                                  const char* key_text,
                                  const char* frame_text)
{
    u32 action, track, key, frame;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(track_text, 4095, &track) ||
        !parse_number(key_text, 65535, &key) ||
        !parse_number(frame_text, 65535, &frame)) {
        fputs("Invalid animation key move.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    const NativeFighterAnimationKeyTime edit = {{track, key}, (int) frame};
    bool ok = nativeFighterAnimation_EditKeys(&project,
        (NativeFighterAnimationClip){NativeFighterAnimation_Action, action},
        &edit, 1, false) && nativeFighterProject_SaveAnimation(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not move that key within its neighboring frames.\n",
              stderr);
        return 1;
    }
    printf("Moved action %lu track %lu key %lu to frame %lu.\n",
           (unsigned long) action, (unsigned long) track,
           (unsigned long) key, (unsigned long) frame);
    return 0;
}

static int project_animation_key_bone(char** args)
{
    u32 action, joint, channel, frame;
    char* end;
    float value = strtof(args[5], &end);
    if (!*args[5] || *end || !isfinite(value)) {
        return 1;
    }
    float base = strtof(args[6], &end);
    if (!*args[6] || *end || !isfinite(base) ||
        !parse_number(args[1], NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(args[2], 511, &joint) ||
        !parse_number(args[3], 10, &channel) ||
        !parse_number(args[4], 65535, &frame))
    {
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(args[0], &project)) {
        return 1;
    }
    bool ok =
        nativeFighterAnimation_KeyChannel(&project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, joint, channel,
                                          (int) frame, value, base) &&
        nativeFighterProject_SaveAnimation(args[0], &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not key and save this bone channel.\n", stderr);
    }
    return ok ? 0 : 1;
}

static int project_animation_insert(const char* path,
                                    const char* action_text,
                                    const char* track_text,
                                    const char* frame_text,
                                    const char* value_text)
{
    u32 action, track, frame;
    char* end;
    errno = 0;
    float value = strtof(value_text, &end);
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(track_text, 4095, &track) ||
        !parse_number(frame_text, 65535, &frame) ||
        *value_text == '\0' || *end != '\0' || errno != 0 ||
        !isfinite(value)) {
        fputs("Invalid animation key insertion request.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    bool ok = nativeFighterAnimation_InsertKey(&project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, track,
                                               (int) frame, value) &&
              nativeFighterProject_SaveAnimation(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not insert or save that animation key.\n", stderr);
        return 1;
    }
    printf("Inserted action %lu track %lu frame %lu value %.9g\n",
           (unsigned long) action, (unsigned long) track,
           (unsigned long) frame, value);
    return 0;
}

static int project_animation_delete(const char* path,
                                    const char* action_text,
                                    const char* track_text,
                                    const char* key_text)
{
    u32 action, track, key;
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        !parse_number(track_text, 4095, &track) ||
        !parse_number(key_text, 65535, &key)) {
        fputs("Invalid animation key deletion request.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    const NativeFighterAnimationKeyTime edit = {{track, key}, 0};
    bool ok = nativeFighterAnimation_EditKeys(&project,
        (NativeFighterAnimationClip){NativeFighterAnimation_Action, action},
        &edit, 1, true) && nativeFighterProject_SaveAnimation(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not delete that animation key.\n", stderr);
        return 1;
    }
    printf("Deleted action %lu track %lu key %lu.\n",
           (unsigned long) action, (unsigned long) track,
           (unsigned long) key);
    return 0;
}

static int project_animation_frames(const char* path,
                                    const char* action_text,
                                    const char* frames_text)
{
    u32 action;
    char* end;
    errno = 0;
    const float frames = strtof(frames_text, &end);
    if (!parse_number(action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1, &action) ||
        *frames_text == '\0' || *end != '\0' || errno != 0 ||
        !isfinite(frames) || frames < 0 || frames > 1000) {
        fputs("Invalid animation end frame.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    bool ok = nativeFighterAnimation_SetFrames(&project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, frames) &&
              nativeFighterProject_SaveAnimation(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        fputs("Could not set the clip end frame; an existing key may extend "
              "beyond it.\n", stderr);
        return 1;
    }
    printf("Changed action %lu animation end to %.6g frames.\n",
           (unsigned long) action, frames);
    return 0;
}

static int project_pose_paste(const char* path, const char* source_action_text,
                              const char* source_frame_text,
                              const char* dest_action_text,
                              const char* dest_frame_text)
{
    u32 source_action, source_frame, dest_action, dest_frame;
    if (!parse_number(source_action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1,
                      &source_action) ||
        !parse_number(source_frame_text, 65535, &source_frame) ||
        !parse_number(dest_action_text,
                      NATIVE_FIGHTER_ACTION_CAPACITY - 1,
                      &dest_action) ||
        !parse_number(dest_frame_text, 65535, &dest_frame)) {
        fputs("Invalid pose capture or destination.\n", stderr);
        return 1;
    }
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) {
        fputs("Could not open a complete fighter draft.\n", stderr);
        return 1;
    }
    NativeFighterPose pose;
    unsigned written, skipped, failed_track;
    bool captured = nativeFighterPose_Capture(&project,
        (NativeFighterAnimationClip){ NativeFighterAnimation_Action, source_action },
                                               (float) source_frame, &pose);
    bool pasted = captured && nativeFighterPose_Paste(
        &project, &pose,
        (NativeFighterAnimationClip){ NativeFighterAnimation_Action, dest_action }, (int) dest_frame, NULL, false, false,
        &written, &skipped, &failed_track);
    bool ok = pasted && nativeFighterProject_SaveAnimation(path, &project);
    nativeFighterProject_Close(&project);
    if (!ok) {
        if (captured && !pasted && failed_track != UINT_MAX)
            fprintf(stderr, "Could not paste pose at track %u.\n",
                    failed_track);
        else fprintf(stderr, "Could not %s that pose.\n",
                     !captured ? "capture" : !pasted ? "paste" : "save");
        return 1;
    }
    printf("Pasted %u channels; %u source channels had no destination track.\n",
           written, skipped);
    return 0;
}

static int module_probe(const char* path)
{
    for (unsigned pass = 0; pass < 2; ++pass) {
        NativeFighterModule module = { 0 };
        if (!nativeFighterModule_Open(path, NULL, &module)) {
            fputs("Could not load the native fighter module.\n", stderr);
            return 1;
        }
        printf("Module %s: %u actions, %u states\n",
               module.definition->code_key, module.definition->action_count,
               module.definition->motion_state_count);
        nativeFighterModule_Close(&module);
    }
    return 0;
}















static int extract_stock(const char* iso, const char* character_name,
                         const char* color_text, const char* output)
{
    int character = character_from_name(character_name);
    long color;
    if (character < 0 || !parse_color(color_text, &color) ||
        color >= gm_GetNumCostumesForCKind(character)) {
        fputs("Invalid stock character or base costume.\n", stderr);
        return 1;
    }
    FighterKind kind = Player_800325C8(character, false);
    const Fighter_CostumeStrings* names = &ftData_803C2360[kind][color];
    if (names->dat_filename == NULL || !aurora_dvd_open(iso)) {
        fputs("Could not open the original costume from the disc.\n", stderr);
        return 1;
    }
    void* bytes = NULL;
    size_t size = 0;
    bool ok = nativeDiscArchive_Read(names->dat_filename, &bytes, &size);
    aurora_dvd_close();
    if (ok) {
        ok = write_file(output, bytes, size);
    }
    _aligned_free(bytes);
    if (!ok) {
        fputs("Could not extract stock costume DAT.\n", stderr);
        return 1;
    }
    printf("Extracted %s base color %ld to %s\n", character_name, color,
           output);
    return 0;
}

static int cache_package(const char* root, const char* path)
{
    uint8_t* package = NULL;
    size_t size = 0;
    if (!read_file(path, &package, &size)) {
        fprintf(stderr, "Could not read package: %s\n", path);
        return 1;
    }
    NativeCostumeValidation validation;
    NativeContentId id;
    NativeCostumePackage costume;
    NativeCostumePackageData costume_data;
    const NativeContentId empty_id = { 0 };
    bool dependent_costume =
        nativeContentStore_ParseCostume(package, size, &costume, &costume_data) &&
        memcmp(&costume.fighter_id, &empty_id,
               sizeof(empty_id)) != 0;
    // A recipient may cache the costume before its parent fighter arrives.
    // Installation still performs the full graph and dependency preflight.
    bool ok = (dependent_costume ||
               valid_package(root, package, size, &validation)) &&
              nativeContentStore_Cache(root, package, size, NULL, &id);
    free(package);
    if (!ok) {
        fputs("Package is invalid or could not be cached.\n", stderr);
        return 1;
    }
    char hex[65];
    nativeContentStore_IdHex(&id, hex);
    printf("Cached (not installed): %s\n", hex);
    return 0;
}

static int accept_package(const char* root, const char* hex)
{
    NativeContentId id;
    uint8_t* package = NULL;
    size_t size = 0;
    if (!nativeContentStore_IdFromHex(hex, &id) ||
        !nativeContentStore_Read(root, &id, &package, &size)) {
        fputs("Package is not in the local cache.\n", stderr);
        return 1;
    }
    NativeFighterPackage fighter;
    NativeFighterPackageData fighter_data;
    if (nativeContentStore_ParseFighter(package, size, &fighter,
                                        &fighter_data)) {
        free(package);
        if (nativeSourceContentTool_Verify(root,hex) != 0 || !nativeContentStore_Install(root,&id)) {
            fputs("Source fighter validation or installation failed.\n",stderr);return 1;
        }
        printf("Installed fighter: %s\n", hex);
        return 0;
    }
    NativeCostumeValidation validation;
    bool ok = valid_package(root, package, size, &validation) &&
              nativeContentStore_Install(root, &id);
    free(package);
    if (!ok) {
        fputs("Cached package is invalid or could not be installed.\n",
              stderr);
        return 1;
    }
    printf("Installed: %s\n", hex);
    return 0;
}

static int export_package(const char* root, const char* hex,
                          const char* output)
{
    NativeContentId id;
    uint8_t* package = NULL;
    size_t size = 0;
    if (!nativeContentStore_IdFromHex(hex, &id) ||
        !nativeContentStore_IsInstalled(root, &id) ||
        !nativeContentStore_Read(root, &id, &package, &size)) {
        fputs("Only installed packages can be exported.\n", stderr);
        return 1;
    }
    bool ok = write_file(output, package, size);
    free(package);
    if (!ok) {
        fprintf(stderr, "Could not export package: %s\n", output);
        return 1;
    }
    printf("Exported %s to %s\n", hex, output);
    return 0;
}

static int list_packages(const char* root)
{
    NativeContentId ids[512];
    size_t count = 0;
    if (!nativeContentStore_ListInstalled(root, ids, 512, &count)) {
        fputs("Could not enumerate installed packages.\n", stderr);
        return 1;
    }
    for (size_t i = 0; i < count; ++i) {
        uint8_t* package = NULL;
        size_t size = 0;
        NativeCostumePackage costume;
    NativeCostumePackageData costume_data;
        NativeFighterPackage fighter;
        NativeFighterPackageData fighter_data;
        if (!nativeContentStore_Read(root, &ids[i], &package, &size)) {
            free(package);
            continue;
        }
        char hex[65];
        nativeContentStore_IdHex(&ids[i], hex);
        if (nativeContentStore_ParseCostume(package, size, &costume, &costume_data)) {
            const NativeContentId empty_id = { 0 };
            if (memcmp(&costume.fighter_id, &empty_id,
                       sizeof(empty_id)) != 0) {
                char fighter_hex[65];
                nativeContentStore_IdHex(&costume.fighter_id, fighter_hex);
                printf("%s  %s  costume for fighter %s\n", hex,
                       costume.title, fighter_hex);
            } else {
                const char* character =
                    costume.character < CKind_Playable_Count
                        ? stock_names[costume.character]
                        : "Unknown character";
                printf("%s  %s  %s (base color %u)\n", hex,
                       costume.title, character, costume.base_costume);
            }
        } else if (nativeContentStore_ParseFighter(package, size, &fighter,
                                                   &fighter_data)) {
            printf("%s  %s  fighter family (%u member%s)\n", hex, fighter.title,
                   fighter.member_count, fighter.member_count == 1 ? "" : "s");
            for (unsigned member = 0; member < fighter.member_count; ++member)
                printf("  source member %s\n", fighter.members[member].key);
        }
        free(package);
    }
    printf("%zu installed package%s\n", count, count == 1 ? "" : "s");
    return 0;
}

static int project_model_scan(const char* source)
{
    NativeGltfScan scan;
    char error[192] = { 0 };
    if (!nativeFighterGltf_Scan(source, &scan, error, sizeof(error))) {
        fprintf(stderr, "Model scan failed: %s\n", error);
        return 1;
    }
    for (unsigned i = 0; i < scan.count; ++i) {
        const NativeGltfPrimitiveInfo* mesh = &scan.primitives[i];
        printf("%u: %s (%u vertices, %u triangles, normal=%d uv=%d skin=%d texture=%d)\n",
               i, mesh->name, mesh->vertices, mesh->triangles,
               mesh->has_normals, mesh->has_uv, mesh->has_skin,
               mesh->has_texture);
    }
    return 0;
}

static int project_model_import(const char* path, const char* source,
                                const char* primitive_text,
                                const char* joint_text,
                                const char* replacement_text,
                                const char* scale_text,
                                const char* binding_text)
{
    char* end;
    unsigned long primitive = strtoul(primitive_text, &end, 10);
    if (*end || primitive > UINT_MAX) return 1;
    unsigned long joint = strtoul(joint_text, &end, 10);
    if (*end || joint > UINT_MAX) return 1;
    unsigned long replacement = UINT_MAX;
    if (strcmp(replacement_text, "add") != 0) {
        replacement = strtoul(replacement_text, &end, 10);
        if (*end || replacement > UINT_MAX) return 1;
    }
    float scale = strtof(scale_text, &end);
    if (*end || !isfinite(scale)) return 1;
    NativeGltfBindingMode binding;
    if (strcmp(binding_text, "rigid") == 0) binding = NATIVE_GLTF_BIND_RIGID;
    else if (strcmp(binding_text, "auto") == 0)
        binding = NATIVE_GLTF_BIND_AUTOMATIC;
    else if (strcmp(binding_text, "transfer") == 0)
        binding = NATIVE_GLTF_BIND_TRANSFER;
    else if (strcmp(binding_text, "source") == 0)
        binding = NATIVE_GLTF_BIND_SOURCE;
    else if (strcmp(binding_text, "keep") == 0)
        binding = NATIVE_GLTF_BIND_KEEP;
    else return 1;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return 1;
    char error[192] = { 0 };
    const NativeGltfImportRequest request={(unsigned)primitive,(unsigned)joint,(unsigned)replacement,binding,scale,false};
    bool ok = nativeFighterGltf_ApplyBatch(&project,source,&request,1,error,sizeof(error)) &&
        nativeFighterProject_SaveAssets(path,&project,error,sizeof(error));
    if (!ok) fprintf(stderr, "Model import failed: %s\n", error);
    else printf("Imported primitive %lu into %s.\n", primitive, path);
    nativeFighterProject_Close(&project);
    return ok ? 0 : 1;
}

static int project_model_weights(const char* path)
{
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return 1;
    NativeFighterModelEditor* model =
        (NativeFighterModelEditor*) calloc(1, sizeof(*model));
    if (model == NULL || !nativeFighterModel_Open(model, &project)) {
        free(model);
        nativeFighterProject_Close(&project);
        return 1;
    }
    for (unsigned i = 0; i < model->mesh_count; ++i) {
        const unsigned count = nativeFighterModel_EnvelopeGroups(
            model, i, NULL, 0);
        if (count) {
            NativeModelEnvelopeGroup* groups =
                calloc(count, sizeof(*groups));
            unsigned max_influences = 0;
            if (groups) {
                nativeFighterModel_EnvelopeGroups(model, i, groups, count);
                for (unsigned g = 0; g < count; ++g)
                    if (groups[g].influence_count > max_influences)
                        max_influences = groups[g].influence_count;
                free(groups);
            }
            printf("mesh %u (%s, joint %u, DObj 0x%lX): %u envelope palette%s, up to %u bone%s\n",
                   i, model->meshes[i].name, model->meshes[i].joint,
                   (unsigned long) model->meshes[i].offset,
                   count, count == 1 ? "" : "s",
                   max_influences, max_influences == 1 ? "" : "s");
        }
    }
    free(model);
    nativeFighterProject_Close(&project);
    return 0;
}

static int project_model_paint(int count, char** arguments)
{
    if (count != 10) return 1;
    char* end;
    unsigned long mesh = strtoul(arguments[2], &end, 10);
    if (*end || mesh > UINT_MAX) return 1;
    unsigned long joint = strtoul(arguments[3], &end, 10);
    if (*end || joint > UINT_MAX) return 1;
    float center[3];
    for (unsigned i = 0; i < 3; ++i) {
        center[i] = strtof(arguments[4 + i], &end);
        if (*end || !isfinite(center[i])) return 1;
    }
    float radius = strtof(arguments[7], &end);
    if (*end || !isfinite(radius)) return 1;
    float strength = strtof(arguments[8], &end);
    if (*end || !isfinite(strength)) return 1;
    bool subtract;
    if (strcmp(arguments[9], "add") == 0) subtract = false;
    else if (strcmp(arguments[9], "subtract") == 0) subtract = true;
    else return 1;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(arguments[1], &project)) return 1;
    char error[192] = { 0 };
    unsigned affected = 0;
    NativeSkinBrushPoint point;
    memcpy(point.position, center, sizeof(center));
    bool ok = nativeFighterSkin_Paint(&project, (unsigned) mesh,
        (unsigned) joint, &point, 1,
        radius, strength, subtract, &affected,
        error, sizeof(error));
    if (ok && affected)
        ok = nativeFighterProject_SaveCostume(arguments[1], &project);
    if (ok) printf("Painted %u vertices.\n", affected);
    else fprintf(stderr, "Model paint failed: %s\n", error);
    nativeFighterProject_Close(&project);
    return ok ? 0 : 1;
}

static int project_model_vertices(const char* path, const char* mesh_text)
{
    char* end;
    unsigned long mesh = strtoul(mesh_text, &end, 10);
    if (*end || mesh > UINT_MAX) return 1;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return 1;
    unsigned count = nativeFighterSkin_Vertices(&project,
        (unsigned) mesh, NULL, 0, NULL, 0, NULL);
    NativeSkinVertex* vertices = count
        ? calloc(count, sizeof(*vertices)) : NULL;
    if (vertices)
        count = nativeFighterSkin_Vertices(&project,
            (unsigned) mesh, vertices, count, NULL, 0, NULL);
    for (unsigned i = 0; vertices && i < count; ++i)
        {
            printf("vertex %u: (%g, %g, %g)",
                i, vertices[i].position[0], vertices[i].position[1],
                vertices[i].position[2]);
            for (unsigned b = 0; b < vertices[i].skin.count; ++b)
                printf(" joint %u %.9g", vertices[i].skin.joints[b],
                       vertices[i].skin.weights[b]);
            putchar('\n');
        }
    free(vertices);
    nativeFighterProject_Close(&project);
    return count ? 0 : 1;
}

static int project_model_joints(const char* path)
{
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return 1;
    NativeFighterModelEditor* model = calloc(1, sizeof(*model));
    if (!model) { nativeFighterProject_Close(&project); return 1; }
    bool ok = nativeFighterModel_Open(model, &project);
    if (ok) for (unsigned i = 0; i < model->joint_count; ++i)
        printf("joint %u parent %d offset 0x%lX flags 0x%lX %s\n", i,
            model->joints[i].parent,
            (unsigned long) model->joints[i].offset,
            (unsigned long) model->joints[i].flags,
            model->joints[i].name);
    free(model);
    nativeFighterProject_Close(&project);
    return ok ? 0 : 1;
}

static int project_model_rest(int count, char** arguments)
{
    if (count != 6) return 1;
    char* end;
    unsigned long joint = strtoul(arguments[2], &end, 10);
    if (*end || joint > UINT_MAX) return 1;
    unsigned channel;
    if (strcmp(arguments[3], "rotation") == 0) channel = 0;
    else if (strcmp(arguments[3], "scale") == 0) channel = 1;
    else if (strcmp(arguments[3], "position") == 0) channel = 2;
    else return 1;
    unsigned long axis = strtoul(arguments[4], &end, 10);
    if (*end || axis >= 3) return 1;
    float value = strtof(arguments[5], &end);
    if (*end || !isfinite(value)) return 1;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(arguments[1], &project)) return 1;
    NativeFighterModelEditor* model = calloc(1, sizeof(*model));
    uint8_t* previous = malloc(project.costume_size);
    bool ok = model && previous && nativeFighterModel_Open(model, &project);
    if (ok) {
        memcpy(previous, project.costume_dat, project.costume_size);
        ok = nativeFighterModel_SetJoint(model, (unsigned) joint, channel,
                                         (unsigned) axis, value) &&
             nativeFighterModel_CompensateRest(model, previous,
                                                project.costume_size,
                                                (unsigned) joint) &&
             nativeFighterProject_SaveCostume(arguments[1], &project);
    }
    if (ok) printf("Rest joint %lu updated with bind compensation.\n", joint);
    else fprintf(stderr, "Rest edit failed.\n");
    free(previous);
    free(model);
    nativeFighterProject_Close(&project);
    return ok ? 0 : 1;
}

static int project_model_texture(const char* path, const char* material_text,
                                 const char* image_path)
{
    char* end;
    const unsigned long material = strtoul(material_text, &end, 10);
    if (*end || material > UINT_MAX) return 1;
    NativeFighterProject project;
    if (!nativeFighterProject_Open(path, &project)) return 1;
    char error[192] = { 0 };
    const bool ok = nativeFighterGltf_SetTextureImage(&project,
        (unsigned) material, 0, 0, 0, image_path, error, sizeof(error)) &&
        nativeFighterProject_SaveCostume(path, &project);
    if (!ok) fprintf(stderr, "Texture import failed: %s\n", error);
    nativeFighterProject_Close(&project);
    return ok ? 0 : 1;
}

static NativeFighterDraftLease command_leases[2];
static void release_command_leases(void)
{
    for (unsigned i = 0; i < 2; ++i) nativeFighterDraft_Release(&command_leases[i]);
}

int main(int argc, char** argv)
{
    if (argc > 1 && (!strcmp(argv[1], "agent") || !strcmp(argv[1], "mcp")))
        return nativeAgentTool_Main(argc - 1, argv + 1);
    char default_root[MAX_PATH];
    if (!nativeContentStore_DefaultRoot(default_root,
                                        sizeof(default_root))) {
        fputs("LOCALAPPDATA is unavailable.\n", stderr);
        return 1;
    }
    const char* root = default_root;
    int index = 1;
    if (argc > 3 && strcmp(argv[index], "--root") == 0) {
        root = argv[index + 1];
        index += 2;
    }
    int arguments = argc - index;
    // Legacy commands can also write drafts. Reserve any existing project in
    // their first two arguments (move-copy has source and destination). Source
    // validation is deliberately read-only and also runs for the live editor.
    if (arguments > 1 && strcmp(argv[index], "source-check") && strcmp(argv[index], "source-audit")) {
        atexit(release_command_leases);
        for (int i = 1; i < arguments && i <= 2; ++i) {
            NativeFighterFamilyProjectInfo info;
            if (!nativeFighterFamilyProject_ReadInfo(argv[index + i], &info)) continue;
            char error[512];
            if (!nativeFighterDraft_Acquire(argv[index + i], &command_leases[i - 1], error, sizeof(error))) {
                fprintf(stderr, "%s\n", error); return 1;
            }
        }
    }
    int authoring_result;
    if(nativeWorkshopAuthoring_Command(arguments,argv+index,&authoring_result))return authoring_result;
    if (arguments == 2 && strcmp(argv[index], "project-model-scan") == 0)
        return project_model_scan(argv[index + 1]);
    if (arguments == 8 && strcmp(argv[index], "project-model-import") == 0)
        return project_model_import(argv[index + 1], argv[index + 2],
            argv[index + 3], argv[index + 4], argv[index + 5],
            argv[index + 6], argv[index + 7]);
    if (arguments == 2 && strcmp(argv[index], "project-model-weights") == 0)
        return project_model_weights(argv[index + 1]);
    if (arguments == 3 && strcmp(argv[index], "project-model-vertices") == 0)
        return project_model_vertices(argv[index + 1], argv[index + 2]);
    if (arguments == 2 && strcmp(argv[index], "project-model-joints") == 0)
        return project_model_joints(argv[index + 1]);
    if (arguments == 6 && strcmp(argv[index], "project-model-rest") == 0)
        return project_model_rest(arguments, argv + index);
    if (arguments == 10 && strcmp(argv[index], "project-model-paint") == 0)
        return project_model_paint(arguments, argv + index);
    if (arguments == 4 && strcmp(argv[index], "project-model-texture") == 0)
        return project_model_texture(argv[index + 1], argv[index + 2],
                                     argv[index + 3]);
    if (arguments == 5 && strcmp(argv[index], "import-costume") == 0) {
        return import_costume(root, argv[index + 1], argv[index + 2],
                              argv[index + 3], argv[index + 4]);
    }
    if ((arguments == 4 || arguments == 5) &&
        strcmp(argv[index], "import-custom-costume") == 0) {
        return import_custom_costume(root, argv[index + 1],
                                     argv[index + 2], argv[index + 3],
                                     arguments == 5 ? argv[index + 4] : NULL);
    }
    if (arguments == 2 && strcmp(argv[index], "cache") == 0) {
        return cache_package(root, argv[index + 1]);
    }
    if (arguments == 2 && strcmp(argv[index], "accept") == 0) {
        return accept_package(root,argv[index+1]);
    }
    if (arguments == 3 && strcmp(argv[index], "install-project") == 0) {
        NativeContentId id;
        bool ok = nativeContentStore_IdFromHex(argv[index + 2], &id) &&
                  nativeFighterProject_Install(argv[index + 1], root, &id,
                                               NULL);
        if (!ok) fputs("Could not install this draft or replace its prior build.\n",
                       stderr);
        else printf("Installed draft: %s\n", argv[index + 2]);
        return ok ? 0 : 1;
    }
    if (arguments == 2 && strcmp(argv[index], "uninstall") == 0) {
        NativeContentId id;
        bool ok = nativeContentStore_IdFromHex(argv[index + 1], &id) &&
                  nativeContentStore_Uninstall(root, &id);
        if (!ok) fputs("This package is not installed or could not be removed.\n",
                       stderr);
        else printf("Uninstalled: %s\n", argv[index + 1]);
        return ok ? 0 : 1;
    }
    if (arguments == 3 && strcmp(argv[index], "export") == 0) {
        return export_package(root, argv[index + 1], argv[index + 2]);
    }
    if (arguments == 5 && strcmp(argv[index], "extract-stock") == 0) {
        return extract_stock(argv[index + 1], argv[index + 2],
                             argv[index + 3], argv[index + 4]);
    }
    if (arguments == 5 && strcmp(argv[index], "clone-stock") == 0) {
        return clone_stock(argv[index + 1], argv[index + 2],
                           argv[index + 3], argv[index + 4]);
    }
    if (arguments==2 && strcmp(argv[index],"project-stock-template")==0) {
        bool ok=nativeFighterTemplate_Apply(argv[index+1]);
        if (!ok) fputs("Could not apply the editable stock template to an empty draft.\n",stderr);
        return ok?0:1;
    }
    if (arguments == 4 && strcmp(argv[index], "import-cpu") == 0) {
        return nativeFighterCpuTool_Import(argv[index + 1], argv[index + 2],
                                           argv[index + 3]);
    }
    if (arguments == 4 && strcmp(argv[index], "clone-installed") == 0) {
        return clone_installed(root, argv[index + 1], argv[index + 2],
                               argv[index + 3]);
    }
    if (arguments == 2 && strcmp(argv[index], "inspect-project") == 0) {
        return inspect_project(argv[index + 1]);
    }
    if ((arguments == 3 || arguments == 4) &&
        strcmp(argv[index], "project-attribute") == 0) {
        return project_attribute(argv[index + 1], argv[index + 2],
                                 arguments == 4 ? argv[index + 3] : NULL);
    }
    if ((arguments == 2 || arguments == 3) &&
        strcmp(argv[index], "project-actions") == 0) {
        return project_actions(argv[index + 1],
                               arguments == 3 ? argv[index + 2] : NULL);
    }
    if (arguments == 5 && strcmp(argv[index], "project-event") == 0) {
        return project_event(argv[index + 1], argv[index + 2],
                             argv[index + 3], argv[index + 4]);
    }
    if (arguments == 5 && strcmp(argv[index], "project-move-event") == 0)
        return project_move_event(argv[index + 1], argv[index + 2], argv[index + 3], argv[index + 4]);
    if ((arguments == 2 || arguments == 3) && strcmp(argv[index], "project-hitbox-intervals") == 0)
        return project_hitbox_intervals(argv[index + 1], arguments == 3 ? argv[index + 2] : NULL);
    if (arguments == 6 &&
        (strcmp(argv[index], "project-insert-command") == 0 || strcmp(argv[index], "project-insert-at-frame") == 0)) {
        return project_insert_command(argv[index + 1], argv[index + 2],
                                      argv[index + 3], argv[index + 4],
                                      argv[index + 5], strcmp(argv[index], "project-insert-at-frame") == 0);
    }
    if (arguments == 4 &&
        strcmp(argv[index], "project-delete-command") == 0) {
        return project_delete_command(argv[index + 1], argv[index + 2],
                                      argv[index + 3]);
    }
    if (arguments == 4 &&
        strcmp(argv[index], "project-localize-command") == 0) {
        return project_localize_command(argv[index + 1], argv[index + 2],
                                        argv[index + 3]);
    }
    if (arguments == 4 && strcmp(argv[index], "project-copy-normal") == 0) {
        return project_copy_normal(argv[index + 1], argv[index + 2],
                                   argv[index + 3], false);
    }
    if (arguments == 4 &&
        strcmp(argv[index], "project-copy-normal-group") == 0) {
        return project_copy_normal(argv[index + 1], argv[index + 2],
                                   argv[index + 3], true);
    }
    if ((arguments == 3 || arguments == 4) &&
        strcmp(argv[index], "project-action-flags") == 0) {
        return project_action_flags(argv[index + 1], argv[index + 2],
                                    arguments == 4 ? argv[index + 3] : NULL);
    }
    if ((arguments == 3 || arguments == 4 || arguments == 6) &&
        strcmp(argv[index], "project-animation") == 0) {
        return project_animation(argv[index + 1], argv[index + 2],
                                 arguments >= 4 ? argv[index + 3] : NULL,
                                 arguments == 6 ? argv[index + 4] : NULL,
                                 arguments == 6 ? argv[index + 5] : NULL,
                                 false);
    }
    if (arguments == 6 &&
        strcmp(argv[index], "project-animation-tangent") == 0) {
        return project_animation(argv[index + 1], argv[index + 2],
                                 argv[index + 3], argv[index + 4],
                                 argv[index + 5], true);
    }
    if (arguments == 6 &&
        strcmp(argv[index], "project-animation-move") == 0) {
        return project_animation_move(argv[index + 1], argv[index + 2],
                                      argv[index + 3], argv[index + 4],
                                      argv[index + 5]);
    }
    if (arguments == 8 &&
        strcmp(argv[index], "project-animation-key-bone") == 0)
    {
        return project_animation_key_bone(argv + index + 1);
    }
    if (arguments == 6 &&
        strcmp(argv[index], "project-animation-insert") == 0) {
        return project_animation_insert(argv[index + 1], argv[index + 2],
                                        argv[index + 3], argv[index + 4],
                                        argv[index + 5]);
    }
    if (arguments == 5 &&
        strcmp(argv[index], "project-animation-delete") == 0) {
        return project_animation_delete(argv[index + 1], argv[index + 2],
                                        argv[index + 3], argv[index + 4]);
    }
    if (arguments == 4 &&
        strcmp(argv[index], "project-animation-frames") == 0) {
        return project_animation_frames(argv[index + 1], argv[index + 2],
                                        argv[index + 3]);
    }
    if (arguments == 6 && strcmp(argv[index], "project-pose-paste") == 0) {
        return project_pose_paste(argv[index + 1], argv[index + 2],
                                  argv[index + 3], argv[index + 4],
                                  argv[index + 5]);
    }
    if (arguments == 5 &&
        strcmp(argv[index], "project-animation-sample") == 0) {
        return project_animation_sample(argv[index + 1], argv[index + 2],
                                        argv[index + 3], argv[index + 4]);
    }
    if (arguments == 2 && strcmp(argv[index], "project-animation-undo") == 0) {
        if (!nativeFighterProject_UndoLastAnimationSave(argv[index + 1])) {
            fputs("No previous animation bank could be restored.\n", stderr);
            return 1;
        }
        puts("Restored the previous animation bank.");
        return 0;
    }
    if ((arguments == 4 || arguments == 6) &&
        strcmp(argv[index], "project-hitbox") == 0) {
        return project_hitbox(argv[index + 1], argv[index + 2],
                              argv[index + 3],
                              arguments == 6 ? argv[index + 4] : NULL,
                              arguments == 6 ? argv[index + 5] : NULL);
    }
    if ((arguments == 4 || arguments == 7) &&
        strcmp(argv[index], "project-sound") == 0) {
        return project_sound(argv[index + 1], argv[index + 2],
                             argv[index + 3],
                             arguments == 7 ? argv[index + 4] : NULL,
                             arguments == 7 ? argv[index + 5] : NULL,
                             arguments == 7 ? argv[index + 6] : NULL);
    }
    if ((arguments == 4 || arguments == 6) &&
        strcmp(argv[index], "project-effect") == 0) {
        return project_effect(argv[index + 1], argv[index + 2],
                              argv[index + 3],
                              arguments == 6 ? argv[index + 4] : NULL,
                              arguments == 6 ? argv[index + 5] : NULL);
    }
    if (arguments == 2 && strcmp(argv[index], "project-undo") == 0) {
        return project_undo(argv[index + 1]);
    }
    if (arguments == 2 && strcmp(argv[index], "module-probe") == 0) {
        return module_probe(argv[index + 1]);
    }
    if (arguments == 3 && strcmp(argv[index], "source-check") == 0) {
        return nativeSourceContentTool_Check(argv[index + 1], argv[index + 2],false,false);
    }
    if (arguments == 3 && strcmp(argv[index], "source-audit") == 0) {
        return nativeSourceContentTool_Check(argv[index + 1], argv[index + 2],false,true);
    }
    if (arguments == 3 && strcmp(argv[index], "script-build") == 0) {
        return nativeSourceContentTool_Check(argv[index + 1], argv[index + 2],true,false);
    }
    if (arguments == 4 && strcmp(argv[index], "import-parameters") == 0) {
        return nativeFighterParameterTool_Import(argv[index + 1],
            argv[index + 2], argv[index + 3]);
    }
    if (arguments == 3 && strcmp(argv[index], "package-project") == 0) {
        return nativeSourceContentTool_Package(root, argv[index + 1], argv[index + 2]);
    }

    if (arguments == 2 && strcmp(argv[index], "verify-package") == 0) {
        return nativeSourceContentTool_Verify(root, argv[index + 1]);
    }
    if (arguments == 3 && strcmp(argv[index], "export-fighter-facts") == 0)
        return nativeSourceResourceTool_ExportFacts(argv[index+1],argv[index+2]);

    if ((arguments == 6 || arguments == 7) &&
        strcmp(argv[index], "inspect-fighter") == 0) {
        return inspect_fighter(argv[index + 1], argv[index + 2],
                               argv[index + 3], argv[index + 4],
                               argv[index + 5],
                               arguments == 7 ? argv[index + 6] : NULL);
    }
    if (arguments == 4 && strcmp(argv[index], "inspect-costume") == 0) {
        return inspect_costume(argv[index + 1], argv[index + 2],
                               argv[index + 3]);
    }

    if (arguments == 1 && strcmp(argv[index], "list") == 0) {
        return list_packages(root);
    }
    usage();
    return 1;
}
