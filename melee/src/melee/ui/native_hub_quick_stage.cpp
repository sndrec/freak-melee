#include "native_hub_quick_stage.h"
#include <melee/gm/native_online_session.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <SDL3/SDL.h>
#include "native_hub_hand.h"

namespace {
constexpr const char* stages[] = {"Battlefield", "Final Destination",
    "Fountain of Dreams", "Dream Land", "Pokemon Stadium", "Yoshi's Story"};
constexpr uint32_t gold = 0xE5C361FF, muted = 0xA6A1AFFF;
struct StageChoice {
    const NativeSignalingParticipant* proposer = nullptr;
    unsigned stage = 255, seat = 0;
    bool local = false;
};
struct {
    uint64_t sent_ms;
    uint32_t sequence;
    float x, y;
    bool dirty;
} cursor;

StageChoice choice()
{
    StageChoice result;
    const auto* quick = nativeOnlineSession_Quick();
    const auto* presence = nativeOnlineSession_Presence();
    if (!quick || !presence) return result;
    // The service numbers votes in participant join order, then local seat.
    const NativeSignalingParticipant* people[NATIVE_SIGNALING_MAX_PARTICIPANTS];
    for (unsigned i = 0; i < presence->participant_count; ++i)
        people[i] = &presence->participants[i];
    std::sort(people, people+presence->participant_count,
        [](const auto* a, const auto* b) { return a->join_order < b->join_order; });
    unsigned port = 0;
    for (unsigned i = 0; i < presence->participant_count; ++i)
        for (unsigned seat = 0; seat < people[i]->local_players && port < 4; ++seat, ++port)
            if (quick->votes[port] < 6) {
                result = {people[i], quick->votes[port], seat,
                    !std::strcmp(people[i]->peer_id, nativeOnlineSession_LocalPeerId())};
                return result;
            }
    return result;
}
}

bool nativeHubQuickStage_OpenRound()
{
    static uint64_t epoch;
    static char room[13];
    const auto* quick = nativeOnlineSession_Quick();
    if (!quick || quick->state != NativeQuickState_Assigned || quick->phase != 3)
        return false;
    const char* current = nativeOnlineSession_RoomCode();
    if (!current || (epoch == quick->completed_epoch && !std::strcmp(room,current)))
        return false;
    epoch = quick->completed_epoch;
    std::snprintf(room,sizeof(room),"%s",current);
    hub.stage_seat = 0;
    cursor = {};
    return true;
}

void nativeHubQuickStage_Tick()
{
    const auto* quick = nativeOnlineSession_Quick();
    if (!hub.visible || hub.page != NativeHubPage_QuickStage || !quick || quick->phase != 3 ||
        quick->team != quick->chooser) return;
    if (cursor.x != hub.pointer_x || cursor.y != hub.pointer_y) {
        cursor.x = hub.pointer_x; cursor.y = hub.pointer_y; cursor.dirty = true;
    }
    uint64_t now = SDL_GetTicks();
    if (!cursor.dirty || now - cursor.sent_ms < 50) return;
    NativeStageCursor update{};
    update.x = int16_t(std::clamp((cursor.x / 640.f * 54.f - 27.f) * 256.f, -6912.f, 6912.f));
    update.y = int16_t(std::clamp((19.f - cursor.y / 480.f * 38.f) * 256.f, -4864.f, 4864.f));
    update.tile = 29;
    for (unsigned stage = 0; stage < 6; ++stage) {
        float x = 175 + (stage % 2) * 220, y = 148 + (stage / 2) * 57;
        if (cursor.x >= x && cursor.x < x+210 && cursor.y >= y && cursor.y < y+49) update.tile = stage;
    }
    update.seat = hub.stage_seat;
    update.sequence = ++cursor.sequence;
    if (nativeOnlineSession_PublishStageCursor(&update)) { cursor.dirty = false; cursor.sent_ms = now; }
}

void nativeHubQuickStage_Render()
{
    const auto* quick = nativeOnlineSession_Quick();
    if (!quick || quick->phase != 3) {
        nativeMenuPaint_Text("Stage selected",175,98,420,12,gold,NativeMenuFont_MenuOption);
        nativeHub_Button("stage-return","Character select",175,118,220,30,HubAction::Close);
        return;
    }
    const StageChoice selected = choice();
    const bool choosing = quick->team == quick->chooser;
    const unsigned players = nativeOnlineSession_LocalPlayers();
    hub.stage_seat = std::min(hub.stage_seat,players ? players-1 : 0);
    const char* hint = !choosing ? "Waiting for the other team to choose."
        : selected.proposer ? "One player picks. Their teammate confirms."
        : quick->mode == NativeQuickMode_Doubles ? "Either teammate can pick a stage."
        : "Choose a stage for the rematch.";
    nativeMenuPaint_Text(hint,175,98,420,9,muted);
    if (choosing && players > 1) {
        nativeMenuPaint_Text("Mouse controls",175,120,100,8,muted);
        for (unsigned seat = 0; seat < players; ++seat) {
            char id[40], label[32];
            std::snprintf(id,sizeof(id),"stage-seat-%u",seat);
            std::snprintf(label,sizeof(label),"Player %u",seat+1);
            nativeHub_Button(id,label,277+seat*91,113,84,24,HubAction::StageSeat,
                int(seat),hub.stage_seat==seat);
        }
    }
    for (unsigned stage = 0; stage < 6; ++stage) {
        char id[40];
        std::snprintf(id,sizeof(id),"stage-pick-%u",stage);
        bool can_pick = choosing && (!selected.proposer || selected.local);
        nativeHub_Button(id,stages[stage],175+(stage%2)*220,148+(stage/2)*57,210,49,
            HubAction::PickStage,int(stage),selected.stage==stage,can_pick);
    }
    if (selected.proposer) {
        char label[160];
        std::snprintf(label,sizeof(label),"%s%s chose %s",selected.proposer->name,
            selected.proposer->local_players > 1 ? (selected.seat ? " / P2" : " / P1") : "",
            stages[selected.stage]);
        nativeMenuPaint_Text(label,175,326,430,10,gold);
        for (unsigned seat = 0; choosing && seat < players; ++seat) {
            const bool own = selected.local && selected.seat == seat;
            char id[40], text[48];
            std::snprintf(id,sizeof(id),own ? "stage-withdraw-%u" : "stage-confirm-%u",seat);
            std::snprintf(text,sizeof(text),own ? "P%u: Clear selection" : "P%u: Confirm stage",seat+1);
            nativeHub_Button(id,text,175+seat*220,347,210,29,
                own ? HubAction::WithdrawStage : HubAction::ConfirmStage,
                int(seat*256+selected.stage),!own);
        }
    }
    nativeHub_Button("stage-leave","Leave match",480,379,125,24,HubAction::LeaveQuick);
    const auto* shared = nativeOnlineSession_StageCursor();
    if (shared && std::strcmp(shared->peer_id, nativeOnlineSession_LocalPeerId())) {
        const auto* presence = nativeOnlineSession_Presence();
        for (unsigned i = 0; presence && i < presence->participant_count; ++i) {
            const auto& person = presence->participants[i];
            if (std::strcmp(person.peer_id, shared->peer_id)) continue;
            char label[128];
            std::snprintf(label, sizeof(label), "%s / P%u is choosing", person.name, shared->seat + 1);
            nativeMenuPaint_Text(label, 175, 414, 430, 10, gold);
            break;
        }
    }
}

void nativeHubQuickStage_DrawCursor(const Mtx canvas)
{
    if (hub.page != NativeHubPage_QuickStage) return;
    const auto* shared = nativeOnlineSession_StageCursor();
    if (!shared || !std::strcmp(shared->peer_id, nativeOnlineSession_LocalPeerId())) return;
    nativeHubHand_Draw((shared->x / 256.f + 27.f) / 54.f * 640.f,
                      (19.f - shared->y / 256.f) / 38.f * 480.f, false, canvas);
}

bool nativeHubQuickStage_Activate(HubAction action, int value)
{
    if (action == HubAction::StageSeat) {
        if (value >= 0 && unsigned(value) < nativeOnlineSession_LocalPlayers()) hub.stage_seat = unsigned(value);
        return true;
    }
    if (action == HubAction::LeaveQuick) {
        if (nativeOnlineSession_SearchQuick(false)) {
            hub.dismissed = true;
            hub.visible = false;
        } else nativeHub_Notice("Could not leave this match.");
        return true;
    }
    if (action != HubAction::PickStage && action != HubAction::ConfirmStage &&
        action != HubAction::WithdrawStage) return false;
    const auto* quick = nativeOnlineSession_Quick();
    const StageChoice selected = choice();
    unsigned seat = unsigned(value)/256, stage = unsigned(value)%256;
    bool own = selected.local && selected.seat == seat;
    bool valid = quick && quick->phase == 3 && quick->team == quick->chooser &&
        seat < nativeOnlineSession_LocalPlayers();
    if (action == HubAction::PickStage) valid &= stage < 6 && (!selected.proposer || own);
    else if (action == HubAction::ConfirmStage) valid &= selected.proposer && !own && stage == selected.stage;
    else { valid &= own; stage = 255; }
    if (!valid || !nativeOnlineSession_PickQuickStage(uint8_t(seat),uint8_t(stage)))
        nativeHub_Notice("The stage choice changed. Choose again.");
    return true;
}
