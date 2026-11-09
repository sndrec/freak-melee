#include "native_netplay_match.h"
#include "native_netplay_timing.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#include "native_match_launch.h"
#include "native_netplay_diagnostics.h"
#include "native_pad_wire.h"
#include "native_home.h"
#include "native_netplay_room.h"
#include "native_netplay_udp.h"
#include "native_online_session.h"
#include "native_replay.h"
#ifdef NATIVE_NETPLAY_DIAGNOSTICS
#include "native_replay_capsule_trace.h"
#endif
#include "gmvs.h"
#include "types.h"
#include "native_match_state_admission.h"
#include "native_match_snapshot.h"
#include <melee/ft/fighter.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/audio_rollback.h>
#include <sysdolphin/baselib/rumble.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/native_random_snapshot.h>
#include <sysdolphin/baselib/native_callback_profile.h>
#include <sysdolphin/baselib/rumble_output.h>

enum {
    NETPLAY_ADMISSION_SCRATCH_BYTES = 64 * 1024 * 1024,
    NETPLAY_INPUT_TIMEOUT_MS = 5000,
    NETPLAY_ROOM_SETUP_TIMEOUT_MS = 60000,
};

static const uint64_t sudden_death_session_tag =
    UINT64_C(0x53554444454E0001);

typedef struct NativeNetplayReplayBoundary {
    uint64_t frame;
    uint64_t replay_sample;
    bool valid;
} NativeNetplayReplayBoundary;

typedef struct NativeNetplayStateDigest {
    uint64_t frame;
    uint64_t value;
    bool valid;
    uint32_t controller[11+GM_MAX_PLAYERS*19];
#ifdef NATIVE_NETPLAY_DIAGNOSTICS
    PADStatus consumed[NATIVE_ROLLBACK_INPUT_PORTS];
    uint64_t first_digest, checkpoint;
    uint32_t rng, pass, simulations;
    uint8_t confirmed_at_tick;
    bool has_consumed, resimulated;
#endif
} NativeNetplayStateDigest;

static struct NativeNetplayMatch {
    NativeRollbackInputHistory inputs;
    NativeConfirmedCheckpoint checkpoint;
    void* checkpoint_backing;
    bool frame_active, rumble_suppressed;
    bool visible_tick_pending;
    bool checksum_trace;
    void* probe_before;
    void* probe_after;
    NativeMatchSnapshotPart probe_parts[NativeMatchSnapshot_Count];
    NativeMatchSnapshotPart probe_compare[NativeMatchSnapshot_Count];
    bool probe_active;
    bool probe_captured,probe_owners;
    uint64_t probe_frame,probe_digest;
    NativeNetplayUdp udp;
    NativeNetplayRoom owned_room;
    NativeNetplayRoom* room;
    NativeNetplayExchange* exchange;
    NativeNetplayReplayBoundary
        replay_boundaries[NATIVE_ROLLBACK_INPUT_CAPACITY];
    NativeNetplayStateDigest
        state_digests[NATIVE_ROLLBACK_INPUT_CAPACITY];
    uint64_t current_frame;
    uint64_t published_digest_frame;
    uint32_t correction_count;
    uint32_t resimulated_frame_count;
    uint32_t input_stall_count;
    uint32_t compared_digest_count;
    uint64_t input_stall_ms;
    uint64_t mismatch_frame, mismatch_peer_digest;
    uint32_t initial_seed;
#ifdef NATIVE_NETPLAY_DIAGNOSTICS
    PADStatus consumed[NATIVE_ROLLBACK_INPUT_PORTS];
    uint8_t confirmed_at_tick;
    bool has_consumed;
#endif
    char failure_reason[128];
    bool has_mismatch;
    s8 physical_ports[4];
    bool active;
    bool replay_playback;
    bool room_transport;
    bool borrowed_online_room;
    bool prepared;
    bool has_published_digest;
    bool failed;
    bool lobby_return_pending;
    bool sudden_death_pending;
} netplay;

static void fail(const char* reason)
{
    if (reason == NULL) {
        reason = "netplay failed without a reason";
    }
    if (!netplay.failed) {
        OSReport("[netplay] FAILED: %s\n", reason);
        snprintf(netplay.failure_reason, sizeof(netplay.failure_reason), "%s", reason);
    }
    netplay.checkpoint.failed = true;
    netplay.failed = true;
    netplay.lobby_return_pending = true;
}

static bool receive_packets(void)
{
    return netplay.room_transport
               ? nativeNetplayRoom_Receive(netplay.room, &netplay.inputs)
               : nativeNetplayUdp_Receive(&netplay.udp, &netplay.inputs);
}

static bool send_packets(void)
{
    return netplay.room_transport ? nativeNetplayRoom_Send(netplay.room)
                                  : nativeNetplayUdp_Send(&netplay.udp);
}

static void shutdown_transport(void)
{
    if (!netplay.prepared) {
        return;
    }
    if (netplay.room_transport) {
        if (netplay.borrowed_online_room) {
            nativeOnlineSession_FinishMatch();
        } else {
            nativeNetplayRoom_Shutdown(netplay.room);
        }
    } else {
        nativeNetplayUdp_Shutdown(&netplay.udp);
    }
    netplay.prepared = false;
}

static uint64_t semantic_state_digest(uint64_t frame)
{
    uint64_t digest = nativeReplay_StateDigest(frame);
    return (digest ^ *seed_ptr) * UINT64_C(1099511628211);
}

static NativeNetplayStateDigest* find_state_digest(uint64_t frame)
{
    NativeNetplayStateDigest* digest =
        &netplay.state_digests[frame % NATIVE_ROLLBACK_INPUT_CAPACITY];
    return digest->valid && digest->frame == frame ? digest : NULL;
}

static void record_confirmed_stream(void)
{
    if (!netplay.room_transport || !netplay.room)
        return;
    NativeMatchStream* stream = &netplay.room->stream;
    if (!stream->records || stream->unavailable || stream->finished) return;
    for (uint64_t frame = stream->next_frame; frame < netplay.checkpoint.boundary; ++frame) {
        NativeNetplayStateDigest* digest = find_state_digest(frame);
        PADStatus pads[4];
        if (!digest || nativeRollbackInput_ConfirmedMask(&netplay.inputs, frame) != 0xF ||
            !nativeRollbackInput_GetFrame(&netplay.inputs, frame, pads) ||
            !nativeMatchStream_AppendFrame(stream, frame, pads, digest->value)) {
            stream->unavailable = true;
            OSReport("[match-stream] Input history unavailable at frame %llu; players continue.\n",
                     (unsigned long long)frame);
            return;
        }
    }
}

static bool publish_confirmed_digests(void)
{
    // Receiving a late input can make an earlier predicted frame fully
    // confirmed while also scheduling that frame for correction. Its stored
    // digest still describes the predicted timeline until EndFrame restores
    // and resimulates it, so do not advertise any newly confirmed state yet.
    if (netplay.current_frame == 0 || netplay.inputs.has_correction) {
        return true;
    }
    record_confirmed_stream();
    uint64_t frame =
        netplay.has_published_digest ? netplay.published_digest_frame + 1 : 0;
    if (!netplay.checkpoint.boundary) return true;
    uint64_t end = netplay.checkpoint.boundary - 1;
    for (; frame <= end; ++frame) {
        NativeNetplayStateDigest* digest = find_state_digest(frame);
        if (digest == NULL || nativeRollbackInput_ConfirmedMask(
                                  &netplay.inputs, frame) != 0xF)
        {
            break;
        }
        if (!nativeNetplayExchange_SetChecksum(netplay.exchange, frame,
                                               digest->value))
        {
            fail("could not publish a confirmed state checksum");
            return false;
        }
        netplay.published_digest_frame = frame;
        netplay.has_published_digest = true;
    }
    return true;
}

static bool compare_peer_digests(void)
{
    if (netplay.exchange->checksum_overflow) {
        fail("peer checksum history overflowed before comparison");
        return false;
    }
    // The newest confirmed remote input can schedule restoration after this
    // frame. Until that happens, local history still contains predicted state
    // and must not be compared with a peer's canonical checksum.
    if (netplay.inputs.has_correction || netplay.checkpoint.replaying) {
        return true;
    }
    for (uint32_t index = 0; index < 4; ++index) {
        NativeNetplayExchangePeer* peer = &netplay.exchange->peers[index];
        if (!peer->configured) {
            continue;
        }
        for (uint32_t slot = 0; slot < NATIVE_ROLLBACK_INPUT_CAPACITY; ++slot) {
            NativeNetplayPendingChecksum* pending =
                &peer->pending_checksums[slot];
            if (!pending->valid) {
                continue;
            }
            NativeNetplayStateDigest* local =
                find_state_digest(pending->frame);
            if (local == NULL ||
                nativeRollbackInput_ConfirmedMask(&netplay.inputs,
                                                  pending->frame) != 0xF)
            {
                if (netplay.current_frame > pending->frame &&
                    netplay.current_frame - pending->frame >
                        NATIVE_ROLLBACK_INPUT_CAPACITY)
                {
                    fail("peer checksum expired before it could be compared");
                    return false;
                }
                continue;
            }
            if (pending->frame >= netplay.checkpoint.boundary) continue;
            if (local->value != pending->value) {
                OSReport("[netplay] Checksum mismatch with peer %u at frame %llu: "
                         "local %016llx, remote %016llx.\n",
                         index, (unsigned long long) pending->frame,
                         (unsigned long long) local->value,
                         (unsigned long long) pending->value);
                nativeReplay_WriteStateDigestSegments(stderr, pending->frame, false);
                netplay.has_mismatch = true;
                netplay.mismatch_frame = pending->frame;
                netplay.mismatch_peer_digest = pending->value;
                fail("peer semantic checksum mismatch");
                return false;
            }
            pending->valid = false;
            ++netplay.compared_digest_count;
        }
    }
    return true;
}

static bool remote_inputs_ready(uint64_t frame)
{
    uint8_t mask=(uint8_t)(~netplay.inputs.local_mask & 0xF);
    if (!mask) return true;
    uint64_t prefix;
    if (!nativeRollbackInput_ConfirmedBoundary(&netplay.inputs,
            netplay.checkpoint.boundary,frame,mask,&prefix)) return false;
    bool actual=(nativeRollbackInput_ConfirmedMask(&netplay.inputs,frame)&mask)==mask;
    if (prefix==frame && actual) return true;
    // Exchange initial inputs before predicting. Thereafter bound prediction
    // from the contiguous remote prefix, never the newest packet timestamp.
    return frame && frame-prefix<NativeConfirmedCheckpoint_MaxPrediction;
}

static bool wait_for_remote_inputs(uint64_t frame)
{
    if (remote_inputs_ready(frame)) {
        return true;
    }
    uint64_t started = GetTickCount64();
    uint64_t deadline = started + NETPLAY_INPUT_TIMEOUT_MS;
    ++netplay.input_stall_count;
    do {
        if (!receive_packets() || !send_packets()) {
            fail("input exchange failed while waiting for a peer");
            return false;
        }
        if (remote_inputs_ready(frame)) {
            netplay.input_stall_ms += GetTickCount64() - started;
            return true;
        }
        if(nativeNetplayTimingEnabled)nativeNetplayTiming_Begin(NativeNetplayTiming_NetworkWait);
        Sleep(1);
        if(nativeNetplayTimingEnabled)nativeNetplayTiming_End(NativeNetplayTiming_NetworkWait);
    } while (GetTickCount64() < deadline);
    netplay.input_stall_ms += GetTickCount64() - started;
    fail(frame == 0 ? "timed out waiting for initial peer inputs"
                    : "peer input timed out during the match");
    return false;
}

static void filter_inputs(PADStatus pads[4])
{
    if (!netplay.active || netplay.failed) {
        return;
    }
    if (!netplay.checkpoint.replaying) {
        if (netplay.borrowed_online_room) {
            // Map this PC's selected physical ports into its assigned global
            // fighter slots. Empty/CPU slots do not consume a controller.
            PADStatus local[4] = { 0 };
            for (unsigned port = 0; port < 4; ++port) {
                local[port].err = PAD_ERR_NO_CONTROLLER;
                if (netplay.physical_ports[port] >= 0) {
                    local[port] = pads[netplay.physical_ports[port]];
                }
            }
            memcpy(pads, local, sizeof(local));
        }
        if (!receive_packets() ||
            !nativeNetplayExchange_RecordLocal(netplay.exchange,
                                               netplay.current_frame, pads) ||
            !publish_confirmed_digests() || !send_packets())
        {
            fail("netplay input exchange failed");
            return;
        }
        if (!wait_for_remote_inputs(netplay.current_frame)) {
            return;
        }
        if (!compare_peer_digests()) {
            return;
        }
    }
    PADStatus resolved[4];
    bool available=netplay.checkpoint.replaying ?
        nativeRollbackInput_GetFrame(&netplay.inputs,netplay.current_frame,resolved) :
        nativeRollbackInput_PrepareFrame(&netplay.inputs,netplay.current_frame,pads,resolved);
    if (!available) {
        fail("input history could not provide the checkpoint tick");
        return;
    }
    memcpy(pads, resolved, sizeof(resolved));
#ifdef NATIVE_NETPLAY_DIAGNOSTICS
    memcpy(netplay.consumed, resolved, sizeof(resolved));
    netplay.confirmed_at_tick =
        nativeRollbackInput_ConfirmedMask(&netplay.inputs, netplay.current_frame);
    netplay.has_consumed = true;
#endif
}

static uint8_t local_human_count(const NativeLobbyMatch* match,
                                 uint8_t port_mask)
{
    uint8_t count = 0;
    for (uint32_t port = 0; port < 4; ++port) {
        count += (port_mask & (1U << port)) != 0 &&
                 match->slots[port].kind == NativeLobbyMatchSlot_Human;
    }
    return count;
}

static bool init_direct_transport(const NativeNetplayLaunchConfig* config)
{
    if (!nativeNetplayUdp_Init(&netplay.udp, config->listen_port,
                               config->session_id, config->local_peer,
                               config->local_port_mask))
    {
        return false;
    }
    for (uint32_t i = 0; i < config->peer_count; ++i) {
        const NativeNetplayPeerLaunch* peer = &config->peers[i];
        if (!nativeNetplayUdp_AddPeer(&netplay.udp, peer->peer,
                                      peer->ipv4_address, peer->port,
                                      peer->port_mask))
        {
            nativeNetplayUdp_Shutdown(&netplay.udp);
            return false;
        }
    }
    if (!nativeNetplayExchange_SetArtificialDelay(
            &netplay.udp.exchange, config->artificial_delay_frames))
    {
        nativeNetplayUdp_Shutdown(&netplay.udp);
        return false;
    }
    netplay.exchange = &netplay.udp.exchange;
    return true;
}

static bool init_room_transport(const NativeNetplayLaunchConfig* config)
{
    NativeLobbyMatch match;
    if (!nativeMatchLaunch_CreateLobbyMatch(&match)) {
        return false;
    }
    NativeNetplayConnectionConfig connection = {
        .signaling = {
            .endpoint = config->room.signaling_endpoint,
            .room_id = config->room.room_id,
            .peer_id = config->room.local_peer_id,
            .name = config->room.local_name,
            .build = config->room.build,
            .local_players =
                local_human_count(&match, config->local_port_mask),
        },
        .ice = {
            .stun_host = config->room.stun_host[0] != '\0'
                             ? config->room.stun_host
                             : NULL,
            .stun_port = config->room.stun_port,
        },
    };
    netplay.room = &netplay.owned_room;
    if (!nativeNetplayRoom_Connect(netplay.room, &connection) ||
        !nativeNetplayRoom_ConfigureMatch(netplay.room, &match,
                                          config->artificial_delay_frames))
    {
        nativeNetplayRoom_Shutdown(netplay.room);
        return false;
    }
    uint64_t deadline = GetTickCount64() + NETPLAY_ROOM_SETUP_TIMEOUT_MS;
    while (!netplay.room->ready && GetTickCount64() < deadline) {
        if (!nativeNetplayRoom_PumpSetup(netplay.room)) {
            nativeNetplayRoom_Shutdown(netplay.room);
            return false;
        }
        Sleep(5);
    }
    if (!netplay.room->ready) {
        nativeNetplayRoom_Shutdown(netplay.room);
        return false;
    }
    if (!nativeMatchLaunch_AdoptLobbyRoom(netplay.room)) {
        nativeNetplayRoom_Shutdown(netplay.room);
        return false;
    }
    netplay.room_transport = true;
    netplay.exchange = &netplay.room->exchange;
    return true;
}

bool nativeNetplayMatch_Prepare(bool sudden_death)
{
    const NativeNetplayLaunchConfig* config = nativeMatchLaunch_Netplay();
    if (config == NULL) {
        return false;
    }
    if (sudden_death) {
        if (!netplay.prepared || !netplay.sudden_death_pending ||
            !nativeNetplayExchange_BeginPhase(
                netplay.exchange,
                config->session_id ^ sudden_death_session_tag)) {
            fail("could not preserve netplay for sudden death");
            return false;
        }
        netplay.sudden_death_pending = false;
        netplay.current_frame = 0;
        netplay.published_digest_frame = 0;
        netplay.correction_count = 0;
        netplay.resimulated_frame_count = 0;
        netplay.input_stall_count = 0;
        netplay.compared_digest_count = 0;
        netplay.input_stall_ms = 0;
        netplay.has_published_digest = false;
        netplay.initial_seed = *seed_ptr;
        memset(netplay.replay_boundaries, 0,
               sizeof(netplay.replay_boundaries));
        memset(netplay.state_digests, 0, sizeof(netplay.state_digests));
        OSReport("[netplay] Continuing session %016llx in sudden death.\n",
                 (unsigned long long) config->session_id);
        return true;
    }
    netplay = (struct NativeNetplayMatch){ 0 };
    netplay.replay_playback = nativeMatchLaunch_ReplayPlayback();
    NativeNetplayRoom* online_room = nativeOnlineSession_ClaimMatchRoom();
    if (online_room != NULL) {
        netplay.room = online_room;
        netplay.room_transport = true;
        netplay.borrowed_online_room = true;
        netplay.exchange = &online_room->exchange;
    } else if (!(config->room.enabled ? init_room_transport(config)
                                      : init_direct_transport(config)))
    {
        fail(config->room.enabled ? "could not establish room ICE routes"
                                  : "could not configure direct UDP netplay");
        return false;
    }
    netplay.prepared = true;
    uint64_t seed_material =
        config->session_id ^ nativeMatchLaunch_Signature();
    seed_material ^= seed_material >> 30;
    seed_material *= UINT64_C(0xBF58476D1CE4E5B9);
    seed_material ^= seed_material >> 27;
    seed_material *= UINT64_C(0x94D049BB133111EB);
    seed_material ^= seed_material >> 31;
    *seed_ptr = (uint32_t) (seed_material ^ (seed_material >> 32));
    if (*seed_ptr == 0) {
        *seed_ptr = 1;
    }
    netplay.initial_seed = *seed_ptr;
    OSReport("[netplay] Agreed initial RNG seed %08x.\n",
             (unsigned) *seed_ptr);
    if(netplay.room_transport &&
       !nativeNetplayRoom_RetainMatchPackages(netplay.room))
        OSReport("[room-cache] Could not retain selected packages; players continue.\n");
    if (netplay.room_transport && config->local_peer == 0) {
        free(netplay.room->stream_storage);
        netplay.room->stream = (NativeMatchStream){0};
        netplay.room->stream_storage = malloc((size_t)NATIVE_MATCH_STREAM_RECORD_CAPACITY *
                                               NATIVE_MATCH_STREAM_RECORD_BYTES);
        NativeMatchStreamDescription description = {
            .match = netplay.room->match, .initial_seed = *seed_ptr,
        };
        if (!netplay.room->stream_storage ||
            !nativeMatchStream_Bind(&netplay.room->stream, &description,
                netplay.room->stream_storage, NATIVE_MATCH_STREAM_RECORD_CAPACITY)) {
            free(netplay.room->stream_storage);
            netplay.room->stream_storage = NULL;
            OSReport("[match-stream] Could not reserve viewing history; players continue.\n");
        }
    }
    return true;
}

bool nativeNetplayMatch_Init(void)
{
    const NativeNetplayLaunchConfig* config = nativeMatchLaunch_Netplay();
    if (config == NULL || !netplay.prepared) {
        return false;
    }
    nativeNetplayTiming_Init();
    nativeCallbackProfile_Init();
    netplay.visible_tick_pending=false;
    const char* checksum_trace=getenv("FREAK_MELEE_NETPLAY_CHECKSUM_TRACE");
    netplay.checksum_trace=checksum_trace && !strcmp(checksum_trace,"1");
    const char* probe_owners=getenv("FREAK_MELEE_NETPLAY_CHECKPOINT_OWNER_PROBE");
    netplay.probe_owners=probe_owners && !strcmp(probe_owners,"1");
    if (!nativeRollbackInput_Init(&netplay.inputs, config->local_port_mask))
    {
        fail("could not initialize checkpoint input history");
        shutdown_transport();
        return false;
    }
    for (unsigned port = 0; port < 4; ++port) {
        netplay.physical_ports[port] = -1;
        if ((config->local_port_mask & (1U << port)) == 0) {
            continue;
        }
        if (netplay.borrowed_online_room) {
            if (netplay.room->match.slots[port].kind !=
                NativeLobbyMatchSlot_Human) {
                // The topology also assigns empty/CPU slots to a peer so it
                // can send their neutral inputs. They do not have motors or
                // physical controllers, and do not consume a local port.
                continue;
            }
            netplay.physical_ports[port] =
                nativeHome_PhysicalPortForLocalPlayer(
                    config->local_seats[port]);
            if (netplay.physical_ports[port] < 0) {
                fail("local controller mapping is incomplete");
                shutdown_transport();
                return false;
            }
        } else {
            netplay.physical_ports[port] = (s8) port;
        }
    }
    if (!HSD_RumbleOutputSetPortMapping(netplay.physical_ports)) {
        fail("local controller mapping is incomplete or duplicated");
        shutdown_transport();
        return false;
    }
    if (netplay.replay_playback) {
        nativeReplay_SetTimelineValidation(false);
    }
    HSD_PadSetRawFilter(filter_inputs);
    // Sudden death retains live objects but starts a new history admission.
    // Renew leases so obsolete spans cannot restore into this phase.
    if((nativeHsdArena.sealed && !nativeHsdArena_ResetHistory()) ||
       (nativeMatchSceneArena.sealed && !nativeMatchScene_ResetHistory())) {
        HSD_PadSetRawFilter(NULL);shutdown_transport();
        fail("cannot renew match history leases");return false;
    }
    void* scratch=malloc(NETPLAY_ADMISSION_SCRATCH_BYTES);
    bool admitted=scratch && nativeMatchState_Admit(scratch,NETPLAY_ADMISSION_SCRATCH_BYTES);
    free(scratch);
    NativeMatchArena* arenas[]={&nativeHsdArena,&nativeMatchSceneArena};
    size_t bytes=admitted ? nativeConfirmedCheckpoint_Bytes(arenas,2) : 0;
    netplay.checkpoint_backing=bytes ? calloc(1,bytes) : NULL;
    if (!netplay.checkpoint_backing ||
        !nativeConfirmedCheckpoint_Bind(&netplay.checkpoint,arenas,2,
            netplay.checkpoint_backing,bytes,nativeGObjPool.lifetime)) {
        fail("could not admit and bind the confirmed match checkpoint");
        HSD_PadSetRawFilter(NULL);
        free(netplay.checkpoint_backing);netplay.checkpoint_backing=NULL;
        shutdown_transport();return false;
    }
    HSD_AudioRollbackReset();
    if (netplay.checksum_trace && netplay.probe_owners) {
        netplay.probe_before=malloc(NETPLAY_ADMISSION_SCRATCH_BYTES);
        netplay.probe_after=malloc(NETPLAY_ADMISSION_SCRATCH_BYTES);
        if (!netplay.probe_before || !netplay.probe_after) {
            free(netplay.probe_before);free(netplay.probe_after);
            netplay.probe_before=netplay.probe_after=NULL;
            free(netplay.checkpoint_backing);netplay.checkpoint_backing=NULL;
            HSD_PadSetRawFilter(NULL);
            shutdown_transport();
            fail("cannot reserve checkpoint diagnostic scratch");return false;
        }
    }
    OSReport("[netplay] Confirmed checkpoint: %zu reserved bytes, %zu copied bytes, no page tracking.\n",
        bytes,netplay.checkpoint.peak_bytes);
    netplay.active = true;
    nativeReplay_SetStateTraceEnabled(true);
    OSReport("[netplay] Diagnostic outbound delay %u frames.\n",
             (unsigned)netplay.exchange->artificial_delay_frames);
    if (netplay.room_transport) {
        OSReport("[netplay] Session %016llx connected through room %s as "
                 "peer %u with port mask 0x%x; signaling %s.\n",
                 (unsigned long long) config->session_id, config->room.room_id,
                 config->local_peer, config->local_port_mask,
                 netplay.borrowed_online_room ? "retained" : "released");
    } else {
        OSReport("[netplay] Session %016llx listening on UDP %u as peer %u "
                 "with port mask 0x%x.\n",
                 (unsigned long long) config->session_id,
                 nativeNetplayUdp_BoundPort(&netplay.udp), config->local_peer,
                 config->local_port_mask);
    }
    return true;
}

#ifdef NATIVE_NETPLAY_DIAGNOSTICS
static void report_pads(FILE* report, const PADStatus pads[NATIVE_ROLLBACK_INPUT_PORTS])
{
    for (unsigned port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
        uint8_t bytes[NATIVE_PAD_WIRE_BYTES];
        nativePadWire_Write(bytes, &pads[port]);
        fputc(' ', report);
        for (unsigned i = 0; i < sizeof(bytes); ++i) fprintf(report, "%02x", bytes[i]);
    }
    fputc('\n', report);
}

static void report_state_history(FILE* report, uint64_t start, uint64_t end)
{
    if (start == end) return;
    uint64_t focus = end - 1;
    if (netplay.has_mismatch && netplay.mismatch_frame >= start &&
        netplay.mismatch_frame < end) {
        focus = netplay.mismatch_frame;
    } else if (netplay.checkpoint.boundary > start &&
               netplay.checkpoint.boundary <= end) {
        // A peer that stopped first can leave us with a tail of predicted ticks.
        focus = netplay.checkpoint.boundary - 1;
    }
    fprintf(report, "[trace-focus] %llu\n", (unsigned long long)focus);
    if (netplay.has_mismatch && focus == netplay.mismatch_frame) {
        nativeReplay_WriteStateDigestSegments(report, focus, false);
    }
    // Write the same summaries on both peers, starting near the suspected frame.
    for (uint64_t distance = 0; distance < end - start; ++distance) {
        long position = ftell(report);
        if (position < 0 || position >= NativeNetplayDiagnostics_MaxBytes / 2) break;
        if (distance <= focus - start) {
            uint64_t frame = focus - distance;
            if (find_state_digest(frame) != NULL)
                nativeReplay_WriteStateDigestSegments(report, frame, true);
        }
        if (distance && distance < end - focus && find_state_digest(focus + distance)) {
            nativeReplay_WriteStateDigestSegments(report, focus + distance, true);
        }
    }
    nativeReplayCapsuleTrace_WriteColumns(report);
    long position = ftell(report);
    if (position < 0 || position >= NativeNetplayDiagnostics_MaxBytes - 80) return;
    size_t remaining = NativeNetplayDiagnostics_MaxBytes - (size_t)position - 80;
    for (uint64_t distance = 0; distance < end - start; ++distance) {
        if (distance <= focus - start && find_state_digest(focus - distance) &&
            !nativeReplay_WriteCapsuleTrace(report, focus - distance, &remaining)) {
            fputs("[capsule-limit] further capsule rows omitted\n", report);
            break;
        }
        if (distance && distance < end - focus && find_state_digest(focus + distance) &&
            !nativeReplay_WriteCapsuleTrace(report, focus + distance, &remaining)) {
            fputs("[capsule-limit] further capsule rows omitted\n", report);
            break;
        }
    }
}

static void report_failure(void)
{
    if (!netplay.active || !netplay.failed || netplay.exchange == NULL) return;
    uint64_t session = netplay.exchange->session_id;
    unsigned peer = netplay.exchange->local_peer;
    FILE* report = nativeNetplayDiagnostics_Open(session, peer);
    if (report == NULL) return;
    uint64_t cursor = netplay.checkpoint.cursor;
    fprintf(report, "session=%016llx peer=%u port_mask=%x reason=%s\n"
            "signature=%016llx seed=%08x frame=%llu checkpoint=%llu "
            "received=%u rejected=%u compared=%u restores=%u replay_ticks=%u\n",
            (unsigned long long)session, peer, netplay.inputs.local_mask,
            netplay.failure_reason, (unsigned long long)nativeMatchLaunch_Signature(),
            netplay.initial_seed, (unsigned long long)cursor,
            (unsigned long long)netplay.checkpoint.boundary,
            netplay.exchange->received_packets, netplay.exchange->rejected_packets,
            netplay.compared_digest_count, netplay.correction_count, netplay.resimulated_frame_count);
    if (netplay.has_mismatch) {
        fprintf(report, "mismatch_frame=%llu remote_digest=%016llx\n",
                (unsigned long long)netplay.mismatch_frame,
                (unsigned long long)netplay.mismatch_peer_digest);
    }
    if (netplay.room_transport && netplay.room != NULL) {
        const NativeLobbyMatch* match = &netplay.room->match;
        fprintf(report, "stage=%llu mode=%u stocks=%u time=%u stock_time=%u "
                "items=%d item_mask=%016llx teams=%u friendly_fire=%u\n",
                (unsigned long long)match->stage_id, match->mode, match->stock_count,
                match->time_limit_minutes, match->stock_time_limit_minutes, match->item_frequency,
                (unsigned long long)match->item_mask, match->teams, match->friendly_fire);
        for (unsigned port = 0; port < NATIVE_ROLLBACK_INPUT_PORTS; ++port) {
            const NativeLobbyMatchSlot* slot = &match->slots[port];
            char fighter[65], costume[65];
            nativeContentStore_IdHex(&slot->character_package_id, fighter);
            nativeContentStore_IdHex(&slot->costume_package_id, costume);
            fprintf(report, "[slot] %u kind=%u character=%016llx costume=%016llx fighter_package=%s costume_package=%s\n",
                    port, slot->kind, (unsigned long long)slot->character_id,
                    (unsigned long long)slot->costume_id, fighter, costume);
        }
    }
    fputs("[fighter-state] columns: frame actor port kind action movement input collision "
          "animation hitboxes damage script attachments pose relations\n", report);
    uint64_t start = cursor > NATIVE_ROLLBACK_INPUT_CAPACITY
                         ? cursor - NATIVE_ROLLBACK_INPUT_CAPACITY : 0;
    for (uint64_t frame = start; frame < cursor; ++frame) {
        const NativeNetplayStateDigest* digest = find_state_digest(frame);
        if (digest == NULL) continue;
        fprintf(report, "[digest] %llu %016llx\n", (unsigned long long)frame,
                (unsigned long long)digest->value);
        fprintf(report, "[tick] %llu rng=%08x checkpoint=%llu pass=%u "
                "simulations=%u replay=%u first_digest=%016llx\n",
                (unsigned long long)frame, digest->rng,
                (unsigned long long)digest->checkpoint, digest->pass,
                digest->simulations, digest->resimulated,
                (unsigned long long)digest->first_digest);
        if (digest->has_consumed) {
            fprintf(report, "[consumed] %llu confirmed=%x",
                    (unsigned long long)frame, digest->confirmed_at_tick);
            report_pads(report, digest->consumed);
        }
        PADStatus pads[NATIVE_ROLLBACK_INPUT_PORTS];
        if (nativeRollbackInput_GetFrame(&netplay.inputs, frame, pads)) {
            fprintf(report, "[inputs] %llu confirmed=%x", (unsigned long long)frame,
                    nativeRollbackInput_ConfirmedMask(&netplay.inputs, frame));
            report_pads(report, pads);
        }
    }
    report_state_history(report, start, cursor);
    nativeNetplayDiagnostics_Send(report, session, peer);
}
#endif

void nativeNetplayMatch_Shutdown(bool continue_into_sudden_death,
                                 const struct MatchEnd* result)
{
    const uint32_t received_packets = netplay.exchange != NULL
                                          ? netplay.exchange->received_packets : 0;
    const uint32_t rejected_packets = netplay.exchange != NULL
                                          ? netplay.exchange->rejected_packets : 0;
    const uint32_t rejected_header_packets =
        netplay.exchange != NULL ? netplay.exchange->rejected_header_packets : 0;
    const uint32_t rejected_checksum_packets =
        netplay.exchange != NULL ? netplay.exchange->rejected_checksum_packets : 0;
    const uint32_t rejected_input_packets =
        netplay.exchange != NULL ? netplay.exchange->rejected_input_packets : 0;
    const uint32_t stale_packets = netplay.exchange != NULL
                                       ? netplay.exchange->stale_packets : 0;
    HSD_PadSetRawFilter(NULL);
#ifdef NATIVE_NETPLAY_DIAGNOSTICS
    report_failure();
#endif
    if (netplay.replay_playback) {
        nativeReplay_SetTimelineValidation(true);
    }
    bool keep_transport = continue_into_sudden_death && netplay.active &&
                          !netplay.failed && netplay.prepared;
    if (netplay.room_transport && netplay.room && netplay.room->stream.records) {
        NativeMatchStream* stream = &netplay.room->stream;
        if (netplay.failed || !netplay.active) stream->unavailable = true;
        else {
            record_confirmed_stream();
            if (stream->next_frame != netplay.checkpoint.cursor ||
                !nativeMatchStream_EndPhase(stream, keep_transport,
                    semantic_state_digest(netplay.checkpoint.cursor))) {
                stream->unavailable = true;
            }
        }
    }
    if (!keep_transport) {
        if (netplay.borrowed_online_room && netplay.prepared &&
            !nativeOnlineSession_ReportMatch(netplay.failed ? NULL : result)) {
            OSReport("[netplay] Could not queue the room match result.\n");
        }
        shutdown_transport();
    }
    HSD_AudioRollbackAbortFrame();netplay.frame_active=false;
    netplay.visible_tick_pending=false;
    if (netplay.rumble_suppressed) {
        HSD_PadRumbleEndResimulation(netplay.inputs.local_mask);netplay.rumble_suppressed=false;
    }
    free(netplay.checkpoint_backing);netplay.checkpoint_backing=NULL;
    nativeRandom_TraceFrame(0,false,false);
    free(netplay.probe_before);free(netplay.probe_after);
    netplay.probe_before=NULL;netplay.probe_after=NULL;
    netplay.checkpoint.valid=false;
    if (!keep_transport && !netplay.borrowed_online_room) {
        const s8 identity[4] = { 0, 1, 2, 3 };
        HSD_RumbleOutputSetPortMapping(identity);
    }
    if (netplay.active) {
        OSReport(
            "[netplay] Received %u input packets; rejected %u.\n",
            received_packets, rejected_packets);
        if (rejected_packets != 0) {
            OSReport("[netplay] Rejections: header %u, checksum %u, input %u.\n",
                     rejected_header_packets, rejected_checksum_packets,
                     rejected_input_packets);
        }
        if (stale_packets != 0) {
            OSReport("[netplay] Ignored %u old-scene input packets.\n",
                     stale_packets);
        }
        OSReport("[netplay] Corrected %u timelines across %u hidden frames.\n",
                 netplay.correction_count, netplay.resimulated_frame_count);
        OSReport("[netplay] Checkpoint saves %llu, restores %llu, replay ticks %llu, peak %zu bytes.\n",
            (unsigned long long)netplay.checkpoint.captures,
            (unsigned long long)netplay.checkpoint.restores,
            (unsigned long long)netplay.checkpoint.replay_ticks,netplay.checkpoint.peak_bytes);
        OSReport("[netplay] Waited %llu ms across %u input stalls.\n",
                 (unsigned long long) netplay.input_stall_ms,
                 netplay.input_stall_count);
        OSReport("[netplay] Compared %u confirmed peer checksums.\n",
                 netplay.compared_digest_count);
        if (netplay.replay_playback) {
            uint64_t sample = nativeReplay_SampleCount();
            OSReport("[netplay] Endpoint sample %llu: fighter digest %016llx, "
                     "RNG %08x.\n",
                     (unsigned long long) sample,
                     (unsigned long long) nativeReplay_StateDigest(sample),
                     (unsigned) *seed_ptr);
        } else {
            OSReport("[netplay] Endpoint frame %llu: fighter digest %016llx, RNG %08x.\n",
                (unsigned long long)netplay.checkpoint.cursor,
                (unsigned long long)nativeReplay_StateDigest(netplay.checkpoint.cursor),
                (unsigned)*seed_ptr);
        }
    }
    nativeNetplayTiming_Report();
    nativeCallbackProfile_Report();
    nativeReplay_SetStateTraceEnabled(false);
    netplay.active = false;
    netplay.sudden_death_pending = keep_transport;
    OSReport("[netplay] Match phase closed: continue=%u.\n",keep_transport ? 1u : 0u);
}

static NativeConfirmedCheckpointStep reconcile_inputs(uint64_t* frame)
{
    bool was_replaying=netplay.checkpoint.replaying;
    if (!was_replaying && netplay.checksum_trace && !netplay.inputs.has_correction) {
        uint64_t confirmed;size_t bytes;
        if (nativeRollbackInput_ConfirmedBoundary(&netplay.inputs,netplay.checkpoint.boundary,
                netplay.checkpoint.cursor,0xF,&confirmed) &&
            confirmed>netplay.checkpoint.boundary && confirmed<netplay.checkpoint.cursor) {
            netplay.probe_frame=netplay.checkpoint.cursor;
            netplay.probe_digest=semantic_state_digest(netplay.probe_frame-1);
            netplay.probe_active=true;
            netplay.probe_captured=netplay.probe_owners && nativeMatchSnapshot_Capture(netplay.probe_parts,
                netplay.probe_before,NETPLAY_ADMISSION_SCRATCH_BYTES,&bytes,NULL);
            if (netplay.probe_owners && !netplay.probe_captured) {
                for(unsigned i=0;i<NativeMatchSnapshot_Count;++i)
                    if (!netplay.probe_parts[i].validated)
                        OSReport("[checkpoint-probe] baseline-refused frame=%llu owner=%s bytes=%zu\n",
                            (unsigned long long)netplay.probe_frame,
                            netplay.probe_parts[i].name ? netplay.probe_parts[i].name : "missing",
                            netplay.probe_parts[i].bytes);
            }
        }
    }
    uint64_t captures=0, restores=0;
    if(nativeNetplayTimingEnabled) {
        captures=netplay.checkpoint.captures;
        restores=netplay.checkpoint.restores;
        nativeNetplayTiming_Begin(NativeNetplayTiming_Checkpoint);
    }
    NativeConfirmedCheckpointStep step=nativeConfirmedCheckpoint_Reconcile(
        &netplay.checkpoint,&netplay.inputs,nativeGObjPool.lifetime,frame);
    if(nativeNetplayTimingEnabled) {
        nativeNetplayTiming_End(NativeNetplayTiming_Checkpoint);
        nativeNetplayTiming_Checkpoints(netplay.checkpoint.captures-captures,
            netplay.checkpoint.restores-restores,netplay.checkpoint.peak_bytes);
    }
    if (step==NativeConfirmedCheckpoint_Failed) {
        fail("confirmed checkpoint reconciliation failed");return step;
    }
    if (!was_replaying && step==NativeConfirmedCheckpoint_Replaying) {
        ++netplay.correction_count;
        for (unsigned i=0;i<NATIVE_ROLLBACK_INPUT_CAPACITY;++i)
            if (netplay.state_digests[i].valid && netplay.state_digests[i].frame>=*frame)
                netplay.state_digests[i].valid=false;
        if (!netplay.rumble_suppressed) {
            HSD_PadRumbleBeginResimulation();netplay.rumble_suppressed=true;
        }
        if (netplay.replay_playback) {
            NativeNetplayReplayBoundary* boundary=&netplay.replay_boundaries[
                *frame%NATIVE_ROLLBACK_INPUT_CAPACITY];
            if (!boundary->valid || boundary->frame!=*frame ||
                !nativeReplay_RewindToSample(boundary->replay_sample)) {
                fail("could not rewind replay to the confirmed checkpoint");
                return NativeConfirmedCheckpoint_Failed;
            }
        }
    }
    record_confirmed_stream();
    return step;
}

bool nativeNetplayMatch_BeginFrame(uint64_t* frame)
{
    if (!frame || !netplay.active || netplay.failed || netplay.frame_active) return false;
    if (!netplay.checkpoint.replaying) {
        // A reconstruction has already reconciled the current packet batch.
        // Give the normal tick its turn before starting another reconstruction.
        // Normal input sampling still receives packets; an exhausted prediction
        // window below must still wait/reconcile rather than advance unsafely.
        if (!netplay.visible_tick_pending &&
            (!receive_packets() || reconcile_inputs(frame)==NativeConfirmedCheckpoint_Failed)) return false;
        uint64_t started=GetTickCount64();bool stalled=false;
        while (!nativeConfirmedCheckpoint_CanAdvance(&netplay.checkpoint,nativeGObjPool.lifetime)) {
            if (!stalled) {++netplay.input_stall_count;stalled=true;}
            if (!receive_packets() || !send_packets() ||
                reconcile_inputs(frame)==NativeConfirmedCheckpoint_Failed) return false;
            if (GetTickCount64()-started>=NETPLAY_INPUT_TIMEOUT_MS) {
                fail("confirmed checkpoint input window timed out");return false;
            }
            if(nativeNetplayTimingEnabled)nativeNetplayTiming_Begin(NativeNetplayTiming_NetworkWait);
            Sleep(1);
            if(nativeNetplayTimingEnabled)nativeNetplayTiming_End(NativeNetplayTiming_NetworkWait);
        }
        if (stalled)netplay.input_stall_ms+=GetTickCount64()-started;
    }
    if (!netplay.checkpoint.replaying) netplay.visible_tick_pending=false;
    if (*frame!=netplay.checkpoint.cursor) {fail("checkpoint frame cursor mismatch");return false;}
    netplay.current_frame=*frame;
#ifdef NATIVE_NETPLAY_DIAGNOSTICS
    netplay.has_consumed = false;
#endif
    nativeRandom_TraceFrame(*frame,netplay.checksum_trace,netplay.checkpoint.replaying);
    if (netplay.checkpoint.replaying)++netplay.resimulated_frame_count;
    if (netplay.replay_playback) {
        netplay.replay_boundaries[*frame%NATIVE_ROLLBACK_INPUT_CAPACITY]=
            (NativeNetplayReplayBoundary){.frame=*frame,.replay_sample=nativeReplay_SampleCount(),.valid=true};
    }
    if (!HSD_AudioRollbackBeginFrame(*frame,netplay.checkpoint.replaying)) {
        fail("could not begin checkpoint audio-intent frame");return false;
    }
    netplay.frame_active=true;
    return true;
}

NativeConfirmedCheckpointStep nativeNetplayMatch_EndFrame(uint64_t frame,uint64_t* next_frame,bool ending)
{
    if (!netplay.frame_active || frame!=netplay.current_frame || !next_frame) {
        fail("invalid checkpoint end frame");return NativeConfirmedCheckpoint_Failed;
    }
    netplay.frame_active=false;
    if (!HSD_AudioRollbackEndFrame()) {
        fail("checkpoint audio-intent history overflowed");return NativeConfirmedCheckpoint_Failed;
    }
    NativeNetplayStateDigest* digest =
        &netplay.state_digests[frame % NATIVE_ROLLBACK_INPUT_CAPACITY];
    uint64_t value = semantic_state_digest(frame);
#ifdef NATIVE_NETPLAY_DIAGNOSTICS
    if (digest->frame != frame || digest->simulations == 0) {
        digest->first_digest = value;
        digest->simulations = 0;
    }
    ++digest->simulations;
    digest->checkpoint = netplay.checkpoint.boundary;
    digest->rng = *seed_ptr;
    digest->pass = netplay.correction_count;
    digest->resimulated = netplay.checkpoint.replaying;
    digest->has_consumed = netplay.has_consumed;
    if (netplay.has_consumed) {
        memcpy(digest->consumed, netplay.consumed, sizeof(digest->consumed));
        digest->confirmed_at_tick = netplay.confirmed_at_tick;
    }
#endif
    digest->frame = frame;
    digest->value = value;
    digest->valid = true;
    if (netplay.checksum_trace) {
        if (frame == 0) nativeReplay_WriteStateDigestSegments(stderr, frame, false);
        for(uint32_t slot=0;slot<nativeGObjPool.initialized_count;++slot)
            if(nativeGObjPool.slots[slot].occupied)
                nativeRandom_TraceBytes("gobj",nativeGObjPool.payload+slot*nativeGObjPool.stride,nativeGObjPool.object_size);
        const VsSceneController* c=gmVs_GetController_0();
        uint32_t values[]={c->unk_0,(uint32_t)c->pauser,c->pause_timer,c->unpause_timer,
            c->terminate_match,c->match_result,c->match_over,c->frame_count,
            c->timer_seconds,c->unk_2C,c->unk_2E};
        memcpy(digest->controller,values,sizeof(values));unsigned at=11;
        for(unsigned p=0;p<GM_MAX_PLAYERS;++p) {
            const struct VsSceneFighter* f=&c->fighters[p];
            uint32_t fields[]={f->x0,f->x1,f->slot_type,(uint32_t)f->spawn_point,
                f->x4_b0,f->x4_b1,f->x4_b2,f->x4_b3,f->x4_b4,f->x4_b5,f->x4_b6,f->x4_b7,
                f->x5,f->x6,f->x8,f->x9,f->respawn_timer,f->xB,f->xC};
            memcpy(digest->controller+at,fields,sizeof(fields));at+=19;
        }
        OSReport("[netplay-control] frame=%llu digest=%016llx rng=%08x values=",
            (unsigned long long)frame,(unsigned long long)digest->value,(unsigned)*seed_ptr);
        for(unsigned i=0;i<at;++i)OSReport("%08x,",digest->controller[i]);
        OSReport("\n");
    }
    uint64_t captures=0, restores=0;
    if(nativeNetplayTimingEnabled) {
        captures=netplay.checkpoint.captures;
        restores=netplay.checkpoint.restores;
        nativeNetplayTiming_Begin(NativeNetplayTiming_Checkpoint);
    }
    NativeConfirmedCheckpointStep step=nativeConfirmedCheckpoint_EndFrame(
        &netplay.checkpoint,&netplay.inputs,frame,nativeGObjPool.lifetime,next_frame);
    if(nativeNetplayTimingEnabled) {
        nativeNetplayTiming_End(NativeNetplayTiming_Checkpoint);
        nativeNetplayTiming_Checkpoints(netplay.checkpoint.captures-captures,
            netplay.checkpoint.restores-restores,netplay.checkpoint.peak_bytes);
    }
    if (step==NativeConfirmedCheckpoint_Failed) {
        fail("confirmed checkpoint tick failed");return step;
    }
    record_confirmed_stream();
    if (step==NativeConfirmedCheckpoint_CaughtUp && !ending)
        netplay.visible_tick_pending=true;
    if (step==NativeConfirmedCheckpoint_CaughtUp && netplay.probe_active) {
        netplay.probe_active=false;size_t bytes;
        bool captured=netplay.probe_captured && nativeMatchSnapshot_Capture(netplay.probe_compare,netplay.probe_after,
            NETPLAY_ADMISSION_SCRATCH_BYTES,&bytes,NULL);
        uint64_t digest=semantic_state_digest(netplay.checkpoint.cursor-1);
        if (digest!=netplay.probe_digest) {
            OSReport("[checkpoint-probe] frame=%llu before=%016llx after=%016llx\n",
                (unsigned long long)netplay.probe_frame,(unsigned long long)netplay.probe_digest,
                (unsigned long long)digest);
            for(unsigned i=0;captured && i<NativeMatchSnapshot_Count;++i) {
                const NativeMatchSnapshotPart *a=&netplay.probe_parts[i],*b=&netplay.probe_compare[i];
                size_t offset=0,n=a->bytes<b->bytes ? a->bytes : b->bytes;
                while(offset<n && ((const uint8_t*)a->data)[offset]==((const uint8_t*)b->data)[offset])++offset;
                if(offset<n || a->bytes!=b->bytes)
                    OSReport("[checkpoint-probe] owner=%s offset=%zu before_bytes=%zu after_bytes=%zu before_byte=%u after_byte=%u\n",
                        a->name,offset,a->bytes,b->bytes,offset<a->bytes?((const uint8_t*)a->data)[offset]:0,
                        offset<b->bytes?((const uint8_t*)b->data)[offset]:0);
            }
            fail("matching-input checkpoint replay changed gameplay state");return NativeConfirmedCheckpoint_Failed;
        }
    }
    if (ending) {
        // A predicted finish may change after correction. Do not leave this
        // scene until its inputs are actual. An earlier finish during replay
        // truncates the old catch-up target; reconsider this live boundary.
        // Artificial frame delay normally advances only when RecordLocal adds
        // another tick. Drain the final recorded inputs even when this peer
        // already has all remote inputs and can leave immediately.
        uint8_t delay=netplay.exchange->artificial_delay_frames;
        do {
            // A packet holds eight frames; jumping straight from a larger
            // delay to zero would omit the older withheld frames permanently.
            uint8_t pending=netplay.exchange->artificial_delay_frames;
            netplay.exchange->artificial_delay_frames=
                pending>NATIVE_NETPLAY_INPUT_MAX_FRAMES
                    ? pending-NATIVE_NETPLAY_INPUT_MAX_FRAMES : 0;
            if (!send_packets()) {
                netplay.exchange->artificial_delay_frames=delay;
                fail("cannot flush final match inputs");return NativeConfirmedCheckpoint_Failed;
            }
        } while (netplay.exchange->artificial_delay_frames);
        uint64_t start=GetTickCount64(),confirmed;
        for (;;) {
            if (!nativeRollbackInput_ConfirmedBoundary(&netplay.inputs,
                    netplay.checkpoint.boundary,netplay.checkpoint.cursor,0xF,&confirmed)) {
                netplay.exchange->artificial_delay_frames=delay;
                fail("match-end inputs no longer retained");return NativeConfirmedCheckpoint_Failed;
            }
            if (confirmed==netplay.checkpoint.cursor) break;
            if (!receive_packets() || !send_packets()) {
                netplay.exchange->artificial_delay_frames=delay;
                fail("match-end input exchange failed");return NativeConfirmedCheckpoint_Failed;
            }
            if (GetTickCount64()-start>=NETPLAY_INPUT_TIMEOUT_MS) {
                netplay.exchange->artificial_delay_frames=delay;
                fail("match-end confirmation timed out");return NativeConfirmedCheckpoint_Failed;
            }
            if(nativeNetplayTimingEnabled)nativeNetplayTiming_Begin(NativeNetplayTiming_NetworkWait);
            Sleep(1);
            if(nativeNetplayTimingEnabled)nativeNetplayTiming_End(NativeNetplayTiming_NetworkWait);
        }
        netplay.exchange->artificial_delay_frames=delay;
        netplay.checkpoint.replaying=false;
    }
    if (step==NativeConfirmedCheckpoint_CaughtUp && netplay.rumble_suppressed) {
        HSD_PadRumbleEndResimulation(netplay.inputs.local_mask);netplay.rumble_suppressed=false;
    }
    if (!netplay.checkpoint.replaying) {
        NativeConfirmedCheckpointStep reconciled=reconcile_inputs(next_frame);
        if (reconciled!=NativeConfirmedCheckpoint_Continue)return reconciled;
        if (ending && netplay.rumble_suppressed) {
            HSD_PadRumbleEndResimulation(netplay.inputs.local_mask);netplay.rumble_suppressed=false;
        }
    }
    return step;
}

void nativeNetplayMatch_Abort(const char* reason)
{
    fail(reason);
}

void nativeNetplayMatch_Inspect(void)
{
    fprintf(stderr,"[netplay-state] frame=%llu checkpoint=%llu cursor=%llu restores=%llu replay_ticks=%llu checksums=%u corrections=%u failed=%d input_stalls=%u wait_ms=%llu\n",
        (unsigned long long)netplay.current_frame,(unsigned long long)netplay.checkpoint.boundary,
        (unsigned long long)netplay.checkpoint.cursor,(unsigned long long)netplay.checkpoint.restores,
        (unsigned long long)netplay.checkpoint.replay_ticks,netplay.compared_digest_count,
        netplay.correction_count,netplay.failed,netplay.input_stall_count,
        (unsigned long long)netplay.input_stall_ms);
    if (!netplay.active) return;
    for (unsigned port = 0; port < 4; ++port) {
        Fighter_GObj* object = Player_GetEntityAtIndex(port, 0);
        if (!object) continue;
        const Fighter* fp = GET_FIGHTER(object);
        fprintf(stderr, "[netplay-player] port=%u physical=%d kind=%u x=%.6f y=%.6f motion=%d\n",
                port, netplay.physical_ports[port], fp->kind,
                fp->cur_pos.x, fp->cur_pos.y, fp->motion_id);
    }
    unsigned item_count = 0;
    for (HSD_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         object; object = object->next) {
        const Item* item = object->user_data;
        if (!item) continue;
        int holder = -1;
        for (unsigned port = 0; port < 4; ++port) {
            for (unsigned member = 0; member < 2; ++member) {
                Fighter_GObj* fighter = Player_GetEntityAtIndex(port, member);
                if (fighter && GET_FIGHTER(fighter)->item_gobj == object)
                    holder = (int) (port * 2 + member);
            }
        }
        const NativeScriptItemProgram* script = nativeScriptItem_Spec(object);
        fprintf(stderr, "[netplay-item] index=%u id=%u kind=%d state=%d holder=%d scripted=%u recipe=%u age=%u pos=%.6f,%.6f vel=%.6f,%.6f\n",
            item_count++, (unsigned) item->x1C, item->kind, item->msid, holder,
            script != NULL, script ? script->recipe_index : 0,
            item->kind == It_Kind_NativeHoldable ? item->xDD4_itemVar.native_holdable.age : 0,
            item->pos.x, item->pos.y, item->x40_vel.x, item->x40_vel.y);
    }
    fprintf(stderr, "[netplay-items] count=%u\n", item_count);
}

bool nativeNetplayMatch_IsResimulating(void)
{
    return netplay.checkpoint.replaying;
}

bool nativeNetplayMatch_HasFailed(void)
{
    return netplay.failed || netplay.checkpoint.failed;
}

bool nativeNetplayMatch_ConsumeLobbyReturn(void)
{
    bool pending = netplay.lobby_return_pending;
    netplay.lobby_return_pending = false;
    return pending;
}

#endif
