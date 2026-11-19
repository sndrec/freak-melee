#include "native_css_participants.h"
#include <melee/pl/forward.h>
#include <stdio.h>
#include <string.h>

static void open_slots(NativeCssParticipants* result, unsigned expected,
                        bool searching, unsigned occupied)
{
    if (!expected) return;
    result->online = true;
    for (unsigned slot = 0; slot < expected && result->count < expected; ++slot) {
        if (occupied & (1U << slot)) continue;
        NativeCssParticipant* card = &result->cards[result->count++];
        card->kind = Gm_PKind_NA;
        card->physical_port = card->canonical_port = card->seat = NATIVE_CSS_NO_PORT;
        card->card_slot = slot;
        snprintf(card->name, sizeof(card->name), "%s", searching ? "Searching..." : "Open slot");
    }
}

bool nativeCssParticipants_Build(const NativeSignalingPresence* presence,
                                 const char* local_peer,
                                 const char* local_name,
                                 const NativeCssLocalSlot local[4],
                                 unsigned expected_players, bool searching,
                                 bool fixed_cards,
                                 NativeCssParticipants* result)
{
    if (!local || !result || expected_players > 4) {
        return false;
    }
    memset(result, 0, sizeof(*result));
    if (!presence) {
        for (unsigned port = 0; port < 4; ++port) {
            if (local[port].kind != Gm_PKind_Human &&
                local[port].kind != Gm_PKind_Cpu)
            {
                continue;
            }
            NativeCssParticipant* card = &result->cards[result->count++];
            card->choice = local[port].choice;
            card->kind = local[port].kind;
            card->physical_port = card->canonical_port = port;
            card->seat = result->count - 1;
            card->local = true;
            snprintf(card->name, sizeof(card->name), "%s",
                     card->kind == Gm_PKind_Cpu ? "CPU"
                     : local_name               ? local_name
                                                : "Player");
        }
        open_slots(result, expected_players, searching, 0);
        return true;
    }
    if (!local_peer ||
        presence->participant_count > NATIVE_SIGNALING_MAX_PARTICIPANTS)
    {
        return false;
    }
    NativeCssParticipant canonical[4] = { 0 };
    unsigned count = 0;
    unsigned occupied = 0;
    uint8_t physical[4], physical_count = 0;
    for (unsigned port = 0; port < 4; ++port) {
        if (local[port].kind == Gm_PKind_Human) {
            physical[physical_count++] = port;
        }
    }
    for (unsigned peer = 0; peer < presence->participant_count; ++peer) {
        const NativeSignalingParticipant* person =
            &presence->participants[peer];
        if (person->local_players > 4 ||
            person->active_mask & ~((1U << person->local_players) - 1))
        {
            return false;
        }
        bool is_local = strcmp(person->peer_id, local_peer) == 0;
        NativeLobbySelection selected = { 0 };
        bool decoded =
            nativeLobbySelection_Decode(&selected, person->selection,
                                        person->selection_size) &&
            selected.player_count == person->local_players;
        for (unsigned seat = 0; seat < person->local_players; ++seat) {
            if (!(person->active_mask & (1U << seat))) {
                continue;
            }
            if (count == 4) {
                return false;
            }
            NativeCssParticipant* card = &canonical[count];
            card->canonical_port = count++;
            card->card_slot = fixed_cards ? person->card_slots[seat]
                                          : card->canonical_port;
            if (card->card_slot >= 4 ||
                (fixed_cards && card->card_slot >= expected_players) ||
                occupied & (1u << card->card_slot)) return false;
            occupied |= 1u << card->card_slot;
            card->physical_port = NATIVE_CSS_NO_PORT;
            card->seat = seat;
            card->kind = Gm_PKind_Human;
            card->local = is_local;
            snprintf(card->peer, sizeof(card->peer), "%s", person->peer_id);
            snprintf(card->name, sizeof(card->name), "%s", person->name);
            if (is_local && seat < physical_count) {
                card->physical_port = physical[seat];
                card->choice = local[physical[seat]].choice;
            } else if (decoded) {
                card->choice = selected.players[seat];
            }
            card->ready = decoded && selected.confirmed;
        }
    }
    // Stable partition: each client sees its local seats first, while every
    // card retains the canonical port negotiated by the room.
    for (unsigned pass = 0; pass < 2; ++pass) {
        for (unsigned i = 0; i < count; ++i) {
            if (canonical[i].local == (pass == 0)) {
                result->cards[result->count++] = canonical[i];
            }
        }
    }
    result->online = true;
    open_slots(result, expected_players, searching, occupied);
    return true;
}
