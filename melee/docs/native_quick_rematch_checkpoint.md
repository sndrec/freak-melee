# Quick Play results and rematches

Updated: 2026-10-17. Tested locally. The stage-confirmation UI still needs design
review, and Quick Play has not been deployed.

Quick Play now keeps the same opponents after a completed match. Every PC
reports the winning side. When those reports agree, the service clears the
finished proposal, unreadies the players, and gives the losing side the next
stage choice. Players can choose their fighters again and return to practice.
A stage agreement and fresh readiness lead to another three-second countdown
and a new match in the same room.

## Result agreement

Each PC submits one result for the current match epoch. A shared PC reports once
for its local players. Singles derives the winning side from the winning port;
doubles uses the game's winning-team result. The native client reads the normal
`MatchEnd` data after play ends.

The service waits for every assigned PC. Reports for an older epoch have no
effect. A duplicate matching report is safe; a report cannot change its winner.
Conflicting reports leave the group waiting and display an error that offers
leaving the match. The service never chooses one conflicting winner itself.

An agreed no-contest unreadies the players and keeps the current stage. A
decisive result grants the pick to the losing side. The next match gets a fresh
session ID and epoch while retaining the room, admission tokens, players and
fixed rules.

Losing a signaling socket preserves the running P2P proposal. Once an end report
arrives, an incomplete group retires to its original parties. This preserves the
current match's signaling-loss policy and establishes an explicit end boundary
for retiring a group that can no longer rematch.

## Stage agreement

Stage choices belong to local seats. The service checks the sender, seat, losing
side, completed epoch and six-stage pool. A winner cannot override the losing
side's choice, including when the winner owns the room.

Every losing teammate must submit the same stage. Two players sharing a PC each
have a vote. Teammates on separate PCs vote independently. A pending vote can be
changed or withdrawn. When all losing seats agree, the service saves the chosen
stage and permits the next ready countdown. Old votes cannot alter a later round.

`nativeOnlineSession_PickQuickStage` is the native entry point for these choices.
The test command `melee_native_control room-stage <local-seat> <0..5|none>` calls
it using the live local session. Stage indices follow the existing Quick Play
stage table. `none` withdraws a pending vote.

The user-facing confirmation controls remain open for review. The tests use the
automation command; they do not establish that players can complete the stage
choice through the current menus.

## Wire and ownership

The current native build identity is `freak-melee-native-dev-quick-return`.

| Client type | Payload |
| --- | --- |
| 14 ReportQuick | current epoch u64, winning side u8; 255 means no contest |
| 15 VoteQuickStage | completed epoch u64, local seat u8, stage u8; 255 withdraws |

Quick State adds completed epoch u64, choosing side u8, and four canonical-port
vote bytes. A choosing side or vote of 255 means none. Its phases are 0 waiting,
1 countdown, 2 playing, 3 stage choice and 4 result agreement. Existing message
fields keep their meanings.

The room's SQLite tables own reports, completed epochs, canonical seat ownership
and pending votes. They survive service hibernation. `quick-round.ts` maps that
ownership to displayed votes and checks agreement. The native lobby session
waits for the service to clear the completed proposal; it skips the ordinary
host's clear-match command for Quick Play.

## Verification

The native rematch harness now also accepts `--split-party` and `--four-solos`.
The first option joins a second client to the first client's doubles party using
the native lobby-code field, then matches those teammates against two players
sharing a third client. The guest's queue control stays disabled; the party
leader queues both teammates. The four-solo option queues four independent
clients and lets the allocator form the teams.

Both runs finish a four-stock game using ordinary controller movement, receive
matching results from every PC, return all local controllers through Results,
and verify each CSS has four cards with the correct number of local seats. The
losing teammates on separate clients must both vote before the stage decision
completes. Every client then readies into practice and starts another match in
the same assigned room with a new session ID. Peer IDs survive the initial
party-to-match handoff.

The separate-client premade run is recorded under
`.local/native-quick-split-party-rematch-3`: sessions `0d91a1bbd58a8c7c` then
`514b6717d5532070` in room `4B2A86016591`. Its first client compared 2,540
confirmed peer checksums and reconstructed 2,540 hidden frames during the first
game. The four-solo run is under `.local/native-quick-four-solos-rematch-1`:
sessions `8fe646949c697cf0` then `010017d0e1b245c9` in room `616ABAB3399A`,
with 4,224 checksum comparisons and 784 hidden frames on its first client.
Neither run reported a gameplay failure or checksum mismatch. The second-match
PNGs were reviewed.

These clients all run on this PC against a local service. They establish native
multi-process party, team, input ownership and rematch behavior. Physical
multi-PC/network acceptance remains the user's later pass, and stage voting
still uses the automation command while its player-facing UI awaits review.

The first harness attempts expected the ordinary two-card layout in a Quick Play
party and tried inspecting CSS cards during practice/gameplay. The harness now
checks the four-card layout on the actual CSS after Results. No game behavior
needed changing for these two scenarios.

`npm run check` passes 44 service tests. The rematch cases cover result consensus,
winner/loser authority, shared-PC and separate-PC teammates, vote withdrawal,
stale reports and votes, no-contests, disagreements, hibernation, and signaling
loss followed by an end report. Tests wait for the relevant revision or vote
state instead of accepting an older queued broadcast as a new response.

`review_native_quick_rematch.py` runs real hidden, muted clients with isolated
profiles. It walks one side off stage until all four stocks are used, reads the
normal result reports, confirms the results screen, submits losing-side votes,
readies again and verifies a second live session in the same room. The doubles
run confirms the results screen with both local controllers. All gameplay uses
ordinary controller input.

Evidence:

- `.local/native-quick-rematch-singles-1`: session `0ff8efb198dc3fc1` to
  `9c0e78b19102afd6`, room `290A8F05C837`.
- `.local/native-quick-rematch-doubles-2`: session `ccf97a93bbe6f0aa` to
  `8526741360fed6e4`, room `14D93F19D71A`.

The clients reported matching winners and no checksum mismatch. Their second
match PNGs were reviewed. Native protocol checks, ordinary connection and
content-transfer regressions, and the service deployment dry-run are recorded
alongside those runs.

Remaining acceptance includes the actual stage-confirmation UI and its complete
controller/mouse flow, late countdown cancellation, leaving/requeueing after
in-game disconnects, and production deployment.
