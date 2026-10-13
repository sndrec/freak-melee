#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_article_refs.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static NativeFighterParameters parameters;

static void put32(unsigned char* p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (unsigned char)(value >> (24 - 8 * i));
}

static void source_dat(unsigned char bytes[120])
{
    memset(bytes, 0, 120);
    put32(bytes, 120); put32(bytes + 4, 64);
    put32(bytes + 8, 1); put32(bytes + 12, 1);
    put32(bytes + 32 + 4, 32); // Fighter root -> extended attributes.
    put32(bytes + 32 + 32, UINT32_C(0x3fa00000));
    put32(bytes + 32 + 36, UINT32_C(0xffffffef));
    bytes[32 + 40] = 1; bytes[32 + 41] = 233;
    put32(bytes + 96, 4); // Relocation.
    put32(bytes + 100, 0); put32(bytes + 104, 0); // Root and name index.
    memcpy(bytes + 108, "ftDataMario", 12);
}

static void parser_bounds(void)
{
    static char text[NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 2];
    size_t size = 0;
    for (unsigned i = 0; i < NATIVE_FIGHTER_PARAMETER_CAPACITY; ++i) {
        char name[128];
        memset(name, 'a', 127); name[127] = 0;
        snprintf(name, 5, "p%03u", i); name[4] = 'a';
        int count = snprintf(text + size, sizeof(text) - size, "int %s = %u\n", name, i);
        assert(count > 0 && (size_t)count < sizeof(text) - size);
        size += (size_t)count;
    }
    assert(size > 32 * 1024);
    assert(nativeFighterParameters_Parse(text, size, &parameters, NULL));
    assert(parameters.count == 256 && parameters.fields[255].bits == 255);
    unsigned line = 0;
    strcpy(text + size, "int overflow = 1\n");
    assert(!nativeFighterParameters_Parse(text, strlen(text), &parameters, &line));
    assert(line == 257);
    for (unsigned at = 0; at < NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY; at += 256) {
        memset(text + at, ' ', 255); text[at] = '#'; text[at + 255] = '\n';
    }
    assert(nativeFighterParameters_Parse(text, NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY,
        &parameters, NULL));
    assert(!parameters.count);
    text[NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY] = '\n';
    assert(!nativeFighterParameters_Parse(text, NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1,
        &parameters, NULL));
}

static void imported_module(bool callbacks, const char* path)
{
    unsigned char dat[120]; source_dat(dat);
    static char source[32768];
    size_t size = 0;
    static const char* types[] = { "float", "int", "bool", "byte" };
    static const unsigned offsets[] = { 0, 4, 8, 9 };
    for (unsigned i = 0; i < 257; ++i) {
        size += (size_t)snprintf(source + size, sizeof(source) - size,
            "attribute %s p%03u %u\n", types[i % 4], i, offsets[i % 4]);
        if (i != 255) continue;
        size_t declarations_size = size;
        const char* body = callbacks
            ? "move Capacity:\n    on state_341_physics:\n"
            : "action Capacity:\n    on load:\n";
        size += (size_t)snprintf(source + size, sizeof(source) - size,
            "%s        velocity.x = attrs.p000\n        velocity.y = attrs.p129\n"
            "        command(0) = attrs.p254\n        command(1) = attrs.p255\n", body);
        char *owned = NULL, *values = NULL; size_t owned_size = 0, values_size = 0;
        unsigned line = 0;
        assert(nativeFighterParameters_Import(dat, sizeof(dat), "ftDataMario", 0,
            source, size, &owned, &owned_size, &values, &values_size, &line));
        assert(nativeFighterParameters_Parse(values, values_size, &parameters, NULL));
        assert(parameters.count == 256);
        assert(parameters.fields[0].bits == UINT32_C(0x3fa00000));
        assert(parameters.fields[129].bits == UINT32_C(0xffffffef));
        assert(parameters.fields[254].bits == 1 && parameters.fields[255].bits == 233);
        FILE* output = path ? fopen(path, "w") : tmpfile(); assert(output);
        NativeFighterScriptError error = { 0 };
        bool valid = nativeFighterScript_GenerateC(owned, owned_size, "capacity_probe",
            NULL, &parameters, callbacks ? &(const NativeFighterScriptConstruction){"ftDataMario",341,1} : NULL, output, &error);
        if (!valid) fprintf(stderr, "Capacity module at %u: %s\n", error.line, error.message);
        assert(valid);
        fprintf(output, "\nstatic const uint64_t test_parameter_schema = 0x%016llxULL;\n",
            (unsigned long long)parameters.schema_hash);
        fclose(output);
        free(owned); free(values);
        size = declarations_size; // Append only the 257th import for rejection.
    }
    char *owned = NULL, *values = NULL; size_t owned_size = 0, values_size = 0;
    assert(!nativeFighterParameters_Import(dat, sizeof(dat), "ftDataMario", 0,
        source, size, &owned, &owned_size, &values, &values_size, NULL));
    assert(!owned && !values && !owned_size && !values_size);
}

static void behavior_source_bounds(const NativeFighterScriptConstruction* construction)
{
    const size_t capacity = NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY;
    assert(capacity == 512 * 1024);
    char* source = malloc(capacity + 2); assert(source);
    const char* body = "action SourceBound:\n    on load:\n        pass\n";
    size_t used = strlen(body); memcpy(source, body, used);
    while (used < capacity + 1) {
        size_t length = capacity + 1 - used;
        if (length > 256) length = 256;
        memset(source + used, ' ', length);
        source[used] = '#';
        if (length > 1) source[used + length - 1] = '\n';
        used += length;
    }
    source[used] = 0;
    const size_t sizes[] = { 128 * 1024 + 1, capacity, capacity + 1 };
    NativeScriptArticleCatalog* catalog = malloc(sizeof(*catalog)); assert(catalog);
    unsigned char dat[120]; source_dat(dat);
    for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        size_t size = sizes[i];
        bool expected = size <= capacity;
        NativeFighterScriptError error = { 0 };
        assert(nativeScriptArticleRefs_CatalogParse(catalog, source, size, &error) == expected);
        FILE* output = tmpfile(); assert(output);
        assert(nativeFighterScript_GenerateC(source, size, "source_bound",
            NULL, NULL, construction, output, &error) == expected);
        fclose(output);
        char *owned = NULL, *values = NULL; size_t owned_size = 0, values_size = 0;
        assert(nativeFighterParameters_Import(dat, sizeof(dat), "ftDataMario", 0,
            source, size, &owned, &owned_size, &values, &values_size, NULL) == expected);
        if (expected) {
            assert(owned_size == size && !memcmp(owned, source, size));
            assert(!values_size);
        } else assert(!owned && !values);
        free(owned); free(values);
    }
    free(catalog); free(source);
}

int main(int argc, char** argv)
{
    assert(NATIVE_FIGHTER_PARAMETER_CAPACITY == 256);
    parser_bounds();
    behavior_source_bounds(NULL);
    imported_module(false, argc == 3 ? argv[1] : NULL);
    imported_module(true, argc == 3 ? argv[2] : NULL);
    puts("256 owned parameters and 512-KiB behavior parse/import/generate; oversized inputs rejected");
    return 0;
}
