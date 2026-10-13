#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file(const char* path, size_t* size)
{
    FILE* file = fopen(path, "rb"); assert(file);
    assert(!fseek(file, 0, SEEK_END)); long length = ftell(file); assert(length >= 0);
    assert(!fseek(file, 0, SEEK_SET));
    char* text = malloc((size_t)length + 1); assert(text);
    *size = fread(text, 1, (size_t)length, file); assert(*size == (size_t)length);
    text[*size] = 0; fclose(file); return text;
}

static bool compile(const char* source, const NativeFighterParameters* parameters,
                    FILE* output, bool diagnose)
{
    NativeFighterScriptError error = { 0 };
    bool result = nativeFighterScript_GenerateC(source, strlen(source),
        "ness_clone", NULL, parameters, NULL, output, &error);
    if (!result && diagnose) fprintf(stderr, "Flash compiler line %u: %s\n", error.line, error.message);
    return result;
}

static void contracts(void)
{
    const char* positive =
        "action Dispatch:\n    on damage:\n"
        "        articles.signal(uint(0xFEDCBA98), It_Kind_Ness_PKFlush, \"detach\", 0)\n"
        "article It_Kind_Ness_PKFlush states 1:\n    state int recipient\n"
        "    state 0 anim -1:\n        on physics:\n"
        "            velocity.x = creator.input.x\n            velocity.y = creator.input.y\n"
        "            camera.quake(3, position.x, position.y, position.z)\n"
        "            item.rumble(14, 20)\n            item.stop_loop_sound()\n"
        "            articles.signal(uint(0xFEDCBA98), It_Kind_Ness_PKFlush, \"detach\", 0)\n"
        "    on signal detach:\n            item.clear_current_owner()\n";
    FILE* file = tmpfile(); assert(file); assert(compile(positive, NULL, file, true));
    assert(!fseek(file, 0, SEEK_END)); long length = ftell(file); assert(length > 0);
    assert(!fseek(file, 0, SEEK_SET)); char* code = malloc((size_t)length + 1); assert(code);
    assert(fread(code, 1, (size_t)length, file) == (size_t)length); code[length] = 0;
    assert(strstr(code, "host->signal_article == 0"));
    assert(strstr(code, "host->article_ref_exists == 0"));
    assert(strstr(code, "article_ref_exists(gobj"));
    assert(strstr(code, "signal_handle"));
    assert(strstr(code, "signal_value"));
    assert(strstr(code, "host->item_creator == 0"));
    assert(strstr(code, "host->item_rumble == 0"));
    assert(strstr(code, "host->item_stop_loop_sound == 0"));
    assert(strstr(code, "host->camera_quake == 0"));
    assert(strstr(code, "host->callback_source_alive == 0"));
    assert(strstr(code, "input.lstick[0].x")); assert(strstr(code, "input.lstick[0].y"));
    assert(strstr(code, "4275878552"));
    assert(!strstr(code, "signal_article(gobj, (uint32_t)(4.275"));
    free(code); fclose(file);
    const char* invalid[] = {
        "action Bad:\n    on damage:\n        articles.signal(1, \"detach\", 0)\n"
        "article It_Kind_Ness_PKFlush states 1:\n    state 0 anim -1:\n",
        "action Bad:\n    on damage:\n        articles.signal(1, It_Kind_Ness_PKThunder, \"detach\", 0)\n"
        "article It_Kind_Ness_PKFlush states 1:\n    state 0 anim -1:\n",
        "action Bad:\n    on damage:\n        articles.signal(1.5, It_Kind_Ness_PKFlush, \"detach\", 0)\n",
        "article It_Kind_Ness_PKFlush states 1:\n    state 0 anim -1:\n        on physics:\n            creator.input.x = 1\n",
        "article It_Kind_Ness_PKFlush states 1:\n    state 0 anim -1:\n        on physics:\n            item.rumble(1.5, 20)\n",
        "article It_Kind_Ness_PKFlush states 1:\n    state 0 anim -1:\n        on physics:\n            camera.quake(3.5, 0, 0, 0)\n",
        "article It_Kind_Ness_PKFlush states 1:\n    state 0 anim -1:\n        on physics:\n            articles.signal(1.5, It_Kind_Ness_PKFlush, \"detach\", 0)\n",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        file = tmpfile(); assert(file); assert(!compile(invalid[i], NULL, file, false)); fclose(file);
    }
}

int main(int argc, char** argv)
{
    contracts();
    if (argc == 4) {
        size_t source_size, parameter_size;
        char* source = read_file(argv[1], &source_size);
        char* text = read_file(argv[2], &parameter_size);
        NativeFighterParameters parameters; unsigned line = 0;
        assert(nativeFighterParameters_Parse(text, parameter_size, &parameters, &line));
        FILE* output = fopen(argv[3], "wb"); assert(output);
        bool valid = compile(source, &parameters, output, true);
        fclose(output); free(source); free(text); if (!valid) return 1;
    } else if (argc != 1) return 2;
    puts("PK Flash generic creator/presentation/signal compiler contracts PASS");
    return 0;
}
