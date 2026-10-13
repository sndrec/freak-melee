#include "../../tools/native_fighter_script_state.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void declarations(void)
{
    ScriptStateTable table = { 0 };
    assert(nativeFighterScript_AddStateField(&table, "state bool enabled"));
    assert(nativeFighterScript_AddStateField(&table, "state float history[16]"));
    assert(nativeFighterScript_AddStateField(&table, "state bool masks[3]"));
    assert(nativeFighterScript_AddStateField(&table, "state int readers[2]"));
    assert(table.count == 4 && table.bytes == 80);
    assert(table.fields[0].offset == 0 && table.fields[0].count == 1 && !table.fields[0].array);
    assert(table.fields[1].offset == 4 && table.fields[1].count == 16 && table.fields[1].array);
    assert(table.fields[2].offset == 68 && table.fields[2].count == 3);
    assert(table.fields[3].offset == 72 && table.fields[3].count == 2);
    ScriptStateTable original = table;
    const char* invalid[] = { "state int readers", "state float history[2]",
        "state bool empty[0]", "state int huge[129]", "state int neg[-1]",
        "state int plus[+1]", "state int fractional[1.0]", "state int tail[2] suffix",
        "state bool no_name[]", "state float bad-name[2]", "state float name [2]" };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        assert(!nativeFighterScript_AddStateField(&table, invalid[i]));
        assert(memcmp(&table, &original, sizeof(table)) == 0);
    }
    ScriptStateTable fighter = { 0 }, article = { .bytes_limit = 512 };
    assert(!nativeFighterScript_AddStateField(&fighter, "state float values[128]"));
    assert(nativeFighterScript_AddStateField(&article, "state float values[128]"));
    assert(article.bytes == 512);
    assert(!nativeFighterScript_AddStateField(&article, "state bool overflow"));
    ScriptStateTable reserved = { .bytes = 2 };
    assert(nativeFighterScript_AddStateField(&reserved, "state bool bit[2]"));
    assert(nativeFighterScript_AddStateField(&reserved, "state int integer[2]"));
    assert(reserved.fields[0].offset == 2 && reserved.fields[1].offset == 4);
    ScriptStateTable limit = { 0 };
    for (unsigned i = 0; i < SCRIPT_STATE_FIELDS; ++i) {
        char line[64]; snprintf(line, sizeof(line), "state bool flag%u[1]", i);
        assert(nativeFighterScript_AddStateField(&limit, line));
    }
    assert(!nativeFighterScript_AddStateField(&limit, "state bool excess"));
}

static void references(void)
{
    ScriptStateTable table = { 0 };
    assert(nativeFighterScript_AddStateField(&table, "state int values[16]"));
    assert(nativeFighterScript_AddStateField(&table, "state bool scalar"));
    NativeScriptStateReference hex;
    assert(nativeScriptState_Reference(&table, "item.state.values[0xF]", "item.state.", &hex) == 1);
    assert(nativeScriptState_Reference(&table, "item.state.values[0x10]", "item.state.", &hex) == -1);
    assert(nativeScriptState_Reference(&table, "item.state.values[-0x1]", "item.state.", &hex) == -1);
    NativeScriptStateReference reference;
    assert(nativeScriptState_Reference(&table, "state.values[15] + 2", "state.", &reference) == 1);
    assert(strcmp(reference.index, "15") == 0 && strcmp(reference.after, " + 2") == 0);
    assert(nativeScriptState_Reference(&table, "item.state.values[state.values[2] + next()] * 3",
                                       "item.state.", &reference) == 1);
    assert(strcmp(reference.index, "state.values[2] + next()") == 0 && strcmp(reference.after, " * 3") == 0);
    assert(nativeScriptState_Reference(&table, "state.scalar + 1", "state.", &reference) == 1);
    assert(reference.index[0] == '\0' && !reference.field->array);
    const char* invalid[] = { "state.values", "state.values[]", "state.values[ ]",
        "state.values[-1]", "state.values[16]", "state.values[1.5]",
        "state.values[99999999999999999999999999]", "state.values[0",
        "state.absent[0]", "state.scalar[0]" };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        assert(nativeScriptState_Reference(&table, invalid[i], "state.", &reference) == -1);
    assert(nativeScriptState_Reference(&table, "other.values[0]", "state.", &reference) == 0);
}

static void metadata(void)
{
    ScriptStateTable table = { .bytes_limit = 512, .bytes = 264 };
    assert(nativeFighterScript_AddStateField(&table, "state float history[16]"));
    assert(nativeFighterScript_AddStateField(&table, "state int readers[2]"));
    assert(nativeFighterScript_AddStateField(&table, "state bool visible[2]"));
    FILE* output = tmpfile(); assert(output != NULL);
    assert(nativeScriptState_EmitMetadata(output, &table, "fixture_fields", true));
    assert(nativeScriptState_EmitCombinedMetadata(output, &table));
    rewind(output); char source[2048] = { 0 }; fread(source, 1, sizeof(source) - 1, output); fclose(output);
    assert(strstr(source, "{ \"history\", 264, 16, 1, 4, true }"));
    assert(strstr(source, "{ \"readers\", 328, 2, 2, 4, true }"));
    assert(strstr(source, "{ \"visible\", 336, 2, 0, 1, true }"));
    assert(strstr(source, "combined_state_fields[base->state_field_count + 0].offset = (uint16_t)(script_state_base + 264)"));
    assert(nativeScriptState_ArrayTypes(&table) == 7);
}

static bool emit_access_fixture(const char* path)
{
    FILE* output = fopen(path, "w"); if (output == NULL) return false;
    bool result = nativeScriptState_EmitArrayAccess(output, 7);
    ScriptStateField field = { .type = ScriptState_Float, .offset = 264, .count = 16, .array = true };
    char read[256], write[256];
    result = result && nativeScriptState_ArrayRead(&field, "bytes", "next_index()", read, sizeof(read)) &&
        nativeScriptState_ArrayWrite(&field, "bytes", "next_index()", "value", write, sizeof(write));
    if (!result) { fclose(output); return false; }
    fprintf(output, "static float fixture_read(uint8_t* bytes) { return %s; }\n"
                    "static void fixture_write(uint8_t* bytes, float value) { %s }\n", read, write);
    result = result && !ferror(output); fclose(output); return result;
}

int main(int argc, char** argv)
{
    declarations(); references(); metadata();
    if (argc > 2 || (argc == 2 && !emit_access_fixture(argv[1]))) return 1;
    puts("Bounded typed state-array declaration, references and metadata passed");
    return 0;
}
