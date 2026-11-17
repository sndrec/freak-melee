#include "native_fighter_parameters.h"
#include "native_fighter_behavior_source.h"
#include "native_fighter_article_bindings.h"
#include "forward.h"

#include <sysdolphin/baselib/dat.h>

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Stock import metadata only. Runtime behavior reads the copied owned value.
 * Apply only an unchanged template literal, preserving explicit author edits. */
static const struct {
    unsigned source_kind;
    const char* parameter;
    int template_value, imported_value;
} stock_integer_defaults[] = {
    { Ft_Kind_CLink, "spin_effect_joint", 24, 26 },
    { Ft_Kind_CLink, "bomb_holding_joint", 64, 68 },
};

bool nativeFighterParameters_Import(const void* fighter_dat, size_t dat_size,
                                   const char* fighter_root, unsigned source_kind,
                                   const char* script, size_t script_size,
                                   char** owned_script, size_t* owned_size,
                                   char** parameter_text,
                                   size_t* parameter_size,
                                   unsigned* error_line)
{
    if (error_line != NULL) *error_line = 0;
    if (owned_script == NULL || owned_size == NULL || parameter_text == NULL ||
        parameter_size == NULL || script == NULL ||
        script_size > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY ||
        fighter_root == NULL) return false;
    *owned_script = NULL;
    *parameter_text = NULL;
    *owned_size = *parameter_size = 0;
    HSD_Dat dat;
    u32 root, attributes;
    const char* external;
    if (HSD_DatOpen(&dat, fighter_dat, dat_size) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&dat, fighter_root, &root) ||
        HSD_DatResolve(&dat, root + 4, &attributes, &external) !=
            HSD_DAT_REF_LOCAL) return false;
    u32 attribute_end = HSD_DatNextTarget(&dat, attributes);
    const u32 fighter_attributes = attributes;
    const u32 fighter_attribute_end = attribute_end;
    char article_name[64] = { 0 };
    bool skip_article = false;
    if (attribute_end < attributes) return false;
    const size_t result_capacity = script_size * 2 + 1;
    char* result = malloc(result_capacity);
    char* values = malloc(NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1);
    if (result == NULL || values == NULL) {
        free(result);
        free(values);
        return false;
    }
    size_t output_size = 0, values_size = 0;
    unsigned line_number = 1;
    for (size_t start = 0; start < script_size; ++line_number) {
        size_t end = start;
        while (end < script_size && script[end] != '\n') ++end;
        size_t length = end - start;
        if (length != 0 && script[start + length - 1] == '\r') --length;
        // Shared stock templates may contain both variants, but only the
        // source fighter's article assets become part of this independent draft.
        if (length != 0 && script[start] != ' ' && script[start] != '#' &&
            script[start] != '\t') {
            article_name[0] = '\0';
            skip_article = false;
            attributes = fighter_attributes;
            attribute_end = fighter_attribute_end;
            if (length >= 8 && memcmp(script + start, "article ", 8) == 0) {
                char header[256];
                if (length >= sizeof(header)) goto invalid;
                memcpy(header, script + start, length);
                header[length] = '\0';
                if (sscanf(header, "article %63s", article_name) != 1) goto invalid;
                const NativeStockArticleSlot* mapping = NULL;
                bool stock_article = false;
                for (unsigned i = 0; i < nativeStockArticleSlotCount; ++i) {
                    if (strcmp(article_name, nativeStockArticleSlots[i].name) == 0) {
                        stock_article = true;
                        // Multiple owned bases may declare the same article kind.
                        // Select this base's slot, not the first matching name.
                        if (nativeStockArticleSlots[i].source_kind == source_kind) {
                            mapping = &nativeStockArticleSlots[i];
                            break;
                        }
                    }
                }
                if (mapping == NULL && stock_article)
                    skip_article = true;
                else if (mapping != NULL) {
                    u32 table, descriptor;
                    if (HSD_DatResolve(&dat, root + 72, &table, &external) != HSD_DAT_REF_LOCAL ||
                        HSD_DatResolve(&dat, table + mapping->slot * 4, &descriptor, &external) != HSD_DAT_REF_LOCAL)
                        goto invalid;
                    if (HSD_DatResolve(&dat, descriptor + 4, &attributes, &external) != HSD_DAT_REF_LOCAL)
                        attributes = 0;
                    attribute_end = attributes ? HSD_DatNextTarget(&dat, attributes) : 0;
                    if (attribute_end < attributes) goto invalid;
                } else {
                    // An unknown article can have literal configuration, but
                    // cannot import tuning from an unrelated fighter block.
                    attributes = attribute_end = 0;
                }
            }
        }
        if (skip_article) {
            // Keep original file/line positions for editor diagnostics.
            if (end < script_size) result[output_size++] = '\n';
            start = end < script_size ? end + 1 : end;
            continue;
        }
        const size_t indent = article_name[0] ? 4 : 0;
        const char* declaration_start = script + start + (length >= indent ? indent : 0);
        const size_t declaration_size = length >= indent ? length - indent : 0;
        const bool integer = declaration_size >= 14 &&
            memcmp(declaration_start, "attribute int ", 14) == 0;
        const bool floating = declaration_size >= 16 &&
            memcmp(declaration_start, "attribute float ", 16) == 0;
        const bool boolean = declaration_size >= 15 &&
            memcmp(declaration_start, "attribute bool ", 15) == 0;
        const bool byte = declaration_size >= 15 &&
            memcmp(declaration_start, "attribute byte ", 15) == 0;
        const size_t values_begin = values_size;
        if (declaration_size >= 10 && memcmp(declaration_start, "parameter ", 10) == 0) {
            // Template-owned defaults have no DAT address. Qualify article
            // names exactly like imported values, then validate with the same
            // project parameter parser (including duplicate/type checks).
            char line[256], type[8], local_name[48], literal[128];
            int consumed = 0;
            if (declaration_size >= sizeof(line)) goto invalid;
            memcpy(line, declaration_start, declaration_size);
            line[declaration_size] = '\0';
            if (sscanf(line, "parameter %7s %47s = %127s%n", type, local_name,
                       literal, &consumed) != 3) goto invalid;
            while (line[consumed] == ' ' || line[consumed] == '\t') ++consumed;
            if (line[consumed] != '\0') goto invalid;
            if (!article_name[0] && strcmp(type, "int") == 0) {
                char* end;
                long value = strtol(literal, &end, 10);
                if (*end == '\0') {
                    for (unsigned i = 0; i < sizeof(stock_integer_defaults) / sizeof(stock_integer_defaults[0]); ++i) {
                        if (stock_integer_defaults[i].source_kind == source_kind &&
                            strcmp(stock_integer_defaults[i].parameter, local_name) == 0 &&
                            stock_integer_defaults[i].template_value == value) {
                            snprintf(literal, sizeof(literal), "%d", stock_integer_defaults[i].imported_value);
                            break;
                        }
                    }
                }
            }
            int written = snprintf(values + values_size, NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1 - values_size,
                "%s %s%s%s = %s\n", type, article_name,
                article_name[0] ? "." : "", local_name, literal);
            if (written <= 0 || (size_t) written >= NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1 - values_size) goto invalid;
            values_size += (size_t) written;
        } else if (integer || floating || boolean || byte) {
            char line[256], name[128], local_name[48];
            if (declaration_size >= sizeof(line)) goto invalid;
            memcpy(line, declaration_start, declaration_size);
            line[declaration_size] = '\0';
            const char* declaration = line + (integer ? 14 : (boolean || byte) ? 15 : 16);
            const char* separator = strchr(declaration, ' ');
            if (separator == NULL || separator == declaration ||
                separator - declaration >= sizeof(local_name)) goto invalid;
            size_t name_size = (size_t) (separator - declaration);
            memcpy(local_name, declaration, name_size);
            local_name[name_size] = '\0';
            int qualified = snprintf(name, sizeof(name), "%s%s%s",
                article_name, article_name[0] ? "." : "", local_name);
            if (qualified <= 0 || qualified >= sizeof(name)) goto invalid;
            char* offset_end;
            const char* offset_text = separator + 1;
            while (*offset_text == ' ' || *offset_text == '\t') ++offset_text;
            if (*offset_text == '-') goto invalid;
            errno = 0;
            unsigned long offset = strtoul(offset_text, &offset_end, 0);
            while (*offset_end == ' ' || *offset_end == '\t') ++offset_end;
            if (errno != 0 || offset_end == offset_text || *offset_end != '\0' ||
                (!(boolean || byte) && (offset & 3) != 0) ||
                attributes == 0 || offset > attribute_end - attributes ||
                attribute_end - attributes - offset < ((boolean || byte) ? 1 : 4)) goto invalid;
            u32 bits;
            if (boolean || byte) {
                const u8* value = HSD_DatBytes(&dat, attributes + (u32) offset, 1);
                if (value == NULL || (boolean && *value > 1)) goto invalid;
                bits = *value;
            } else if (!HSD_DatReadU32(&dat, attributes + (u32) offset, &bits)) goto invalid;
            int written;
            if (boolean) {
                written = snprintf(values + values_size, NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1 - values_size,
                                   "bool %s = %s\n", name, bits ? "true" : "false");
            } else if (integer || byte) {
                written = snprintf(values + values_size, NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1 - values_size,
                                   "int %s = %ld\n", name, (long) (int32_t) bits);
            } else {
                float value;
                memcpy(&value, &bits, sizeof(value));
                if (!isfinite(value)) goto invalid;
                // Nine significant decimal digits round-trip every binary32
                // value, preserving stock constants exactly on native load.
                written = snprintf(values + values_size, NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1 - values_size,
                                   "float %s = %.9g\n", name, (double) value);
            }
            if (written <= 0 || (size_t) written >= NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY + 1 - values_size)
                goto invalid;
            values_size += (size_t) written;
        } else {
            const size_t next = end < script_size ? end + 1 : end;
            for (size_t cursor = start; cursor < next;) {
                static const char imported[] = "item.attr.";
                static const char owned[] = "item.params.";
                if (article_name[0] && cursor + sizeof(imported) - 1 <= next &&
                    memcmp(script + cursor, imported, sizeof(imported) - 1) == 0) {
                    if (sizeof(owned) - 1 >= result_capacity - output_size) goto invalid;
                    memcpy(result + output_size, owned, sizeof(owned) - 1);
                    output_size += sizeof(owned) - 1;
                    cursor += sizeof(imported) - 1;
                } else {
                    if (output_size + 1 >= result_capacity) goto invalid;
                    result[output_size++] = script[cursor++];
                }
            }
        }
        if (values_size != values_begin) {
            // The interpreter indexes declarations, not sidecar names. Keep an
            // owned literal declaration in its original scope, replacing the
            // import offset. The editable sidecar can override that default.
            const char* definition = values + values_begin;
            const char* space = strchr(definition, ' ');
            if (space == NULL) goto invalid;
            const char* local = space + 1 + (article_name[0] ? strlen(article_name) + 1 : 0);
            int written = snprintf(result + output_size, result_capacity - output_size,
                "%*sparameter %.*s %s", (int) indent, "", (int) (space - definition), definition, local);
            if (written <= 0 || (size_t) written >= result_capacity - output_size) goto invalid;
            output_size += (size_t) written;
        }
        start = end < script_size ? end + 1 : end;
    }
    if (output_size > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY) goto invalid;
    NativeFighterParameters checked;
    if (!nativeFighterParameters_Parse(values, values_size, &checked, NULL))
        goto invalid;
    result[output_size] = '\0';
    values[values_size] = '\0';
    *owned_script = result;
    *owned_size = output_size;
    *parameter_text = values;
    *parameter_size = values_size;
    return true;
invalid:
    if (error_line != NULL) *error_line = line_number;
    free(result);
    free(values);
    return false;
}
