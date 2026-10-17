# Quick Play service foundation

Updated: 2026-10-17. Tested locally; the new service class is not deployed.

Quick Play's service can assemble 1v1 and 2v2 matches from real room members.
Doubles supports four solos, two premade pairs, or a pair and two solos. A pair
can occupy two seats on one PC or use one seat on each of two PCs. Teammates
remain together. Readiness and fighter selection are independent of searching;
queue admission does not require either.

The native Quick Play controls, three-second ready countdown, fixed-rule match
construction, loser stage picks and rematches remain open. The native menu still
reports Quick Play as unavailable. This checkpoint proves allocation and party
handoff behavior, rather than a playable native Quick Play match.

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
without resetting its queue position. The client will send them every ten
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
rotation commands are rejected within the Quick Play flow. The service-owned
ready/countdown and match-proposal construction still need implementation;
storing the requested rule metadata does not establish those parts as complete.

The losing team's stage-agreement interface remains a design review item for
when the user returns. The plan already requires both teammates to agree on
one stage; its confirmation controls have not been selected.

## Wire additions

The room envelope remains `FMS1`, version 5, with the existing eight-byte header.
New client message types are:

| Type | Payload |
| --- | --- |
| 10 ConfigureQuick | mode u8: 0 ordinary, 1 singles, 2 doubles |
| 11 QueueQuick | searching u8: 0 cancel, 1 enter/heartbeat |
| 12 RequestQuick | empty |

Server type `0x89` sends mode u8, state u8, generation u64, match-room string8,
admission-token string8, team u8 and first-stage u8. State is 0 idle, 1 searching
or preparing, 2 assigned, or 3 aborted. Assignment data is private to each PC's
socket. Server type `0x8a` sends a string8 rejection reason. These rejections
leave ordinary room signaling connected.

The native decoder and session integration must add these types together. A
Quick Play session will keep its peer identity across the party-to-match-room
handoff and pass its admission token in the new room connection. The existing
signaling transport and gameplay ICE setup can then serve the assigned room.

## Verification

`npm run check` regenerates bindings, checks application/test types and runs the
service tests. The current suite has 33 passing tests, including 12 Quick Play
cases. Quick Play coverage includes all doubles group sizes, premades on one or
two PCs, independent admission tokens, build separation, duplicate searches,
leader authority, malformed commands, capacity checks, cancellation/requeueing,
connection replacement, lease expiry and service hibernation. A persisted
committed-allocation fixture verifies recovery after one party has handed off.

The existing signaling and global/lobby chat suites also pass. The empty-room
chat check now waits for the close event's actual SQL result; an unrelated RPC
is not an ordering barrier for a WebSocket close still in transit.

`npm run deploy:dry` accepts the new namespace binding and appended class
migration. Logs are local under `.local/quick-play-service-check.log` and
`.local/quick-play-service-dry-run.log`. Tests use the local Worker environment;
no matchmaking or public chat traffic was sent to the deployed service.

## Next implementation

1. Add the native wire records and persistent Quick Play session ownership.
2. Connect mode selection to CSS, expected empty/occupied player cards, queue
   controls and ready practice. Preserve the identity and selected character
   across assignment and reconnect to the allocated room with its private token.
3. Add service-owned content/connectivity receipts and the cancellable three-second
   countdown. Publish a validated native match proposal with the fixed rules.
4. Connect results, losing-side stage selection, rematches and party return.
5. Run the native multi-client acceptance for the full approved flow before
   deploying the new service class and enabling the menu controls.
