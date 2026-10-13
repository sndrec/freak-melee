#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned programs;
static char* generate(const char* source, bool expected, const NativeFighterScriptConstruction* construction)
{
    FILE* out = tmpfile(); assert(out); NativeFighterScriptError error = {0};
    ++programs;
    bool ok = nativeFighterScript_GenerateC(source, strlen(source), "queries", NULL, NULL, construction, out, &error);
    if (ok != expected) fprintf(stderr, "Unexpected %s at %u: %s\n%s", ok ? "accept" : "reject", error.line, error.message, source);
    assert(ok == expected);
    if (!ok) { assert(error.line); fclose(out); return NULL; }
    fseek(out, 0, SEEK_END); long size = ftell(out); rewind(out);
    char* code = malloc(size + 1); assert(code);
    assert(fread(code, 1, size, out) == (size_t)size); code[size] = 0; fclose(out); return code;
}

static const char fields[] =
    "state int handle\nstate int order\nstate int after\nstate bool found\n"
    "state float x\nstate float y\nstate float z\nstate float array[2]\n";
static const char helpers[] =
    "func X() -> float:\n    state.order = state.order * 10 + 1\n    state.basic_landing()\n    return 1.25\n"
    "func Y() -> float:\n    state.order = state.order * 10 + 2\n    state.basic_landing()\n    return -2.5\n"
    "func Z() -> float:\n    state.order = state.order * 10 + 3\n    state.basic_landing()\n    return 3.75\n"
    "func Handle() -> int:\n    state.order = state.order * 10 + 4\n    state.basic_landing()\n    return state.handle\n"
    "export func ReadPosition():\n"
    "    state.found = targets.read_position(Handle(), state.x, state.y, state.z)\n"
    "    state.after = 99\n";
static const char article[] =
    "article It_Kind_Ness_PKFire states 1:\n"
    "    state int handle\n    state int after\n    state bool found\n"
    "    state float x\n    state float y\n    state float z\n"
    "    on created:\n"
    "            let float a = item.state.x\n            let float b = item.state.y\n            let float c = item.state.z\n"
    "            item.state.found = articles.read_primary_translation(item.state.handle, It_Kind_Ness_PKFire, a, b, c)\n"
    "            item.state.x = a\n            item.state.y = b\n            item.state.z = c\n            item.state.after = 99\n"
    "    on dropped:\n"
    "            let float a = item.state.x\n            let float b = item.state.y\n            let float c = item.state.z\n"
    "            item.state.found = targets.read_position(item.state.handle, a, b, c)\n"
    "            item.state.x = a\n            item.state.y = b\n            item.state.z = c\n            item.state.after = 99\n"
    "    on thrown:\n"
    "            item.state.found = targets.read_position(item.state.handle, item.state.x, item.state.y, item.state.z)\n"
    "            item.state.after = 99\n"
    "    state 0 anim -1:\n"
    "        on physics:\n"
    "            item.state.found = articles.read_primary_translation(item.state.handle, It_Kind_Ness_PKFire, item.state.x, item.state.y, item.state.z)\n"
    "            item.state.after = 99\n";

int main(int argc, char** argv)
{
    assert(argc == 1 || argc == 3);
    const char* bad[] = {
        "state.handle = cpu.nearest_enemy_at()", "state.handle = cpu.nearest_enemy_at(1,2)",
        "state.handle = cpu.nearest_enemy_at(1,2,3,4)", "state.handle = cpu.nearest_enemy_at(true,2,3)",
        "state.found = articles.read_primary_translation(1.0, It_Kind_Ness_PKFire, state.x,state.y,state.z)",
        "state.found = articles.read_primary_translation(true, It_Kind_Ness_PKFire, state.x,state.y,state.z)",
        "state.found = articles.read_primary_translation(1, It_Kind_Link_Bow, state.x,state.y,state.z)",
        "state.found = articles.read_primary_translation(1, state.handle, state.x,state.y,state.z)",
        "state.found = articles.read_primary_translation(1, It_Kind_Ness_PKFire, state.x,state.x,state.z)",
        "state.found = articles.read_primary_translation(1, It_Kind_Ness_PKFire, state.handle,state.y,state.z)",
        "state.found = articles.read_primary_translation(1, It_Kind_Ness_PKFire, state.found,state.y,state.z)",
        "state.found = articles.read_primary_translation(1, It_Kind_Ness_PKFire, state.array[0],state.y,state.z)",
        "state.found = articles.read_primary_translation(1, It_Kind_Ness_PKFire, state.x+1,state.y,state.z)",
        "state.found = articles.read_primary_translation(1, It_Kind_Ness_PKFire, item.state.x,state.y,state.z)",
        "state.found = articles.read_primary_translation(1, It_Kind_Ness_PKFire, state.x,state.y)",
        "state.found = articles.read_primary_translation(1, It_Kind_Ness_PKFire, state.x,state.y,state.z,state.x)",
        "state.found = targets.read_position()",
        "state.found = targets.read_position(1,state.x,state.y)",
        "state.found = targets.read_position(1,state.x,state.y,state.z,state.x)",
        "state.found = targets.read_position(true,state.x,state.y,state.z)",
        "state.found = targets.read_position(1.0,state.x,state.y,state.z)",
        "state.found = targets.read_position(1,state.x,state.x,state.z)",
        "state.found = targets.read_position(1,state.handle,state.y,state.z)",
        "state.found = targets.read_position(1,state.found,state.y,state.z)",
        "state.found = targets.read_position(1,state.array[0],state.y,state.z)",
        "state.found = targets.read_position(1,state.x+1,state.y,state.z)",
        "state.found = targets.read_position(1,item.state.x,state.y,state.z)",
    };
    char source[12000];
    for (unsigned route = 0; route < 2; ++route) {
        const char* entry = route ? "move Queries:\n    on state_341_physics:\n" : "action Queries:\n    on load:\n";
        const char* base = route ? "" : "";
        for (unsigned i = 0; i < sizeof(bad)/sizeof(*bad); ++i) {
            snprintf(source, sizeof(source), "%s%s%s        %s\n%s", base, fields, entry, bad[i], article);
            free(generate(source, false, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL));
        }
        snprintf(source, sizeof(source), "%s%s%s%s"
            "        state.handle = cpu.nearest_enemy_at(X(), Y(), Z())\n"
            "        state.found = articles.read_primary_translation(Handle(), It_Kind_Ness_PKFire, state.x, state.y, state.z)\n"
            "        state.after = 99\n%s", base, fields, helpers, entry, article);
        char* code = generate(source, true, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        assert(strstr(code,"host->cpu_nearest_enemy_at == 0"));
        assert(strstr(code,"host->article_ref_primary_translation == 0"));
        assert(strstr(code,"host->target_position == 0"));
        assert(strstr(code,"host->callback_source_alive == 0"));
        assert(strstr(code,"NativeCallbackSource_Item"));
        if (argc == 3) { FILE* out = fopen(argv[route+1], "wb"); assert(out); assert(fwrite(code,1,strlen(code),out)==strlen(code)); fclose(out); }
        free(code);
        snprintf(source,sizeof(source),"%s%s%s        let float a = 0\n        let float b = 0\n        let float c = 0\n"
            "        articles.read_primary_translation(state.handle, It_Kind_Ness_PKFire, a,b,c)\n"
            "        targets.read_position(state.handle, a,state.y,c)\n"
            "        cpu.nearest_enemy_at(1,2.5,3)\n%s",base,fields,entry,article);
        free(generate(source,true, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL));
    }
    const char* item_bad[] = {
        "let int result = cpu.nearest_enemy_at(1,2,3)",
        "item.state.found = articles.read_primary_translation(1,It_Kind_Ness_PKFire,state.x,item.state.y,item.state.z)",
        "item.state.found = articles.read_primary_translation(1,It_Kind_Ness_PKFire,creator.state.x,item.state.y,item.state.z)",
        "item.state.found = articles.read_primary_translation(1,It_Kind_Ness_PKFire,item.state.x,item.state.x,item.state.z)",
        "item.state.found = articles.read_primary_translation(1.0,It_Kind_Ness_PKFire,item.state.x,item.state.y,item.state.z)",
        "item.state.found = articles.read_primary_translation(1,It_Kind_Link_Bow,item.state.x,item.state.y,item.state.z)",
        "item.state.found = targets.read_position(1,state.x,item.state.y,item.state.z)",
        "item.state.found = targets.read_position(1,creator.state.x,item.state.y,item.state.z)",
        "item.state.found = targets.read_position(true,item.state.x,item.state.y,item.state.z)",
        "item.state.found = targets.read_position(1.0,item.state.x,item.state.y,item.state.z)",
        "item.state.found = targets.read_position(1,item.state.x,item.state.x,item.state.z)",
    };
    for(unsigned i=0;i<sizeof(item_bad)/sizeof(*item_bad);++i) {
        snprintf(source,sizeof(source),"%saction Queries:\n    on load:\n        pass\n"
            "article It_Kind_Ness_PKFire states 1:\n    state float x\n    state float y\n    state float z\n    state bool found\n"
            "    on created:\n            %s\n    state 0 anim -1:\n",fields,item_bad[i]);
        free(generate(source,false, NULL));
    }
    const char* effect_bad[]={"cpu.nearest_enemy_at(1,2,3)","articles.read_primary_translation(1,It_Kind_Ness_PKFire,state.x,state.y,state.z)","targets.read_position(1,state.x,state.y,state.z)"};
    for(unsigned i=0;i<3;++i){snprintf(source,sizeof(source),"%saction Queries:\n    on load:\n        pass\n"
        "model_effect QueryEffect 0x47c:\n    on update:\n        let bool result = %s\n%s",fields,effect_bad[i],article);free(generate(source,false, NULL));}
    printf("PASS %u query compiler programs, both routes/article, typed output/context/arity/catalog checks\n",programs);
    return 0;
}
