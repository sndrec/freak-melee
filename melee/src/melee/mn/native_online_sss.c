#include "native_online_sss.h"

#ifdef MELEE_NATIVE

#include <string.h>
#include <stdio.h>
#include <SDL3/SDL.h>

#include <melee/gm/gmvsmelee.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/types.h>
#include <melee/mn/native_menu_paint.h>
#include <sysdolphin/baselib/cobj.h>

static struct NativeOnlineSssState {
    bool active;
    bool match_submitted;
    bool stage_submitted;
    bool fonts, dirty;
    uint64_t sent_ms;
    uint32_t sequence, received_sequence;
    char received_peer[NATIVE_SIGNALING_MAX_PEER_ID_BYTES + 1];
    NativeStageCursor sent;
} online_sss;

void nativeOnlineSss_Prepare(SSSData* sss)
{
    memset(&online_sss, 0, sizeof(online_sss));
    online_sss.active =
        sss != NULL && !nativeOnlineSession_Quick() &&
        (nativeOnlineSession_State() == NativeLobbySessionState_Waiting ||
         nativeOnlineSession_State() == NativeLobbySessionState_Starting ||
         nativeOnlineSession_State() == NativeLobbySessionState_Ready);
    if (online_sss.active) {
        nativeMenuFont_Acquire();
        online_sss.fonts = true;
        // The service owns automatic picks; local saved stage rules cannot
        // bypass the shared stage-selection step.
        sss->force_stage_id=-1;
        sss->unk_stage=0;
    }
}

bool nativeOnlineSss_IsActive(void)
{
    return online_sss.active;
}

NativeOnlineSssAction nativeOnlineSss_Pump(SSSData* sss,
                                           bool stage_selected,
                                           uint8_t stage)
{
    if (!online_sss.active || sss == NULL) {
        return NativeOnlineSssAction_Offline;
    }
    if (!nativeOnlineSession_Pump()) {
        NativeLobbySessionState state = nativeOnlineSession_State();
        return state == NativeLobbySessionState_Failed ||
                       state == NativeLobbySessionState_Disconnected
                   ? NativeOnlineSssAction_Failed
                   : NativeOnlineSssAction_Wait;
    }

    switch (nativeOnlineSession_State()) {
    case NativeLobbySessionState_Ready:
        // Adoption applies the canonical proposal to the persistent VS data.
        // Carry that exact data through this already-entered SSS scene.
        sss->vs = *gmVsMelee_GetVsData();
        online_sss.active = false;
        return NativeOnlineSssAction_Start;
    case NativeLobbySessionState_Starting:
        return NativeOnlineSssAction_Wait;
    case NativeLobbySessionState_Waiting:
        {
            bool owner_requested;
            if (!nativeHome_RoomRosterReady(&owner_requested, NULL, NULL) ||
                !owner_requested) {
                return NativeOnlineSssAction_Cancelled;
            }
        }
        break;
    case NativeLobbySessionState_Connecting:
    case NativeLobbySessionState_Returning:
        return NativeOnlineSssAction_Wait;
    case NativeLobbySessionState_Disconnected:
    case NativeLobbySessionState_Failed:
        return NativeOnlineSssAction_Failed;
    }

    const NativeSignalingPresence* presence = nativeOnlineSession_Presence();
    if (!presence) return NativeOnlineSssAction_Wait;
    if (presence->stage == UINT8_MAX) {
        if (nativeOnlineSession_StagePort() < 0 || online_sss.stage_submitted)
            return NativeOnlineSssAction_Wait;
        if (!stage_selected) return NativeOnlineSssAction_PlayerInput;
        if (!nativeOnlineSession_PickStage(stage))
            return NativeOnlineSssAction_Failed;
        online_sss.stage_submitted = true;
    } else if (nativeOnlineSession_IsOwner() && !online_sss.match_submitted) {
        if (!nativeHome_PublishRoomMatch(presence->stage))
            return NativeOnlineSssAction_Failed;
        online_sss.match_submitted = true;
    }
    return NativeOnlineSssAction_Wait;
}

void nativeOnlineSss_Reset(void)
{
    if (online_sss.fonts) nativeMenuFont_Release();
    memset(&online_sss, 0, sizeof(online_sss));
}

bool nativeOnlineSss_ReadCursor(float* x, float* y, uint8_t* tile)
{
    const NativeStageCursor* cursor = nativeOnlineSession_StageCursor();
    if (!online_sss.active || !cursor ||
        (cursor->sequence == online_sss.received_sequence &&
         !strcmp(cursor->peer_id, online_sss.received_peer))) return false;
    online_sss.received_sequence = cursor->sequence;
    snprintf(online_sss.received_peer, sizeof(online_sss.received_peer), "%s", cursor->peer_id);
    if (!strcmp(cursor->peer_id, nativeOnlineSession_LocalPeerId())) return false;
    *x = cursor->x / 256.0f;
    *y = cursor->y / 256.0f;
    *tile = cursor->tile;
    online_sss.dirty = false;
    return true;
}

void nativeOnlineSss_WriteCursor(float x, float y, uint8_t tile, bool moved)
{
    if (!online_sss.active || tile > 29 || nativeOnlineSession_StagePort() < 0) return;
    int16_t qx = x * 256, qy = y * 256;
    if (moved && (qx != online_sss.sent.x || qy != online_sss.sent.y || tile != online_sss.sent.tile))
        online_sss.dirty = true;
    // Seed a new selection once. Idle clients subsequently follow the last
    // moving player, including when the policy allows any active player.
    if (!online_sss.sequence && !nativeOnlineSession_StageCursor()) online_sss.dirty = true;
    uint64_t now = SDL_GetTicks();
    if (!online_sss.dirty || now - online_sss.sent_ms < 50) return;
    NativeStageCursor cursor = { 0 };
    cursor.x = qx; cursor.y = qy; cursor.tile = tile;
    cursor.sequence = ++online_sss.sequence;
    if (nativeOnlineSession_PublishStageCursor(&cursor)) {
        online_sss.sent = cursor;
        online_sss.sent_ms = now;
        online_sss.dirty = false;
    }
}

void nativeOnlineSss_DrawChooser(HSD_GObj* object, int pass)
{
    (void) object; (void) pass;
    const NativeSignalingPresence* presence = nativeOnlineSession_Presence();
    if (!online_sss.active || !online_sss.fonts || !presence) return;
    const NativeStageCursor* cursor = nativeOnlineSession_StageCursor();
    const char* peer = cursor ? cursor->peer_id : presence->stage_peer_id;
    unsigned seat = cursor ? cursor->seat : presence->stage_seat;
    char label[128] = "Choosing a stage...";
    for (unsigned i = 0; i < presence->participant_count; ++i) {
        const NativeSignalingParticipant* person = &presence->participants[i];
        if (strcmp(person->peer_id, peer)) continue;
        if (person->local_players > 1)
            snprintf(label, sizeof(label), "%s (player %u) is choosing", person->name, seat + 1);
        else snprintf(label, sizeof(label), "%s is choosing", person->name);
        break;
    }
    HSD_CObj* camera = HSD_CObjGetCurrent();
    if (!camera) return;
    Mtx view, model = { { .1f, 0, 0, 0 }, { 0, .1f, 0, 0 }, { 0, 0, 1, 5 } }, canvas;
    HSD_CObjGetViewingMtx(camera, view);
    C_MTXConcat(view, model, canvas);
    nativeMenuPaint_Begin(canvas);
    nativeMenuPaint_Text(label, -260, 200, 520, 15, 0xFFF4E9FF,
                         NativeMenuFont_MenuCaption, true, true);
    nativeMenuPaint_End();
}

#endif
