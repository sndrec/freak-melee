#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/gm/gm_1601.h>
#include <melee/gm/gmresult_assets.h>
#include <melee/lb/lbfile.h>
#include <melee/pl/player.h>

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
    DVDFileInfo file;
    char path[64];
    require(argc == 2, "usage: native_result_assets_test <iso>\n");
    require(gm_80168B34(CKind_GameWatch, 0, 0) == 3.0f &&
                gm_80168B34(CKind_PopoNana, 0, 0) == 14.0f &&
                gm_80168B34(CKind_Zelda, 7, 0) == 25.0f &&
                gm_80168B34(CKind_Ganon, 0, 0) == 24.0f &&
                gm_80168B34(CKind_GameWatch, 0, 1) == 33.0f,
            "result portrait frame mapping\n");
    Player_SetPlayerCharacter(3, CKind_GameWatch);
    Player_SetCostumeId(3, 0);
    require(gm_80168BF8(3) == 3.0f,
            "stock icon uses the portrait frame mapping\n");
    require(aurora_dvd_open(argv[1]), "open ISO\n");
    snprintf(path, sizeof(path), "/%s", lbFileGetFullName("GmRst"));
    require(DVDOpen(path, &file), "open GmRst archive\n");
    size_t size = file.length;
    size_t padded = (size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    void* arena = _aligned_malloc(GM_RESULT_IMPORT_ARENA_SIZE,
                                  sizeof(void*));
    require(bytes != NULL && arena != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                DVDClose(&file),
            "read GmRst archive\n");

    GmResultAssets assets;
    if (!gmResultAssets_Import(bytes, size, arena,
                               GM_RESULT_IMPORT_ARENA_SIZE,
                               GM_RESULT_IMPORT_RECORD_CAPACITY, &assets))
    {
        fprintf(stderr, "results import failed: status=%d offset=0x%lX "
                        "records=%lu arena=%zu\n", assets.import.status,
                assets.import.error_offset, assets.import.record_count,
                assets.import.arena_used);
        return 1;
    }
    require(assets.panel.models[0] != NULL &&
                assets.panel.models[1] == NULL &&
                assets.film.models[0] != NULL &&
                assets.film.models[1] != NULL &&
                assets.film.models[2] != NULL &&
                assets.film.models[3] != NULL &&
                assets.film.models[4] == NULL &&
                assets.panel.cameras->desc != NULL &&
                assets.panel.lights[0]->desc != NULL &&
                assets.panel.lights[1]->desc != NULL &&
                assets.panel.lights[2] == NULL,
            "results scene graph\n");
    printf("records=%lu arena=%zu\n", assets.import.record_count,
           assets.import.arena_used);

    GmResultAssets undersized;
    require(!gmResultAssets_Import(bytes, size, arena, 32,
                                   GM_RESULT_IMPORT_RECORD_CAPACITY,
                                   &undersized) &&
                undersized.import.status == HSD_DAT_CAPACITY,
            "reject undersized results arena\n");
    _aligned_free(arena);
    _aligned_free(bytes);
    aurora_dvd_close();
    return 0;
}
