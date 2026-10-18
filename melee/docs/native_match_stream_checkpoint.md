# Confirmed match input history

The match authority now records the inputs needed to reconstruct an online
match. This is the recording and packet foundation for spectator viewing.
The Watch match control, transport requests and viewer playback remain open.
On a PC with no active fighters, local practice continues until its player
chooses **Watch match**.

## Recording

Each record contains the four controller inputs, phase and frame numbers,
and the semantic state digest for that frame. The description contains the
agreed match rules, exact fighter and outfit package IDs, and initial RNG seed.
Records use explicit byte fields, so they carry no process pointers or native
structure padding.

Only fully confirmed frames enter the history. Recording runs when the
confirmed checkpoint advances, including during reconstruction. Predicted
frames stay out of the history until correction has finished for those frames.
Phase markers preserve sudden-death boundaries and the final state digest.

The authority reserves 16,420,864 bytes at match preparation: one hour of
60 Hz inputs plus 64 phase markers. This storage sits outside the copied
simulation arenas. Recording appends to that buffer without per-tick allocation
or file access. A full buffer or missing history disables viewing history;
active players continue their match. The buffer survives Results and is
released when the room closes or the authority prepares its next match.

## Packets

Packets carry up to six records in 476 bytes. The receiver accepts a contiguous
prefix and exact duplicate or overlapping delivery. Gaps, conflicting records,
invalid phase transitions and malformed packet headers reject the whole packet
without changing the accepted prefix. A rolling digest covers the description
and accepted records.

The input packet and viewing-history codecs share the same 15-byte controller
encoding. Existing gameplay packet bytes remain unchanged.

## Checks

The stream and existing input-codec CTests pass. The stream fixture covers
controller fields, duplicate and overlapping packets, atomic rejection of
2,152 mutations to previously accepted packets, invalid new tails, capacity
exhaustion and sudden-death phase markers. Sudden-death scene playback still
needs a native viewer test.

`tools/review_native_room_rotation.py --require-stream` checks each completed
authority history against that match's endpoint and requires checkpoint
restores on the recording peer. `--delay-peer` and `--delay-frames` choose the
diagnostic sender and its outbound delay. The driver also checks the runtime's
reported delay in each round.

Entering home CSS used to clear the parsed diagnostic delay before room setup.
That setting now belongs to the process launch settings and survives match
transitions. Enabling the actual 14-frame delay exposed another issue: the
match-end flush sent only the newest eight withheld frames. It now drains the
withheld range in packet-sized steps before the peer leaves the match.

The 2026-10-18 native run uses two hidden, muted clients and copied source-only
fighter profiles. The guest has a verified 14-frame outbound delay in both
rounds. The first round uses Fox; the second promotes the host's waiting
controller and uses its Popo/Nana family. Both rotation policies, controller
isolation, Results and return to the lobby pass.

| Round | Final frame | Authority restores | Authority replay ticks | History records |
| --- | ---: | ---: | ---: | ---: |
| Winner out | 584 | 568 | 8,520 | 585 |
| Winner stays | 628 | 612 | 8,568 | 629 |

Each history contains one input record per frame and a final phase marker.
Its tail digest matches the final semantic state. Both peers have identical
endpoint fighter digests and RNG: `e7cbce9ec5387eea` / `1a186f27` in round one,
and `8bbc84118cf7b3dc` / `00300950` in round two. Each new match starts a fresh
history with its own session and seed.

Receipt: `.local/native-match-stream-runtime-4/receipt.json`.
Binary/source hashes and endpoint audit: the adjacent `audit.json`.
Codec checks: `.local/native-match-stream-unit-2.log`.
The first two attempts rejected the missing authority-rollback coverage; the
third reproduced the final-flush timeout. The fourth passes both rounds after
the diagnostic lifetime and flush fixes. No cross-process viewer has run yet.

## Remaining viewing work

- Request the current match description and history on a separate connection.
- Cache the exact fighter and outfit dependencies before constructing a viewer
  scene. Cached packages remain separate from installed roster content.
- Reconstruct from the initial seed and confirmed inputs, check semantic state,
  then follow the live confirmed stream. Carry sudden death and match completion
  through the same scene flow.
- Keep viewer disconnect, slow delivery and catch-up outside the active players'
  confirmation window. Measure transfer and chat activity during live matches.
- Support a room owner with no active fighter starting a match for other players.
  Current match proposal setup still requires that owner to participate.

These items require a three-client native test before the spectator gate can
be marked complete.
