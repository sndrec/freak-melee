#include "native_hub_internal.h"
#include <melee/mn/native_menu_skin.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cstring>

namespace {
    struct Line { unsigned start, end, next; };
    unsigned character(const char* value, unsigned index)
    {
        ++index;
        while (value[index] && (value[index] & 0xC0) == 0x80) ++index;
        return index;
    }
    Line line(const char* value, unsigned start, float width, float size)
    {
        char text[2049]; unsigned end = start, space = start;
        while (value[end] && value[end] != '\n' && end - start < 2048) {
            unsigned next = character(value, end);
            if (next - start >= sizeof(text)) break;
            std::memcpy(text, value + start, next - start); text[next - start] = 0;
            if (nativeMenuFont_Width(NativeMenuFont_Notice, text, size) > width && end > start) {
                if (space > start) return { start, space, space + 1 };
                return { start, end, end };
            }
            if (value[end] == ' ') space = end;
            end = next;
        }
        return { start, end, end + 1 };
    }
    float prefix_width(const char* value, unsigned start, unsigned end, float size)
    {
        char text[2049];
        unsigned count = std::min(end - start, 2048U);
        std::memcpy(text, value + start, count); text[count] = 0;
        return nativeMenuFont_Width(NativeMenuFont_Notice, text, size);
    }
}
unsigned nativeHub_TextBlock(const char* value, float x, float y, float w, float h,
    float size, uint32_t color, unsigned first_line)
{
    const unsigned length = unsigned(strnlen(value, 2048));
    unsigned start = 0, index = 0;
    do {
        auto current = line(value, start, w, size);
        if (index >= first_line && (index - first_line + 1) * (size + 3) <= h + 3) {
            char text[2049]; unsigned count = current.end - current.start;
            std::memcpy(text, value + current.start, count); text[count] = 0;
            nativeMenuPaint_Text(text, x, y + (index - first_line) * (size + 3), w, size, color);
        }
        ++index; start = current.next;
    } while (start <= length);
    return index;
}
unsigned nativeHub_TextCaret(const char* value, unsigned caret, int direction, float width)
{
    const unsigned length = unsigned(strnlen(value, 2048));
    unsigned start = 0, index = 0, current_index = 0;
    Line current{};
    do {
        current = line(value, start, width, 10);
        if (caret < current.next || current.next > length) { current_index = index; break; }
        ++index; start = current.next;
    } while (start <= length);
    const float column = prefix_width(value, current.start, std::min(caret, current.end), 10);
    const unsigned wanted = direction < 0 ? (current_index ? current_index - 1 : 0) : current_index + 1;
    start = index = 0;
    do {
        current = line(value, start, width, 10);
        if (index == wanted || current.next > length) break;
        ++index; start = current.next;
    } while (start <= length);
    unsigned end = current.start;
    while (end < current.end && prefix_width(value, current.start, end, 10) < column) end = character(value, end);
    return end;
}
void nativeHub_Field(const char* id, const char* value, float x, float y,
    float w, float h, HubAction action, bool secret)
{
    const bool focused = hub.field == value, multiline = h >= 40;
    char masked[2049];
    if (secret) {
        size_t count = std::min(std::strlen(value), sizeof(masked) - 1);
        std::memset(masked, '*', count); masked[count] = 0; value = masked;
    }
    nativeMenuPaint_Round(x + 1, y + 1, w - 2, h - 2, 4, 0x050408F0);
    nativeMenuSkin_Frame(x, y, w, h, focused ? 0xFFE451FF : 0x87818DFF);
    if (focused && hub.select_all) nativeMenuPaint_Rect(x + 5, y + 4, w - 10, h - 8, 0x725730FF);
    unsigned first_line = 0, caret_line = 0, caret_start = 0, caret_end = 0;
    hub.caret = focused ? std::min(hub.caret, unsigned(std::strlen(value))) : hub.caret;
    if (multiline) {
        unsigned start = 0;
        for (;;) {
            auto current = line(value, start, w - 14, 10);
            if (!focused || hub.caret < current.next) { caret_start = current.start; caret_end = std::min(hub.caret, current.end); break; }
            start = current.next; ++caret_line;
        }
        const unsigned visible = std::max(1U, unsigned((h - 11) / 13));
        first_line = caret_line >= visible ? caret_line - visible + 1 : 0;
        nativeHub_TextBlock(value, x + 7, y + 7, w - 14, h - 14, 10, 0xF2ECEDFF, first_line);
    } else {
        nativeMenuPaint_Text(*value ? value : (action == HubAction::FieldChat || action == HubAction::FieldRoomChat) ? "Write a message..." :
            action == HubAction::FieldCode ? "Lobby code" : "", x + 7, y + 7, w - 14, 10, *value ? 0xF2ECEDFF : 0x81778EFF);
        caret_end = hub.caret;
    }
    if (focused && (SDL_GetTicks() / 500) % 2 == 0) {
        float full = multiline ? w - 14 : nativeMenuFont_Width(NativeMenuFont_Notice, value, 10);
        float scale = full > w - 14 ? (w - 14) / full : 1;
        float caret_x = prefix_width(value, caret_start, caret_end, 10) * scale;
        nativeMenuPaint_Rect(x + 7 + caret_x, y + 6 + (caret_line - first_line) * 13, .7f, 13, 0xD9B754FF);
    }
    nativeHub_Region(id, x, y, w, h, action);
}
