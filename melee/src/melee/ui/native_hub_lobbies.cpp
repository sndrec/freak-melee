#include "native_hub_lobbies.h"
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_services.h>
#include <melee/mn/native_online_menu.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
    NativeSignalingClient browser;
    NativeLobbyDirectory page;
    bool connected, received, pending;
    uint16_t requested_offset;
    uint64_t requested_at, retry_at;
    char failure[128];
    NativeLobbySessionState previous_state;
    constexpr unsigned gold = 0xD9B754FF, muted = 0xB9ADBDFF;

    void request(uint16_t offset)
    {
        if (!connected || pending || (requested_at && SDL_GetTicks() - requested_at < 500)) return;
        if (nativeSignalingClient_RequestLobbies(&browser, offset)) {
            pending = true;
            requested_offset = offset;
            requested_at = SDL_GetTicks();
            failure[0] = 0;
        }
    }
}
void nativeHubLobbies_Tick()
{
    const auto state = nativeOnlineSession_State();
    if (state == NativeLobbySessionState_Failed && previous_state != state)
        nativeHub_Notice(nativeOnlineSession_Failure());
    previous_state = state;
    if (hub.pending_listing && nativeOnlineSession_IsOwner()) {
        if (nativeOnlineSession_SetListing(hub.lobby_title, hub.lobby_listed)) hub.pending_listing = false;
    } else if (hub.pending_listing && state == NativeLobbySessionState_Failed) hub.pending_listing = false;
    const bool visible = hub.visible && hub.page == NativeHubPage_Lobbies &&
        (state == NativeLobbySessionState_Disconnected || state == NativeLobbySessionState_Failed);
    if (browser.thread_handle && !nativeSignalingClient_IsRunning(&browser)) {
        nativeSignalingClient_Stop(&browser);
        connected = pending = false;
        retry_at = SDL_GetTicks() + 5000;
    }
    if (visible && !browser.thread_handle && SDL_GetTicks() >= retry_at) {
        const char* endpoint = std::getenv("FREAK_MELEE_SERVICE_ENDPOINT");
        NativeSignalingClientConfig config{};
        config.endpoint = endpoint && *endpoint ? endpoint : NATIVE_SERVICE_ENDPOINT;
        config.name = "Lobby browser";
        config.build = nativeOnlineSession_Build();
        config.directory = true;
        if (!nativeSignalingClient_Start(&browser, &config)) retry_at = SDL_GetTicks() + 5000;
    }
    NativeSignalingClientEvent event;
    while (nativeSignalingClient_Poll(&browser, &event)) {
        if (event.type == NativeSignalingClientEvent_Connected) {
            connected = true;
            requested_at = 0;
            request(requested_offset);
        } else if (event.type == NativeSignalingClientEvent_Disconnected) {
            connected = pending = false;
            std::snprintf(failure, sizeof(failure), "Could not connect to the lobby browser.");
            if (visible && page.count) nativeHub_Notice(failure);
        } else if (event.type == NativeSignalingClientEvent_Message) {
            if (event.message.type == NativeSignalingServerMessage_Lobbies) {
                page = event.message.data.directory;
                received = true;
                pending = false;
                failure[0] = 0;
            } else if (event.message.type == NativeSignalingServerMessage_Error) {
                pending = false;
                std::snprintf(failure, sizeof(failure), "%.127s", event.message.data.error);
                if (visible && page.count) nativeHub_Notice(failure);
            }
        }
    }
    if (pending && SDL_GetTicks() - requested_at > 10000) {
        nativeSignalingClient_Stop(&browser);
        pending = connected = false;
        std::snprintf(failure, sizeof(failure), "The lobby browser timed out. Reconnecting...");
        retry_at = SDL_GetTicks() + 5000;
    }
    if (visible && !pending && SDL_GetTicks() - requested_at >= 10000) request(page.offset);
}
void nativeHubLobbies_Render(bool create)
{
    if (create) {
        nativeHub_Panel(175, 118, 430, 252);
        nativeMenuPaint_Text("Lobby name", 194, 138, 370, 12, gold);
        nativeHub_Field("lobby-title", hub.lobby_title, 194, 162, 390, 28, HubAction::FieldLobbyTitle);
        nativeHub_Button("list-lobby", hub.lobby_listed ? "Listed in Lobbies" : "Unlisted - join with code",
            194, 213, 390, 29, HubAction::ToggleListing, 0, hub.lobby_listed);
        char invitation[96]; std::snprintf(invitation, sizeof(invitation), "Invite @%s when the lobby is ready.", hub.lobby_invite_to);
        nativeMenuPaint_Text(*hub.lobby_invite_to ? invitation : hub.lobby_listed ? "Players can find and join your lobby." : "Give your code to the people you want to invite.",
            194, 250, 390, 9, muted);
        nativeMenuPaint_Text("Players on this PC", 194, 283, 180, 10, gold);
        for (unsigned players = 1; players <= 4; ++players) {
            char label[2] = {char('0' + players), 0}, id[24];
            std::snprintf(id, sizeof(id), "local-players-%u", players);
            nativeHub_Button(id, label, 389 + (players - 1) * 48, 275, 42, 26,
                HubAction::LocalPlayers, players, unsigned(hub.local_players) == players);
        }
        nativeHub_Button("confirm-create-lobby", *hub.lobby_invite_to ? "Create & invite" : "Create lobby", 194, 322, 390, 30,
            HubAction::CreateNamedLobby, 0, true, hub.lobby_title[0] != 0);
        return;
    }
    nativeHub_Button("create-lobby", "Create lobby", 175, 116, 130, 27, HubAction::NewLobby, 0, true);
    nativeHub_Button("refresh-lobbies", pending ? "Refreshing..." : "Refresh", 503, 116, 102, 27,
        HubAction::RefreshLobbies, 0, false, connected && !pending && SDL_GetTicks() - requested_at >= 500);
    nativeHub_Panel(175, 151, 430, 205);
    for (unsigned i = 0; i < page.count; ++i) {
        const auto& entry = page.entries[i];
        const float y = 158 + i * 24;
        char id[32], status[128];
        std::snprintf(id, sizeof(id), "join-listed-%u", i);
        nativeMenuPaint_Text(entry.title, 188, y, 301, 11, gold);
        std::snprintf(status, sizeof(status), "%u / 4 fighting | %u waiting | %u spectating%s",
            entry.fighting, entry.waiting, entry.spectating, entry.playing ? " | In match" : "");
        nativeMenuPaint_Text(status, 188, y + 12, 301, 7, muted);
        nativeHub_Button(id, "Join", 516, y, 77, 21, HubAction::JoinListedLobby, i, false,
            connected && !pending, entry.code, entry.title);
    }
    if (!page.count) nativeMenuPaint_Text(failure[0] ? failure : !received ? "Finding lobbies..." : "No listed lobbies. Create one to get started.",
        192, 191, 390, 11, muted);
    if (page.total > 8) {
        char count[48];
        std::snprintf(count, sizeof(count), "%u-%u / %u", page.offset + 1, page.offset + page.count, page.total);
        nativeMenuPaint_Text(count, 328, 127, 115, 8, muted);
        nativeHub_Button("lobbies-prev", "<", 443, 117, 23, 24, HubAction::BrowseLobbies, -8, false, !pending && page.offset > 0);
        nativeHub_Button("lobbies-next", ">", 472, 117, 23, 24, HubAction::BrowseLobbies, 8, false, !pending && page.offset + page.count < page.total);
    }
    nativeMenuPaint_Text("Lobby code", 181, 365, 85, 9, gold);
    nativeHub_Field("lobby-code", hub.code, 271, 360, 218, 25, HubAction::FieldCode);
    nativeHub_Button("join-lobby", "Join", 503, 360, 102, 25, HubAction::Join, 0, true, std::strlen(hub.code) >= 6);
    nativeMenuPaint_Text("Players on this PC", 181, 395, 180, 9, gold);
    for (unsigned players = 1; players <= 4; ++players) {
        char label[2] = {char('0' + players), 0}, id[24];
        std::snprintf(id, sizeof(id), "local-players-%u", players);
        nativeHub_Button(id, label, 389 + (players - 1) * 48, 387, 42, 25,
            HubAction::LocalPlayers, players, unsigned(hub.local_players) == players);
    }
}
bool nativeHubLobbies_Activate(HubAction action, int value)
{
    switch (action) {
    case HubAction::NewLobby:
        hub.lobby_invite_to[0] = 0; hub.lobby_invite_queued = false;
        std::snprintf(hub.lobby_title, sizeof(hub.lobby_title), "%.32s's lobby", hub.public_name);
        nativeHub_ChangePage(NativeHubPage_CreateLobby);
        return true;
    case HubAction::ToggleListing: hub.lobby_listed = !hub.lobby_listed; return true;
    case HubAction::CreateNamedLobby:
        {
            size_t begin = 0, end = std::strlen(hub.lobby_title);
            while (begin < end && hub.lobby_title[begin] == ' ') ++begin;
            while (end > begin && hub.lobby_title[end - 1] == ' ') --end;
            std::memmove(hub.lobby_title, hub.lobby_title + begin, end - begin);
            hub.lobby_title[end - begin] = 0;
            if (end == begin) { nativeHub_Notice("Enter a name for your lobby."); return true; }
        }
        if (nativeOnlineSession_State() == NativeLobbySessionState_Failed) nativeOnlineSession_Stop();
        if (nativeOnlineMenu_Request(NativeOnlineMenuAction_Create, hub.public_name, "", uint8_t(hub.local_players))) {
            hub.pending_listing = true;
            hub.lobby_invite_queued = *hub.lobby_invite_to != 0;
            nativeHub_ChangePage(NativeHubPage_Lobbies);
        } else nativeHub_Notice("Could not create the lobby. Try again shortly.");
        return true;
    case HubAction::RefreshLobbies: request(page.offset); return true;
    case HubAction::BrowseLobbies: request(uint16_t(std::max(0, int(page.offset) + value))); return true;
    case HubAction::JoinListedLobby:
        // Use the code captured under the pointer, even if a refresh changes the rows.
        std::snprintf(hub.code, sizeof(hub.code), "%.12s", hub.pending_peer);
        nativeHub_Activate(HubAction::Join, 0);
        return true;
    case HubAction::ChangeListing: {
        const auto* listing = nativeOnlineSession_Listing();
        if (!listing || !listing->title[0] || !nativeOnlineSession_SetListing(listing->title, !listing->listed))
            nativeHub_Notice("Could not change the lobby listing.");
        return true;
    }
    default: return false;
    }
}
void nativeHubLobbies_Shutdown()
{
    nativeSignalingClient_Stop(&browser);
    browser = {};
    page = {};
    connected = received = pending = false;
    requested_at = retry_at = requested_offset = 0;
    failure[0] = 0;
    previous_state = NativeLobbySessionState_Disconnected;
}
void nativeHubLobbies_Inspect()
{
    std::fprintf(stderr, "[hub-lobbies] connected=%d pending=%d count=%u total=%u offset=%u error=%s\n",
        connected, pending, page.count, page.total, page.offset, failure);
    for (unsigned i = 0; i < page.count; ++i)
        std::fprintf(stderr, "[hub-lobby] row=%u code=%s title=%s host=%s fighting=%u waiting=%u spectating=%u playing=%u\n",
            i, page.entries[i].code, page.entries[i].title, page.entries[i].host, page.entries[i].fighting,
            page.entries[i].waiting, page.entries[i].spectating, page.entries[i].playing);
}
