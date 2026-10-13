# Players sharing one PC in online rooms

The room service now stores active and waiting masks for each connected PC,
with one bit per local player seat. A seat in neither mask spectates. Joining
fills the four available fighter spots and puts excess local seats in waiting.
The native client publishes one ordered selection array per PC; match assembly
copies only active selections and the topology assigns them match ports.

The match proposal now carries `local_seat` for each human fighter. A seat is
the player's stable index within that PC's selection for this room, independent
of the match port and the physical controller port. Both the gameplay input
map and the results-screen input map resolve a match port through this seat.
That identity allows seat 1 to fight while seat 0 waits on the same PC. The
signaling protocol and native topology tests cover this shape. A live native
signaling probe verified a three-seat PC followed by another three-seat PC:
the second received one active and two waiting seats. It exchanged signaling
with the first through a local Durable Object room.
The deployed service also passed this native 3+3 signaling probe; a separate
two-peer probe carried rollback packets over ICE after signaling closed.

The room sidebar now shows a control for each local seat. A player can choose
fighting, waiting, or spectating for their own seats; the room owner can assign
any seat. An owner change does not lock that seat: its player can change it
again before the match. The service enforces the four-fighter limit and
rejects active-seat changes while a match proposal is published. The client
currently disables every seat control during a published match, including
waiting-to-spectating changes that the service itself permits. The room now
tracks waiting order per seat and publishes a rank for each waiting player.
Entering the queue goes to its tail; staying in it preserves position, even
when another seat on the same PC changes role. Owner overrides follow the same
rule, and WebSocket attachments preserve order through room hibernation. The
service and native client use signaling wire version 5 for these ranks and
the room rotation policy. Rotation defaults to manual. The owner can cycle
between manual, winner-stays, and winner-out in the sidebar; each waiting seat
shows its server-assigned queue rank. After the final versus or sudden-death
scene, an active client
reports the result before releasing its match transport. The native client
marks only a completed, non-team, two-human, no-CPU match with one winner as
eligible. The room then clears the published proposal and atomically exchanges
the outgoing fighter with the first waiting seat, placing the outgoing seat at
the queue tail. A canceled match, tie, team match, free-for-all, failed match,
or match with no waiting seat clears without rotating. Reports carry the
agreed match epoch, so a late duplicate cannot rotate the next match. An
owner's later clear request is harmless once a result has cleared the proposal.
A published match freezes its active seat mapping. A waiting seat can
change its waiting/spectating state without disturbing the fighters.

The selection array stays indexed by local seat; match assembly copies only
active selections in canonical room order and writes their original seat
indices into the proposal. Ready checks must depend only on active seats and
their locked characters; the current selection confirmation still requires
every local seat to have a character before that PC enters practice. A waiting
or spectating seat may remain in the room without consuming a rollback port.
If an active PC disconnects, the match
aborts even if it also had waiting seats. A spectator-only disconnect does not.

The client UI was checked against the deployed room service for a single PC
cycling a second seat through fighting, waiting, spectating, and back. A
native two-connection signaling probe also checks a host override followed by
the other participant reclaiming its choice. These are same-machine checks.
The service's three-PC test verifies waiting ranks across owner and self
reassignments and after Durable Object hibernation. The native wire parser test
checks the added rank fields and rejects an inconsistent waiting mask.
The matching service was deployed as version
`2c539446-69e1-44b8-a6a5-11d050e6d032`. A fresh native two-client probe
against that deployment passed its 3+3 seat assignment, rank-preserving owner
override and self-reassignment, directed signaling, and owner rotation-policy
update. The probe then reduced both PCs to one active seat, published a synthetic
1v1 proposal, and had the guest report a win. Both native connections observed
the host's active seat move to the queue tail and the guest's next waiting seat
become active. The full 374 native tests and 17 signaling service tests pass. Service
tests cover both policies, queue order, canceled and stale reports, and rotation
between seats sharing a PC. This synthetic report does not prove the rendered
waiting queue or the multi-PC controller path.

On 2026-09-24, two native processes on the same Windows PC joined the deployed
room service through the normal UI, locked Mewtwo on separate local controllers,
entered the practice grid, and started a four-stock Final Destination match.
One fighter walked off four times. Both clients displayed the same Mewtwo
result, then returned to the same open room after advancing past the results
screen; both players' Ready states cleared. The host received 2,415 input
packets with zero rejections and compared 2,146 confirmed peer checksums; the
guest received 2,403 packets with zero rejections and compared 2,402 checksums.
Neither client reported a rollback correction. This proves the rendered
two-client completion and return-to-room path for a room with no waiting seat.
It does not prove result-driven seat rotation, shared-PC controller mapping,
or cross-PC connectivity. During an earlier setup attempt, a room showed a
signaling disconnect while guest processes were being relaunched; its cause
remains unverified.

A second 2026-09-24 live run used two local seats on each of two native
processes on the same PC. The host owned match ports P1/P2 and the guest owned
P3/P4; each selected Ice Climbers and Samus, respectively. All four entered
the six-minute, four-stock Final Destination match through normal room and
stage-select UI. Independent virtual inputs moved the host's P2 left and the
guest's two fighters right while P1 remained neutral. Both clients reached
the same four-player results, then returned to the same open room with Ready
cleared after each local seat confirmed the results. Each side received 5,499
input packets with zero rejections. The host corrected four timelines over
eight hidden frames and compared 5,491 confirmed peer checksums; the guest
corrected one timeline over four hidden frames and compared 5,488 checksums.
This validates a rendered 2+2 shared-PC topology and live rollback correction
on one machine. It does not establish physical multi-controller or cross-PC
behavior, nor a waiting seat's promotion after rotation.

A third 2026-09-24 live run placed two local seats on the host and one on the
guest. Host P1 fought as Bowser, host P2 waited as Samus, and guest P1 fought
as Bowser. The owner selected winner-out rotation through the native control
channel, then started a four-stock match from the normal room and stage-select
UI. Guest Bowser self-destructed four times; both clients showed host Bowser
winning. After both active players advanced the results screen, the room
returned to character select with host P1 Waiting and host P2 Fighting. The
guest remained Fighting, and Ready cleared on both PCs. The host received
2,681 input packets and the guest 2,697, with zero rejections; both compared
2,679 confirmed peer checksums. This proves result-driven winner-out promotion
on one Windows PC with a waiting seat on the host. It does not yet prove that
the promoted seat's physical controller drives the next online match.

Full acceptance later requires a two-PC match with two local seats on one PC: seat 1
fights while seat 0 waits, each controller drives only its assigned fighter,
the result input belongs to seat 1, and the next round can promote seat 0.
Exercise the same mapping with a third and fourth active fighter, a manual
reassignment, and both 1v1 rotation policies. Service wire tests and the
native topology tests alone cannot prove live controller routing.

The sidebar was checked in a live two-process room on 2026-09-24. One PC
entered with one local seat, the other with four; the fifth seat displayed
`Waiting #1` on both views. The owner cycled the rotation control from Manual
through Winner stays to Winner out, and the guest's read-only control updated
from service presence. This verifies the controls and queue rendering, not
promotion of that fifth seat into a second live match.
