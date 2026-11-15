#include "native_hub_controls.h"
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_home.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>

namespace {
    char players_path[560];
}

void nativeHubControls_Load()
{
    char buffer[512];
    const char* root = nativeHubCatalog_Root(buffer);
    if (!root) return;
    std::snprintf(players_path, sizeof(players_path), "%s/local-players.txt", root);
    FILE* file = std::fopen(players_path, "rb");
    if (!file) return;
    unsigned count;
    if (std::fscanf(file, "%u", &count) == 1 && count >= 1 && count <= 4)
        hub.local_players = int(count);
    std::fclose(file);
}

bool nativeHubControls_SetPlayers(unsigned count)
{
    if (count < 1 || count > 4 || !players_path[0]) return false;
    char temporary[576];
    std::snprintf(temporary, sizeof(temporary), "%s.new", players_path);
    FILE* file = std::fopen(temporary, "wb");
    if (!file) return false;
    bool ok = std::fprintf(file, "%u\n", count) > 0;
    ok = std::fclose(file) == 0 && ok;
    if (!ok || !MoveFileExA(temporary, players_path,
                           MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temporary);
        return false;
    }
    hub.local_players = int(count);
    return true;
}

void nativeHubControls_Render()
{
    nativeHub_Button("controls-back", "< Settings", 175, 112, 130, 26, HubAction::Settings);
    nativeMenuPaint_Text("Players on this PC", 193, 175, 380, 18, 0xE5C361FF,
                         NativeMenuFont_MenuOption);
    for (unsigned count = 1; count <= 4; ++count) {
        char label[2] = {char('0' + count), 0}, id[32];
        std::snprintf(id, sizeof(id), "local-players-%u", count);
        nativeHub_Button(id, label, 193 + (count - 1) * 91, 214, 77, 32,
                         HubAction::LocalPlayers, count, unsigned(hub.local_players) == count);
    }
    nativeMenuPaint_Text("Used when joining lobbies and Quick Play.", 193, 260, 380, 10);
    const auto state = nativeOnlineSession_State();
    if (state != NativeLobbySessionState_Disconnected && state != NativeLobbySessionState_Failed)
        nativeMenuPaint_Text("Changes apply the next time you join.", 193, 280, 380, 10);
    nativeHub_Button("edit-controls", "Player profiles & bindings", 193, 325, 365, 35,
                     HubAction::EditControls, 0, false, nativeHome_IsSelection());
}
