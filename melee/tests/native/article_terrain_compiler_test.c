#include "../../tools/native_fighter_item_program.h"
#include "../../tools/native_fighter_item_terrain.h"

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned occurrences(const char* source, const char* token)
{
    unsigned count = 0;
    for (const char* p = source; (p = strstr(p, token)) != NULL; p += strlen(token))
        ++count;
    return count;
}

static void expression_contracts(void)
{
    ArticleDeclaration* article = calloc(1, sizeof(*article)); assert(article);
    static const char* declarations[] = {
        "state int line", "state float x1", "state float y1", "state float z1",
        "state float x2", "state float y2", "state float z2", "state int wrong_int",
        "state bool wrong_bool", "state float array[2]",
    };
    for (unsigned i = 0; i < sizeof(declarations) / sizeof(*declarations); ++i)
        assert(nativeFighterScript_AddStateField(&article->local_state, declarations[i]));
    const char* expression =
        "stage.read_line_island_bounds(int(item.state.x1), item.state.x1, item.state.y1, "
        "item.state.z1, item.state.x2, item.state.y2, item.state.z2) + 1";
    char code[2048]; const char* after; ScriptExpressionType type;
    assert(nativeItemTerrain_Expression(expression, article, NULL, code, sizeof(code), &after, &type) == 1);
    assert(type == ScriptType_Bool);
    assert(!strcmp(after, " + 1"));
    assert(occurrences(code, "engine->stage_line_island_bounds(") == 1);
    assert(occurrences(code, "scripted_article_state_float(") == 1);
    assert(occurrences(code, "scripted_article_set_state_float(") == 6);
    assert(strstr(code, "int32_t terrain_line =") < strstr(code, "scripted_article_set_state_float("));
    assert(strstr(code, "terrain_first = {0}, terrain_last = {0}"));
    assert(strstr(code, "terrain_found; })"));
    assert(nativeItemTerrain_Expression(expression, article, NULL, code, 32, &after, &type) == -1);
    const char* invalid[] = {
        "stage.read_line_island_bounds(1.5,item.state.x1,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(true,item.state.x1,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,item.state.x1,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.x1)",
        "stage.read_line_island_bounds(1,item.state.absent,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,item.state.wrong_int,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,item.state.wrong_bool,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,item.state.array,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,item.state.array[0],item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,owner.state.x1,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,local_x,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,item.state.x1 + 0,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2)",
        "stage.read_line_island_bounds(1,item.state.x1,item.state.y1,item.state.z1,item.state.x2,item.state.y2)",
        "stage.read_line_island_bounds(1,item.state.x1,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2,0)",
        "stage.read_line_island_bounds(1,item.state.x1,item.state.y1,item.state.z1,item.state.x2,item.state.y2,item.state.z2",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i)
        assert(nativeItemTerrain_Expression(invalid[i], article, NULL, code, sizeof(code), &after, &type) == -1);
    assert(nativeItemTerrain_Expression(expression, NULL, NULL, code, sizeof(code), &after, &type) == -1);
    assert(nativeItemTerrain_Expression("stage.line_exists(1)", article, NULL, code, sizeof(code), &after, &type) == 0);
    assert(!strcmp(nativeItemTerrain_Constant("Collision.floor_mask"), "Collide_FloorMask"));
    assert(!strcmp(nativeItemTerrain_Constant("Collision.wall_mask"), "Collide_WallMask"));
    assert(nativeItemTerrain_Constant("Collision.floor_mask.extra") == NULL);
    assert(nativeItemTerrain_Constant("Collision.ceiling_mask") == NULL);
    free(article);
}

static const char article_prefix[] =
    "article It_Kind_Link_Hookshot states 1:\n"
    "    state int line\n    state bool found\n"
    "    state float x1\n    state float y1\n    state float z1\n"
    "    state float x2\n    state float y2\n    state float z2\n"
    "    state int wrong_int\n    state bool wrong_bool\n    state float array[2]\n"
    "    state 0 anim 0:\n        on physics:\n";

static bool generate(const char* body, FILE* file, bool diagnose)
{
    char source[4096]; int length = snprintf(source, sizeof(source), "%s%s", article_prefix, body);
    assert(length > 0 && (size_t)length < sizeof(source));
    NativeFighterScriptError error = {0};
    bool good = nativeFighterScript_GenerateC(source, (size_t)length,
        "article_terrain_fixture", NULL, NULL, NULL, file, &error);
    if (!good && diagnose) fprintf(stderr, "Terrain compile line %u: %s\n", error.line, error.message);
    return good;
}

static void generated_contracts(void)
{
    const char* body =
        "            let bool found = stage.read_line_island_bounds(random.int(8), item.state.x1, item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.z2)\n"
        "            item.state.found = found\n"
        "            let int masks = Collision.floor_mask | Collision.wall_mask\n"
        "            item.state.line = item.state.line & masks\n"
        "            stage.read_line_island_bounds(item.state.line, item.state.x1, item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.z2)\n";
    FILE* file = tmpfile(); assert(file); assert(generate(body, file, true));
    assert(!fseek(file, 0, SEEK_END)); long length = ftell(file); assert(length >= 0); rewind(file);
    char* code = malloc((size_t)length + 1); assert(code);
    assert(fread(code, 1, (size_t)length, file) == (size_t)length); code[length] = 0;
    assert(strstr(code, "host->stage_line_island_bounds == 0"));
    assert(occurrences(code, "engine->stage_line_island_bounds(") == 2);
    assert(occurrences(code, "engine->random_int(") == 1);
    assert(strstr(code, "Collide_FloorMask") && strstr(code, "Collide_WallMask"));
    assert(strstr(code, "bool local_found"));
    assert(!strstr(code, "mpIsland_8005AB54") && !strstr(code, "mp_UnkStruct0"));
    free(code); fclose(file);
    const char* invalid[] = {
        "            let bool x = stage.read_line_island_bounds(1.0, item.state.x1, item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.z2)\n",
        "            let bool x = stage.read_line_island_bounds(1, item.state.x1, item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.y2)\n",
        "            let bool x = stage.read_line_island_bounds(1, item.state.wrong_int, item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.z2)\n",
        "            let bool x = stage.read_line_island_bounds(1, item.state.array[0], item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.z2)\n",
        "            let bool x = stage.read_line_island_bounds(1, item.state.missing, item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.z2)\n",
        "            stage.read_line_island_bounds(1, item.state.x1, item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.z2).extra\n",
        "            item.state.line = Collision.floor_mask.extra\n",
        "            item.state.line = Collision.floor_mask(1)\n",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        file = tmpfile(); assert(file);
        if (generate(invalid[i], file, false)) {
            fprintf(stderr, "Unexpected terrain acceptance: %s", invalid[i]);
            abort();
        }
        fclose(file);
    }
    const char* wrong_context =
        "state float x\naction Terrain:\n    on damage:\n"
        "        let bool found = stage.read_line_island_bounds(1, item.state.x1, item.state.y1, item.state.z1, item.state.x2, item.state.y2, item.state.z2)\n";
    NativeFighterScriptError error = {0}; file = tmpfile(); assert(file);
    assert(!nativeFighterScript_GenerateC(wrong_context, strlen(wrong_context),
        "article_terrain_bad", NULL, NULL, NULL, file, &error)); fclose(file);
}

int main(void)
{
    expression_contracts(); generated_contracts();
    puts("Article terrain strict destinations, single-evaluation emission, typed constants and real compiler validation PASS");
    return 0;
}
