#include "native_fighter_script_sound_policy.h"
#include "native_fighter_script_code.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static bool integer_arguments(const char* source, int64_t* values, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        if (!isspace((unsigned char)*source)) return false;
        while (isspace((unsigned char)*source)) ++source;
        bool negative = *source == '-';
        if (negative) ++source;
        if (!isdigit((unsigned char)*source)) return false;
        unsigned base = source[0] == '0' &&
            (source[1] == 'x' || source[1] == 'X') ? 16 : 10;
        errno = 0;
        char* after;
        unsigned long long magnitude = strtoull(source, &after, base);
        if (errno || after == source || magnitude > UINT32_MAX) return false;
        values[i] = negative ? -(int64_t)magnitude : (int64_t)magnitude;
        source = after;
    }
    while (isspace((unsigned char)*source)) ++source;
    return *source == 0 || *source == '#';
}

static bool routing_declaration(const char* text, size_t length, bool remap,
                                ScriptSoundPolicy* policy)
{
    char declaration[256];
    if (length >= sizeof(declaration) || memchr(text, 0, length)) return false;
    memcpy(declaration, text, length); declaration[length] = 0;
    int64_t args[5];
    if (!integer_arguments(declaration + (remap ? 11 : 12), args,
                           remap ? 5 : 2) || args[0] <= 0 ||
        args[0] > UINT32_MAX) return false;
    if (remap) {
        if (policy->rule_count == NATIVE_FIGHTER_SOUND_RULE_CAPACITY ||
            args[1] < 0 || args[1] >= 64 || args[2] < 0 ||
            args[3] < args[2] || args[3] > INT32_MAX ||
            args[4] < INT32_MIN || args[4] > INT32_MAX ||
            args[2] + args[4] < 0 || args[3] + args[4] > INT32_MAX) return false;
        policy->rules[policy->rule_count++] = (NativeFighterSoundIdRule){
            .costume_mask = (uint32_t)args[0], .bank = (int32_t)args[1],
            .first = (int32_t)args[2], .last = (int32_t)args[3],
            .offset = (int32_t)args[4],
        };
    } else {
        if (policy->ko_cue_count == NATIVE_FIGHTER_KO_CUE_CAPACITY ||
            args[1] < 0 || args[1] > INT32_MAX) return false;
        for (unsigned i = 0; i < policy->ko_cue_count; ++i)
            if (policy->ko_cues[i].costume_mask & (uint32_t)args[0]) return false;
        policy->ko_cues[policy->ko_cue_count++] = (NativeFighterKoCue){
            .costume_mask = (uint32_t)args[0], .sound_id = (int32_t)args[1],
        };
    }
    return true;
}

static bool is_keyword(const char* text, size_t length, const char* word)
{
    const size_t n = strlen(word);
    return length >= n && !memcmp(text, word, n) &&
        (length == n || (!isalnum((unsigned char)text[n]) && text[n] != '_'));
}

bool nativeScriptSoundPolicy_Parse(char* source, size_t size,
    ScriptSoundPolicy* policy, NativeFighterScriptError* error)
{
    *policy = (ScriptSoundPolicy){0};
    bool code_seen = false;
    unsigned line = 0;
    const char keyword[] = "sound_mute_channels";
    for (size_t start = 0; start < size;) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        ++line;
        size_t text_start = start;
        while (text_start < end && (source[text_start] == ' ' || source[text_start] == '\t')) ++text_start;
        size_t length = end - text_start;
        const char* text = source + text_start;
        const bool remap = is_keyword(text, length, "sound_remap");
        const bool ko_cue = is_keyword(text, length, "sound_ko_cue");
        if (remap || ko_cue) {
            if (text_start != start || code_seen)
                return nativeScriptCode_Fail(error, line,
                    "Sound routing declarations belong at top level before code blocks.");
            if (!routing_declaration(text, length, remap, policy))
                return nativeScriptCode_Fail(error, line, remap
                    ? "Expected sound_remap COSTUME_MASK BANK FIRST LAST OFFSET (Int literals; max16 rules; mapped IDs must stay nonnegative Int32)."
                    : "Expected sound_ko_cue COSTUME_MASK SOUND_ID (Int literals; max8 disjoint costume masks; nonnegative Int32 ID).");
            memset(source + start, ' ', end - start);
        } else if (length >= sizeof(keyword)-1 && !memcmp(text, keyword, sizeof(keyword)-1) &&
            (length == sizeof(keyword)-1 || !isalnum((unsigned char)text[sizeof(keyword)-1]) && text[sizeof(keyword)-1] != '_')) {
            char declaration[96];
            if (text_start != start || code_seen || policy->declared)
                return nativeScriptCode_Fail(error, line,
                    "sound_mute_channels is one global declaration before code blocks.");
            if (length >= sizeof(declaration) || memchr(text, 0, length))
                return nativeScriptCode_Fail(error, line, "Invalid sound_mute_channels literal.");
            memcpy(declaration, text, length); declaration[length] = 0;
            char* value = declaration + sizeof(keyword)-1;
            if (!isspace((unsigned char)*value))
                return nativeScriptCode_Fail(error, line, "Expected sound_mute_channels Int literal (0..127).");
            while (isspace((unsigned char)*value)) ++value;
            if (!isdigit((unsigned char)*value))
                return nativeScriptCode_Fail(error, line, "Expected sound_mute_channels Int literal (0..127).");
            errno = 0; char* after;
            unsigned base = value[0] == '0' && (value[1] == 'x' || value[1] == 'X') ? 16 : 10;
            unsigned long mask = strtoul(value, &after, base);
            while (isspace((unsigned char)*after)) ++after;
            if (errno || mask > 0x7F || (*after && *after != '#'))
                return nativeScriptCode_Fail(error, line, "sound_mute_channels needs one Int literal in 0..127.");
            policy->declared = true; policy->mute_channels = (uint8_t)mask;
            memset(source + start, ' ', end - start); // Preserve original diagnostics.
        } else if (text_start == start && length) {
            static const char* blocks[] = {"on ","action ","move ","func ","export func ","article ","item ","model_effect "};
            for (unsigned i = 0; i < sizeof(blocks)/sizeof(blocks[0]); ++i)
                code_seen |= length >= strlen(blocks[i]) && !memcmp(text, blocks[i], strlen(blocks[i]));
        }
        start = end < size ? end + 1 : end;
    }
    return true;
}

void nativeScriptSoundPolicy_EmitDefinition(const ScriptSoundPolicy* policy, FILE* output)
{
    if (policy && policy->declared)
        fprintf(output, "    generated.sound_mute_channels = %u;\n", policy->mute_channels);
    if (policy == NULL || (policy->rule_count == 0 && policy->ko_cue_count == 0)) return;
    if (policy->rule_count) {
        fputs("    static const NativeFighterSoundIdRule authored_sound_rules[] = {\n", output);
        for (unsigned i = 0; i < policy->rule_count; ++i) {
            const NativeFighterSoundIdRule* rule = &policy->rules[i];
            fprintf(output, "        { UINT32_C(%lu), %ld, %ld, %ld, %lld },\n",
                    (unsigned long)rule->costume_mask, (long)rule->bank,
                    (long)rule->first, (long)rule->last, (long long)rule->offset);
        }
        fputs("    };\n", output);
    }
    if (policy->ko_cue_count) {
        fputs("    static const NativeFighterKoCue authored_ko_cues[] = {\n", output);
        for (unsigned i = 0; i < policy->ko_cue_count; ++i)
            fprintf(output, "        { UINT32_C(%lu), %ld },\n",
                    (unsigned long)policy->ko_cues[i].costume_mask,
                    (long)policy->ko_cues[i].sound_id);
        fputs("    };\n", output);
    }
    fprintf(output, "    static const NativeFighterSoundPolicy authored_sound_policy = { %s, %s, %u, %u };\n"
                    "    generated.sound_policy = &authored_sound_policy;\n",
            policy->rule_count ? "authored_sound_rules" : "0",
            policy->ko_cue_count ? "authored_ko_cues" : "0",
            policy->rule_count, policy->ko_cue_count);
}
