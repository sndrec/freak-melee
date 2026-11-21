#include <melee/mn/native_css_participants.h>
#include <melee/pl/forward.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void participant(NativeSignalingParticipant* p, const char* peer,
                        unsigned players, unsigned active, unsigned waiting,
                        bool ready)
{
    snprintf(p->peer_id, sizeof(p->peer_id), "%s", peer);
    snprintf(p->name, sizeof(p->name), "%s", peer);
    p->local_players = players;
    p->active_mask = active;
    p->waiting_mask = waiting;
    NativeLobbySelection choice = { .player_count = players,
                                    .confirmed = ready };
    for (unsigned i = 0; i < players; ++i) {
        choice.players[i].selected = true;
        choice.players[i].character_package_id.bytes[0] = 10 + i;
    }
    p->selection_size = nativeLobbySelection_Encode(&choice, p->selection,
                                                    sizeof(p->selection));
    assert(p->selection_size);
}
int main(void)
{
    NativeCssLocalSlot local[4] = { 0 };
    for (unsigned i = 0; i < 4; ++i) {
        local[i].kind = Gm_PKind_NA;
    }
    local[2].kind = Gm_PKind_Human;
    local[2].choice.selected = true;
    local[2].choice.character_package_id.bytes[0] = 91;
    local[3].kind = Gm_PKind_Cpu;
    NativeCssParticipants shown;
    assert(nativeCssParticipants_Build(NULL, NULL, "Twilight", local, 0, false,
                                       false, &shown));
    assert(!shown.online && shown.count == 2);
    assert(shown.cards[0].physical_port == 2 &&
           shown.cards[0].canonical_port == 2);
    assert(shown.cards[1].kind == Gm_PKind_Cpu);

    NativeSignalingPresence room = { .participant_count = 4 };
    participant(&room.participants[0], "remote", 2, 3, 0, true);
    participant(&room.participants[1], "waiting", 1, 0, 1, false);
    participant(&room.participants[2], "local", 2, 2, 0, false);
    participant(&room.participants[3], "spectator", 1, 0, 0, false);
    local[3].kind = Gm_PKind_Human;
    local[3].choice.selected = true;
    local[3].choice.character_package_id.bytes[0] = 92;
    NativeSignalingPresence before = room;
    assert(nativeCssParticipants_Build(&room, "local", "Twilight", local, 0, false,
                                       false, &shown));
    assert(shown.online && shown.count == 3);
    assert(shown.cards[0].local && shown.cards[0].seat == 1);
    assert(shown.cards[0].physical_port == 3 &&
           shown.cards[0].canonical_port == 2);
    assert(shown.cards[0].choice.character_package_id.bytes[0] == 92);
    assert(!shown.cards[0].ready);
    assert(!shown.cards[1].local && shown.cards[1].canonical_port == 0);
    assert(shown.cards[1].physical_port == NATIVE_CSS_NO_PORT &&
           shown.cards[1].ready);
    assert(shown.cards[2].seat == 1 && shown.cards[2].canonical_port == 1);
    assert(!memcmp(&before, &room, sizeof(room)));
    room.participants[0].card_slots[0] = 3;
    room.participants[0].card_slots[1] = 0;
    room.participants[2].card_slots[1] = 2;
    assert(nativeCssParticipants_Build(&room, "local", "Twilight", local, 4, false,
                                       true, &shown));
    assert(shown.online && shown.count == 4);
    assert(shown.cards[0].canonical_port == 1 && !shown.cards[0].local);
    assert(shown.cards[1].kind == Gm_PKind_NA && shown.cards[1].card_slot == 1);
    assert(shown.cards[2].card_slot == 2 && shown.cards[2].local &&
           shown.cards[2].physical_port == 3);
    assert(shown.cards[3].canonical_port == 0 && !shown.cards[3].local);
    assert(nativeCssParticipants_Build(&room, "remote", "Other player", local,
                                       4, false, true, &shown));
    assert(shown.cards[0].canonical_port == 1 && shown.cards[0].local);
    assert(shown.cards[1].kind == Gm_PKind_NA);
    assert(shown.cards[2].canonical_port == 2 && !shown.cards[2].local);
    assert(shown.cards[3].canonical_port == 0 && shown.cards[3].local);
    room.participants[0].selection[0] = 0;
    assert(nativeCssParticipants_Build(&room, "local", "Twilight", local, 0, false,
                                       false, &shown));
    assert(!shown.cards[1].choice.selected && !shown.cards[1].ready);
    room.participants[1].active_mask = 1;
    assert(nativeCssParticipants_Build(&room, "local", "Twilight", local, 0, false,
                                       false, &shown) &&
           shown.count == 4);
    room.participants[3].active_mask = 1;
    assert(!nativeCssParticipants_Build(&room, "local", "Twilight", local, 0, false,
                                        false, &shown));
    room.participants[3].active_mask = 2;
    assert(!nativeCssParticipants_Build(&room, "local", "Twilight", local, 0, false,
                                        false, &shown));
    room.participant_count = 1;
    participant(&room.participants[0], "local", 1, 1, 0, false);
    assert(nativeCssParticipants_Build(&room, "local", "Twilight", local,
                                       4, true, false, &shown));
    assert(shown.online && shown.count == 4 && shown.cards[0].local);
    for (unsigned i = 1; i < 4; ++i) {
        assert(shown.cards[i].kind == Gm_PKind_NA);
        assert(shown.cards[i].canonical_port == NATIVE_CSS_NO_PORT);
        assert(!shown.cards[i].peer[0] && !strcmp(shown.cards[i].name, "Searching..."));
    }
    assert(!nativeCssParticipants_Build(&room, "local", "Twilight", local,
                                        5, true, false, &shown));
    puts("CSS participant identity and presentation checks passed.");
    return 0;
}
