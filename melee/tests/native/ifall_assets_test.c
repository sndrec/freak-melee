#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/if/ifall_assets.h>

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
    require(argc == 2, "usage: native_ifall_assets_test <iso>\n");
    require(aurora_dvd_open(argv[1]) && DVDOpen("/IfAll.dat", &file),
            "open IfAll.dat\n");
    size_t size = file.length;
    size_t padded = (size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    void* arena = _aligned_malloc(IF_ALL_IMPORT_ARENA_SIZE,
                                  sizeof(void*));
    require(bytes != NULL && arena != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                DVDClose(&file),
            "read IfAll.dat\n");
    IfAllAssets assets;
    if (!ifAllAssets_Import(bytes, size, arena, IF_ALL_IMPORT_ARENA_SIZE,
                            IF_ALL_IMPORT_RECORD_CAPACITY, &assets))
    {
        fprintf(stderr, "IfAll import failed: status=%d offset=0x%lX "
                        "records=%lu arena=%zu\n", assets.import.status,
                assets.import.error_offset, assets.import.record_count,
                assets.import.arena_used);
        return 1;
    }
    for (u32 i = 0; i < IfAllModel_RootCount; ++i) {
        require(assets.models[i] != NULL &&
                    assets.models[i][0] != NULL &&
                    assets.models[i][0]->joint != NULL,
                "decoded HUD model root\n");
    }
    require(assets.models[IfAllModel_Count][7] != NULL &&
                assets.models[IfAllModel_Count][8] == NULL &&
                assets.models[IfAllModel_Stock][1] != NULL &&
                assets.models[IfAllModel_Stock][2] == NULL &&
                assets.scene.models[0]->joint != NULL &&
                assets.scene.cameras[0].desc != NULL &&
                assets.scene.lights[0]->desc != NULL,
            "complete native HUD scene\n");
    printf("roots=%u records=%lu arena=%zu\n", IfAllModel_RootCount + 1,
           assets.import.record_count, assets.import.arena_used);
    _aligned_free(arena);
    _aligned_free(bytes);
    aurora_dvd_close();
    return 0;
}
