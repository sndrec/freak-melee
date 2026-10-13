#include <melee/ft/kinds/ftMario/ftmario.h>
#include <melee/ft/kinds/ftMario/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/types.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftparts.h>
#include <melee/lb/native_disc_archive.h>
#include <aurora/dvd.h>
#include <melee/it/it_3F14.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>

#include <stdio.h>
#include <stdlib.h>

static void mario_onload(Fighter* fighter, ftData* data,
                         ftMario_DatAttrs* attributes,
                         Article** articles, FighterKind kind)
{
    *fighter = (Fighter){ 0 };
    *data = (ftData){ 0 };
    *attributes = (ftMario_DatAttrs){ 0 };
    attributes->specials.cape_kind = It_Kind_Mario_Cape;
    data->ext_attr = attributes;
    data->x48_items = (UNK_T*) articles;
    fighter->kind = kind;
    fighter->behavior_kind = Ft_Kind_Mario;
    fighter->ft_data = data;
    fighter->dat_attrs_backup = attributes;
    HSD_GObj object = { 0 };
    object.user_data = fighter;
    ftMr_Init_OnLoad(&object);
}

static bool article_is(const Fighter* fighter, ItemKind kind,
                       bool expected_mapping, Article* expected_article)
{
    Article* article = NULL;
    bool mapped = nativeFighterRuntime_ResolveArticleForItem(
        fighter, kind, &article);
    return mapped == expected_mapping &&
           (!mapped || article == expected_article);
}

int main(int argc, char** argv)
{
    if (argc != 6 || !aurora_dvd_open(argv[1])) return 2;
    void* common = NULL;
    size_t size = 0;
    enum { ARENA_SIZE = 16 * 1024 * 1024 };
    void* arena = _aligned_malloc(ARENA_SIZE, 32);
    static FtCommonAssets common_assets;
    bool loaded = arena != NULL &&
        nativeDiscArchive_Read("PlCo.dat", &common, &size) &&
        ftCommonAssets_Import(common, size, arena, ARENA_SIZE, 32768, &common_assets);
    aurora_dvd_close();
    if (!loaded) return 1;
    ftPartsTable = common_assets.parts;
    Fighter_804D6540 = common_assets.omissions;
    Fighter_804D64FC = &common_assets.cpu_data.descriptor;
    for (unsigned i = 0; i < 3; ++i) {
        NativeContentId id;
        if (!nativeContentStore_IdFromHex(argv[3 + i], &id) ||
            !nativeFighterRuntime_Load(&id, NULL, ChKind_CustomFirst + i,
                                      Ft_Kind_CustomFirst + i, argv[2], false, false))
            return 1;
    }
    Article* global_articles[It_PKind_Start - It_Kind_Kuriboh] = { 0 };
    Article** previous_globals = it_804D6D38;
    it_804D6D38 = global_articles;
    Article stock_fire = { 0 }, stock_cape = { 0 };
    Article first_fire = { 0 }, first_cape = { 0 };
    Article second_fire = { 0 }, second_cape = { 0 };
    Article* stock_articles[3] = { &stock_fire, NULL, &stock_cape };
    Article* first_articles[3] = { &first_fire, NULL, &first_cape };
    Article* second_articles[3] = { &second_fire, NULL, &second_cape };
    Fighter stock, first, second;
    ftData stock_data, first_data, second_data;
    ftMario_DatAttrs stock_attrs, first_attrs, second_attrs;
    mario_onload(&stock, &stock_data, &stock_attrs, stock_articles,
                 Ft_Kind_Mario);
    mario_onload(&first, &first_data, &first_attrs, first_articles,
                 Ft_Kind_CustomFirst);
    mario_onload(&second, &second_data, &second_attrs, second_articles,
                 Ft_Kind_CustomFirst + 1);
    bool valid =
        global_articles[It_Kind_Mario_Fire - It_Kind_Kuriboh] == &stock_fire &&
        global_articles[It_Kind_Mario_Cape - It_Kind_Kuriboh] == &stock_cape &&
        article_is(&stock, It_Kind_Mario_Fire, false, NULL) &&
        article_is(&first, It_Kind_Mario_Fire, true, &first_fire) &&
        article_is(&first, It_Kind_Mario_Cape, true, &first_cape) &&
        article_is(&second, It_Kind_Mario_Fire, true, &second_fire) &&
        article_is(&second, It_Kind_Mario_Cape, true, &second_cape) &&
        article_is(&first, It_Kind_Luigi_Fire, false, NULL);
    first_articles[0] = NULL;
    valid = valid &&
            article_is(&first, It_Kind_Mario_Fire, true, NULL) &&
            global_articles[It_Kind_Mario_Fire - It_Kind_Kuriboh] == &stock_fire;
    Article clone_pill = { 0 }, clone_sheet = { 0 };
    Article* dr_articles[4] = { NULL, &clone_pill, NULL, &clone_sheet };
    ftData dr_data = { .x48_items = (UNK_T*) dr_articles };
    Fighter doctor = {
        .kind = Ft_Kind_CustomFirst + 2,
        .behavior_kind = Ft_Kind_DrMario,
        .ft_data = &dr_data,
    };
    valid = valid &&
            article_is(&doctor, It_Kind_DrMario_Vitamin, true,
                       &clone_pill) &&
            article_is(&doctor, It_Kind_DrMario_Sheet, true,
                       &clone_sheet) &&
            article_is(&doctor, It_Kind_Mario_Fire, false, NULL);
    ftMario_DatAttrs doctor_attrs = { 0 };
    doctor_attrs.specials.cape_kind = It_Kind_Mario_Cape;
    doctor.dat_attrs = &doctor_attrs;
    valid = valid && article_is(&doctor, It_Kind_DrMario_Sheet, false, NULL) &&
            article_is(&doctor, It_Kind_Mario_Cape, true, &clone_sheet);
    nativeFighterRuntime_Clear();
    valid = valid && article_is(&first, It_Kind_Mario_Fire, false, NULL);
    it_804D6D38 = previous_globals;
    _aligned_free(common);
    _aligned_free(arena);
    if (!valid) {
        fputs("Stock-clone article ownership crossed fighter instances.\n", stderr);
        return 1;
    }
    puts("Stock and two cloned Marios own separate fireball/cape articles.");
    return 0;
}
