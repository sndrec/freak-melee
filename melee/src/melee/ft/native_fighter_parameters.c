#include "native_fighter_parameters.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint64_t hash_byte(uint64_t hash, uint8_t byte)
{
    return (hash ^ byte) * UINT64_C(1099511628211);
}

static bool identifier(const char* name, size_t size)
{
    if (size == 0 || size >= sizeof(((NativeFighterParameter*) 0)->name) ||
        !(isalpha((unsigned char) name[0]) || name[0] == '_')) return false;
    for (size_t i = 1; i < size; ++i)
        if (!(isalnum((unsigned char) name[i]) || name[i] == '_' ||
              (name[i] == '.' && i + 1 < size &&
               (isalpha((unsigned char) name[i + 1]) || name[i + 1] == '_'))))
            return false;
    return true;
}

static bool parse_value(const char* text, NativeFighterParameter* field)
{
    if (*text == '\0') return false;
    if (field->type == NativeFighterParameter_Bool) {
        if (strcmp(text, "true") == 0) field->bits = 1;
        else if (strcmp(text, "false") == 0) field->bits = 0;
        else return false;
        return true;
    }
    char* end;
    errno = 0;
    if (field->type == NativeFighterParameter_Int) {
        long long value = strtoll(text, &end, 0);
        if (errno != 0 || end == text || *end != '\0' ||
            value < INT32_MIN || value > INT32_MAX) return false;
        field->bits = (uint32_t) (int32_t) value;
        return true;
    }
    float value = strtof(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !isfinite(value))
        return false;
    memcpy(&field->bits, &value, sizeof(value));
    return true;
}

bool nativeFighterParameters_Parse(const char* source, size_t size,
                                   NativeFighterParameters* parameters,
                                   unsigned* error_line)
{
    if (parameters == NULL || source == NULL ||
        size > NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY ||
        memchr(source, '\0', size) != NULL)
        return false;
    memset(parameters, 0, sizeof(*parameters));
    if (error_line != NULL) *error_line = 0;
    uint64_t hash = UINT64_C(14695981039346656037);
    unsigned line_number = 1;
    for (size_t start = 0; start < size; ++line_number) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        size_t length = end - start;
        if (length != 0 && source[start + length - 1] == '\r') --length;
        char line[256];
        if (length >= sizeof(line)) goto invalid;
        memcpy(line, source + start, length);
        line[length] = '\0';
        char* cursor = line;
        while (*cursor == ' ' || *cursor == '\t') ++cursor;
        if (*cursor != '\0' && *cursor != '#') {
            if (parameters->count == NATIVE_FIGHTER_PARAMETER_CAPACITY)
                goto invalid;
            NativeFighterParameter* field =
                &parameters->fields[parameters->count];
            if (strncmp(cursor, "float ", 6) == 0) {
                field->type = NativeFighterParameter_Float;
                cursor += 6;
            } else if (strncmp(cursor, "int ", 4) == 0) {
                field->type = NativeFighterParameter_Int;
                cursor += 4;
            } else if (strncmp(cursor, "bool ", 5) == 0) {
                field->type = NativeFighterParameter_Bool;
                cursor += 5;
            } else goto invalid;
            char* separator = strchr(cursor, ' ');
            if (separator == NULL ||
                !identifier(cursor, (size_t) (separator - cursor)))
                goto invalid;
            size_t name_size = (size_t) (separator - cursor);
            memcpy(field->name, cursor, name_size);
            field->name[name_size] = '\0';
            for (unsigned i = 0; i < parameters->count; ++i)
                if (strcmp(parameters->fields[i].name, field->name) == 0)
                    goto invalid;
            cursor = separator;
            while (*cursor == ' ' || *cursor == '\t') ++cursor;
            if (*cursor++ != '=') goto invalid;
            while (*cursor == ' ' || *cursor == '\t') ++cursor;
            size_t value_size = strlen(cursor);
            while (value_size != 0 &&
                   (cursor[value_size - 1] == ' ' ||
                    cursor[value_size - 1] == '\t'))
                cursor[--value_size] = '\0';
            if (!parse_value(cursor, field)) goto invalid;
            hash = hash_byte(hash, (uint8_t) field->type);
            for (size_t i = 0; i < name_size; ++i)
                hash = hash_byte(hash, (uint8_t) field->name[i]);
            hash = hash_byte(hash, 0);
            ++parameters->count;
        }
        start = end < size ? end + 1 : size;
    }
    parameters->schema_hash = hash;
    return true;
invalid:
    if (error_line != NULL) *error_line = line_number;
    return false;
}
