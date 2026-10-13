#include <aurora/aurora.h>
#include <aurora/dvd.h>
extern "C" {
#include <sysdolphin/baselib/devcom.h>
#include <sysdolphin/baselib/dat_rumble.h>
#include <sysdolphin/baselib/dat_sis.h>
#include <sysdolphin/baselib/dat_audio.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/dat_particle.h>
#include <melee/ef/efassets.h>
#include <melee/lb/lbrefract_data.h>
#include <melee/mn/mncharsel_assets.h>
#include <melee/mn/mnstagesel_assets.h>
#include <melee/pl/plcommon_data.h>
#include <sysdolphin/baselib/rumble.h>
#include <sysdolphin/baselib/synth.h>
#include <dolphin/ar.h>
#include <dolphin/dvd.h>
}
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_timer.h>

namespace aurora { extern AuroraConfig g_config; }
static SDL_ThreadID owner;
static unsigned completions;
static int context;
static void require(bool ok, const char* message)
{
    if (!ok) { std::fputs(message, stderr); std::exit(1); }
}
static void complete(int request, void* args, void* buffer, bool canceled)
{
    require(SDL_GetCurrentThreadID() == owner, "DVD game callback on worker\n");
    require(args == &context && !canceled, "DVD completion context\n");
    ++completions;
}
static void wait_for(unsigned expected)
{
    Uint64 deadline = SDL_GetTicksNS() + 5000000000ULL;
    while (completions < expected && SDL_GetTicksNS() < deadline) {
        HSD_DevComPoll();
        SDL_Delay(1);
    }
    require(completions == expected && !HSD_DevComIsBusy(1), "DVD transfer timeout/state\n");
}
int main(int argc, char** argv)
{
    require(argc == 2, "expected ISO path\n");
    owner = SDL_GetCurrentThreadID();
    // Exercise the real DevCom relay allocator rather than overriding symbols
    // from the linked runtime. Its transfer buffers belong to the audio heap.
    alignas(32) static u8 audio_heap[1024 * 1024];
    void* heap_start = OSInitAlloc(audio_heap, audio_heap + sizeof(audio_heap), 1);
    require(heap_start != nullptr, "audio allocator startup\n");
    HSD_Synth_804D6018 = OSCreateHeap(heap_start, audio_heap + sizeof(audio_heap));
    require(HSD_Synth_804D6018 >= 0, "audio transfer heap\n");
    aurora::g_config.mem2Size = 16 * 1024 * 1024;
    u32 lengths[4];
    require(ARInit(lengths, 4) == 0x4000, "ARAM startup\n");
    require(aurora_dvd_open(argv[1]), "open test ISO\n");
    int entry = DVDConvertPathToEntrynum("/PlMrNr.dat");
    require(entry >= 0, "find original fighter asset\n");
    DVDFileInfo file{};
    require(DVDFastOpen(entry, &file), "open original fighter asset\n");
    size_t length = file.length & ~size_t(31);
    require(length > 0x4000 && length < 1024 * 1024, "asset size\n");
    auto* reference = static_cast<unsigned char*>(_aligned_malloc(length, 32));
    auto* received = static_cast<unsigned char*>(_aligned_malloc(length, 32));
    require(reference && received, "test buffers\n");
    require(DVDReadPrio(&file, reference, length, 0, 2) == length, "reference read\n");
    require(DVDClose(&file), "close reference asset\n");
    HSD_DevComRequest(entry, 0, reinterpret_cast<uintptr_t>(received), length,
                      0x21, 1, complete, &context);
    wait_for(1);
    require(std::memcmp(reference, received, length) == 0, "DVD to native RAM bytes\n");
    HSD_DevComRequest(entry, 0, 0x4000, length, 0x23, 1, complete, &context);
    wait_for(2);
    require(std::memcmp(reference, static_cast<unsigned char*>(ARGetStorageAddress()) + 0x4000,
                        length) == 0, "DVD to ARAM bytes\n");
    HSD_DevComRequest(entry, 16, reinterpret_cast<uintptr_t>(received), length - 32,
                      0x21, 1, complete, &context);
    wait_for(3);
    require(std::memcmp(reference + 16, received, length - 32) == 0,
            "DVD metadata offset inside a DMA block\n");
    require(DVDOpen("/LbRb.dat", &file), "open rumble archive\n");
    size_t rumble_size = file.length;
    require(DVDReadPrio(&file, received, (rumble_size + 31) & ~size_t(31), 0, 2)
                >= rumble_size, "read rumble archive\n");
    require(DVDClose(&file), "close rumble archive\n");
    HSD_Dat dat{};
    u32 pattern_count, word_count;
    require(HSD_DatOpen(&dat, received, rumble_size) == HSD_DAT_OK &&
                HSD_DatRumbleLayout(&dat, &pattern_count, &word_count) &&
                pattern_count == 40 && word_count == 256, "original rumble layout\n");
    HSD_RumblePattern patterns[40]{};
    u16 words[256]{};
    require(!HSD_DatImportRumble(&dat, patterns, 39, words, 256) &&
                !HSD_DatImportRumble(&dat, patterns, 40, words, 255),
            "rumble capacity checks\n");
    require(HSD_DatImportRumble(&dat, patterns, 40, words, 256), "rumble import\n");
    for (u32 i = 0; i < 256; ++i)
        require(words[i] == (u16(dat.data[2 * i]) << 8 | dat.data[2 * i + 1]),
                "rumble word endian conversion\n");
    for (u32 i = 0; i < 40; ++i) {
        const u8* record = dat.data + 512 + 8 * i;
        u32 offset = u32(record[0]) << 24 | u32(record[1]) << 16 |
                     u32(record[2]) << 8 | record[3];
        require(patterns[i].commands == words + offset / 2 &&
                    patterns[i].priority == record[4], "rumble native record\n");
    }
    u16 program[] = { 0x4002, 0x2001, 0 };
    HSD_PadRumbleListData rumble{};
    rumble.frame = -2;
    rumble.headp = rumble.listp = program;
    u8 motor = 0;
    require(!HSD_PadRumbleInterpret1(&rumble, &motor) && motor == 1 &&
                !HSD_PadRumbleInterpret1(&rumble, &motor) && motor == 1 &&
                !HSD_PadRumbleInterpret1(&rumble, &motor) && motor == 2 &&
                HSD_PadRumbleInterpret1(&rumble, &motor),
            "native rumble opcode and wait timing\n");
    require(DVDOpen("/LbRf.dat", &file), "open refraction archive\n");
    size_t refract_size = file.length;
    require(DVDReadPrio(&file, received,
                        (refract_size + 31) & ~size_t(31), 0, 2) >=
                refract_size &&
                DVDClose(&file),
            "read refraction archive\n");
    lbRefractData refract{};
    f32 refract_parameters[32]{};
    require(lbRefractData_Import(received, refract_size, &refract,
                                 refract_parameters, 32) &&
                refract.count > 0 &&
                refract.parameters == refract_parameters,
            "refraction data import\n");
    require(!lbRefractData_Import(received, refract_size, &refract,
                                  refract_parameters,
                                  refract.count * 2 - 1),
            "refraction data capacity check\n");
    struct EffectArchive {
        const char* file;
        const char* root;
    };
    static const EffectArchive effect_archives[] = {
        { "/EfCoData.dat", "effCommonDataTable" },
        { "/EfMrData.dat", "effMarioDataTable" },
        { "/EfSsData.dat", "effSamusDataTable" },
        { "/EfFxData.dat", "effFoxDataTable" },
        { "/EfCaData.dat", "effCaptainDataTable" },
        { "/EfKbData.dat", "effKirbyDataTable" },
        { "/EfLkData.dat", "effLinkDataTable" },
        { "/EfPkData.dat", "effPikachuDataTable" },
        { "/EfDkData.dat", "effDonkeyDataTable" },
        { "/EfYsData.dat", "effYoshiDataTable" },
        { "/EfNsData.dat", "effNessDataTable" },
        { "/EfPrData.dat", "effPurinDataTable" },
        { "/EfKpData.dat", "effKoopaDataTable" },
        { "/EfMtData.dat", "effMewtwoDataTable" },
        { "/EfIcData.dat", "effIceclimberDataTable" },
        { "/EfPeData.dat", "effPeachDataTable" },
        { "/EfMsData.dat", "effMarsDataTable" },
        { "/EfZdData.dat", "effZeldaDataTable" },
        { "/EfLgData.dat", "effLuigiDataTable" },
        { "/EfGnData.dat", "effGanonDataTable" },
        { "/EfKbMs.dat", "effKirbyMarsDataTable" },
        { "/EfKbZd.dat", "effKirbyZeldaDataTable" },
        { "/EfMnData.dat", "effMenuDataTable" },
        { "/EfKbMr.dat", "effKirbyMarioDataTable" },
        { "/EfKbFx.dat", "effKirbyFoxDataTable" },
        { "/EfKbSs.dat", "effKirbySamusDataTable" },
        { "/EfKbPk.dat", "effKirbyPikachuDataTable" },
        { "/EfKbLg.dat", "effKirbyLuigiDataTable" },
        { "/EfKbCa.dat", "effKirbyCaptainDataTable" },
        { "/EfKbDk.dat", "effKirbyDonkeyDataTable" },
        { "/EfKbKp.dat", "effKirbyKoopaDataTable" },
        { "/EfKbIc.dat", "effKirbyIceDataTable" },
        { "/EfKbGn.dat", "effKirbyGanonDataTable" },
        { "/EfKbFe.dat", "effKirbyEmblemDataTable" },
        { "/EfFeData.dat", "effEmblemDataTable" },
    };
    for (size_t effect_index = 0;
         effect_index < sizeof(effect_archives) / sizeof(effect_archives[0]);
         ++effect_index)
    {
        const EffectArchive& archive = effect_archives[effect_index];
        require(DVDOpen(const_cast<char*>(archive.file), &file),
                "open effect archive\n");
        size_t effect_size = file.length;
        auto* effect_bytes = static_cast<u8*>(
            _aligned_malloc((effect_size + 31) & ~size_t(31), 32));
        require(effect_bytes &&
                    DVDReadPrio(&file, effect_bytes,
                                (effect_size + 31) & ~size_t(31), 0, 2) >=
                        effect_size &&
                    DVDClose(&file),
                "read effect archive\n");
        size_t effect_arena_size = effect_size * 4 + 2 * 1024 * 1024;
        auto* effect_arena =
            static_cast<u8*>(std::malloc(effect_arena_size));
        require(effect_arena != nullptr, "effect import arena\n");
        HSD_DatResult effect_status;
        u32 effect_error;
        EF_ImportedData* effects = efAssets_Import(
            effect_bytes, effect_size, archive.root, effect_arena,
            effect_arena_size, &effect_status, &effect_error);
        if (effects == nullptr)
            std::fprintf(stderr, "%s import status=%d offset=%lu\n",
                         archive.file, effect_status, effect_error);
        require(effects != nullptr &&
                    (effects->effect_count > 0 ||
                     effects->particle_bank.command_count > 0),
                "typed effect archive import\n");
        if (effect_index == 0) {
            require(effects->effect_count == 47 &&
                        effects->particle_bank.command_count == 592 &&
                        effects->particle_bank.texture_group_count == 36,
                    "common effect archive dimensions\n");
        } else if (effect_index == 4) {
            require(effects->particle_bank.command_count == 4017 &&
                        effects->particle_bank.commands[3999] == nullptr &&
                        effects->particle_bank.commands[4000] != nullptr,
                    "character particle command base\n");
        }
        std::free(effect_arena);
        _aligned_free(effect_bytes);
    }
    require(DVDOpen("/PdPm.dat", &file), "open player common archive\n");
    size_t player_common_size = file.length;
    auto* player_common_bytes = static_cast<u8*>(
        _aligned_malloc((player_common_size + 31) & ~size_t(31), 32));
    require(player_common_bytes &&
                DVDReadPrio(&file, player_common_bytes,
                            (player_common_size + 31) & ~size_t(31), 0, 2) >=
                    player_common_size &&
                DVDClose(&file),
            "read player common archive\n");
    alignas(4) u8 player_common[PL_COMMON_DATA_SIZE]{};
    require(plCommonData_Import(player_common_bytes, player_common_size,
                                reinterpret_cast<pl_804D6470_t*>(player_common)),
            "player common data import\n");
    require(!plCommonData_Import(player_common_bytes, player_common_size - 1,
                                 reinterpret_cast<pl_804D6470_t*>(player_common)),
            "player common data truncation check\n");
    _aligned_free(player_common_bytes);
    const char* menus[] = { "/MnMaAll.dat", "/MnMaAll.usd" };
    for (const char* menu : menus) {
        require(DVDFastOpen(DVDConvertPathToEntrynum(menu), &file), "open menu archive\n");
        size_t menu_size = file.length;
        size_t padded = (menu_size + 31) & ~size_t(31);
        auto* menu_bytes = static_cast<u8*>(_aligned_malloc(padded, 32));
        require(menu_bytes && DVDReadPrio(&file, menu_bytes, padded, 0, 2) >= menu_size,
                "read menu archive\n");
        require(DVDClose(&file), "close menu archive\n");
        require(HSD_DatOpen(&dat, menu_bytes, menu_size) == HSD_DAT_OK,
                "open menu DAT\n");
        alignas(8) static u8 arena[4 * 1024 * 1024];
        HSD_DatImport importer{};
        u32 camera_offset, fog_offset;
        require(HSD_DatImportInit(&importer, &dat, arena, sizeof(arena), 128) == HSD_DAT_OK &&
                    HSD_DatFindRoot(&dat, "ScMenMain_cam_int1_camera", &camera_offset) &&
                    HSD_DatFindRoot(&dat, "ScMenMain_fog", &fog_offset), "menu scene roots\n");
        auto* camera = static_cast<HSD_CObjDesc*>(HSD_DatImportGraph(&importer, camera_offset, DAT_CAMERA));
        auto* fog = static_cast<HSD_FogDesc*>(HSD_DatImportGraph(&importer, fog_offset, DAT_FOG));
        require(camera && fog && camera->common.projection_type == PROJ_PERSPECTIVE &&
                    camera->common.viewport.xmax == 640 && camera->common.viewport.ymax == 480 &&
                    camera->common.scissor.right == 640 && camera->common.scissor.bottom == 480 &&
                    camera->common.nnear == 1.0f && camera->common.ffar == 5000.0f &&
                    camera->common.eyepos && camera->common.interest &&
                    fog->start == 60.0f && fog->end == 250.0f &&
                    fog->color.b == 25 && fog->color.a == 255,
                "original menu camera/fog values\n");
        require(HSD_DatImportGraph(&importer, camera_offset, DAT_CAMERA) == camera,
                "shared camera descriptor identity\n");
        u32 lights_offset;
        require(HSD_DatFindRoot(&dat, "ScMenMain_scene_lights", &lights_offset),
                "menu light root\n");
        for (u32 i = 0; i < 4; ++i) {
            u32 list_offset, desc_offset;
            const char* external;
            auto kind = HSD_DatResolve(&dat, lights_offset + i * 4, &list_offset, &external);
            if (i == 3) {
                require(kind == HSD_DAT_REF_NULL, "menu light list terminator\n");
                break;
            }
            require(kind == HSD_DAT_REF_LOCAL &&
                        HSD_DatResolve(&dat, list_offset, &desc_offset, &external) == HSD_DAT_REF_LOCAL,
                    "menu light descriptor reference\n");
            auto* light = static_cast<HSD_LightDesc*>(HSD_DatImportGraph(&importer, desc_offset, DAT_LIGHT));
            const u8* source = dat.data + desc_offset;
            require(light && light->flags == (u16(source[8]) << 8 | source[9]) &&
                        light->attnflags == (u16(source[10]) << 8 | source[11]) &&
                        !std::memcmp(&light->color, source + 12, 4), "menu light values\n");
            u32 position_offset;
            kind = HSD_DatResolve(&dat, desc_offset + 16, &position_offset, &external);
            if (kind == HSD_DAT_REF_LOCAL) {
                float x, y, z;
                HSD_DatReadF32(&dat, position_offset + 4, &x);
                HSD_DatReadF32(&dat, position_offset + 8, &y);
                HSD_DatReadF32(&dat, position_offset + 12, &z);
                require(light->position && light->position->pos.x == x &&
                            light->position->pos.y == y && light->position->pos.z == z,
                        "menu light position\n");
            } else require(kind == HSD_DAT_REF_NULL && !light->position, "null light position\n");
        }
        HSD_DatImportInit(&importer, &dat, arena, sizeof(arena), 16384);
        u32 shapes = 0;
        for (u32 i = 0; i < dat.root_count; ++i) {
            u32 offset;
            const char* name;
            require(HSD_DatSymbol(&dat, false, i, &offset, &name), "menu root record\n");
            constexpr const char suffix[] = "_shapeanim_joint";
            size_t length = std::strlen(name);
            if (length < sizeof(suffix) - 1 ||
                std::strcmp(name + length - (sizeof(suffix) - 1), suffix)) continue;
            auto* shape = static_cast<HSD_ShapeAnimJoint*>(
                HSD_DatImportGraph(&importer, offset, DAT_SHAPE_JOINT));
            if (!shape) std::fprintf(stderr, "%s: import error %d at %lx\n",
                                     name, importer.status, importer.error_offset);
            require(shape != nullptr, "original menu shape animation graph\n");
            ++shapes;
        }
        require(shapes != 0, "menu shape roots were exercised\n");
        std::printf("%s: imported %lu shape roots, %lu descriptors\n",
                    menu, shapes, importer.record_count);
        u32 model_roots = 0;
        for (u32 i = 0; i < dat.root_count; ++i) {
            u32 offset;
            const char* name;
            require(HSD_DatSymbol(&dat, false, i, &offset, &name), "menu model root\n");
            HSD_DatImportType type;
            if (std::strstr(name, "_shapeanim_joint")) continue;
            if (std::strstr(name, "_matanim_joint")) type = DAT_MATANIM_JOINT;
            else if (std::strstr(name, "_animjoint")) type = DAT_ANIMJOINT;
            else if (std::strstr(name, "_joint")) type = DAT_JOINT;
            else continue;
            void* model = HSD_DatImportGraph(&importer, offset, type);
            if (!model) std::fprintf(stderr, "%s: import error %d at %lx\n",
                                     name, importer.status, importer.error_offset);
            require(model != nullptr, "original menu model/animation graph\n");
            ++model_roots;
        }
        std::printf("%s: imported %lu model/animation roots, %lu descriptors\n",
                    menu, model_roots, importer.record_count);
        menu_bytes[32 + camera_offset + 7] = 0;
        HSD_DatImportInit(&importer, &dat, arena, sizeof(arena), 128);
        require(!HSD_DatImportGraph(&importer, camera_offset, DAT_CAMERA) &&
                    importer.status == HSD_DAT_DESCRIPTOR, "invalid camera projection\n");
        _aligned_free(menu_bytes);
    }
    char css_archives[][13] = { "/MnSlChr.dat", "/MnSlChr.usd" };
    for (char* path : css_archives) {
        require(DVDOpen(path, &file), "open character-select archive\n");
        size_t size = file.length;
        size_t padded = (size + 31) & ~size_t(31);
        auto* bytes = static_cast<u8*>(_aligned_malloc(padded, 32));
        auto* arena = static_cast<u8*>(_aligned_malloc(16 * 1024 * 1024, 8));
        require(bytes && arena && DVDReadPrio(&file, bytes, padded, 0, 2) >= size,
                "read character-select archive\n");
        require(DVDClose(&file), "close character-select archive\n");
        u32 root;
        HSD_DatImport importer{};
        require(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
                    HSD_DatFindRoot(&dat, "MnSelectChrDataTable", &root) &&
                    HSD_DatImportInit(&importer, &dat, arena,
                                      16 * 1024 * 1024, 32768) == HSD_DAT_OK,
                "character-select import setup\n");
        MnSelectChrDataTable* table =
            mnCharSelAssets_Decode(&importer, root);
        if (!table) {
            std::fprintf(stderr, "%s: import error %d at %lx\n", path,
                         importer.status, importer.error_offset);
        }
        require(table && importer.status == HSD_DAT_OK,
                "character-select graph conversion\n");
        static_assert(sizeof(MnSelectChrDataTable) == 40 * sizeof(void*));
        void** fields = reinterpret_cast<void**>(table);
        for (u32 i = 0; i < 40; ++i) {
            u32 target;
            const char* external;
            HSD_DatReferenceKind kind =
                HSD_DatResolve(&dat, root + i * 4, &target, &external);
            require((kind == HSD_DAT_REF_LOCAL && fields[i] != nullptr) ||
                        (kind == HSD_DAT_REF_NULL && fields[i] == nullptr),
                    "character-select pointer conversion\n");
        }
        std::printf("%s: imported %lu descriptors into %zu bytes\n", path,
                    importer.record_count, importer.arena_used);
        _aligned_free(arena);
        _aligned_free(bytes);
    }
    char stage_select_archives[][13] = { "/MnSlMap.dat", "/MnSlMap.usd" };
    for (char* path : stage_select_archives) {
        require(DVDOpen(path, &file), "open stage-select archive\n");
        size_t size = file.length;
        size_t padded = (size + 31) & ~size_t(31);
        auto* bytes = static_cast<u8*>(_aligned_malloc(padded, 32));
        auto* arena = static_cast<u8*>(_aligned_malloc(16 * 1024 * 1024, 8));
        require(bytes && arena && DVDReadPrio(&file, bytes, padded, 0, 2) >= size,
                "read stage-select archive\n");
        require(DVDClose(&file), "close stage-select archive\n");
        u32 root;
        HSD_DatImport importer{};
        require(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
                    HSD_DatFindRoot(&dat, "MnSelectStageDataTable", &root) &&
                    HSD_DatImportInit(&importer, &dat, arena,
                                      16 * 1024 * 1024, 32768) == HSD_DAT_OK,
                "stage-select import setup\n");
        MnSelectStageDataTable* table =
            mnStageSelAssets_Decode(&importer, root);
        if (!table) {
            std::fprintf(stderr, "%s: import error %d at %lx\n", path,
                         importer.status, importer.error_offset);
        }
        require(table && importer.status == HSD_DAT_OK,
                "stage-select graph conversion\n");
        static_assert(sizeof(MnSelectStageDataTable) == 52 * sizeof(void*));
        void** fields = reinterpret_cast<void**>(table);
        for (u32 i = 0; i < 52; ++i) {
            u32 target;
            const char* external;
            HSD_DatReferenceKind kind =
                HSD_DatResolve(&dat, root + i * 4, &target, &external);
            require((kind == HSD_DAT_REF_LOCAL && fields[i] != nullptr) ||
                        (kind == HSD_DAT_REF_NULL && fields[i] == nullptr),
                    "stage-select pointer conversion\n");
        }
        std::printf("%s: imported %lu descriptors into %zu bytes\n", path,
                    importer.record_count, importer.arena_used);
        _aligned_free(arena);
        _aligned_free(bytes);
    }
    char text_archives[][12] = { "/SdMenu.dat", "/SdMenu.usd" };
    for (char* menu : text_archives) {
        require(DVDOpen(menu, &file), "open menu text archive\n");
        size_t size = file.length;
        auto* bytes = static_cast<unsigned char*>(_aligned_malloc((size + 31) & ~size_t(31), 32));
        require(bytes && DVDReadPrio(&file, bytes, (size + 31) & ~size_t(31), 0, 2) >= size,
                "read menu text archive\n");
        DVDClose(&file);
        u32 root, count;
        require(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
                    HSD_DatFindRoot(&dat, "SIS_MenuData", &root) &&
                    HSD_DatSisLayout(&dat, root, &count) && count == 1604,
                "original SIS pointer table layout\n");
        void* table[1604]{};
        require(!HSD_DatImportSis(&dat, root, table, 1603) && table[0] == nullptr,
                "SIS pointer capacity leaves destination unchanged\n");
        require(HSD_DatImportSis(&dat, root, table, 1604), "native SIS import\n");
        for (u32 i = 0; i < count; ++i) {
            u32 offset;
            require(HSD_DatReadU32(&dat, root + 4 * i, &offset) &&
                        table[i] == dat.data + offset, "SIS native pointer identity\n");
        }
        _aligned_free(bytes);
    }
    require(DVDOpen("/LbAd.dat", &file), "open audio-load archive\n");
    size_t audio_load_size = file.length;
    auto* audio_load_bytes = static_cast<unsigned char*>(
        _aligned_malloc((audio_load_size + 31) & ~size_t(31), 32));
    require(audio_load_bytes &&
                DVDReadPrio(&file, audio_load_bytes,
                            (audio_load_size + 31) & ~size_t(31), 0, 2) >=
                    audio_load_size,
            "read audio-load archive\n");
    DVDClose(&file);
    u32 audio_root, audio_value_count;
    require(HSD_DatOpen(&dat, audio_load_bytes, audio_load_size) == HSD_DAT_OK &&
                HSD_DatFindRoot(&dat, "lbAudioLoadData", &audio_root) &&
                HSD_DatAudioLoadLayout(&dat, audio_root,
                                       &audio_value_count) &&
                audio_value_count == 3360,
            "original audio-load layout\n");
    HSD_AudioLoadData audio_load{};
    u32* audio_lists[HSD_AUDIO_LOAD_TABLES * HSD_AUDIO_LOAD_LISTS]{};
    u32 audio_values[3360]{};
    require(!HSD_DatImportAudioLoad(&dat, audio_root, &audio_load,
                                    audio_lists,
                                    ARRAY_SIZE(audio_lists) - 1,
                                    audio_values, ARRAY_SIZE(audio_values)) &&
                audio_load.tables[0] == nullptr,
            "audio-load capacity leaves destination unchanged\n");
    require(HSD_DatImportAudioLoad(&dat, audio_root, &audio_load,
                                   audio_lists, ARRAY_SIZE(audio_lists),
                                   audio_values, ARRAY_SIZE(audio_values)),
            "native audio-load import\n");
    for (u32 language = 0; language < HSD_AUDIO_LOAD_TABLES; ++language) {
        for (u32 list = 0; list < HSD_AUDIO_LOAD_LISTS; ++list) {
            u32 target;
            const char* external;
            require(HSD_DatResolve(&dat,
                        audio_value_count * 4 +
                            (language * HSD_AUDIO_LOAD_LISTS + list) * 4,
                        &target, &external) == HSD_DAT_REF_LOCAL &&
                    audio_load.tables[language][list] ==
                        audio_values + target / 4,
                    "audio-load native list identity\n");
            bool terminated = false;
            for (u32* value = audio_load.tables[language][list];
                 value < audio_values + ARRAY_SIZE(audio_values); ++value) {
                if (*value == 0x83D60) {
                    terminated = true;
                    break;
                }
            }
            require(terminated, "audio-load native list terminator\n");
        }
    }
    _aligned_free(audio_load_bytes);
    aurora_dvd_close();
    _aligned_free(reference);
    _aligned_free(received);
    std::printf("Original asset DVD transfers verified: %zu bytes per path\n", length);
    return 0;
}
