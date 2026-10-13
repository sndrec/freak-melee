#include "../../tools/native_fighter_article_operations.h"
#undef NDEBUG
#include <assert.h>
#include <string.h>

static void syntax(void)
{
    NativeArticleScriptNames names = { 0 };
    assert(nativeArticleOperations_DeclareModel("    model_animation Bundle 80", "model_animation", &names));
    assert(names.entries[0].animation_components[0].attribute_offset == 80 &&
           names.entries[0].animation_components[1].attribute_offset == 84 && names.entries[0].animation_components[2].attribute_offset == 88);
    assert(nativeArticleOperations_DeclareModel("    model_animation Spin material 88", "model_animation", &names));
    assert(names.entries[1].animation_components[0].attribute_offset == UINT32_MAX &&
           names.entries[1].animation_components[1].attribute_offset == 88 && names.entries[1].animation_components[2].attribute_offset == UINT32_MAX);
    assert(nativeArticleOperations_DeclareModel("    model_animation Scattered shape 24 joint 8 material None", "model_animation", &names));
    assert(names.entries[2].animation_components[0].attribute_offset == 8 &&
           names.entries[2].animation_components[1].attribute_offset == UINT32_MAX && names.entries[2].animation_components[2].attribute_offset == 24);
    assert(nativeArticleOperations_DeclareModel("    model_animation Array joint 116[0] material 120[2] shape 124[1]", "model_animation", &names));
    assert(names.entries[3].animation_components[0].attribute_offset == 116 && names.entries[3].animation_components[0].array_index == 0);
    assert(names.entries[3].animation_components[1].attribute_offset == 120 && names.entries[3].animation_components[1].array_index == 2);
    assert(names.entries[3].animation_components[2].attribute_offset == 124 && names.entries[3].animation_components[2].array_index == 1);
    assert(names.entries[2].animation_components[0].array_index == UINT32_MAX);
    const char* invalid[] = {
        "    model_animation Spin material 88", // Duplicate name.
        "    model_animation Other material 88 material 92",
        "    model_animation Other material 89",
        "    model_animation Other material -4",
        "    model_animation Other material 4294967296",
        "    model_animation Other material 88 trailing",
        "    model_animation Other material",
        "    model_animation Other 4294967292", // Bundle addition overflow.
        "    model_animation Other joint 4 shape 8 material 12 extra 16",
        "    model_animation Other source 88",
        "    model_animation Other joint 4[]",
        "    model_animation Other joint 4[-1]",
        "    model_animation Other joint 4[+1]",
        "    model_animation Other joint 4[1]tail",
        "    model_animation Other joint 4[1][2]",
        "    model_animation Other joint 4[1073741823]",
        "    model_animation Other joint 4[4294967295]",
        "    model_animation Other joint None[0]",
        "    model_animation Other joint 5[0]",
        "    model_root Other material 88"
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        unsigned count = names.count;
        assert(!nativeArticleOperations_DeclareModel(invalid[i],
            i + 1 == sizeof(invalid) / sizeof(*invalid) ? "model_root" : "model_animation", &names));
        assert(names.count == count);
    }
}

static void generated(bool callbacks, const char* path)
{
    char source[2048];
    snprintf(source, sizeof(source), "%s"
        "article It_Kind_Ness_PKFire states 1:\n"
        "    model_animation Spin material 88\n"
        "    model_animation Bundle 80\n"
        "    model_animation Scattered shape 24 joint 8 material None\n"
        "    model_animation Array joint 116[0] material 120[2] shape 124[1]\n"
        "    state int completed\n"
        "    state bool added\n"
        "    state 0 anim -1:\n"
        "        on physics:\n"
        "            item.add_model_animation(0, Spin)\n"
        "            item.request_model_animation(0, 2)\n"
        "            item.advance_model_animation(0)\n"
        "            item.remove_model_animation(0)\n"
        "            item.state.added = item.add_model_animation(0, \"0\", Bundle)\n"
        "            if not item.state.added:\n"
        "                item.state.completed = -9\n"
        "                return\n"
        "            item.request_model_animation(0, \"0\", 9)\n"
        "            item.advance_model_animation(0, \"0\")\n"
        "            item.remove_model_animation(0, \"0\")\n"
        "            item.state.completed = 23\n",
        callbacks ? "move Components:\n    on state_341_physics:\n        velocity.x = 0\n"
                  : "action Components:\n    on load:\n        velocity.x = 0\n");
    FILE* output = path ? fopen(path, "w+") : tmpfile(); assert(output);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source), "components_probe",
        NULL, NULL, callbacks ? &(const NativeFighterScriptConstruction){"ftDataNess",341,1} : NULL, output, &error);
    if (!valid) fprintf(stderr, "Components at %u: %s\n", error.line, error.message);
    assert(valid); rewind(output);
    static char code[65536]; size_t count = fread(code, 1, sizeof(code) - 1, output);
    assert(feof(output)); code[count] = 0;
    assert(strstr(code, "{ UINT32_C(88), UINT32_C(4294967295) }"));
    assert(strstr(code, "{ UINT32_C(80), UINT32_C(4294967295) }"));
    assert(strstr(code, "{ UINT32_C(8), UINT32_C(4294967295) }"));
    assert(strstr(code, "{ UINT32_C(116), UINT32_C(0) }, { UINT32_C(120), UINT32_C(2) }, { UINT32_C(124), UINT32_C(1) }"));
    assert(strstr(code, "item_add_model_animation(gobj, 0, \"\", UINT32_C("));
    assert(strstr(code, "item_add_model_animation(gobj, 0, \"0\", UINT32_C("));
    assert(strstr(code, "item_request_model_animation(gobj, 0, \"\", 2)"));
    assert(strstr(code, "item_request_model_animation(gobj, 0, \"0\", 9)"));
    assert(strstr(code, "item_advance_model_animation(gobj, 0, \"0\")"));
    assert(strstr(code, "item_remove_model_animation(gobj, 0, \"0\")"));
    assert(strstr(code, "host->callback_source_alive == 0"));
    fclose(output);
}

static void path_errors(const NativeFighterScriptConstruction* construction)
{
    const char* invalid[] = {
        "item.add_model_animation(0, 0, Spin)",
        "item.add_model_animation(0, \"0/\", Spin)",
        "item.add_model_animation(4, \"0\", Spin)",
        "item.add_model_animation(0, \"256\", Spin)",
        "item.add_model_animation(0, \"0\", Missing)",
        "item.request_model_animation(0, \"-1\", 2)",
        "item.request_model_animation(0, \"0\", 2, 3)",
        "item.advance_model_animation(0, \"0/0/0/0/0/0/0/0/0/0/0/0/0\")",
        "item.remove_model_animation(0, \"0x1\")"
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        char source[512];
        snprintf(source, sizeof(source),
            "article It_Kind_Ness_PKFire states 1:\n"
            "    model_animation Spin material 88\n"
            "    state 0 anim -1:\n        on physics:\n            %s\n", invalid[i]);
        FILE* file = tmpfile(); assert(file);
        NativeFighterScriptError error = { 0 };
        assert(!nativeFighterScript_GenerateC(source, strlen(source), "bad_path", NULL, NULL, construction, file, &error));
        fclose(file);
    }
}

int main(int argc, char** argv)
{
    syntax(); path_errors(NULL); generated(false, argc == 3 ? argv[1] : NULL);
    generated(true, argc == 3 ? argv[2] : NULL);
    puts("Bundle and independent joint/material/shape model-animation declarations compile");
    return 0;
}
