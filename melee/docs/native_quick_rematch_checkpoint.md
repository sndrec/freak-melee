# Quick Play results and rematches

Updated: 2026-10-17. Tested locally, including the native stage-confirmation page.
User visual review and Quick Play deployment remain open.

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

In doubles, either losing teammate selects a stage. That player can change or
clear the selection. The other teammate confirms that stage. The service rejects
a confirmation for a different stage and keeps the existing selection. This
also rejects a delayed confirmation after the selector has changed the stage.
When both seats agree, the service saves it and permits the next ready countdown.
Old votes cannot alter a later round. Singles accepts the losing player's choice
immediately.

`nativeOnlineSession_PickQuickStage` is the native entry point for these choices.
The test command `melee_native_control room-stage <local-seat> <0..5|none>` calls
it using the live local session. Stage indices follow the existing Quick Play
stage table. `none` withdraws a pending vote.

## Native stage page

The page opens after Results and offers the same six stages to singles and
doubles. Winners see the pending selection with disabled stage controls. In
doubles, the selected stage and player name appear above the confirmation
controls. Two players sharing a PC have separate controls; the mouse can select
which local player it acts for. Each controller can confirm only its own seat.

B returns to character select. Next stage reopens the pending selection. Leave
match returns to the original party. After agreement the page closes, and players
can ready into practice and the normal three-second countdown.

The page uses the hub's GX renderer, fonts and Melee hand. Its buttons use the
existing menu enter/exit animation. `native_hub_quick_stage.cpp` owns presentation
and input validation. The service owns the selection and confirmation. No local
selection can bypass its losing-side, seat or epoch checks.

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
multi-PC/network acceptance remains the user's later pass. These older runs used
the automation command for stage choices; the native-page runs below exercise
the visible controls.

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
normal result reports, confirms the results screen, uses the native stage page,
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

The native-page checks additionally passed:

- `.local/native-quick-stage-shared-1`: two shared-PC pairs, session
  `ca69fca66dc21072` to `64d172008f620432`, room `DC97E15601B0`.
  Local P2 selected each of the six stages and cleared the choice. P2 could not
  confirm for P1; P1's controller confirmed and the rematch started.
- `.local/native-quick-stage-split-1`: separate-client premade teammates against
  a shared-PC pair, session `56f6d558d7ccf095` to `1f7b046f8ea6da42`, room
  `6AEA9400CE06`. One client selected and the teammate confirmed using A.
- `.local/native-quick-stage-singles-4`: final native build, session
  `8b0bfbf554705704` to `b95db070984d9954`, room `D375576559B4`. B returned to CSS,
  Next stage reopened the page, and the losing player's pick started a rematch.

Both runs retained the same opponents and room, reported no checksum mismatch,
and stopped all test clients. Their native page and rematch PNGs were reviewed.
Service type checks and all 44 tests pass, including changed selections, stale
confirmations, withdrawal and separate-client ownership. The final label pass
uses ASCII Pokemon while the current hub glyph sheets cover ASCII characters.

The first singles harness timed out while both fighters jostled at the ledge.
Its revised input jumps past the opponent while retaining rightward movement.
One intermediate run stopped on the retained-history OS heap allocation guard
in both clients during Battlefield gameplay (`native-quick-stage-singles-2`).
The caller is not yet identified. The successful singles run had an abort
breakpoint attached and did not reproduce it. An earlier debugger command-line
attempt ended that test process; it is not counted as a runtime failure or pass.
The guarded allocation remains a separate runtime investigation.

The cancellation and original-party recovery checks remain recorded in the
[return checkpoint](native_quick_return_checkpoint.md). User visual review and
production deployment remain open.
