#include "../../../tools/native_fighter_script.h"
#include "../../../tools/native_fighter_script_terrain.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool generate(const char* source, FILE* out, const NativeFighterScriptConstruction* construction)
{
    NativeFighterScriptError error = {0};
    bool ok = nativeFighterScript_GenerateC(source, strlen(source), "capsule_positions",
        NULL, NULL, construction, out, &error);
    if (!ok) fprintf(stderr, "line %u: %s\n", error.line, error.message);
    return ok;
}
int main(int argc, char** argv)
{
    assert(argc == 4);
    FILE* file = fopen(argv[1], "rb"); assert(file);
    fseek(file, 0, SEEK_END); long size = ftell(file); rewind(file);
    char* source = malloc(size + 1); assert(source);
    assert(fread(source, 1, size, file) == (size_t)size); source[size] = 0; fclose(file);
    file = fopen(argv[2], "wb"); assert(file); assert(generate(source, file, NULL)); fclose(file);
    const char* tick = strstr(source, "    on tick:"); assert(tick);
    size_t prefix = (size_t)(strstr(source, "action CapsuleCoordinates:") - source); const char* rest = tick + strlen("    on tick:");
    char* state_source = malloc(strlen(source) + 128); assert(state_source);
    snprintf(state_source, strlen(source) + 128, "%.*smove CapsuleCoordinates:\n    on state_341_physics:%s", (int)prefix, source, rest);
    file = fopen(argv[3], "wb"); assert(file); assert(generate(state_source, file, &(const NativeFighterScriptConstruction){"ftDataTest",341,1})); fclose(file);
    free(state_source); free(source);
    ScriptCodeBlock block = {0}; char code[2048];
    for (unsigned period = 0; period < 2; ++period) for (char axis = 'x'; axis <= 'z'; ++axis) {
        char expression[128]; snprintf(expression, sizeof(expression), "hitbox.%s(1 + 2).%c", period ? "previous" : "current", axis);
        assert(nativeScriptCode_ExpressionType(&block, expression) == ScriptType_Float);
        assert(nativeScriptCode_Expression(&block, expression, code, sizeof(code)));
        assert(strstr(code, period ? ".x58." : ".x4C."));
    }
    const char* bad[] = {
        "hitbox.current(1.0).x = 2", "hitbox.current(true).x = 2",
        "hitbox.current(0).x = true", "hitbox.current(0).x = missing",
        "hitbox.current(0, 1).x = 2", "hitbox.current().x = 2",
        "hitbox.current(0).q = 2", "hitbox.previous(0).xy = 2",
        "hitbox.current(0).x() = 2", "hitbox.current(0).x (1) = 2",
        "hitbox.current(0).x.y = 2", "hitbox.current(0).x += 2",
        "let float value = hitbox.current(0).x()",
        "let float value = hitbox.current(0).x (1)",
        "let float value = hitbox.current(0).x[0]",
        "let int value = hitbox.previous(0).x",
    };
    for (unsigned i = 0; i < sizeof(bad)/sizeof(*bad); ++i) {
        char program[1024]; snprintf(program, sizeof(program), "action Invalid:\n    on tick:\n        %s\n", bad[i]);
        file = tmpfile(); assert(file);
        bool accepted = generate(program, file, &(const NativeFighterScriptConstruction){"ftDataTest",341,1}); fclose(file);
        if (accepted) fprintf(stderr, "unexpected accepted %s\n", bad[i]);
        assert(!accepted);
    }
    const char* context[] = {
        "article It_Kind_Seak_NeedleThrow states 1:\n    on created:\n        hitbox.current(0).x = 1\n",
        "model_effect Invalid 0x4DA:\n    on update:\n        let float value = hitbox.previous(0).y\naction Alive:\n    on tick:\n        pass\n",
    };
    for (unsigned i = 0; i < 2; ++i) { file = tmpfile(); assert(file); assert(!generate(context[i], file, &(const NativeFighterScriptConstruction){"ftDataTest",341,1})); fclose(file); }
    assert(!nativeScriptTerrain_Action(&block, "hitbox.current(0).x = 1", code, 1));
    block.model_effect = true;
    assert(!nativeScriptTerrain_Action(&block, "hitbox.current(0).x = 1", code, sizeof(code)));
    assert(nativeScriptTerrain_Type(&block, "hitbox.current(0).x") == ScriptType_Invalid);
    assert(!nativeScriptTerrain_Action(&block, "hitbox.current(0).x = 1", code, 1));
    puts("PASS six capsule Float accessors, strict Int/numeric/context/syntax negatives");
    return 0;
}
