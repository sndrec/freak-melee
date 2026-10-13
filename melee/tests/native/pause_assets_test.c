#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/gm/gmpause_assets.h>
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
    DVDFileInfo file;
    char path[64];
    require(argc == 2, "usage: native_pause_assets_test <iso>\n");
    require(aurora_dvd_open(argv[1]), "open ISO\n");
    snprintf(path, sizeof(path), "/%s", lbFileGetFullName("GmPause"));
    require(DVDOpen(path, &file), "open GmPause archive\n");
    size_t size = file.length;
    size_t padded = (size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    void* arena = _aligned_malloc(GM_PAUSE_IMPORT_ARENA_SIZE,
                                  sizeof(void*));
    require(bytes != NULL && arena != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                DVDClose(&file),
            "read GmPause archive\n");

    GmPauseAssets assets;
    if (!gmPauseAssets_Import(bytes, size, arena,
                              GM_PAUSE_IMPORT_ARENA_SIZE,
                              GM_PAUSE_IMPORT_RECORD_CAPACITY, &assets))
    {
        fprintf(stderr, "pause import failed: status=%d offset=0x%lX "
                        "records=%lu arena=%zu\n", assets.import.status,
                assets.import.error_offset, assets.import.record_count,
                assets.import.arena_used);
        return 1;
    }
    require(assets.scene.models == assets.models &&
                assets.models[0] == &assets.model &&
                assets.models[1] == NULL &&
                assets.model.joint != NULL &&
                assets.model.matanims == assets.matanims &&
                assets.matanims[0] != NULL &&
                assets.matanims[1] == NULL,
            "native pause scene and animation\n");
    printf("records=%lu arena=%zu\n", assets.import.record_count,
           assets.import.arena_used);

    GmPauseAssets undersized;
    require(!gmPauseAssets_Import(bytes, size, arena, 32,
                                  GM_PAUSE_IMPORT_RECORD_CAPACITY,
                                  &undersized) &&
                undersized.import.status == HSD_DAT_CAPACITY,
            "reject undersized pause arena\n");
    _aligned_free(arena);
    _aligned_free(bytes);
    aurora_dvd_close();
    return 0;
}
