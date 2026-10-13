#include "native_fighter_script_state.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static bool identifier(const char* text, size_t length, size_t capacity)
{
    if (length == 0 || length >= capacity ||
        (!isalpha((unsigned char)text[0]) && text[0] != '_')) return false;
    for (size_t i = 1; i < length; ++i)
        if (!isalnum((unsigned char)text[i]) && text[i] != '_') return false;
    return true;
}

static const char* type_name(ScriptStateType type)
{
    return type == ScriptState_Bool ? "bool" :
           type == ScriptState_Float ? "float" : "int";
}

const ScriptStateField* nativeFighterScript_StateField(
    const ScriptStateTable* table, const char* name)
{
    if (table == NULL || name == NULL) return NULL;
    for (unsigned i = 0; i < table->count; ++i)
        if (strcmp(table->fields[i].name, name) == 0) return &table->fields[i];
    return NULL;
}

bool nativeFighterScript_AddStateField(ScriptStateTable* table,
                                       const char* line)
{
    const char* name;
    ScriptStateType type;
    if (table == NULL || line == NULL || table->count >= SCRIPT_STATE_FIELDS)
        return false;
    if (strncmp(line, "state bool ", 11) == 0) {
        name = line + 11; type = ScriptState_Bool;
    } else if (strncmp(line, "state float ", 12) == 0) {
        name = line + 12; type = ScriptState_Float;
    } else if (strncmp(line, "state int ", 10) == 0) {
        name = line + 10; type = ScriptState_Int;
    } else return false;
    const char* bracket = strchr(name, '[');
    size_t length = bracket != NULL ? (size_t)(bracket - name) : strlen(name);
    char field_name[sizeof(table->fields[0].name)];
    if (!identifier(name, length, sizeof(field_name))) return false;
    memcpy(field_name, name, length); field_name[length] = '\0';
    if (nativeFighterScript_StateField(table, field_name) != NULL) return false;
    unsigned count = 1;
    if (bracket != NULL) {
        const char* cursor = bracket + 1;
        if (!isdigit((unsigned char)*cursor)) return false;
        count = 0;
        while (isdigit((unsigned char)*cursor)) {
            count = count * 10 + (unsigned)(*cursor++ - '0');
            if (count > SCRIPT_STATE_ARRAY_LIMIT) return false;
        }
        if (count == 0 || *cursor++ != ']' || *cursor != '\0') return false;
    }
    unsigned stride = type == ScriptState_Bool ? 1 : 4;
    unsigned limit = table->bytes_limit != 0 ? table->bytes_limit : 256;
    if (limit > SCRIPT_ARTICLE_STATE_BYTES || table->bytes > limit) return false;
    unsigned offset = stride == 4 ? (table->bytes + 3u) & ~3u : table->bytes;
    unsigned next = offset + count * stride;
    if (next > limit) return false;
    ScriptStateField* field = &table->fields[table->count++];
    memcpy(field->name, field_name, length + 1);
    field->type = type; field->offset = (uint16_t)offset;
    field->count = (uint16_t)count; field->array = bracket != NULL;
    table->bytes = next;
    table->has_float |= type == ScriptState_Float;
    table->has_int |= type == ScriptState_Int;
    return true;
}

// Nonconstant expressions are translated by the owning fighter/article parser.
static bool valid_literal_index(const ScriptStateField* field, const char* text)
{
    const char* begin = text;
    while (isspace((unsigned char)*begin)) ++begin;
    char* end;
    errno = 0;
    const char* digits = begin + (*begin == '+' || *begin == '-' ? 1 : 0);
    int base = digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X') ? 16 : 10;
    long value = strtol(begin, &end, base);
    if (end == begin) return true;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0') {
        // A fractional literal is not an integer index.
        char* numeric_end;
        strtod(begin, &numeric_end);
        while (isspace((unsigned char)*numeric_end)) ++numeric_end;
        return *numeric_end != '\0';
    }
    return errno != ERANGE && value >= 0 && (unsigned long)value < field->count;
}

int nativeScriptState_Reference(const ScriptStateTable* table,
                                const char* source, const char* prefix,
                                NativeScriptStateReference* reference)
{
    size_t prefix_length = strlen(prefix);
    if (strncmp(source, prefix, prefix_length) != 0) return 0;
    const char* name = source + prefix_length;
    size_t length = 0;
    while (isalnum((unsigned char)name[length]) || name[length] == '_') ++length;
    char field_name[32];
    if (!identifier(name, length, sizeof(field_name))) return -1;
    memcpy(field_name, name, length); field_name[length] = '\0';
    const ScriptStateField* field = nativeFighterScript_StateField(table, field_name);
    if (field == NULL) return -1;
    const char* cursor = name + length;
    const char* bracket = cursor;
    while (isspace((unsigned char)*bracket)) ++bracket;
    reference->field = field; reference->index[0] = '\0';
    if (!field->array) {
        if (*bracket == '[') return -1;
        reference->after = cursor; return 1;
    }
    if (*bracket != '[') return -1;
    const char* begin = bracket + 1;
    cursor = begin;
    unsigned depth = 0;
    char quote = 0;
    while (*cursor) {
        if (quote) {
            if (*cursor == '\\' && cursor[1]) ++cursor;
            else if (*cursor == quote) quote = 0;
        } else if (*cursor == '"' || *cursor == '\'') quote = *cursor;
        else if (*cursor == '[') ++depth;
        else if (*cursor == ']') {
            if (depth == 0) break;
            --depth;
        }
        ++cursor;
    }
    length = (size_t)(cursor - begin);
    if (*cursor != ']' || length == 0 || length >= sizeof(reference->index)) return -1;
    memcpy(reference->index, begin, length); reference->index[length] = '\0';
    bool nonspace = false;
    for (size_t i = 0; i < length; ++i)
        nonspace |= !isspace((unsigned char)reference->index[i]);
    if (!nonspace || !valid_literal_index(field, reference->index)) return -1;
    reference->after = cursor + 1;
    return 1;
}

bool nativeScriptState_ArrayRead(const ScriptStateField* field,
                                 const char* bytes, const char* index,
                                 char* output, size_t capacity)
{
    if (field == NULL || !field->array || field->count == 0) return false;
    int written = snprintf(output, capacity,
        "script_state_array_%s(%s, %u, (int32_t)(%s), %u)",
        type_name(field->type), bytes, field->offset, index, field->count);
    return written >= 0 && (size_t)written < capacity;
}

bool nativeScriptState_ArrayWrite(const ScriptStateField* field,
                                  const char* bytes, const char* index,
                                  const char* value, char* output, size_t capacity)
{
    if (field == NULL || !field->array || field->count == 0) return false;
    int written = snprintf(output, capacity,
        "script_state_array_set_%s(%s, %u, (int32_t)(%s), %u, %s);",
        type_name(field->type), bytes, field->offset, index, field->count, value);
    return written >= 0 && (size_t)written < capacity;
}

unsigned nativeScriptState_ArrayTypes(const ScriptStateTable* table)
{
    unsigned mask = 0;
    if (table != NULL)
        for (unsigned i = 0; i < table->count; ++i)
            if (table->fields[i].array) mask |= 1u << table->fields[i].type;
    return mask;
}

bool nativeScriptState_EmitArrayAccess(FILE* output, unsigned mask)
{
    for (unsigned type = 0; type <= ScriptState_Int; ++type) {
        if (!(mask & (1u << type))) continue;
        const char* name = type_name((ScriptStateType)type);
        const char* ctype = type == ScriptState_Bool ? "bool" :
                            type == ScriptState_Float ? "float" : "int32_t";
        fprintf(output, "static %s script_state_array_%s(const uint8_t* bytes, unsigned offset, int32_t index, unsigned count) {\n"
                        "    if (bytes == NULL || (uint32_t)index >= count) return 0;\n", ctype, name);
        if (type == ScriptState_Bool)
            fputs("    return bytes[offset + (uint32_t)index] != 0;\n}\n", output);
        else
            fprintf(output, "    %s value; memcpy(&value, bytes + offset + 4u * (uint32_t)index, 4); return value;\n}\n", ctype);
        fprintf(output, "static void script_state_array_set_%s(uint8_t* bytes, unsigned offset, int32_t index, unsigned count, %s value) {\n"
                        "    if (bytes == NULL || (uint32_t)index >= count) return;\n", name, ctype);
        if (type == ScriptState_Bool)
            fputs("    bytes[offset + (uint32_t)index] = (uint8_t)value;\n}\n", output);
        else
            fputs("    memcpy(bytes + offset + 4u * (uint32_t)index, &value, 4);\n}\n", output);
    }
    return !ferror(output);
}

bool nativeScriptState_EmitMetadata(FILE* output, const ScriptStateTable* table,
                                    const char* symbol, bool constant)
{
    if (table->count == 0) return true;
    fprintf(output, "static %sNativeFighterStateField %s[] = {\n", constant ? "const " : "", symbol);
    for (unsigned i = 0; i < table->count; ++i) {
        const ScriptStateField* field = &table->fields[i];
        fprintf(output, "    { \"%s\", %u, %u, %u, %u, %s },\n", field->name,
                field->offset, field->count, (unsigned)field->type,
                field->type == ScriptState_Bool ? 1 : 4, field->array ? "true" : "false");
    }
    fputs("};\n", output);
    return !ferror(output);
}

bool nativeScriptState_EmitCombinedMetadata(FILE* output, const ScriptStateTable* table)
{
    fprintf(output, "    if (base->state_field_count > 64 - %u || (base->state_field_count != 0 && base->state_fields == NULL)) return 0;\n", table->count);
    if (table->count) {
        fprintf(output, "    for (unsigned i = 0; i < base->state_field_count; ++i)\n"
                        "        for (unsigned j = 0; j < %u; ++j)\n"
                        "            if (base->state_fields[i].name == NULL || strcmp(base->state_fields[i].name, authored_state_fields[j].name) == 0) return 0;\n", table->count);
    }
    fputs("    if (base->state_field_count) memcpy(combined_state_fields, base->state_fields, base->state_field_count * sizeof(NativeFighterStateField));\n", output);
    if (table->count) {
        fprintf(output, "    memcpy(combined_state_fields + base->state_field_count, authored_state_fields, %u * sizeof(NativeFighterStateField));\n", table->count);
        for (unsigned i = 0; i < table->count; ++i)
            fprintf(output, "    combined_state_fields[base->state_field_count + %u].offset = (uint16_t)(script_state_base + %u);\n", i, table->fields[i].offset);
    }
    fprintf(output, "    generated.state_field_count = base->state_field_count + %u;\n"
                    "    generated.state_fields = generated.state_field_count ? combined_state_fields : NULL;\n", table->count);
    return !ferror(output);
}
