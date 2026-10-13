#include "../../tools/native_fighter_script_article_refs.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NativeScriptArticleCatalog catalog;
static NativeFighterScriptError error;

static bool parse(const char* source)
{
    return nativeScriptArticleRefs_CatalogParse(&catalog, source,
                                               strlen(source), &error);
}

static void catalog_limits(void)
{
    char source[8192]; size_t used = 0;
    for (unsigned i = 0; i < SCRIPT_ARTICLE_DECLARATIONS; ++i)
        used += snprintf(source + used, sizeof(source) - used,
                         "article It_Kind_Test%u states 1:\n    state int id\n", i);
    assert(parse(source) && catalog.count == 16);
    strcat(source, "article It_Kind_Overflow states 1:\n");
    assert(!parse(source) && error.line == 33);
    assert(!parse("article It_Kind_Test states 1:\narticle It_Kind_Test states 1:\n"));
    assert(error.line == 2);
    assert(!parse("article It_Kind_Test states 0:\n"));
    assert(!parse("article It_Kind_Test states 33:\n"));
    assert(!parse("article It_Kind_Test states 1: junk\n"));
    assert(parse("article It_Kind_Test states 32:\r\n    state float values[128]\r\n"));
    assert(catalog.declarations[0].state.bytes == 512);
    assert(!parse("article It_Kind_Test states 1:\n    state float values[128]\n    state bool overflow\n"));
    assert(!parse("article It_Kind_Test states 1:\n    state int duplicate\n    state bool duplicate\n"));
    used = snprintf(source, sizeof(source), "article It_Kind_Test states 1:\n");
    for (unsigned i = 0; i < SCRIPT_STATE_FIELDS; ++i)
        used += snprintf(source + used, sizeof(source) - used,
                         "    state bool flag%u\n", i);
    assert(parse(source) && catalog.declarations[0].state.count == 64);
    strcat(source, "    state bool extra\n"); assert(!parse(source));
}

static void references(void)
{
    // Deliberately different descriptor order between forward target and caller.
    assert(parse(
        "article It_Kind_Ball states 1:\n"
        "    state int trail\n"
        "    on created:\n"
        "        item.life = articles.ref(item.state.trail, It_Kind_Trail).state.position[1]\n"
        "    state 0 anim -1:\n"
        "article It_Kind_Trail states 1:\n"
        "    state bool active\n"
        "    state float position[8]\n"
        "    state int parent\n"
        "    state 0 anim -1:\n"));
    assert(catalog.count == 2 && catalog.declarations[0].state.count == 1);
    assert(catalog.declarations[1].state.fields[1].offset == 4);
    NativeScriptArticleReference ref;
    assert(nativeScriptArticleRefs_Reference(&catalog,
        "articles.ref(item.state.trail, It_Kind_Trail).state.position[next_index()] + 2", &ref) == 1);
    assert(ref.type == ScriptState_Float && ref.field_index == 1);
    assert(!strcmp(ref.handle, "item.state.trail"));
    assert(!strcmp(ref.index, "next_index()") && !strcmp(ref.after, " + 2"));
    char generated[1024];
    assert(nativeScriptArticleRefs_Emit(&ref, "read_id()", "next_index()",
                                      generated, sizeof(generated)));
    assert(!strcmp(generated,
        "engine->article_ref_float(gobj, (uint32_t)(read_id()), It_Kind_Trail, 1, (int32_t)(next_index()))"));
    assert(!nativeScriptArticleRefs_Emit(&ref, "id", "0", generated, 8));
    assert(nativeScriptArticleRefs_Reference(&catalog,
        "articles.ref(0xFEDCBA98, It_Kind_Trail).exists", &ref) == 1);
    assert(ref.type == ScriptState_Bool && ref.field == NULL);
    assert(nativeScriptArticleRefs_Emit(&ref, ref.handle, NULL, generated, sizeof(generated)));
    assert(strstr(generated, "(uint32_t)(0xFEDCBA98)") != NULL);
    assert(nativeScriptArticleRefs_Reference(&catalog,
        "articles.ref(0xFEDCBA98, It_Kind_Trail).current_owner + 1", &ref) == 1);
    assert(ref.type == ScriptState_Int && ref.field == NULL && ref.current_owner);
    assert(!strcmp(ref.after, " + 1"));
    assert(nativeScriptArticleRefs_Emit(&ref, ref.handle, NULL, generated, sizeof(generated)));
    assert(strstr(generated, "engine->article_ref_current_owner(gobj, (uint32_t)(0xFEDCBA98), It_Kind_Trail)"));
    assert(nativeScriptArticleRefs_Reference(&catalog,
        "articles.ref(ids[next(1, 2)], It_Kind_Trail).state.parent", &ref) == 1);
    assert(ref.type == ScriptState_Int && ref.field_index == 2 && !strcmp(ref.index, "0"));
    assert(nativeScriptArticleRefs_Reference(&catalog,
        "articles.ref(7, It_Kind_Trail).state.active", &ref) == 1);
    assert(ref.type == ScriptState_Bool && ref.field_index == 0);
    assert(nativeScriptArticleRefs_Reference(&catalog,
        "articles.ref(7, It_Kind_Trail).state.position[-1]", &ref) == 1);
    assert(nativeScriptArticleRefs_Reference(&catalog,
        "articles.ref(7, It_Kind_Trail).state.position[999]", &ref) == 1);
    // Runtime bounds return zero, rather than rejecting integer indices here.
    const char* invalid[] = {
        "articles.ref(, It_Kind_Trail).exists",
        "articles.ref(7, It_Kind_Missing).exists",
        "articles.ref(7, It_Kind_Trail + 1).exists",
        "articles.ref(7, It_Kind_Trail, 2).exists",
        "articles.ref(next(], It_Kind_Trail).exists",
        "articles.ref(7, It_Kind_Trail).exists_other",
        "articles.ref(7, It_Kind_Trail).exists.field",
        "articles.ref(7, It_Kind_Trail).current_owner_other",
        "articles.ref(7, It_Kind_Trail).current_owner.field",
        "articles.ref(7, It_Kind_Trail).current_owner[0]",
        "articles.ref(7, It_Kind_Trail).state.missing",
        "articles.ref(7, It_Kind_Trail).state.active[0]",
        "articles.ref(7, It_Kind_Trail).state.active [0]",
        "articles.ref(7, It_Kind_Trail).state.position",
        "articles.ref(7, It_Kind_Trail).state.position[]",
        "articles.ref(7, It_Kind_Trail).state.position[ ]",
        "articles.ref(7, It_Kind_Trail).state.position[next(1,2)]extra",
        "articles.ref(7, It_Kind_Trail).state.position[1][0]"
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(nativeScriptArticleRefs_Reference(&catalog, invalid[i], &ref) == -1);
    assert(nativeScriptArticleRefs_Reference(&catalog, "item.state.parent", &ref) == 0);
}

int main(void)
{
    catalog_limits(); references();
    puts("Article reference catalog/grammar/emission checks passed");
    return 0;
}
