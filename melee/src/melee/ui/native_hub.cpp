#include "native_hub.h"
#include "native_hub_hand.h"
#include "native_hub_internal.h"
#include "native_hub_quick_stage.h"
#include "native_shell_workshop.h"
#include "native_shell_profiles.h"
#include "native_shell_rules.h"
#include "native_shell_css_original.h"
#include "native_shell.h"
#include "native_play_menu.h"
#include "native_shell_collection.h"
#include "native_shell_costumes.h"
#include <melee/gm/native_home.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/native_online_menu.h>
#include <melee/mn/native_css_original.h>
#include <melee/mn/native_css_interaction.h>
#include <melee/mn/native_menu_scene.h>
#include <dolphin/gx.h>
extern "C" {
#include <sysdolphin/baselib/controller.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/gm/gm_1A45.h>
}
#include <aurora/rmlui.hpp>
#include <RmlUi/Core.h>
#include <SDL3/SDL.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

HubState hub;
namespace {
    constexpr uint32_t gold = 0xD9B754FF, white = 0xF2ECEDFF;
    const char* page_names[] = { "Quick Play",  "Lobbies", "Chatroom",
                                 "Friends",     "Inbox",   "Workshop",
                                 "Settings",    "Profile", "Clone fighter",
                                 "Send fighter", "Lobby chat", "Stage select" };
    const char* descriptions[] = {
        "Find someone to play",      "Create a lobby or join your friends",
        "Talk with everyone online", "Find your friends",
        "Letters and invitations",   "Clone, tweak and import fighters",
        "Make yourself at home"
    };
    SDL_Window* text_window;
    bool wants_text;
    bool hand_cursor_hidden;
    bool mouse_pressed;

    void update_pointer_cursor()
    {
        bool css_pointer = false;
        for (unsigned port = 0; port < 4; ++port)
            css_pointer |= nativeCssInteraction_PointerPort(port);
        const bool hide = (((hub.visible || nativePlayMenu_PointerActive()) && nativeHubHand_Ready()) || css_pointer) &&
                          SDL_GetMouseFocus() && hub.pointer_x >= 0 &&
                          hub.pointer_x < 640 && hub.pointer_y >= 0 &&
                          hub.pointer_y < 480;
        if (hide == hand_cursor_hidden) {
            return;
        }
        if (hide) {
            SDL_HideCursor();
        } else {
            SDL_ShowCursor();
        }
        hand_cursor_hidden = hide;
    }

    void text_focus(char* field, unsigned capacity)
    {
        hub.field = field;
        hub.field_capacity = capacity;
        hub.caret = field ? unsigned(std::strlen(field)) : 0;
        hub.select_all = false;
        if (field && !wants_text) {
            int count = 0;
            SDL_Window** windows = SDL_GetWindows(&count);
            if (windows && count > 0) {
                text_window = windows[0];
            }
            SDL_free(windows);
            if (text_window) {
                SDL_StartTextInput(text_window);
            }
        } else if (!field && wants_text && text_window) {
            SDL_StopTextInput(text_window);
        }
        wants_text = field != nullptr;
    }
    void show()
    {
        hub.dismissed = false;
        hub.visible = nativeHome_IsSelection() || nativeHome_IsPractice();
        if (hub.visible) nativeMenuScene_Load();
        if (hub.visible && !nativeHubHand_Ready()) nativeHubHand_Load(nullptr);
        hub.region_count = 0;
        text_focus(nullptr, 0);
        if (auto* context = aurora::rmlui::get_context()) {
            if (auto* focused = context->GetFocusElement()) {
                focused->Blur();
            }
        }
    }
    bool save_profile()
    {
        if (!*hub.name || strpbrk(hub.name, "\r\n\t")) {
            return false;
        }
        char root_buffer[512], path[560], temporary[568];
        const char* root = nativeHubCatalog_Root(root_buffer);
        if (!root || !SDL_CreateDirectory(root)) {
            return false;
        }
        std::snprintf(path, sizeof(path), "%s/hub-profile.txt", root);
        std::snprintf(temporary, sizeof(temporary), "%s.tmp", path);
        FILE* file = std::fopen(temporary, "wb");
        if (!file) {
            return false;
        }
        bool ok = std::fprintf(file, "%s\n%s\n", hub.name, hub.bio) > 0;
        ok = std::fclose(file) == 0 && ok;
        ok = ok &&
             MoveFileExA(temporary, path,
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        if (ok) {
            std::snprintf(hub.public_name, sizeof(hub.public_name), "%s",
                          hub.name);
        }
        return ok;
    }
    void load_profile()
    {
        char root_buffer[512], path[560];
        const char* root = nativeHubCatalog_Root(root_buffer);
        if (!root) {
            return;
        }
        std::snprintf(path, sizeof(path), "%s/hub-profile.txt", root);
        FILE* file = std::fopen(path, "rb");
        if (!file) {
            return;
        }
        char name[35]{}, bio[257]{};
        if (std::fgets(name, sizeof(name), file) && std::strchr(name, '\n')) {
            name[std::strcspn(name, "\r\n")] = 0;
            if (*name && std::strlen(name) < sizeof(hub.name)) {
                std::snprintf(hub.name, sizeof(hub.name), "%s", name);
            }
            if (std::fgets(bio, sizeof(bio), file)) {
                bio[std::strcspn(bio, "\r\n")] = 0;
                std::snprintf(hub.bio, sizeof(hub.bio), "%s", bio);
            }
        }
        std::fclose(file);
    }
    void reload_catalog()
    {
        char selected[512]{};
        NativeContentId id{};
        if (hub.selected >= 0 &&
            unsigned(hub.selected) < nativeHubFighterCount)
        {
            id = nativeHubFighters[hub.selected].id;
            std::snprintf(selected, sizeof(selected), "%s",
                          nativeHubFighters[hub.selected].draft);
        }
        if (!nativeHubCatalog_Load()) {
            nativeHub_Notice(
                "Some fighters could not be read. Your files have been kept.");
        }
        hub.selected = nativeHubFighterCount ? 0 : -1;
        for (unsigned i = 0; i < nativeHubFighterCount; ++i) {
            if ((*selected &&
                 !std::strcmp(selected, nativeHubFighters[i].draft)) ||
                (nativeHubFighters[i].installed &&
                 !std::memcmp(&id, &nativeHubFighters[i].id, sizeof(id))))
            {
                hub.selected = int(i);
            }
        }
        hub.first_fighter = std::min(
            hub.first_fighter,
            int((nativeHubFighterCount ? nativeHubFighterCount - 1 : 0) / 14 *
                14));
    }
    void open_editor(const char* path)
    {
        text_focus(nullptr, 0);
        nativeShellWorkshop_Open(path, NativeWorkshopOrigin::Hub);
        hub.returning_from_tool = nativeShellWorkshop_IsOpen();
        hub.visible = !hub.returning_from_tool;
    }
    void queue(HubAction action, int value = 0, const char* peer = nullptr,
               const char* name = nullptr)
    {
        if (hub.pending_action || nativeMenuScene_Active()) {
            return;
        }
        hub.pending_action = unsigned(action);
        hub.pending_value = value;
        std::snprintf(hub.pending_peer, sizeof(hub.pending_peer), "%s",
                      peer ? peer : "");
        std::snprintf(hub.pending_name, sizeof(hub.pending_name), "%s",
                      name ? name : "");
    }
    void click(unsigned port = 255)
    {
        for (unsigned i = hub.region_count; i > 0; --i) {
            const auto& r = hub.regions[i - 1];
            if (nativeHub_Hover(r.x, r.y, r.w, r.h)) {
                if (r.enabled) {
                    int value = r.value;
                    if (r.action == HubAction::PickStage) {
                        const unsigned seat = port < 4 ? port : hub.stage_seat;
                        value += int(seat*256);
                    } else if ((r.action == HubAction::ConfirmStage ||
                                r.action == HubAction::WithdrawStage) &&
                               port < 4 && unsigned(value)/256 != port) return;
                    queue(r.action, value, r.peer, r.peer_name);
                }
                return;
            }
        }
        text_focus(nullptr, 0);
    }
    unsigned previous_character(const char* text, unsigned caret)
    {
        if (!caret) {
            return 0;
        }
        --caret;
        while (caret && (text[caret] & 0xC0) == 0x80) {
            --caret;
        }
        return caret;
    }
    unsigned next_character(const char* text, unsigned caret)
    {
        unsigned length = unsigned(std::strlen(text));
        if (caret == length) {
            return length;
        }
        ++caret;
        while (caret < length && (text[caret] & 0xC0) == 0x80) {
            ++caret;
        }
        return caret;
    }
    void key(SDL_Keycode key, SDL_Keymod modifiers)
    {
        if (key == SDLK_ESCAPE) {
            if (hub.field) {
                text_focus(nullptr, 0);
                return;
            }
            queue(HubAction::Back);
            return;
        }
        if (!hub.field) {
            if (key == SDLK_RETURN || key == SDLK_SPACE) {
                click();
            }
            return;
        }
        char* field = hub.field;
        unsigned length = unsigned(std::strlen(field));
        hub.caret = std::min(hub.caret, length);
        if (key == SDLK_RETURN) {
            if (field == hub.chat) {
                queue(HubAction::SendChat);
            } else if (field == hub.room_chat) {
                queue(HubAction::SendRoomChat);
            } else if (field == hub.code) {
                queue(HubAction::Join);
            } else if (field == hub.title) {
                queue(hub.rename ? HubAction::ApplyRename
                                 : HubAction::CreateClone);
            } else {
                text_focus(nullptr, 0);
                queue(HubAction::SaveProfile);
            }
            return;
        }
        if (key == SDLK_LEFT) {
            hub.caret = previous_character(field, hub.caret);
        }
        if (key == SDLK_RIGHT) {
            hub.caret = next_character(field, hub.caret);
        }
        if (key == SDLK_HOME) {
            hub.caret = 0;
        }
        if (key == SDLK_END) {
            hub.caret = length;
        }
        if (key == SDLK_A && (modifiers & SDL_KMOD_CTRL)) {
            hub.select_all = true;
            return;
        }
        if ((key == SDLK_C || key == SDLK_X) && (modifiers & SDL_KMOD_CTRL) &&
            hub.select_all)
        {
            SDL_SetClipboardText(field);
            if (key == SDLK_X) {
                field[0] = 0;
                hub.caret = 0;
                hub.select_all = false;
            }
            return;
        }
        if ((key == SDLK_BACKSPACE || key == SDLK_DELETE) && hub.select_all) {
            field[0] = 0;
            hub.caret = 0;
            hub.select_all = false;
            return;
        }
        if (key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_HOME ||
            key == SDLK_END)
        {
            hub.select_all = false;
        }
        if (key == SDLK_BACKSPACE && hub.caret) {
            unsigned start = previous_character(field, hub.caret);
            std::memmove(field + start, field + hub.caret,
                         length - hub.caret + 1);
            hub.caret = start;
        }
        if (key == SDLK_DELETE && hub.caret < length) {
            unsigned end = next_character(field, hub.caret);
            std::memmove(field + hub.caret, field + end, length - end + 1);
        }
        if (key == SDLK_V && (modifiers & SDL_KMOD_CTRL)) {
            char* text = SDL_GetClipboardText();
            nativeHub_EnterText(text);
            SDL_free(text);
        }
    }
}
bool nativeHub_Hover(float x, float y, float w, float h)
{
    return !hub.passive_page && hub.pointer_x >= x && hub.pointer_x < x + w && hub.pointer_y >= y &&
           hub.pointer_y < y + h;
}
void nativeHub_Region(const char* id, float x, float y, float w, float h,
                      HubAction action, int value, bool enabled,
                      const char* peer, const char* name)
{
    if (hub.passive_page || nativeMenuScene_Active()) return;
    if (hub.region_count == sizeof(hub.regions) / sizeof(hub.regions[0])) {
        return;
    }
    HubRegion& r = hub.regions[hub.region_count++];
    r = { x, y, w, h, action, value, {}, {}, {}, enabled };
    std::snprintf(r.id, sizeof(r.id), "%s", id);
    std::snprintf(r.peer, sizeof(r.peer), "%s", peer ? peer : "");
    std::snprintf(r.peer_name, sizeof(r.peer_name), "%s", name ? name : "");
}
void nativeHub_Panel(float x, float y, float w, float h)
{
    nativeMenuPaint_Round(x + 1, y + 1, w - 2, h - 2, 5, 0x09080EDC);
    nativeMenuScene_DrawFrame(x, y, w, h, 0xC2BEC7FF);
}
void nativeHub_Button(const char* id, const char* label, float x, float y,
                      float w, float h, HubAction action, int value,
                      bool selected, bool enabled, const char* peer,
                      const char* name)
{
    bool hover = nativeHub_Hover(x, y, w, h) && enabled;
    bool active = enabled && (selected || hover);
    nativeMenuPaint_Gradient(x + 2, y + 2, w - 4, h - 4,
        active ? 0xFFE451FF : enabled ? 0x302A25F4 : 0x17151AF0,
        active ? 0xC1971FFF : enabled ? 0x0A090BF4 : 0x0A090BF0);
    nativeMenuScene_DrawFrame(x, y, w, h,
        active ? 0xFFF5A5FF : enabled ? 0xBDAA6EFF : 0x5B5660FF);
    nativeMenuPaint_Text(label, x + 7, y + (h - std::min(13.0f, h * .4f)) * .5f,
                        w - 14, std::min(13.0f, h * .4f),
                        active ? 0x17121CFF
                        : enabled         ? white
                                          : 0x81788AFF,
                        NativeMenuFont_MenuOption, true, !active);
    nativeHub_Region(id, x, y, w, h, action, value, enabled, peer, name);
}
void nativeHub_Field(const char* id, const char* value, float x, float y,
                     float w, float h, HubAction action)
{
    bool focused = hub.field == value;
    nativeMenuPaint_Round(x + 1, y + 1, w - 2, h - 2, 4, 0x050408F0);
    nativeMenuScene_DrawFrame(x, y, w, h, focused ? 0xFFE451FF : 0x87818DFF);
    if (focused && hub.select_all) {
        nativeMenuPaint_Rect(x + 5, y + 4, w - 10, 17, 0x725730FF);
    }
    nativeMenuPaint_Text(*value                           ? value
                        : (action == HubAction::FieldChat || action == HubAction::FieldRoomChat) ? "Write a message..."
                        : action == HubAction::FieldCode ? "Lobby code"
                                                         : "",
                        x + 7, y + 7, w - 14, 10, *value ? white : 0x81778EFF);
    if (focused && (SDL_GetTicks() / 500) % 2 == 0) {
        char prefix[513];
        hub.caret = std::min(hub.caret, unsigned(std::strlen(value)));
        std::memcpy(prefix, value, hub.caret);
        prefix[hub.caret] = 0;
        float full = nativeMenuFont_Width(NativeMenuFont_Notice, value, 10);
        float scale = full > w - 14 ? (w - 14) / full : 1;
        float caret_x =
            nativeMenuFont_Width(NativeMenuFont_Notice, prefix, 10) * scale;
        nativeMenuPaint_Rect(x + 7 + caret_x, y + 6, .7f, 13, gold);
    }
    nativeHub_Region(id, x, y, w, h, action);
}
void nativeHub_ChangePage(NativeHubPage page, bool backwards)
{
    if (hub.page != page) {
        hub.motion_from = hub.page;
        BOOL animations = TRUE;
        SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0);
        if (hub.visible && SDL_GetHintBoolean("FREAK_MELEE_MENU_MOTION", animations))
            nativeMenuScene_Start(backwards);
        hub.previous = hub.page;
        hub.page = page;
    }
    text_focus(nullptr, 0);
    hub.region_count = 0;
    hub.rename = false;
}
extern "C" void nativeHub_Notice(const char* text)
{
    std::snprintf(hub.notice, sizeof(hub.notice), "%s", text ? text : "");
    hub.notice_until = SDL_GetTicks() + 8000;
}
extern "C" const char* nativeHub_LocalName(void)
{
    return hub.public_name;
}
extern "C" const char* nativeHub_NoticeText(void)
{
    return SDL_GetTicks() < hub.notice_until ? hub.notice : "";
}
extern "C" bool nativeHub_OpenFighterDelivery(const NativeContentId* fighter,
                                              const char* recipient)
{
    if (!fighter || !nativeHome_IsSelection() ||
        !nativeOnlineSession_Presence())
    {
        return false;
    }
    reload_catalog();
    const NativeHubFighter* found = nullptr;
    for (unsigned i = 0; i < nativeHubFighterCount; ++i) {
        if (nativeHubFighters[i].installed &&
            !std::memcmp(&nativeHubFighters[i].id, fighter, sizeof(*fighter)))
        {
            found = &nativeHubFighters[i];
            break;
        }
    }
    if (!found) {
        return false;
    }
    const auto* presence = nativeOnlineSession_Presence();
    const NativeSignalingParticipant* target = nullptr;
    if (recipient && *recipient) {
        for (unsigned i = 0; i < presence->participant_count; ++i) {
            const auto* candidate = &presence->participants[i];
            if (!std::strcmp(candidate->peer_id, recipient) &&
                std::strcmp(recipient, nativeOnlineSession_LocalPeerId()))
            {
                target = candidate;
                break;
            }
        }
        if (!target) {
            return false;
        }
    }
    hub.gift_id = *fighter;
    std::snprintf(hub.gift_title, sizeof(hub.gift_title), "%s", found->title);
    std::snprintf(hub.recipient, sizeof(hub.recipient), "%s",
                  target ? target->peer_id : "");
    std::snprintf(hub.recipient_name, sizeof(hub.recipient_name), "%s",
                  target ? target->name : "");
    show();
    hub.send_from_css = true;
    unsigned recipient_row = 0;
    for (unsigned i = 0; target && &presence->participants[i] != target; ++i) {
        if (std::strcmp(presence->participants[i].peer_id,
                        nativeOnlineSession_LocalPeerId()))
        {
            ++recipient_row;
        }
    }
    hub.recipient_scroll = int(recipient_row / 7 * 7);
    nativeHub_ChangePage(NativeHubPage_Send);
    return true;
}
void nativeHub_Activate(HubAction action, int value)
{
    lbAudioAx_80024030(action == HubAction::Back ? 0 : 1);
    if (nativeHubQuickStage_Activate(action,value)) return;
    if (action >= HubAction::QuickPlay && action <= HubAction::Settings) {
        nativeHub_ChangePage(static_cast<NativeHubPage>(
            unsigned(action) - unsigned(HubAction::QuickPlay)));
        return;
    }
    char path[512]{};
    switch (action) {
    case HubAction::Profile:
        hub.profile_self = true;
        nativeHub_ChangePage(NativeHubPage_Profile);
        break;
    case HubAction::Back:
        if (hub.rename) {
            hub.rename = false;
            text_focus(nullptr, 0);
        } else if (hub.page == NativeHubPage_Send && hub.send_from_css) {
            hub.send_from_css = false;
            hub.dismissed = true;
            hub.visible = false;
        } else if (hub.page == NativeHubPage_QuickStage) {
            hub.dismissed = true;
            hub.visible = false;
        } else if (hub.page == NativeHubPage_RoomChat) {
            nativeHub_ChangePage(NativeHubPage_Lobbies, true);
        } else if (hub.opened_from_play && hub.page <= NativeHubPage_Settings) {
            hub.dismissed = true;
            hub.visible = false;
            text_focus(nullptr, 0);
        } else {
            nativeHub_ChangePage(hub.page == NativeHubPage_Profile ||
                                         hub.page == NativeHubPage_Clone ||
                                         hub.page == NativeHubPage_Send
                                     ? hub.previous
                                     : NativeHubPage_Workshop, true);
        }
        break;
    case HubAction::Close:
        hub.dismissed = true;
        hub.visible = false;
        text_focus(nullptr, 0);
        break;
    case HubAction::Fighter:
        hub.selected = value;
        break;
    case HubAction::Previous:
        hub.first_fighter = std::max(0, hub.first_fighter - 14);
        break;
    case HubAction::Next:
        hub.first_fighter = std::min(int(nativeHubFighterCount / 14 * 14),
                                     hub.first_fighter + 14);
        break;
    case HubAction::NewClone:
        nativeHub_ChangePage(NativeHubPage_Clone);
        hub.template_kind = Ft_Kind_Mario;
        std::snprintf(hub.title, sizeof(hub.title), "Mario");
        break;
    case HubAction::StockTemplate:
        text_focus(nullptr, 0);
        hub.template_kind = value;
        std::snprintf(hub.title, sizeof(hub.title), "%s",
                      nativeFighterProject_StockBaseName(
                          static_cast<FighterKind>(value)));
        break;
    case HubAction::CreateClone:
        text_focus(nullptr, 0);
        if (nativeHubCatalog_CreateStock(
                static_cast<FighterKind>(hub.template_kind), hub.title, path))
        {
            reload_catalog();
            nativeHub_ChangePage(NativeHubPage_Workshop);
            for (unsigned i = 0; i < nativeHubFighterCount; ++i) {
                if (!std::strcmp(path, nativeHubFighters[i].draft)) {
                    hub.selected = int(i);
                }
            }
            nativeHub_Notice("Fighter added to your roster. Edit it or head "
                             "into practice.");
        } else {
            reload_catalog();
            nativeHub_Notice("Could not finish cloning. Any completed draft "
                             "remains in the Workshop.");
        }
        break;
    case HubAction::Edit:
        if (nativeHubCatalog_MakeEditable(unsigned(hub.selected), path)) {
            open_editor(path);
        } else {
            nativeHub_Notice(
                "Could not open an editable copy of this fighter.");
        }
        break;
    case HubAction::OpenImport:
        open_editor(nullptr);
        break;
    case HubAction::CopyFighter:
        if (hub.selected >= 0) {
            char name[64];
            std::snprintf(name, sizeof(name), "%.55s copy",
                          nativeHubFighters[hub.selected].title);
            if (nativeHubCatalog_Copy(unsigned(hub.selected), name, path)) {
                reload_catalog();
                nativeHub_Notice("Independent fighter copy created.");
            } else {
                nativeHub_Notice("Could not copy this fighter.");
            }
        }
        break;
    case HubAction::RenameFighter:
        if (hub.selected >= 0) {
            hub.rename = true;
            std::snprintf(hub.title, sizeof(hub.title), "%s",
                          nativeHubFighters[hub.selected].title);
            text_focus(hub.title, sizeof(hub.title));
        }
        break;
    case HubAction::ApplyRename:
        text_focus(nullptr, 0);
        if (nativeHubCatalog_Rename(unsigned(hub.selected), hub.title)) {
            hub.rename = false;
            reload_catalog();
            nativeHub_Notice("Fighter renamed.");
        } else {
            nativeHub_Notice("Could not rename this fighter.");
        }
        break;
    case HubAction::Cancel:
        hub.rename = false;
        text_focus(nullptr, 0);
        break;
    case HubAction::Practice:
        hub.dismissed = true;
        hub.visible = false;
        text_focus(nullptr, 0);
        break;
    case HubAction::EditLayout:
        nativeShellCssOriginal_Open();
        hub.returning_from_tool = nativeShellCssOriginal_IsEditing();
        hub.visible = !hub.returning_from_tool;
        break;
    case HubAction::Controls:
        nativeShellProfiles_Open();
        hub.returning_from_tool = nativeShellProfiles_IsOpen();
        hub.visible = !hub.returning_from_tool;
        break;
    case HubAction::Selection:
        if (nativeHome_ReturnToSelection()) {
            hub.dismissed = true;
            hub.visible = false;
        }
        break;
    case HubAction::Rules:
        nativeShellRules_Open();
        hub.returning_from_tool = nativeShellRules_IsOpen();
        hub.visible = !hub.returning_from_tool;
        break;
    case HubAction::Costumes:
        nativeShellCostumes_Open();
        hub.returning_from_tool = nativeShellCostumes_IsOpen();
        hub.visible = !hub.returning_from_tool;
        break;
    case HubAction::Packages:
        nativeShellCollection_Open();
        hub.returning_from_tool = nativeShellCollection_IsOpen();
        hub.visible = !hub.returning_from_tool;
        break;
    case HubAction::Rotation: {
        const auto* presence = nativeOnlineSession_Presence();
        if (presence && !nativeOnlineSession_SetRotation(static_cast<NativeRoomRotationPolicy>(
            (presence->rotation_policy+1)%(NativeRoomRotation_WinnerOut+1))))
            nativeHub_Notice("Only the host can change rotation.");
        break;
    }
    case HubAction::FieldName:
        text_focus(hub.name, sizeof(hub.name));
        break;
    case HubAction::FieldBio:
        text_focus(hub.bio, sizeof(hub.bio));
        break;
    case HubAction::FieldChat:
        text_focus(hub.chat, sizeof(hub.chat));
        break;
    case HubAction::FieldRoomChat:
        text_focus(hub.room_chat, sizeof(hub.room_chat));
        break;
    case HubAction::FieldCode:
        text_focus(hub.code, sizeof(hub.code));
        break;
    case HubAction::FieldTitle:
        text_focus(hub.title, sizeof(hub.title));
        break;
    case HubAction::SaveProfile:
        text_focus(nullptr, 0);
        nativeHub_Notice(save_profile() ? "Profile saved on this PC."
                                        : "Could not save the profile.");
        break;
    case HubAction::SendChat:
        if (nativeChat_Send(hub.public_name, hub.chat)) {
            hub.chat[0] = 0;
            hub.caret = 0;
        } else {
            nativeHub_Notice(
                "Chat is unavailable. Your message is still here.");
        }
        break;
    case HubAction::RoomChat:
        nativeHub_ChangePage(NativeHubPage_RoomChat);
        break;
    case HubAction::SendRoomChat:
        if (nativeRoomChat_Post(nativeOnlineSession_Chat(), hub.room_chat)) {
            hub.room_chat[0] = 0;
            hub.caret = 0;
            hub.room_chat_scroll = 0;
        } else nativeHub_Notice("Could not send yet. Your message is still here.");
        break;
    case HubAction::RetryRoomChat:
        if (!nativeRoomChat_Retry(nativeOnlineSession_Chat()))
            nativeHub_Notice("Chat is reconnecting. Try again shortly.");
        break;
    case HubAction::LobbyScroll: {
        const auto* presence = nativeOnlineSession_Presence();
        int last = presence && presence->participant_count ? (presence->participant_count - 1) / 6 * 6 : 0;
        hub.lobby_scroll = std::clamp(hub.lobby_scroll + value, 0, last);
        break;
    }
    case HubAction::LocalPlayers:
        hub.local_players = std::clamp(value, 1, 4);
        break;
    case HubAction::Create:
    case HubAction::Join:
    case HubAction::Leave:
    case HubAction::StartMatch: {
        if (action == HubAction::StartMatch && nativeOnlineSession_Quick()) {
            const auto* quick = nativeOnlineSession_Quick();
            if (!nativeOnlineSession_SearchQuick(quick->state == NativeQuickState_Idle))
                nativeHub_Notice("Could not change the Quick Play search.");
            break;
        }
        auto intent =
            action == HubAction::Create  ? NativeOnlineMenuAction_Create
            : action == HubAction::Join  ? NativeOnlineMenuAction_Join
            : action == HubAction::Leave ? NativeOnlineMenuAction_Leave
                                         : NativeOnlineMenuAction_StartMatch;
        if (action == HubAction::Join) {
            size_t size = std::strlen(hub.code);
            bool valid = size >= 6 && size <= 12;
            for (size_t i = 0; i < size; ++i) {
                valid = valid && ((hub.code[i] >= 'A' && hub.code[i] <= 'Z') ||
                                  (hub.code[i] >= 'a' && hub.code[i] <= 'z') ||
                                  (hub.code[i] >= '0' && hub.code[i] <= '9'));
            }
            if (!valid) {
                nativeHub_Notice(
                    "Enter a lobby code using 6 to 12 letters or numbers.");
                break;
            }
        }
        if (action == HubAction::Create || action == HubAction::Join) {
            if (nativeOnlineSession_State() == NativeLobbySessionState_Failed)
            {
                nativeOnlineSession_Stop();
            }
        }
        text_focus(nullptr, 0);
        if (!nativeOnlineMenu_Request(intent, hub.public_name, hub.code,
                                      uint8_t(hub.local_players)))
        {
            nativeHub_Notice("The lobby is busy. Try again shortly.");
        }
        break;
    }
    case HubAction::CopyCode:
        if (nativeOnlineSession_RoomCode() &&
            SDL_SetClipboardText(nativeOnlineSession_RoomCode()))
        {
            nativeHub_Notice("Lobby code copied.");
        }
        break;
    case HubAction::Seat: {
        const auto* p = nativeOnlineSession_Presence();
        unsigned seat = unsigned(value);
        const NativeSignalingParticipant* person_ptr = nullptr;
        for (unsigned i = 0; p && i < p->participant_count; ++i) {
            if (!std::strcmp(p->participants[i].peer_id, hub.pending_peer)) {
                person_ptr = &p->participants[i];
            }
        }
        if (!person_ptr || seat >= person_ptr->local_players) {
            break;
        }
        const auto& person = *person_ptr;
        auto next = (person.active_mask & (1U << seat))
                        ? NativeRoomSeatState_Waiting
                    : (person.waiting_mask & (1U << seat))
                        ? NativeRoomSeatState_Spectating
                        : NativeRoomSeatState_Fighting;
        if (!nativeOnlineSession_ChooseSeat(person.peer_id, uint8_t(seat),
                                            next))
        {
            nativeHub_Notice(
                "Only this player or the host can change this seat.");
        } else {
            nativeHome_CancelStartMatch();
        }
        break;
    }
    case HubAction::ShowProfile: {
        if (*hub.pending_peer) {
            std::snprintf(hub.viewed_name, sizeof(hub.viewed_name), "%s",
                          hub.pending_name);
            std::snprintf(hub.viewed_peer, sizeof(hub.viewed_peer), "%s",
                          hub.pending_peer);
            const char* local = nativeOnlineSession_LocalPeerId();
            hub.profile_self =
                (local && !std::strcmp(hub.pending_peer, local)) ||
                !std::strcmp(hub.pending_peer, nativeChat_LocalPeerId());
            nativeHub_ChangePage(NativeHubPage_Profile);
        }
        break;
    }
    case HubAction::SendFighter:
        if (hub.selected < 0 ||
            unsigned(hub.selected) >= nativeHubFighterCount ||
            !nativeHubFighters[hub.selected].installed)
        {
            break;
        }
        hub.gift_id = nativeHubFighters[hub.selected].id;
        hub.send_from_css = false;
        hub.recipient_scroll = 0;
        std::snprintf(hub.gift_title, sizeof(hub.gift_title), "%s",
                      nativeHubFighters[hub.selected].title);
        hub.recipient[0] = 0;
        nativeHub_ChangePage(NativeHubPage_Send);
        break;
    case HubAction::RecipientScroll:
        hub.recipient_scroll = std::clamp(hub.recipient_scroll + value, 0, 28);
        break;
    case HubAction::Recipient: {
        const auto* p = nativeOnlineSession_Presence();
        for (unsigned i = 0; p && i < p->participant_count; ++i) {
            if (std::strcmp(p->participants[i].peer_id, hub.pending_peer)) {
                continue;
            }
            std::snprintf(hub.recipient, sizeof(hub.recipient), "%s",
                          hub.pending_peer);
            std::snprintf(hub.recipient_name, sizeof(hub.recipient_name), "%s",
                          hub.pending_name);
            break;
        }
        break;
    }
    case HubAction::ConfirmSend:
        if (nativeOnlineSession_OfferGift(hub.recipient, &hub.gift_id)) {
            if (hub.send_from_css) {
                hub.send_from_css = false;
                hub.dismissed = true;
                hub.visible = false;
            } else {
                nativeHub_ChangePage(NativeHubPage_Workshop);
            }
            nativeHub_Notice("Fighter offered.");
        } else {
            nativeHub_Notice("Could not send yet. Check the recipient's "
                             "connection and try again.");
        }
        break;
    case HubAction::AcceptGift:
        nativeHub_Notice(nativeOnlineSession_AcceptGift()
                             ? "Receiving the fighter..."
                             : "Could not accept this delivery.");
        break;
    case HubAction::DeclineGift:
        nativeOnlineSession_DeclineGift();
        break;
    case HubAction::Singles:
    case HubAction::Doubles: {
        unsigned mode = action == HubAction::Singles ? NativeQuickMode_Singles
                                                     : NativeQuickMode_Doubles;
        unsigned players = std::clamp(unsigned(hub.local_players), 1U, mode);
        if (nativeHome_IsPractice() && !nativeHome_ReturnToSelection()) break;
        if (!nativeOnlineSession_EnterQuick(mode, hub.public_name, players)) {
            nativeHub_Notice("Could not open Quick Play.");
            break;
        }
        nativeCssInteraction_SetLocalPlayers(players);
        nativeHub_Notice("");
        hub.dismissed = true;
        hub.visible = false;
        text_focus(nullptr, 0);
        break;
    }
    case HubAction::PeopleScroll:
        if (hub.page == NativeHubPage_RoomChat) {
            const auto* presence = nativeOnlineSession_Presence();
            hub.room_people_scroll = std::clamp(hub.room_people_scroll + value, 0,
                std::max(0, presence ? int(presence->participant_count) - 12 : 0));
            break;
        }
        hub.people_scroll =
            std::clamp(hub.people_scroll + value, 0,
                       std::max(0, int(nativeChat_View()->player_count) - 12));
        break;
    case HubAction::ChatScroll: {
        int& scroll = hub.page == NativeHubPage_RoomChat
                          ? hub.room_chat_scroll : hub.chat_scroll;
        scroll = std::clamp(scroll + value, 0, 64 * 34);
        break;
    }
    case HubAction::WriteLetter:
    case HubAction::AddFriend:
    case HubAction::Invite:
        nativeHub_Notice(
            "This action needs account sign-in and the inbox service.");
        break;
    default:
        break;
    }
}
extern "C" bool nativeHub_IsVisible(void)
{
    return hub.visible;
}
extern "C" bool nativeHub_ConsumesGameInput(void)
{
    return hub.visible || hub.consumes_input;
}
extern "C" void nativeHub_Tick(void)
{
    if (nativeMatchLaunch_IsDirect()) {
        return;
    }
    hub.consumes_input = hub.visible;
    const bool selection = nativeHome_IsSelection();
    const bool practice = nativeHome_IsPractice();
    bool tool = nativeShellWorkshop_IsOpen() || nativeShellProfiles_IsOpen() ||
                nativeShellRules_IsOpen() ||
                nativeShellCssOriginal_IsEditing() || nativeShellCollection_IsOpen() ||
                nativeShellCostumes_IsOpen();
    hub.consumes_input |= tool &&
        !(nativeShellWorkshop_IsOpen() && nativeShellWorkshop_IsTesting());
    if (!hub.initialized && selection) {
        load_profile();
        std::snprintf(hub.public_name, sizeof(hub.public_name), "%s",
                      hub.name);
        reload_catalog();
        // An existing saved layout or installed collection always wins. The
        // empty roster is only the first-run default for an empty
        // installation.
        if (!nativeHubFighterCount) {
            char path[512];
            NativeCssLayout layout{};
            bool missing = false;
            if (nativeCssLayout_DefaultPath(path, sizeof(path)) &&
                !nativeCssLayout_LoadFile(&layout, path, &missing) && missing)
            {
                if (nativeCssLayout_SaveFile(&layout, path)) {
                    nativeCssOriginal_ReloadInstalledFighters();
                }
            }
        }
        hub.initialized = true;
        show();
    }
    if (hub.returning_from_tool && !tool && selection) {
        hub.returning_from_tool = false;
        reload_catalog();
        show();
    }
    if (selection && !hub.selection_before && hub.initialized && !tool) {
        hub.opened_from_play = false;
        if (nativeOnlineSession_Quick()) hub.dismissed = true;
        else {
            hub.dismissed = false;
            show();
        }
    }
    hub.selection_before = selection;
    if ((selection || practice) && !tool && nativeHubQuickStage_OpenRound()) {
        show();
        nativeHub_ChangePage(NativeHubPage_QuickStage);
        hub.opened_from_play = true;
    }
    if (hub.page == NativeHubPage_QuickStage) {
        const auto* quick = nativeOnlineSession_Quick();
        if (!quick || quick->state != NativeQuickState_Assigned || quick->phase != 3) {
            nativeHub_ChangePage(NativeHubPage_QuickPlay);
            hub.dismissed = true;
            hub.visible = false;
        }
    }
    hub.visible = hub.initialized && (selection || practice) && !tool && !hub.dismissed;
    nativeCssInteraction_EnablePointer(selection && !tool && !hub.visible);
    if (hub.initialized) {
        nativeChat_Identify(hub.public_name);
    }
    static bool installed_gift_handled;
    NativeNetplayGiftStatus gift{};
    if (!nativeOnlineSession_GiftStatus(true, &gift) ||
        gift.state != NativeNetplayGift_Installed)
    {
        installed_gift_handled = false;
    } else if (!installed_gift_handled) {
        installed_gift_handled = true;
        bool placed = gift.kind == NativeNetplayGift_Fighter &&
                      nativeCssOriginal_AddInstalled(&gift.id);
        nativeShellCssOriginal_RefreshInstalledFighters();
        reload_catalog();
        nativeHub_Notice(
            gift.kind == NativeNetplayGift_Costume ? "Costume received."
            : placed ? "Fighter received and added to your roster."
                     : "Fighter received. Use Edit CSS to add its portrait.");
    }
    if (selection && !tool && hub.dismissed) {
        bool held = false;
        for (unsigned port = 0; port < 4; ++port) {
            held |= !HSD_PadCopyStatus[port].err &&
                    (HSD_PadCopyStatus[port].button & PAD_BUTTON_B) != 0;
        }
        hub.back_frames = held ? hub.back_frames + 1 : 0;
        if (hub.back_frames >= 30) {
            hub.back_frames = 0;
            show();
        }
    } else {
        hub.back_frames = 0;
    }
    if (practice && !tool && !hub.visible && gm_GetDbPauseFlag(1)) {
        for (unsigned port = 0; port < 4; ++port) {
            if (!HSD_PadCopyStatus[port].err &&
                (HSD_PadCopyStatus[port].trigger & PAD_TRIGGER_Z)) {
                nativeHub_Open(NativeHubPage_Lobbies);
                break;
            }
        }
    }
    nativePlayMenu_Tick(selection && !tool && !hub.visible,
                       practice && !tool && !hub.visible);
    hub.consumes_input |= hub.visible;
    update_pointer_cursor();
    if (!hub.visible) {
        return;
    }
    if (hub.pending_action) {
        auto action = static_cast<HubAction>(hub.pending_action);
        int value = hub.pending_value;
        hub.pending_action = 0;
        nativeHub_Activate(action, value);
    }
    if (!hub.visible) {
        return;
    }
    // The hub consumes the same already-normalized sample as original menus.
    // Avoid applying a second deadzone or turning the stick into focus steps.
    hub.pressed = mouse_pressed;
    for (unsigned port = 0; port < 4; ++port) {
        const HSD_PadStatus& pad = HSD_PadCopyStatus[port];
        if (pad.err) {
            continue;
        }
        if (hub.page == NativeHubPage_QuickStage && port < nativeOnlineSession_LocalPlayers() &&
            (pad.trigger || pad.stickX || pad.stickY)) hub.stage_seat = port;
        float x = pad.stickX, y = pad.stickY, magnitude = x * x + y * y;
        if (magnitude >= 200) {
            float scale = (magnitude - 200) / std::sqrt(magnitude) * .0016f;
            hub.pointer_x =
                std::clamp(hub.pointer_x + x * scale, 4.0f, 632.0f);
            hub.pointer_y =
                std::clamp(hub.pointer_y - y * scale, 4.0f, 470.0f);
        }
        if (pad.trigger & PAD_BUTTON_A) {
            click(port);
        }
        if (pad.trigger & PAD_BUTTON_B) {
            if (hub.field) {
                text_focus(nullptr, 0);
            } else {
                queue(HubAction::Back);
            }
        }
        if (pad.trigger & PAD_BUTTON_START && hub.page == NativeHubPage_Lobbies) {
            queue(HubAction::Practice);
        }
        hub.pressed |= (pad.button & PAD_BUTTON_A) != 0;
        if (hub.page != NativeHubPage_QuickStage) break;
    }
}
extern "C" void nativeHub_Draw(void)
{
    if (!hub.visible) {
        nativePlayMenu_DrawPractice();
        return;
    }
    if (!hub.fonts) {
        nativeMenuFont_Acquire();
        hub.fonts = true;
    }
    const char* caption = hub.page == NativeHubPage_Profile
        ? (hub.profile_self ? hub.name : hub.viewed_name)
        : page_names[unsigned(hub.page)];
    nativeMenuScene_DrawBackdrop(caption);
    nativeMenuPaint_Begin(nullptr);
    hub.region_count = 0;
    nativeHub_Button("your-profile", hub.name, 494, 21, 103, 19,
                     HubAction::Profile);
    if (hub.opened_from_play || nativeHome_IsPractice())
        nativeHub_Button("hub-close", nativeHome_IsPractice() ? "Return to practice" : "Character select",
                         352, 21, 132, 19, HubAction::Close);
    for (unsigned i = 0; i < 7; ++i) {
        float y = 88 + i * 38;
        bool selected = unsigned(hub.page) == i || (hub.page == NativeHubPage_RoomChat && i == NativeHubPage_Lobbies);
        bool hover = nativeHub_Hover(33, y, 127, 31);
        nativeMenuScene_DrawRibbon(i, 33, y, 127, 31, selected || hover);
        nativeMenuPaint_Begin(nullptr);
        nativeMenuPaint_Text(page_names[i], 46, y + 9, 99, 14,
                            selected || hover ? 0x19121AFF : 0xFFFFFFFF,
                            NativeMenuFont_MenuOption, true, !(selected || hover));
        char id[40];
        std::snprintf(id, sizeof(id), "nav-%u", i);
        nativeHub_Region(
            id, 33, y, 124, 31,
            static_cast<HubAction>(unsigned(HubAction::QuickPlay) + i));
    }
    if (nativeMenuScene_Active()) {
        Mtx view;
        Mtx44 projection;
        hub.passive_page = true;
        if (nativeMenuScene_Sample(true, view, projection)) {
            GXSetProjection(projection, GX_PERSPECTIVE);
            nativeMenuPaint_Begin(view);
            nativeHub_RenderPage(hub.motion_from);
        }
        if (nativeMenuScene_Sample(false, view, projection)) {
            GXSetProjection(projection, GX_PERSPECTIVE);
            nativeMenuPaint_Begin(view);
        } else nativeMenuPaint_Begin(nullptr);
        nativeHub_RenderPage(hub.page);
        hub.passive_page = false;
        nativeMenuPaint_Begin(nullptr);
    } else nativeHub_RenderPage(hub.page);
    nativeHub_Panel(38, 408, 565, 32);
    const char* hint = SDL_GetTicks() < hub.notice_until ? hub.notice
                       : unsigned(hub.page) < 7
                           ? descriptions[unsigned(hub.page)]
                           : "A: Choose   B: Back   Keyboard to type";
    nativeMenuPaint_Text(hint, 48, 418, 425, 11, white);
    nativeMenuPaint_Text("A Choose   B Back", 488, 419, 105, 8, gold,
                        NativeMenuFont_Notice, true);
    nativeHubHand_Draw(hub.pointer_x, hub.pointer_y, hub.pressed);
    nativePlayMenu_DrawCountdown();
    nativeMenuPaint_End();
}
extern "C" void nativeHub_MovePointer(int x, int y)
{
    auto* context = aurora::rmlui::get_context();
    if (!context) {
        return;
    }
    auto size = context->GetDimensions();
    float scale = std::min(size.x / 640.0f, size.y / 480.0f);
    if (scale <= 0) {
        return;
    }
    hub.pointer_x = (x - (size.x - 640 * scale) * .5f) / scale;
    hub.pointer_y = (y - (size.y - 480 * scale) * .5f) / scale;
}
extern "C" void nativeHub_ClickAt(int x, int y, int button)
{
    nativeHub_MovePointer(x, y);
    if (button == 0) {
        click();
    } else if (button == 1) {
        queue(HubAction::Back);
    }
}
extern "C" void nativeHub_EnterText(const char* text)
{
    if (!hub.field || !text) {
        return;
    }
    size_t length = hub.select_all ? 0 : std::strlen(hub.field),
           add = std::strlen(text);
    hub.caret = std::min(hub.caret, unsigned(length));
    if (length + add >= hub.field_capacity) {
        return;
    }
    for (size_t i = 0; i < add; ++i) {
        if (static_cast<unsigned char>(text[i]) < 32 || text[i] == 127) {
            return;
        }
    }
    if (hub.field == hub.code) {
        for (size_t i = 0; i < add; ++i) {
            if (!((text[i] >= '0' && text[i] <= '9') ||
                  (text[i] >= 'a' && text[i] <= 'z') ||
                  (text[i] >= 'A' && text[i] <= 'Z')))
            {
                return;
            }
        }
    }
    if (hub.select_all) {
        hub.field[0] = 0;
        hub.caret = 0;
        hub.select_all = false;
    }
    std::memmove(hub.field + hub.caret + add, hub.field + hub.caret,
                 length - hub.caret + 1);
    std::memcpy(hub.field + hub.caret, text, add);
    hub.caret += unsigned(add);
}
extern "C" void nativeHub_PressKey(const char* name, unsigned modifiers)
{
    SDL_Keycode code = 0;
    if (!std::strcmp(name, "escape")) {
        code = SDLK_ESCAPE;
    } else if (!std::strcmp(name, "enter")) {
        code = SDLK_RETURN;
    } else if (!std::strcmp(name, "delete")) {
        code = SDLK_DELETE;
    } else if (!std::strcmp(name, "backspace")) {
        code = SDLK_BACKSPACE;
    } else if (!std::strcmp(name, "left")) {
        code = SDLK_LEFT;
    } else if (!std::strcmp(name, "right")) {
        code = SDLK_RIGHT;
    } else if (*name && !name[1]) {
        code = *name;
    }
    key(code, SDL_Keymod((modifiers & 1 ? SDL_KMOD_SHIFT : 0) |
                         (modifiers & 2 ? SDL_KMOD_CTRL : 0) |
                         (modifiers & 4 ? SDL_KMOD_ALT : 0)));
}
extern "C" void nativeHub_Event(const SDL_Event* event)
{
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        mouse_pressed = false;
        hub.pressed = false;
    }
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F1 &&
        !event->key.repeat && hub.initialized &&
        (nativeHome_IsSelection() || nativeHome_IsPractice()) &&
        !nativeShellWorkshop_IsOpen() && !nativeShellProfiles_IsOpen() &&
        !nativeShellRules_IsOpen() && !nativeShellCssOriginal_IsEditing() &&
        !nativeShellCostumes_IsOpen() && !nativeShellCollection_IsOpen())
    {
        if (hub.visible) {
            hub.dismissed = true;
            hub.visible = false;
            text_focus(nullptr, 0);
        } else {
            hub.opened_from_play = true;
            show();
        }
        return;
    }
    if (!hub.visible) {
        if (event->type == SDL_EVENT_MOUSE_MOTION) {
            nativeHub_MovePointer(int(event->motion.x), int(event->motion.y));
            nativeShell_CssPointerEvent(int(event->motion.x),
                                        int(event->motion.y),
                                        NativeCssPointer_Move);
        } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                   (event->button.button == SDL_BUTTON_LEFT ||
                    event->button.button == SDL_BUTTON_RIGHT))
        {
            nativeShell_CssPointerEvent(int(event->button.x),
                                        int(event->button.y),
                                        event->button.button == SDL_BUTTON_LEFT
                                            ? NativeCssPointer_Press
                                            : NativeCssPointer_Back);
        } else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
                   event->button.button == SDL_BUTTON_LEFT)
        {
            nativeShell_CssPointerEvent(int(event->button.x),
                                        int(event->button.y),
                                        NativeCssPointer_Release);
        }
        return;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        nativeHub_MovePointer(int(event->motion.x), int(event->motion.y));
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event->button.button == SDL_BUTTON_LEFT ||
            event->button.button == SDL_BUTTON_RIGHT)
        {
            mouse_pressed = true;
            hub.pressed = true;
            nativeHub_ClickAt(int(event->button.x), int(event->button.y),
                              event->button.button == SDL_BUTTON_LEFT ? 0 : 1);
        }
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        hub.pressed = false;
    }
    if (event->type == SDL_EVENT_TEXT_INPUT) {
        nativeHub_EnterText(event->text.text);
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        key(event->key.key, event->key.mod);
    }
    if (event->type == SDL_EVENT_MOUSE_WHEEL &&
        (hub.page == NativeHubPage_Chat || hub.page == NativeHubPage_RoomChat)) {
        bool room = hub.page == NativeHubPage_RoomChat;
        const auto* presence = nativeOnlineSession_Presence();
        const auto* chat = nativeOnlineSession_Chat();
        int people = room ? (presence ? presence->participant_count : 0) : nativeChat_View()->player_count;
        int messages = room ? (chat ? chat->count : 0) : nativeChat_View()->count;
        if (nativeHub_Hover(479, 116, 126, 234)) {
            int& scroll = room ? hub.room_people_scroll : hub.people_scroll;
            scroll = std::clamp(scroll - int(event->wheel.y) * 3, 0, std::max(0, people - 12));
        } else {
            int& scroll = room ? hub.room_chat_scroll : hub.chat_scroll;
            scroll = std::clamp(scroll + int(event->wheel.y) * 3, 0, std::max(0, messages * 34 - 1));
        }
    }
}
extern "C" void nativeHub_Open(NativeHubPage page)
{
    if ((!nativeHome_IsSelection() && !nativeHome_IsPractice()) ||
        page > NativeHubPage_QuickStage) {
        return;
    }
    hub.opened_from_play = true;
    show();
    nativeHub_ChangePage(page);
}
extern "C" void nativeHub_Inspect(const char* id)
{
    if (!std::strcmp(id, "hub-assets")) { nativeMenuScene_Inspect(); return; }
    if (!std::strcmp(id, "hub-motion")) {
        std::fprintf(stderr, "[hub-inspect] hub-motion from=%u to=%u active=%d frame=%.3f\n",
            unsigned(hub.motion_from), unsigned(hub.page), nativeMenuScene_Active(), nativeMenuScene_Frame());
        return;
    }
    if (!std::strcmp(id, "lobby-chat-state")) {
        const auto* chat = nativeOnlineSession_Chat();
        std::fprintf(stderr, "[hub-inspect] lobby-chat-state page=%u connected=%d ready=%d count=%u delivery=%u sequence=%llu\n",
            unsigned(hub.page), chat && chat->connected, chat && chat->ready,
            chat ? chat->count : 0, chat ? unsigned(chat->delivery) : 0,
            chat ? (unsigned long long) chat->last_sequence : 0);
        for (unsigned i = 0; chat && i < chat->count; ++i)
            std::fprintf(stderr, "[lobby-chat-entry] %llu peer=%s name=%s text=%s\n",
                (unsigned long long) chat->messages[i].sequence, chat->messages[i].peer_id,
                chat->messages[i].name, chat->messages[i].text);
        return;
    }
    for (unsigned i = 0; i < hub.region_count; ++i) {
        const auto& r = hub.regions[i];
        if (std::strcmp(id, r.id)) {
            continue;
        }
        auto size = aurora::rmlui::get_context()->GetDimensions();
        float scale = std::min(size.x / 640.f, size.y / 480.f);
        std::fprintf(stderr,
                     "[hub-inspect] %s origin=%.1f,%.1f size=%.1f,%.1f "
                     "enabled=%d value=%d\n",
                     id, r.x * scale + (size.x - 640 * scale) * .5f,
                     r.y * scale + (size.y - 480 * scale) * .5f, r.w * scale,
                     r.h * scale, r.enabled, r.value);
        return;
    }
    std::fprintf(stderr, "[hub-inspect] %s: absent page=%u visible=%d\n", id,
                 unsigned(hub.page), hub.visible);
}
extern "C" void nativeHub_Shutdown(void)
{
    if (hand_cursor_hidden) {
        SDL_ShowCursor();
    }
    hand_cursor_hidden = false;
    mouse_pressed = false;
    text_focus(nullptr, 0);
    nativeHubCatalog_Clear();
    nativeHubHand_Unload();
    nativeMenuScene_Unload();
    nativePlayMenu_Shutdown();
    if (hub.fonts) {
        nativeMenuFont_Release();
    }
    hub = HubState{};
}
