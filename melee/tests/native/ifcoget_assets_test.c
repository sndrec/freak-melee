#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/if/ifcoget_assets.h>

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
    require(argc == 2, "usage: native_ifcoget_assets_test <iso>\n");
    require(aurora_dvd_open(argv[1]) && DVDOpen("/IfCoGet.dat", &file),
            "open IfCoGet.dat\n");
    size_t size = file.length;
    size_t padded = (size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    void* arena = _aligned_malloc(IF_COGET_IMPORT_ARENA_SIZE,
                                  sizeof(void*));
    require(bytes != NULL && arena != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                DVDClose(&file),
            "read IfCoGet.dat\n");
    IfCoGetAssets assets;
    if (!ifCoGetAssets_Import(bytes, size, arena,
                              IF_COGET_IMPORT_ARENA_SIZE,
                              IF_COGET_IMPORT_RECORD_CAPACITY, &assets))
    {
        fprintf(stderr, "IfCoGet import failed: status=%d offset=0x%lX "
                        "records=%lu arena=%zu\n", assets.import.status,
                assets.import.error_offset, assets.import.record_count,
                assets.import.arena_used);
        return 1;
    }
    require(assets.scene.models[0]->joint != NULL &&
                assets.scene.models[0]->anims[0] != NULL &&
                assets.scene.cameras[0].desc != NULL &&
                assets.scene.lights[0]->desc != NULL &&
                assets.scene.lights[1]->desc != NULL &&
                assets.scene.lights[2] == NULL,
            "complete native coin HUD scene\n");
    printf("records=%lu arena=%zu\n", assets.import.record_count,
           assets.import.arena_used);
    _aligned_free(arena);
    _aligned_free(bytes);
    aurora_dvd_close();
    return 0;
}
