#include "native_fighter_item_links.h"
#include <melee/it/native_article.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int bone_channel(const char* suffix, const char** after)
{
    static const char* groups[] = { ".position.", ".rotation.", ".scale." };
    for (unsigned i = 0; i < 3; ++i) {
        size_t length = strlen(groups[i]);
        if (strncmp(suffix, groups[i], length)) continue;
        char axis = suffix[length];
        if (axis < 'x' || axis > 'z') return -1;
        char next = suffix[length + 1];
        if (isalnum((unsigned char)next) || next == '_' || next == '.') return -1;
        *after = suffix + length + 1;
        return i * 3 + axis - 'x';
    }
    return -1;
}

bool nativeItemLinks_Expression(const char* source, char* output, size_t capacity,
                                 const char** after)
{
    unsigned slot, backlink, bone;
    int consumed = 0, written;
    if (sscanf(source, "item.link_reciprocal(%u, %u)%n", &slot, &backlink, &consumed) == 2 &&
        consumed && slot < 4 && backlink < 4) {
        *after = source + consumed;
        written = snprintf(output, capacity, "engine->item_link_reciprocal(gobj, %u, %u)", slot, backlink);
    } else if ((consumed = 0, sscanf(source, "item.link(%u)%n", &slot, &consumed)) == 1 &&
               consumed && slot < 4) {
        static const struct { const char* text; unsigned property; } fields[] = {
            { ".exists", NativeArticleLink_Exists },
            { ".position.x", NativeArticleLink_PositionX },
            { ".position.y", NativeArticleLink_PositionY },
            { ".position.z", NativeArticleLink_PositionZ },
            { ".surface_normal.x", NativeArticleLink_NormalX },
            { ".surface_normal.y", NativeArticleLink_NormalY },
            { ".surface_normal.z", NativeArticleLink_NormalZ },
            { ".facing", NativeArticleLink_Facing },
        };
        unsigned i = 0;
        for (; i < sizeof(fields) / sizeof(fields[0]); ++i) {
            size_t length = strlen(fields[i].text);
            if (strncmp(source + consumed, fields[i].text, length)) continue;
            char next = source[consumed + length];
            if (isalnum((unsigned char)next) || next == '_' || next == '.') continue;
            *after = source + consumed + length;
            break;
        }
        if (i == sizeof(fields) / sizeof(fields[0])) return false;
        written = snprintf(output, capacity, "engine->item_link_value(gobj, %u, %u)", slot, fields[i].property);
    } else if ((consumed = 0, sscanf(source, "item.link_bone(%u, %u)%n", &slot, &bone, &consumed)) == 2 &&
               consumed && slot < 4 && bone < 100) {
        int channel = bone_channel(source + consumed, after);
        if (channel < 0) return false;
        written = snprintf(output, capacity, "engine->item_link_bone_value(gobj, %u, %u, %u)", slot, bone, channel);
    } else if ((consumed = 0, sscanf(source, "item.bone(%u)%n", &bone, &consumed)) == 1 &&
               consumed && bone < 100) {
        int channel = bone_channel(source + consumed, after);
        if (channel < 0) return false;
        written = snprintf(output, capacity, "engine->item_bone_value(gobj, %u, %u)", bone, channel);
    } else return false;
    return written >= 0 && (size_t)written < capacity;
}

bool nativeItemLinks_Action(const char* source, char* output, size_t capacity)
{
    unsigned slot;
    int consumed = 0;
    const char* host = NULL;
    if (sscanf(source, "item.unlink(%u)%n", &slot, &consumed) == 1 && consumed)
        host = "item_unlink";
    else if ((consumed = 0, sscanf(source, "item.copy_link_collision(%u)%n", &slot, &consumed)) == 1 && consumed)
        host = "item_copy_link_collision";
    if (host == NULL || slot >= 4 || source[consumed]) return false;
    int written = snprintf(output, capacity, "engine->%s(gobj, %u);", host, slot);
    return written >= 0 && (size_t)written < capacity;
}
