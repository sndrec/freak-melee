#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdevice.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftmaterial.h>
#include <melee/ft/inlines.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tev.h>
#include <sysdolphin/baselib/texp.h>
#include <melee/lb/lb_00B0.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static unsigned checks, failures, import_cases, query_cases, material_pairs, invalid_material_cases;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; fprintf(stderr, "failure line %d: %s\n", __LINE__, #c); } } while (0)
ftCommonData* p_ftCommonData;
static ColorOverlay overlay;
typedef struct { unsigned colors, stages; u8 rgb[4][3]; int b, c, alpha, kcsel; } MaterialTrace;
static MaterialTrace trace;
ColorOverlay* ftCo_800C0658(Fighter* fighter) { (void)fighter; return &overlay; }
s32 lbGetFreeColorRegister(s32 start, HSD_MObj* mobj, HSD_TExp* expression)
{ (void)start; (void)mobj; return expression == NULL ? 0 : 1; }
s32 lb_8000CC8C(s32 reg) { return reg; }
s32 lb_8000CCA4(s32 reg) { return reg + 10; }
s32 lb_8000CD90(s32 reg) { return reg + 20; }
s32 lb_8000CDA8(s32 reg) { return reg + 30; }
int HSD_StateAssignTev(void) { return 2; }
void HSD_TExpSetReg(HSD_TExp* expression)
{
    CHECK(trace.colors < 4);
    if (trace.colors < 4) memcpy(trace.rgb[trace.colors++], expression->cnst.val, 3);
}
void HSD_SetupTevStage(HSD_TevDesc* stage)
{
    ++trace.stages;
    trace.b = stage->u.tevconf.clr_b;
    trace.c = stage->u.tevconf.clr_c;
    trace.alpha = stage->u.tevconf.alpha_d;
    trace.kcsel = stage->u.tevconf.kcsel;
}
void OSReport(char* format, ...) { (void)format; abort(); }
void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "%s:%u %s\n", file, (unsigned)line, expression); abort(); }

#include "query_material_bodies.h"
bool palette_legacy_import(const void*, size_t, void*, size_t, u32, FtCommonAssets*);

static void write_word(u8* bytes, u32 at, u32 value)
{ for (int i = 0; i < 4; ++i) bytes[32 + at + i] = (u8)(value >> (24 - 8 * i)); }
static void test_import(u8* bytes, size_t size)
{
    enum { ARENA_SIZE = 16 * 1024 * 1024 };
    void* arena = malloc(ARENA_SIZE);
    FtCommonAssets* assets = calloc(1, sizeof(*assets));
    HSD_Dat dat; u32 root, common; const char* external;
    CHECK(arena && assets);
    CHECK(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK);
    CHECK(HSD_DatFindRoot(&dat, "ftLoadCommonData", &root));
    CHECK(HSD_DatResolve(&dat, root, &common, &external) == HSD_DAT_REF_LOCAL);
    u8 original[24]; memcpy(original, bytes + 32 + common + 0x6D8, sizeof(original));
    CHECK(palette_legacy_import(bytes, size, arena, ARENA_SIZE, 32768, assets));
    CHECK(memcmp(assets->common.x6DC_colorsByPlayer,
                 bytes + 32 + common + 0x6DC, 16) != 0);
    /* Real old importer demonstrably reverses RGBA bytes on this host. */
    for (unsigned i = 0; i < 5; ++i)
        for (unsigned j = 0; j < 4; ++j)
            CHECK(((u8*)&assets->common.x6DC_colorsByPlayer)[i*4+j] ==
                  bytes[32 + common + 0x6DC + i*4 + 3-j]);
    for (unsigned variant = 0; variant < 17; ++variant) {
        if (variant) {
            write_word(bytes, common + 0x6D8, 0xA17E0000u + variant);
            for (unsigned i = 0; i < 20; ++i)
                bytes[32 + common + 0x6DC + i] = (u8)(variant * 19 + i * 13);
        }
        CHECK(ftCommonAssets_Import(bytes, size, arena, ARENA_SIZE, 32768, assets));
        ++import_cases;
        CHECK(memcmp(assets->common.x6DC_colorsByPlayer, bytes + 32 + common + 0x6DC, 16) == 0);
        CHECK(memcmp(&assets->common.x6EC_specialMaterialColor, bytes + 32 + common + 0x6EC, 4) == 0);
        u32 scalar;
        CHECK(HSD_DatReadU32(&dat, common + 0x6D8, &scalar));
        CHECK((u32)assets->common.x6D8[0] == scalar);
        CHECK(offsetof(ftCommonData, x6EC_specialMaterialColor) == 0x6EC);
        CHECK(sizeof(ftCommonData) == 0x818);
    }
    memcpy(bytes + 32 + common + 0x6D8, original, sizeof(original));
    CHECK(!ftCommonAssets_Import(bytes, size - 1, arena, ARENA_SIZE, 32768, assets));
    free(assets); free(arena);
}
static void test_queries(void)
{
    static Fighter fighter; HSD_GObj gobj = {0}; gobj.user_data = &fighter;
    static ftCommonData common; p_ftCommonData = &common;
    const GXColor colors[4] = {{255,255,255,80},{0,0,0,80},{0,0,0,255},{128,128,128,255}};
    memcpy(common.x6DC_colorsByPlayer, colors, sizeof(colors));
    common.x6D8[0] = 0x12345678;
    common.x6EC_specialMaterialColor = (GXColor){255,128,128,255};
    for (unsigned selector = 0; selector < 256; ++selector) {
        fighter.x61A_controller_index = selector;
        GXColor output = {17,29,43,61}, before = output;
        bool result = ftLib_800870BC(&gobj, &output); ++query_cases;
        CHECK(result == (selector >= 1 && selector <= 4));
        CHECK(memcmp(&output, result ? (const void*)&colors[selector-1] : (const void*)&before, 4) == 0);
        CHECK(common.x6D8[0] == 0x12345678);
    }
    GXColor output = {7,13,19,23}, before = output;
    CHECK(!ftLib_800870BC(NULL, &output));
    CHECK(!ftLib_800870BC(&gobj, NULL));
    gobj.user_data = NULL; CHECK(!ftLib_800870BC(&gobj, &output));
    gobj.user_data = &fighter; p_ftCommonData = NULL;
    CHECK(!ftLib_800870BC(&gobj, &output));
    CHECK(memcmp(&output, &before, 4) == 0); p_ftCommonData = &common;
}
static MaterialTrace material(Fighter* fp, bool old)
{
    trace = (MaterialTrace){0};
    if (old) palette_legacy_material(fp, NULL, NULL);
    else ftMaterial_800BF6BC(fp, NULL, NULL);
    return trace;
}
static void test_materials(void)
{
    static Fighter fp;
    const unsigned alphas[] = {0,1,80,128,254,255};
    for (unsigned selector = 0; selector < 6; ++selector)
        for (unsigned flags = 0; flags < 64; ++flags)
            for (unsigned ai = 0; ai < 6; ++ai) {
                memset(&fp, 0, sizeof(fp)); memset(&overlay, 0, sizeof(overlay));
                fp.x61A_controller_index = selector;
                fp.x2228_b0 = flags & 1; fp.x2224_b0 = (flags >> 1)&1;
                fp.is_metal = (flags >> 2)&1; fp.x2227_b3 = (flags >> 3)&1;
                fp.x2223_b2 = (flags >> 4)&1; fp.x2223_b3 = (flags >> 5)&1;
                fp.x610_color_rgba[0] = (GXColor){67,88,99,80};
                fp.x610_color_rgba[1] = (GXColor){240,231,222,128};
                fp.x61D = alphas[ai];
                for (unsigned enabled = 0; enabled < 2; ++enabled) {
                    overlay.x7C_color_enable = enabled;
                    overlay.x2C_hex = (GXColor){20,40,60,alphas[ai]};
                    MaterialTrace expected = material(&fp, true), actual = material(&fp, false);
                    ++material_pairs; CHECK(memcmp(&actual, &expected, sizeof(actual)) == 0);
                    if (!fp.x2223_b3 && !fp.x2223_b2 && !enabled) {
                        unsigned selected = selector;
                        if (fp.x2228_b0 && fp.x2224_b0) {
                            if (fp.is_metal) selected = 4;
                            else if (fp.x2227_b3) selected = 5;
                        }
                        CHECK(actual.colors == (selected ? 2 : 0));
                        if (selected) {
                            const GXColor* color = selected == 5 ? &p_ftCommonData->x6EC_specialMaterialColor :
                                &p_ftCommonData->x6DC_colorsByPlayer[selected-1];
                            CHECK(memcmp(actual.rgb[0], color, 3) == 0);
                            CHECK(actual.rgb[1][0] == color->a);
                            CHECK(actual.rgb[1][1] == color->a && actual.rgb[1][2] == color->a);
                        }
                    }
                }
            }
    /* Unsupported source selector: no palette lookup; overlay and outlines stay meaningful. */
    for (unsigned selector = 6; selector < 256; ++selector)
        for (unsigned mode = 0; mode < 4; ++mode) {
            memset(&fp, 0, sizeof(fp)); memset(&overlay, 0, sizeof(overlay));
            fp.x61A_controller_index = selector; fp.x61D = 255;
            fp.x2223_b2 = mode == 2; fp.x2223_b3 = mode == 3;
            fp.x610_color_rgba[1] = (GXColor){10,20,30,40};
            overlay.x7C_color_enable = mode == 1;
            overlay.x2C_hex = (GXColor){11,22,33,44};
            MaterialTrace actual = material(&fp, false); ++invalid_material_cases;
            CHECK(actual.colors == (mode == 1 || mode == 2 ? 2 : 0));
            if (mode == 1) CHECK(actual.rgb[0][0] == 11 && actual.rgb[1][0] == 44);
            if (mode == 2) CHECK(actual.rgb[0][0] == 10 && actual.rgb[1][0] == 40);
        }
}
int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    FILE* file = fopen(argv[1], "rb"); if (!file) return 2;
    fseek(file, 0, SEEK_END); size_t size = ftell(file); rewind(file);
    u8* bytes = malloc(size); if (!bytes || fread(bytes, 1, size, file) != size) return 2; fclose(file);
    test_import(bytes, size); free(bytes); test_queries(); test_materials();
    printf("{\"checks\":%u,\"import_cases\":%u,\"query_cases\":%u,\"material_pairs\":%u,\"invalid_material_cases\":%u,\"failures\":%u}\n",
           checks, import_cases, query_cases, material_pairs, invalid_material_cases, failures);
    return failures ? 1 : 0;
}
