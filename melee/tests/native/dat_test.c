#include <sysdolphin/baselib/dat.h>
#include <sysdolphin/baselib/dat_texture.h>
#include <sysdolphin/baselib/dat_import.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_dat_textures(void);
void test_dat_import(void);
void test_dat_animation(void);
void test_dat_model(void);

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static void put32(u8* bytes, u32 value)
{
    bytes[0] = (u8) (value >> 24);
    bytes[1] = (u8) (value >> 16);
    bytes[2] = (u8) (value >> 8);
    bytes[3] = (u8) value;
}

static void test_relocation_order(void)
{
    enum { SLOT_COUNT = 256, DATA_SIZE = SLOT_COUNT * 8 };
    u8 bytes[32 + DATA_SIZE + SLOT_COUNT * 4] = { 0 };
    put32(bytes + 4, DATA_SIZE);
    for (int shape = 0; shape < 3; ++shape) {
        u32 count = shape == 2 ? HSD_DAT_LINEAR_RELOCATIONS : SLOT_COUNT;
        u32 length = 32 + DATA_SIZE + count * 4;
        put32(bytes, length);
        put32(bytes + 8, count);
        for (u32 i = 0; i < count; ++i) {
            u32 index;
            if (shape == 0) {
                index = count - 1 - i;
            } else if (shape == 1) {
                index = i < count / 2 ? i * 2 : (count - i) * 2 - 1;
            } else {
                index = i % 2 ? count - 1 - i / 2 : i / 2;
            }
            put32(bytes + 32 + DATA_SIZE + i * 4, index * 8);
            put32(bytes + 32 + index * 8, (count - index - 1) * 4);
        }
        HSD_Dat dat;
        require(HSD_DatOpen(&dat, bytes, length) == HSD_DAT_OK,
                "Could not open an unordered relocation table");
        for (u32 i = 0; i < count; ++i) {
            u32 target;
            const char* external;
            require(HSD_DatResolve(&dat, i * 8, &target, &external) ==
                        HSD_DAT_REF_LOCAL && target == (count - i - 1) * 4,
                    "Unordered table resolved the wrong local target");
            require(HSD_DatResolve(&dat, i * 8 + 4, &target, &external) ==
                        HSD_DAT_REF_NULL, "Unordered table matched an absent slot");
        }
    }
    put32(bytes, sizeof(bytes));
    put32(bytes + 8, SLOT_COUNT);
    for (u32 i = 0; i < SLOT_COUNT; ++i) {
        u32 index = i % 2 ? SLOT_COUNT - 1 - i / 2 : i / 2;
        put32(bytes + 32 + DATA_SIZE + i * 4, index * 8);
    }
    HSD_Dat dat;
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_RESOURCE_LIMIT &&
                !dat.data, "Large fragmented table exceeded the lookup budget");
}

static void test_external_budget(void)
{
    enum { SLOTS = HSD_DAT_EXTERNAL_BUDGET, DATA_SIZE = SLOTS * 4 };
    u8 bytes[32 + DATA_SIZE + 8 + 2] = { 0 };
    put32(bytes, sizeof(bytes));
    put32(bytes + 4, DATA_SIZE);
    put32(bytes + 16, 1);
    for (u32 i = 0; i < SLOTS - 1; ++i) {
        put32(bytes + 32 + i * 4, (i + 1) * 4);
    }
    put32(bytes + 32 + DATA_SIZE - 4, U32_MAX);
    put32(bytes + 32 + DATA_SIZE, 4);
    memcpy(bytes + sizeof(bytes) - 2, "x", 2);
    HSD_Dat dat;
    u32 value;
    const char* name;
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK &&
                HSD_DatResolve(&dat, DATA_SIZE - 4, &value, &name) ==
                    HSD_DAT_REF_EXTERNAL && strcmp(name, "x") == 0,
            "Could not resolve the last external at the budget limit");
    put32(bytes + 32 + DATA_SIZE, 0);
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_RESOURCE_LIMIT &&
                !dat.data, "External table exceeded its lookup budget");
}

static void test_root_budget(void)
{
    enum { DATA_END = 36, ROOTS = HSD_DAT_MAX_ROOTS, EXTRA = 1 };
    u8 file[DATA_END + (ROOTS + EXTRA) * 8 + HSD_DAT_MAX_SYMBOL_BYTES + 2] = { 0 };
    size_t length = DATA_END + ROOTS * 8 + HSD_DAT_MAX_SYMBOL_BYTES + 2;
    put32(file, (u32) length);
    put32(file + 4, 4);
    put32(file + 12, ROOTS);
    u8* names = file + DATA_END + ROOTS * 8;
    memset(names, 'a', HSD_DAT_MAX_SYMBOL_BYTES);
    // One root uses the final letter of the same string.
    put32(file + DATA_END + (ROOTS - 1) * 8 + 4, HSD_DAT_MAX_SYMBOL_BYTES - 1);
    HSD_Dat dat;
    u32 offset;
    require(HSD_DatOpen(&dat, file, length) == HSD_DAT_OK &&
                HSD_DatFindRoot(&dat, "a", &offset) && offset == 0,
            "Full root table and shared name suffix");
    names[HSD_DAT_MAX_SYMBOL_BYTES] = 'a';
    require(HSD_DatOpen(&dat, file, length) == HSD_DAT_RESOURCE_LIMIT && !dat.data,
            "Symbol name exceeds the archive limit");
    put32(file, sizeof(file));
    put32(file + 12, ROOTS + EXTRA);
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_RESOURCE_LIMIT && !dat.data,
            "Root count exceeds the archive limit");
}

static void test_target_index(void)
{
    enum { COUNT = 257, DATA_BYTES = COUNT * 4 };
    u8 file[32 + DATA_BYTES + COUNT * 4] = { 0 };
    put32(file, sizeof(file));
    put32(file + 4, DATA_BYTES);
    put32(file + 8, COUNT);
    for (u32 i = 0; i < COUNT; ++i) {
        // Many pointers share targets. Targets may be unaligned or at data end.
        put32(file + 32 + i * 4, i == 0 ? DATA_BYTES : (i * 73 % 157) * 5);
        put32(file + 32 + DATA_BYTES + i * 4, (COUNT - 1 - i) * 4);
    }
    HSD_Dat dat;
    HSD_DatImport import;
    u8 arena[2048];
    require(HSD_DatOpen(&dat, file, sizeof(file)) == HSD_DAT_OK &&
                HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 2) == HSD_DAT_OK &&
                import.sorted_targets, "Index a large archive");
    for (u32 at = 0; at <= DATA_BYTES + 1; ++at) {
        u32 next = at < 780 ? (at / 5 + 1) * 5 : DATA_BYTES;
        require(HSD_DatImportNextTarget(&import, at) == next &&
                    HSD_DatNextTarget(&dat, at) == next, "Next distinct target address");
    }
    require(HSD_DatImportInit(&import, &dat, arena, 128, 2) == HSD_DAT_CAPACITY,
            "An undersized arena does not publish an index");
}

static void inspect_file(const char* path, bool decode_image, HSD_DatImportType import_type, u32 image_offset)
{
    FILE* file = fopen(path, "rb");
    require(file != NULL, "Cannot open DAT input");
    require(fseek(file, 0, SEEK_END) == 0, "Cannot seek DAT input");
    long length = ftell(file);
    require(length >= 0 && fseek(file, 0, SEEK_SET) == 0, "Cannot size DAT input");
    u8* bytes = malloc((size_t) length);
    require(bytes && fread(bytes, 1, length, file) == length, "Cannot read DAT input");
    fclose(file);
    HSD_Dat dat;
    HSD_DatResult result = HSD_DatOpen(&dat, bytes, (size_t) length);
    if (result != HSD_DAT_OK) {
        fprintf(stderr, "%s: DAT parse result %d\n", path, result);
        exit(1);
    }
    printf("%s: %lu data bytes, %lu relocations, %lu roots, %lu externs\n",
           path, dat.data_size, dat.relocation_count, dat.root_count, dat.extern_count);
    if (decode_image) {
        HSD_ImageDesc image;
        require(HSD_DatDecodeImage(&dat, image_offset, &image, NULL) == HSD_DAT_OK,
                "Original image conversion failed");
        printf("  native image at %08lx: %u x %u, format %u, mipmap %lu, LOD %.2f..%.2f\n",
               image_offset, image.width, image.height, image.format, image.mipmap,
               image.minLOD, image.maxLOD);
    }
    if (import_type) {
        void* arena = malloc(1024 * 1024);
        HSD_DatImport import;
        require(arena != NULL && HSD_DatImportInit(&import, &dat, arena, 1024 * 1024, 8192) == HSD_DAT_OK,
                "Cannot initialize material import");
        void* graph = HSD_DatImportGraph(&import, image_offset, import_type);
        if (graph == NULL) {
            fprintf(stderr, "Material import failed: status %d, offset %08lx\n",
                    import.status, import.error_offset);
            exit(1);
        }
        printf("  native graph at %08lx: %lu records, %zu arena bytes\n",
               image_offset, import.record_count, import.arena_used);
        free(arena);
    }
    for (u32 i = 0; i < dat.root_count; ++i) {
        u32 offset;
        const char* name;
        HSD_DatSymbol(&dat, false, i, &offset, &name);
        printf("  %08lx %s\n", offset, name);
        size_t name_length = strlen(name);
        if (name_length >= 10 && strcmp(name + name_length - 10, "_tlut_desc") == 0) {
            HSD_TlutDesc palette;
            require(HSD_DatDecodePalette(&dat, offset, &palette) == HSD_DAT_OK,
                    "Original palette conversion failed");
            printf("    native palette: %u entries\n", palette.n_entries);
        }
    }
    free(bytes);
}

int main(int argc, char** argv)
{
    if (argc == 4 && (strcmp(argv[1], "--image") == 0 || strcmp(argv[1], "--material") == 0 ||
                     strcmp(argv[1], "--matanim") == 0 || strcmp(argv[1], "--joint") == 0)) {
        inspect_file(argv[3], strcmp(argv[1], "--image") == 0,
                     strcmp(argv[1], "--material") == 0 ? DAT_MOBJ :
                     strcmp(argv[1], "--matanim") == 0 ? DAT_MATANIM_JOINT :
                     strcmp(argv[1], "--joint") == 0 ? DAT_JOINT : 0,
                     strtoul(argv[2], NULL, 0));
        return 0;
    }
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            inspect_file(argv[i], false, false, 0);
        }
        return 0;
    }
    u8 bytes[102] = { 0 };
    u8 original[sizeof(bytes)];
    HSD_Dat dat;
    u32 value;
    f32 scalar;
    const char* name;
    put32(bytes, sizeof(bytes));
    put32(bytes + 4, 32);
    put32(bytes + 8, 2);
    put32(bytes + 12, 1);
    put32(bytes + 16, 1);
    put32(bytes + 32, 16);
    put32(bytes + 36, 0xBFC00000); // -1.5
    put32(bytes + 40, 12);
    put32(bytes + 44, U32_MAX);
    put32(bytes + 64, 20); // Unsorted relocation slots, including a target of zero.
    put32(bytes + 68, 0);
    put32(bytes + 80, 8);
    put32(bytes + 84, 5);
    memcpy(bytes + 88, "root\0external\0", 14);
    memcpy(original, bytes, sizeof(bytes));
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK, "Valid DAT rejected");
    require(HSD_DatContainsReference(&dat, 0, 4) &&
                HSD_DatContainsReference(&dat, 19, 24) &&
                HSD_DatContainsReference(&dat, 5, 12) &&
                !HSD_DatContainsReference(&dat, 1, 11) &&
                !HSD_DatContainsReference(&dat, 16, 20),
            "Range lookups include complete local and external slots");
    require(memcmp(bytes, original, sizeof(bytes)) == 0, "DAT open modified file bytes");
    require(dat.data_size == 32 && dat.relocation_count == 2 &&
                dat.root_count == 1 && dat.extern_count == 1, "Wrong native metadata");
    require(HSD_DatReadF32(&dat, 4, &scalar) && scalar == -1.5f, "Wrong float byte order");
    require(HSD_DatReadU32(&dat, 0, &value) && value == 16, "Wrong integer byte order");
    require(HSD_DatRelocation(&dat, 0, &value) && value == 20 &&
                HSD_DatReadU32(&dat, value, &value) && value == 0,
            "Relocation to offset zero was lost");
    require(HSD_DatFindRoot(&dat, "root", &value) && value == 0 &&
                !HSD_DatFindRoot(&dat, "absent", &value), "Root lookup failed");
    require(HSD_DatSymbol(&dat, true, 0, &value, &name) && value == 8 &&
                strcmp(name, "external") == 0, "External symbol lookup failed");
    require(HSD_DatResolve(&dat, 20, &value, &name) == HSD_DAT_REF_LOCAL && value == 0,
            "Local offset zero was mistaken for NULL");
    require(HSD_DatResolve(&dat, 16, &value, &name) == HSD_DAT_REF_NULL,
            "Unrelocated zero pointer was mistaken for a local object");
    require(HSD_DatResolve(&dat, 12, &value, &name) == HSD_DAT_REF_EXTERNAL &&
                strcmp(name, "external") == 0, "External chain slot was not identified");
    require(!HSD_DatReadU32(&dat, 30, &value) &&
                HSD_DatBytes(&dat, U32_MAX, 8) == NULL,
            "Out-of-range data access was accepted");

    require(HSD_DatOpen(&dat, bytes, 31) == HSD_DAT_HEADER && dat.data == NULL,
            "Truncated header accepted or stale view retained");
    put32(bytes + 8, U32_MAX);
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_TABLE, "Oversized table accepted");
    memcpy(bytes, original, sizeof(bytes));
    put32(bytes + 64, 31);
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_RELOCATION, "Invalid slot accepted");
    memcpy(bytes, original, sizeof(bytes));
    put32(bytes + 32, 33);
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_RELOCATION, "Invalid target accepted");
    memcpy(bytes, original, sizeof(bytes));
    bytes[101] = 'X';
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_SYMBOL, "Unterminated name accepted");
    memcpy(bytes, original, sizeof(bytes));
    put32(bytes + 40, 8);
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_EXTERNAL, "Cyclic external list accepted");
    puts("Immutable DAT metadata and endian reads passed");
    test_relocation_order();
    test_external_budget();
    test_root_budget();
    test_target_index();
    test_dat_textures();
    test_dat_import();
    test_dat_animation();
    test_dat_model();
    return 0;
}
