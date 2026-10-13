#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_code.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static bool generate(const char* source, const char* path, bool inspect, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w+") : tmpfile(); assert(output);
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "typed_locals_probe", NULL, NULL, construction, output, &error);
    if (!valid && inspect) fprintf(stderr, "Typed locals at%u: %s\n", error.line, error.message);
    if (valid && inspect) {
        rewind(output);
        char code[65536]; size_t size = fread(code, 1, sizeof(code) - 1, output);
        assert(feof(output)); code[size] = 0;
        assert(strstr(code, "int32_t local_above = 16777217;"));
        assert(strstr(code, "bool local_active = "));
        assert(strstr(code, "float local_old = local_above;"));
        assert(strstr(code, "int32_t local_id = item->x1C;"));
        assert(strstr(code, "3735928559u"));
        assert(strstr(code, "script_fn_store(recipient, local_id, local_active, 1.25f)"));
    }
    fclose(output); return valid;
}

static void positive(bool callbacks, const char* path)
{
    char source[8192];
    snprintf(source, sizeof(source),
        "%sstate int above\nstate int high\nstate int negative\nstate int quotient\n"
        "state int received\nstate bool ready\nstate float amount\nstate float old\n"
        "func store(int value, bool active, float amount):\n"
        "    state.received = value\n    state.ready = active\n    state.amount = amount\n"
        "%s"
        "        let int above = 16777217\n"
        "        let int high = int(uint(0x80000001))\n"
        "        let int negative = -1\n"
        "        let float amount = 1.25\n"
        "        let old = above\n"
        "        let bool active = above > 16777216 and not false\n"
        "        let int quotient = 7 / 2\n"
        "        quotient = quotient + active\n"
        "        if active:\n"
        "            let int inside = above + 2\n"
        "            above = inside\n"
        "        state.above = above\n        state.high = high\n"
        "        state.negative = negative\n        state.quotient = quotient\n"
        "        state.old = old\n        store(high, active, amount)\n"
        "%s"
        "        let int result = 16777217\n        return result\n"
        "article It_Kind_Ness_PKFire states 1:\n"
        "    state int id\n    state int high\n    state int negative\n    state int sum\n"
        "    state bool active\n    state bool nonzero\n    state float old\n"
        "    on created:\n"
        "            let int id = item.creation_id\n"
        "            let int odd = 16777217\n"
        "            let bool active = id != 0 and creator.exists\n"
        "            let bool nonzero = 2.5\n"
        "            let old = odd\n"
        "            creator.call(store, id, active, 1.25)\n"
        "            item.state.id = id\n"
        "            item.state.high = int(0xDEADBEEF)\n"
        "            item.state.negative = -1\n"
        "            item.state.sum = odd + 2\n"
        "            item.state.active = active\n"
        "            item.state.nonzero = nonzero\n"
        "            item.state.old = old\n"
        "    state 0 anim -1:\n",
        callbacks ? "" : "",
        callbacks ? "move Locals:\n    on state_341_physics:\n" : "action Locals:\n    on load:\n",
        callbacks ? "    on jab_start:\n" : "    on jab_start:\n");
    assert(generate(source, path, true, callbacks ? &(const NativeFighterScriptConstruction){"ftDataNess",341,1} : NULL));
}

static void rejection(const NativeFighterScriptConstruction* construction)
{
    assert(generate("state float amount\naction Math:\n    on load:\n"
        "        state.amount = wrap_signed_angle(atan2_float(1, 2))\n"
        "        let float root = sqrt(max(1, min(2, abs(-3))))\n"
        "        let float cast = float(int(uint(1)))\n"
        "        let bool grouped = not (false) and (true) or (false)\n",
        NULL, false, construction));
    const char* statements[] = {
        "let int value = 1.5",
        "let int value = velocity.x",
        "let value = 16777217\n        let int id = value",
        "let int value = 3\n        value = 0.5",
        "let int value = 3\n        if true: value = 0.5",
        "let int value = 3 + 0.5",
        "let int value = 1.5 & 1",
        "let int value = true % 0.5",
        "let int value = 1\n        let bool value = true",
        "if true:\n            let int inside = 1\n        let int outside = inside",
        "let int value = (1 + )",
        "let int value = int(1.5) + 0.5",
        "let int value = state.amount",
        "let bool value = input.special_held()",
        "let float value = velocity.x ()",
        "let bool value = guard.front_only()",
        "let float value = state.amount()",
        "let float scalar = 1\n        let float value = scalar ()",
    };
    for (unsigned i = 0; i < sizeof(statements) / sizeof(statements[0]); ++i) {
        char source[4096];
        snprintf(source, sizeof(source), "state float amount\naction Probe:\n    on load:\n        %s\n", statements[i]);
        assert(!generate(source, NULL, false, construction));
        // Article callbacks have their own indentation and scope stack.
        char indented[2048]; size_t used = 0;
        for (const char* p = statements[i]; *p; ++p) {
            indented[used++] = *p;
            if (*p == '\n') { memcpy(indented + used, "    ", 4); used += 4; }
        }
        indented[used] = 0;
        snprintf(source, sizeof(source), "state float amount\narticle It_Kind_Ness_PKFire states 1:\n"
            "    state 0 anim -1:\n        on physics:\n            %s\n", indented);
        // state.amount is fighter-only; use the matching owner field in articles.
        char* atom = strstr(source, "state.amount");
        if (atom) {
            size_t tail = strlen(atom); memmove(atom + 6, atom, tail + 1);
            memcpy(atom, "owner.", 6);
        }
        assert(!generate(source, NULL, false, construction));
    }
    const char* bad_calls[] = {
        "state float amount\nfunc store(int value):\n    state.amount = value\n"
        "action Probe:\n    on load:\n        store(1.5)\n",
        "state float amount\nfunc store(int value):\n    value = 0.5\n"
        "action Probe:\n    on load:\n        store(1)\n",
        "state float amount\nfunc store(int value):\n    state.amount = value\n"
        "article It_Kind_Ness_PKFire states 1:\n    state 0 anim -1:\n"
        "        on physics:\n            creator.call(store, 1.5)\n",
        "action Probe:\n    on jab_start:\n        return 1.5\n",
    };
    for (unsigned i = 0; i < sizeof(bad_calls) / sizeof(bad_calls[0]); ++i)
        assert(!generate(bad_calls[i], NULL, false, construction));
    assert(generate("action Probe:\n    on jab_start:\n        return int(1.5)\n", NULL, false, construction));
}

static ScriptExpressionType atom(const void* context, const char* name)
{
    (void)context;
    return strcmp(name, "floating") == 0 ? ScriptType_Float :
           strcmp(name, "integer") == 0 ? ScriptType_Int : nativeScriptTypes_Builtin(name);
}

static void precedence(void)
{
    struct { const char* source; ScriptExpressionType type; } cases[] = {
        { "16777217", ScriptType_Int }, { "0xDEADBEEF", ScriptType_Int },
        { "2147483648", ScriptType_Int }, { "-1", ScriptType_Int },
        { "1.0", ScriptType_Float }, { "1e-3", ScriptType_Float },
        { "integer + true", ScriptType_Int }, { "integer / 2", ScriptType_Int },
        { "integer + floating", ScriptType_Float },
        { "int(floating) + integer", ScriptType_Int },
        { "integer > floating and not false", ScriptType_Bool },
        { "integer + integer * floating", ScriptType_Float },
        { "integer | integer == floating", ScriptType_Int },
        { "integer | floating", ScriptType_Invalid },
        { "integer << floating", ScriptType_Invalid },
        { "integer % floating", ScriptType_Invalid },
        { "(integer + )", ScriptType_Invalid },
        { "not", ScriptType_Invalid }, { "()", ScriptType_Invalid },
        { "signal.value", ScriptType_Float },
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        assert(nativeScriptTypes_Expression(cases[i].source, atom, NULL) == cases[i].type);
    char nested[256]; memset(nested, '(', 80); nested[80] = '1';
    memset(nested + 81, ')', 80); nested[161] = 0;
    assert(nativeScriptTypes_Expression(nested, atom, NULL) == ScriptType_Invalid);
}

int main(int argc, char** argv)
{
    if (argc != 1 && argc != 3) return 2;
    precedence(); rejection(NULL);
    positive(false, argc == 3 ? argv[1] : NULL);
    positive(true, argc == 3 ? argv[2] : NULL);
    puts("Typed locals preserve integer C expressions and reject implicit float conversion in both contexts.");
    return 0;
}
