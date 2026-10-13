#include "native_script_model_metadata.h"
#include "native_source_asset_facts.h"
#include <string.h>

typedef struct ModelWord { uint32_t start, length; } ModelWord;
static bool letter(char c)
{ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static bool space(char c) { return c == ' ' || c == '\t'; }
static bool equal(const char* text, ModelWord word, const char* expected)
{ return word.length == strlen(expected) && !memcmp(text + word.start, expected, word.length); }

static bool integer(const char* text, uint32_t length, uint32_t* result)
{
    uint32_t at = 0, value = 0, digits = 0;
    unsigned base = 10;
    if (length >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16; at = 2;
    }
    while (at < length) {
        char c = text[at++]; unsigned digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else return false;
        if (digit >= base || value > (UINT32_MAX - digit) / base) return false;
        value = value * base + digit; ++digits;
    }
    if (!digits) return false;
    *result = value; return true;
}

static bool component(const char* text, ModelWord word,
                      NativeFighterArticleAnimationComponent* result)
{
    if (equal(text, word, "None")) return true;
    const char* value = text + word.start;
    uint32_t prefix = 0;
    while (prefix < word.length && value[prefix] != '[') ++prefix;
    if (!integer(value, prefix, &result->attribute_offset) || (result->attribute_offset & 3))
        return false;
    if (prefix == word.length) return true;
    if (word.length < prefix + 3 || value[word.length - 1] != ']' ||
        !integer(value + prefix + 1, word.length - prefix - 2, &result->array_index) ||
        result->array_index >= UINT32_MAX / 4) return false;
    return true;
}

bool nativeScriptModelMetadata_Parse(const NativeScriptSourceTree* tree,
    NativeScriptSourceSpan span, bool animation, NativeScriptModelMetadata* output)
{
    if (!tree || !tree->source || !output || !span.length ||
        span.offset > tree->source_bytes || span.length > tree->source_bytes - span.offset)
        return false;
    const char* text = tree->source + span.offset;
    ModelWord words[7]; uint32_t count = 0, at = 0;
    while (at < span.length) {
        while (at < span.length && space(text[at])) ++at;
        if (at == span.length) break;
        if (count == sizeof(words) / sizeof(words[0])) return false;
        uint32_t start = at;
        while (at < span.length && !space(text[at])) ++at;
        words[count++] = (ModelWord){ start, at - start };
    }
    if (count < 2 || words[0].length >= NATIVE_SOURCE_NAME_BYTES ||
        !letter(text[words[0].start])) return false;
    for (uint32_t i = 1; i < words[0].length; ++i) {
        char c = text[words[0].start + i];
        if (!letter(c) && !(c >= '0' && c <= '9')) return false;
    }
    NativeScriptModelMetadata candidate = { .name = span, .animation = animation };
    candidate.name.offset += words[0].start;
    candidate.name.column += words[0].start;
    candidate.name.length = words[0].length;
    if (!animation) {
        if (count != 2 || !integer(text + words[1].start, words[1].length, &candidate.root_offset) ||
            (candidate.root_offset & 3)) return false;
    } else {
        for (unsigned i = 0; i < 3; ++i)
            candidate.components[i] = (NativeFighterArticleAnimationComponent){ UINT32_MAX, UINT32_MAX };
        if (count == 2) {
            uint32_t offset;
            if (!integer(text + words[1].start, words[1].length, &offset) ||
                (offset & 3) || offset > UINT32_MAX - 8) return false;
            for (unsigned i = 0; i < 3; ++i) candidate.components[i].attribute_offset = offset + 4 * i;
        } else {
            if (!(count & 1)) return false;
            unsigned seen = 0;
            const char* const names[] = { "joint", "material", "shape" };
            for (uint32_t i = 1; i < count; i += 2) {
                unsigned channel = 0;
                while (channel < 3 && !equal(text, words[i], names[channel])) ++channel;
                if (channel == 3 || (seen & (1u << channel)) ||
                    !component(text, words[i + 1], &candidate.components[channel])) return false;
                seen |= 1u << channel;
            }
        }
    }
    *output = candidate; return true;
}
