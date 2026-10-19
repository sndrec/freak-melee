#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/lb/lbfile.h>

#include <stdio.h>
#include <stdlib.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

int main(int argc, char** argv)
{
    static const char* purin_hats[] = {
        NULL,
        "PlyPurinReHat_TopN_joint",
        "PlyPurinBuHat_TopN_joint",
        "PlyPurinGrHat_TopN_joint",
        "PlyPurinYeHat_TopN_joint",
    };
    u32 imported = 0;
    u32 max_records = 0;
    size_t max_arena_used = 0;
    require(argc == 2, "usage: native_fighter_costume_assets_test <iso>\n");
    require(aurora_dvd_open(argv[1]), "open ISO\n");

    for (FighterKind kind = Ft_Kind_Mario; kind < Ft_Kind_StockCount; ++kind) {
        for (u32 costume_id = 0;
             costume_id < CostumeListsForeachCharacter[kind].numCostumes;
             ++costume_id)
        {
            const Fighter_CostumeStrings* names =
                &ftData_803C2360[kind][costume_id];
            DVDFileInfo file;
            char path[64];
            require(names->dat_filename != NULL && names->joint_name != NULL,
                    "costume archive metadata\n");
            snprintf(path, sizeof(path), "/%s",
                     lbFileGetFullName(names->dat_filename));
            if (!DVDOpen(path, &file)) {
                fprintf(stderr, "open costume archive: kind=%u costume=%lu "
                                "file=%s\n", kind, costume_id, path);
                return 1;
            }
            size_t size = file.length;
            size_t padded = (size + 31) & ~(size_t) 31;
            void* bytes = _aligned_malloc(padded, 32);
            void* arena = _aligned_malloc(FT_COSTUME_IMPORT_ARENA_SIZE,
                                          sizeof(void*));
            require(bytes != NULL && arena != NULL &&
                        DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                        DVDClose(&file),
                    "read costume archive\n");
            FtCostumeAssets assets;
            if (!ftCostumeAssets_Import(
                    bytes, size, names->joint_name,
                    names->matanim_joint_name, arena,
                    FT_COSTUME_IMPORT_ARENA_SIZE,
                    FT_COSTUME_IMPORT_RECORD_CAPACITY, &assets))
            {
                fprintf(stderr,
                        "%s import failed: status=%d offset=0x%lX "
                        "records=%lu arena=%zu root=%s\n",
                        names->dat_filename, assets.import.status,
                        assets.import.error_offset, assets.import.record_count,
                        assets.import.arena_used, names->joint_name);
                return 1;
            }
            require(assets.joint != NULL &&
                        (names->matanim_joint_name == NULL ||
                         assets.matanim_joint != NULL),
                    "decoded costume roots\n");
            FtCostumeAssets owned;
            require(ftCostumeAssets_Load(bytes, size, names->joint_name,
                        names->matanim_joint_name, &owned) &&
                        owned.import.record_count == assets.import.record_count &&
                        owned.import.arena_used == assets.import.arena_used &&
                        owned.joint_count == assets.joint_count &&
                        owned.arena == owned.import.arena &&
                        owned.import.dat == &owned.dat,
                    "owned costume import matches caller-supplied storage\n");
            free(owned.arena);
            if (kind == Ft_Kind_Purin && purin_hats[costume_id] != NULL) {
                u32 root;
                require(HSD_DatFindRoot(&assets.dat,
                                         purin_hats[costume_id], &root) &&
                            HSD_DatImportGraph(&assets.import, root,
                                                DAT_JOINT) != NULL,
                        "decoded Jigglypuff costume hat\n");
            }
            if (assets.import.record_count > max_records) {
                max_records = assets.import.record_count;
            }
            if (assets.import.arena_used > max_arena_used) {
                max_arena_used = assets.import.arena_used;
            }
            ++imported;
            _aligned_free(arena);
            _aligned_free(bytes);
        }
    }
    aurora_dvd_close();
    require(imported == 133, "all retail costume archives covered\n");
    printf("imported=%lu max_records=%lu max_arena=%zu\n",
           imported, max_records, max_arena_used);
    return 0;
}
