#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <melee/gm/native_lobby_topology.h>

static NativeLobbyMatch example_match(void)
{
    NativeLobbyMatch match = {
        .slots = {
            { .character_id = 1, .costume_id = 1,
              .kind = NativeLobbyMatchSlot_Human, .handicap = 9 },
            { .character_id = 2, .costume_id = 1,
              .kind = NativeLobbyMatchSlot_Human, .handicap = 9 },
            { .character_id = 3, .costume_id = 1,
              .kind = NativeLobbyMatchSlot_Cpu, .cpu_level = 9,
              .handicap = 9 },
            { .kind = NativeLobbyMatchSlot_Empty },
        },
        .session_id = 1,
        .stage_id = 1,
        .mode = NativeLobbyMatchMode_Stock,
        .time_limit_minutes = 2,
        .stock_count = 4,
        .item_frequency = -1,
        .damage_ratio = 10,
        .pause = true,
    };
    match.content_digest[0] = 1;
    return match;
}

static NativeSignalingParticipant participant(
    const char* peer_id, const char* build, uint64_t join_order,
    uint8_t local_players, uint8_t active_mask, uint8_t waiting_mask)
{
    NativeSignalingParticipant result = {
        .join_order = join_order,
        .local_players = local_players,
        .active_mask = active_mask,
        .waiting_mask = waiting_mask,
    };
    strcpy(result.peer_id, peer_id);
    strcpy(result.name, peer_id);
    strcpy(result.build, build);
    return result;
}

int main(void)
{
    NativeSignalingPresence presence = {
        .participant_count = 4,
        .participants = {
            participant("peer_two", "same-build", 20, 1, 1, 0),
            participant("viewer_one", "other-build", 5, 0, 0, 0),
            participant("peer_one", "same-build", 10, 1, 1, 0),
            participant("waiting_one", "same-build", 7, 1, 0, 1),
        },
    };
    NativeLobbyMatch match = example_match();
    assert(nativeLobbyTopology_AssignMatchPeers(&match, &presence,
                                                "same-build"));
    assert(match.peer_count == 2);
    assert(strcmp(match.peers[0].peer_id, "peer_one") == 0);
    assert(match.peers[0].numeric_peer == 0);
    assert(match.peers[0].port_mask == 0xD);
    assert(strcmp(match.peers[1].peer_id, "peer_two") == 0);
    assert(match.peers[1].numeric_peer == 1);
    assert(match.peers[1].port_mask == 0x2);
    assert(nativeLobbyMatch_Validate(&match));

    NativeSignalingPresence shared = presence;
    shared.participants[2].local_players = 2;
    shared.participants[2].active_mask = 3;
    NativeLobbyMatch shared_match = example_match();
    shared_match.slots[2].kind = NativeLobbyMatchSlot_Human;
    shared_match.slots[2].cpu_level = 0;
    assert(nativeLobbyTopology_AssignMatchPeers(&shared_match, &shared,
                                                "same-build"));
    assert(shared_match.peers[0].port_mask == 0xB);
    assert(shared_match.peers[1].port_mask == 0x4);
    assert(shared_match.slots[0].local_seat == 0);
    assert(shared_match.slots[1].local_seat == 1);
    assert(shared_match.slots[2].local_seat == 0);

    shared.participants[2].active_mask = 2;
    shared.participants[2].waiting_mask = 1;
    NativeLobbyMatch mixed_match = example_match();
    assert(nativeLobbyTopology_AssignMatchPeers(&mixed_match, &shared,
                                                "same-build"));
    assert(mixed_match.slots[0].local_seat == 1);
    assert(mixed_match.slots[1].local_seat == 0);

    NativeLobbyMatch unchanged = match;
    NativeSignalingPresence invalid = presence;
    strcpy(invalid.participants[0].build, "wrong-build");
    assert(!nativeLobbyTopology_AssignMatchPeers(&unchanged, &invalid,
                                                 "same-build"));
    assert(memcmp(&unchanged, &match, sizeof(match)) == 0);

    invalid = presence;
    invalid.participants[0].local_players = 2;
    invalid.participants[0].active_mask = 3;
    unchanged = match;
    assert(!nativeLobbyTopology_AssignMatchPeers(&unchanged, &invalid,
                                                 "same-build"));
    assert(memcmp(&unchanged, &match, sizeof(match)) == 0);

    invalid = presence;
    invalid.participants[0].join_order =
        invalid.participants[2].join_order;
    unchanged = match;
    assert(!nativeLobbyTopology_AssignMatchPeers(&unchanged, &invalid,
                                                 "same-build"));
    assert(memcmp(&unchanged, &match, sizeof(match)) == 0);

    puts("room presence deterministically owns every controller port");
    return 0;
}
