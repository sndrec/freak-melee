# Native Quick Play queue, countdown and match launch

Updated: 2026-10-17. Tested locally; the new service class is not deployed.

Quick Play's service can assemble 1v1 and 2v2 matches from real room members.
Doubles supports four solos, two premade pairs, or a pair and two solos. A pair
can occupy two seats on one PC or use one seat on each of two PCs. Teammates
remain together. Readiness and fighter selection are independent of searching;
queue admission does not require either.

The native mode buttons now open CSS. Enter queue and Cancel search work from
CSS and practice. START locks the local choices and enters practice independently
of searching. CSS shows two or four cards, filling open seats with grey waiting
cards. The assigned room keeps each PC's peer identity, local controller slots,
fighter choices and readiness. Leaving an assignment returns the clients to their
parties, where they can search again.

The service now starts a three-second countdown when every player has locked
their choice and every PC reports connected peers and available fighter content.
B unreadies the local players and returns directly to CSS. Readying again starts
a new countdown. After the countdown, both clients enter the same fixed-rule
match through the existing content and rollback handshake. Result agreement and
same-opponent rematches now pass local checks using native stage-vote automation;
the player-facing stage-confirmation UI and deployment remain open. See the
[rematch checkpoint](native_quick_rematch_checkpoint.md).

## Native session

`native_online_session.c` owns the party code, assigned-room transition and
ten-second search heartbeat. Each new room connection copies the existing peer
identity and adds its private admission token. Room shutdown releases its ICE
and content jobs before the new connection starts. Practice and CSS choices live
outside that connection and publish again when the assigned room is ready.

The native decoder handles the Quick Play state and rejection messages. Rejections
are shown as queue errors. An explicit allocation cancellation returns to the
party; ordinary signaling loss retains the established gameplay policy. The
native build identity is `freak-melee-native-dev-quick-rematch`.

For local service checks, `FREAK_MELEE_SERVICE_ENDPOINT` overrides the native
room-service origin. Without it, the client uses the configured hosted origin.
The new service class remains undeployed while the stage-confirmation UI is incomplete.

The preparation report names the current presence revision and stock-content
digest. Each PC checks its actual ICE connections, locked choices and verified
remote cache. A selection or membership change invalidates preparation. The
service compares all reports and persists the countdown deadline. Its alarm
rechecks the roster and reports, then writes one canonical match proposal. An
early or repeated alarm cannot skip the deadline or publish a second match.
The countdown survives service hibernation.

Native match adoption waits until practice has exited. The existing SSS scene
completes the content/connection handshake without accepting a manual first-stage
pick. Team assignment comes from the allocation. Stock fighters use their team
costume when their choices are published for doubles.

## State ownership

The existing `Room` owns party sockets, names, membership and the party leader.
Only that leader can configure a queue mode or enter/cancel the party's search.
Quick Play parties contain active human seats. Singles permits one seat;
doubles permits one or two. The party's membership stays fixed while searching.
A disconnect or replacement cancels that search. An obsolete socket closing
after replacement cannot cancel the new connection's search.

`QuickPlayQueue` is a Durable Object per mode and build. Its SQLite tables own
queue order, thirty-second search leases, duplicate-peer protection, cancelled
search generations and pending allocations. Heartbeats renew an existing search
without resetting its queue position. The native client sends them every ten
seconds while searching. A late request from a cancelled generation is rejected.

The coordinator reserves whole parties in one synchronous storage transaction.
It checks the actual party generations and connection identities before creating
a match room. Each PC receives a separate random admission token. The new room
accepts the assigned peer, build and local-seat count with that token. A room code
alone does not grant admission to the assigned match.

Pending deliveries remain in SQLite. Before publishing assignments, the service
persists the committed allocation. Alarms retry interrupted delivery. An assigned
party may close its old sockets during handoff; replaying its committed delivery
remains safe after that. Cancellation and incompatible membership roll back a
pending allocation. Compatible unaffected searches retain their original queue
position while their leases remain valid.

This uses the current private room-code/session identity model. Account sign-in,
persistent friends and inbox delivery remain separate decisions.

## Rules and stage metadata

Allocations carry four stocks, a six-minute timer, items off and one random first
stage from all six approved stages. The ordered stage indices are:

| Index | Stage |
| --- | --- |
| 0 | Battlefield |
| 1 | Final Destination |
| 2 | Fountain of Dreams |
| 3 | Dream Land |
| 4 | Pokemon Stadium |
| 5 | Yoshi's Story |

Both modes use that list. Ordinary owner match proposals, seat overrides and
rotation commands are rejected within the Quick Play flow. The service builds
the native FMLM proposal from locked choices and assigned teams: four stocks,
six-minute timer, no items, handicap off, normal damage, pause off and friendly
fire on for doubles. The native lobby codec and handshake consume that proposal.

The losing team's stage-agreement interface remains a design review item for
when the user returns. The service already requires both teammates to agree on
one stage; its confirmation controls have not been selected.

## Wire additions

The room envelope remains `FMS1`, version 5, with the existing eight-byte header.
New client message types are:

| Type | Payload |
| --- | --- |
| 10 ConfigureQuick | mode u8: 0 ordinary, 1 singles, 2 doubles |
| 11 QueueQuick | searching u8: 0 cancel, 1 enter/heartbeat |
| 12 RequestQuick | empty |
| 13 PrepareQuick | presence revision u64, prepared u8, digest bytes16 (32 bytes when prepared, empty otherwise) |
| 14 ReportQuick | current epoch u64, winning side u8; 255 means no contest |
| 15 VoteQuickStage | completed epoch u64, local seat u8, stage u8; 255 withdraws |

Server type `0x89` sends mode u8, state u8, generation u64, match-room string8,
admission-token string8, team u8, first-stage u8, phase u8 and remaining-ms u16.
These are followed by completed epoch u64, choosing side u8 and four vote bytes
in canonical-port order; 255 means no choosing side or no vote. Phase is 0
waiting, 1 countdown, 2 playing, 3 stage choice or 4 result agreement. State is 0 idle, 1 searching
or preparing, 2 assigned, or 3 aborted. Assignment data is private to each PC's
socket. Server type `0x8a` sends a string8 rejection reason. These rejections
leave ordinary room signaling connected.

The native decoder and session use those same types. The existing signaling
transport and gameplay ICE setup serve the assigned room.

## Verification

`npm run check` regenerates bindings, checks application/test types and runs the
service tests. The current suite has 42 passing tests, including 21 Quick Play
cases. Quick Play coverage includes all doubles group sizes, premades on one or
two PCs, independent admission tokens, build separation, duplicate searches,
leader authority, malformed commands, capacity checks, cancellation/requeueing,
connection replacement, lease expiry and service hibernation. A persisted
committed-allocation fixture verifies recovery after one party has handed off.

`review_native_quick_play.py` now exercises the real native clients against a
local Worker. The runs use hidden, muted clients and isolated fixture profiles.
The singles run readies one player into practice before entering the queue.
The doubles run connects two virtual controllers per PC, selects both local
fighters, and readies one pair into practice. Both verify the search heartbeat,
assigned-room handoff, unchanged peer identities, local-first cards, preserved
choices and readiness, cancellation, requeueing and immediate cancellation of
the second assignment. The doubles check also verifies the second local seat
still maps to physical controller port 1.

Final evidence is in `.local/native-quick-singles-final` and
`.local/native-quick-doubles-final`. Their PNGs were reviewed. The native build
and six focused CTest checks pass. The ordinary connection probe still carries
validated rollback input after signaling closes. The content-channel probe
still passes its four-player plus waiting/spectating recipient scenario.

`review_native_quick_match.py` verifies the complete first-match flow, with
evidence in `.local/native-quick-match-singles-final` and
`.local/native-quick-match-doubles-1`. Both runs ready before searching, cancel
the first countdown with B, return directly to CSS, ready again and enter one
shared live netplay session. The doubles run uses two local controllers per PC.
Both clients receive movement input and stay in gameplay without a reported
checksum mismatch. Reviewed PNGs show the countdown, four stocks, six-minute
timer, and all four doubles fighters.

The first larger-window run reached gameplay but failed on a screenshot
readback allocation while Windows was near its commit limit. The final checks
use smaller hidden windows; both complete successfully. The harness resizes
only its own process's main window and preserves visibility, focus and Z order.
Service checks and the Wrangler deployment dry-run pass. No production deployment
has been made for Quick Play.

The existing signaling and global/lobby chat suites also pass. The empty-room
chat check now waits for the close event's actual SQL result; an unrelated RPC
is not an ordering barrier for a WebSocket close still in transit.

`npm run deploy:dry` accepts the new namespace binding and appended class
migration. Logs are local under `.local/quick-play-service-check.log` and
`.local/quick-play-service-dry-run.log`. Tests use the local Worker environment;
no matchmaking or public chat traffic was sent to the deployed service.

## Next implementation

1. Review and build the player-facing losing-side stage-confirmation controls.
   Result agreement, per-seat stage authority, rematches and party return have
   local service/native automation evidence.
2. Exercise the complete controller/mouse flow, late countdown cancellation and
   leaving/requeueing after in-game disconnects.
3. Run the full native multi-client acceptance before deploying the new service
   class. Physical multi-PC testing remains the user's later pass.
