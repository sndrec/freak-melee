#include "native_css_chat.h"
#include "native_hub_internal.h"
#include <melee/gm/native_online_session.h>
#include <melee/mn/native_menu_skin.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    bool visible, focused, select_all;
    SDL_Window* window;
    char message[513], history[2049];
    unsigned caret, line_count;
    char room[13];
    uint64_t last_sequence = UINT64_MAX;

    unsigned next(unsigned index)
    {
        if (message[index]) ++index;
        while (message[index] && (message[index] & 0xC0) == 0x80) ++index;
        return index;
    }
    unsigned previous(unsigned index)
    {
        if (index) --index;
        while (index && (message[index] & 0xC0) == 0x80) --index;
        return index;
    }
    void erase(unsigned begin, unsigned end)
    {
        std::memmove(message + begin, message + end, std::strlen(message + end) + 1);
        caret = begin;
    }
    void insert(const char* text)
    {
        if (!text) return;
        if (select_all) { message[0] = 0; caret = 0; select_all = false; }
        const size_t length = std::strlen(message), size = std::strlen(text);
        if (length + size >= sizeof(message)) return;
        for (size_t i = 0; i < size; ++i)
            if (static_cast<unsigned char>(text[i]) < 32) return;
        std::memmove(message + caret + size, message + caret, length - caret + 1);
        std::memcpy(message + caret, text, size);
        caret += unsigned(size);
    }
}

void nativeCssChat_Focus(bool value)
{
    if (value == focused) return;
    focused = value && visible;
    if (focused) {
        int count;
        SDL_Window** windows = SDL_GetWindows(&count);
        window = windows && count ? windows[0] : nullptr;
        SDL_free(windows);
        if (window) SDL_StartTextInput(window);
        caret = unsigned(std::strlen(message));
    } else {
        if (window) SDL_StopTextInput(window);
        window = nullptr;
    }
    select_all = false;
}

void nativeCssChat_SetVisible(bool value)
{
    if (!value) nativeCssChat_Focus(false);
    if (value != visible) last_sequence = UINT64_MAX;
    const char* code = nativeOnlineSession_RoomCode();
    if (value && code && std::strcmp(code, room)) {
        std::snprintf(room, sizeof(room), "%s", code);
        message[0] = 0; caret = 0; last_sequence = UINT64_MAX;
    }
    visible = value;
}

bool nativeCssChat_Focused() { return focused; }

void nativeCssChat_Send()
{
    NativeRoomChat* chat = nativeOnlineSession_Chat();
    if (chat && *message && nativeRoomChat_Post(chat, message)) {
        message[0] = 0;
        caret = 0;
        select_all = false;
    }
}

bool nativeCssChat_Event(const SDL_Event* event)
{
    if (!visible || !focused) return false;
    if (event->type == SDL_EVENT_TEXT_INPUT) {
        insert(event->text.text);
        return true;
    }
    if (event->type != SDL_EVENT_KEY_DOWN) return false;
    const auto key = event->key.key;
    const bool control = event->key.mod & SDL_KMOD_CTRL;
    if (key == SDLK_ESCAPE) nativeCssChat_Focus(false);
    else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) nativeCssChat_Send();
    else if (control && key == SDLK_A) select_all = true;
    else if (control && key == SDLK_V) {
        char* text = SDL_GetClipboardText(); insert(text); SDL_free(text);
    } else if (key == SDLK_BACKSPACE || key == SDLK_DELETE) {
        if (select_all) { message[0] = 0; caret = 0; }
        else if (key == SDLK_BACKSPACE) erase(previous(caret), caret);
        else erase(caret, next(caret));
        select_all = false;
    } else if (key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_HOME || key == SDLK_END) {
        caret = key == SDLK_LEFT ? previous(caret) : key == SDLK_RIGHT ? next(caret)
              : key == SDLK_HOME ? 0 : unsigned(std::strlen(message));
        select_all = false;
    }
    return true;
}

void nativeCssChat_Draw()
{
    if (!visible) return;
    NativeRoomChat* chat = nativeOnlineSession_Chat();
    if (!chat) return;
    constexpr uint32_t gold = 0xE5C361FF;
    nativeMenuSkin_Panel(451, 275, 177, 140, 0xC2BEC7FF, 0x09080EDC);
    nativeMenuPaint_Text("Lobby chat", 457, 281, 97, 9, gold);
    if (last_sequence != chat->last_sequence) {
        last_sequence = chat->last_sequence;
        history[0] = 0;
        unsigned begin = chat->count > 8 ? chat->count - 8 : 0;
        for (;;) {
            size_t bytes = 0;
            for (unsigned i = begin; i < chat->count; ++i)
                bytes += std::strlen(chat->messages[i].name) + std::strlen(chat->messages[i].text) + 3;
            if (bytes < sizeof(history) || begin == chat->count) break;
            ++begin;
        }
        for (unsigned i = begin; i < chat->count; ++i) {
            const auto& entry = chat->messages[i];
            const size_t offset = std::strlen(history);
            std::snprintf(history + offset, sizeof(history) - offset, "%s: %s\n", entry.name, entry.text);
        }
        line_count = nativeHub_TextBlock(history, 457, 299, 165, 0, 7, 0xF2ECEDFF);
    }
    const bool delivery_error = *chat->failure || chat->delivery == NativeRoomChatDelivery_Unconfirmed;
    const unsigned shown = delivery_error ? 7 : 8;
    if (chat->count) nativeHub_TextBlock(history, 457, 299, 165, shown * 10 - 3, 7,
                                       0xF2ECEDFF, line_count > shown ? line_count - shown : 0);
    else nativeMenuPaint_Text(chat->ready ? "No messages yet." : "Connecting...", 457, 303, 165, 8);
    nativeMenuPaint_Round(455, 385, 129, 22, 3, 0x050408F0);
    nativeMenuSkin_Frame(455, 385, 129, 22, focused ? 0xFFE451FF : 0x87818DFF);
    if (focused && select_all) nativeMenuPaint_Rect(460, 388, 119, 16, 0x725730FF);
    unsigned start = 0;
    char prefix[513];
    for (;;) {
        std::memcpy(prefix, message + start, caret - start); prefix[caret - start] = 0;
        if (nativeMenuFont_Width(NativeMenuFont_Notice, prefix, 8) <= 115 || start == caret) break;
        start = next(start);
    }
    nativeMenuFont_Clip(460, 579, 0);
    const char* displayed = *message ? message + start : "Write a message...";
    nativeMenuPaint_Text(displayed, 460, 392,
                         nativeMenuFont_Width(NativeMenuFont_Notice, displayed, 8) + 1,
                         8, *message ? 0xF2ECEDFF : 0xA6A1AFFF);
    if (focused && (SDL_GetTicks() / 500) % 2 == 0) {
        float x = 460 + nativeMenuFont_Width(NativeMenuFont_Notice, prefix, 8);
        nativeMenuPaint_Line(x, 389, x, 403, 1, gold);
    }
    nativeMenuFont_Clip(0, 0, 0);
    if (delivery_error)
        nativeMenuPaint_Text(*chat->failure ? chat->failure : "Delivery unconfirmed", 457, 374, 165, 7, 0xF0A882FF);
}

void nativeCssChat_Inspect()
{
    const NativeRoomChat* chat = nativeOnlineSession_Chat();
    std::fprintf(stderr, "[play-menu] css-chat visible=%d focused=%d messages=%u draft=%s\n",
                 visible, focused, chat ? chat->count : 0, message);
}
