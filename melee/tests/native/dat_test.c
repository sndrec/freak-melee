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
    test_dat_textures();
    test_dat_import();
    test_dat_animation();
    test_dat_model();
    return 0;
}
