# Native netplay bring-up

The native client has two active-match transports wired to the same
`NativeConfirmedCheckpoint`: diagnostic direct IPv4 UDP and room-signaled ICE.
Packet history, ownership, redundancy, and checksum validation live in a
transport-independent exchange layer, so both paths apply exactly the same
rollback data-plane rules. Distributed-PC and forced-relay acceptance remain
outside the current locally validated scope.

Production keeps one snapshot at the contiguous fully confirmed input boundary.
Corrections restore it, reconstruct a newer confirmed checkpoint once, then
replay the predicted suffix without intermediate saves. Admission and ownership
checks run during construction; ordinary copies use sealed mutable spans and
do not track page faults. The sixteen-slot history/page session remains a
diagnostic reference. Sudden death renews history leases while retaining the
live objects and transport. Required CPU display/pose/effect work still runs during reconstruction; GPU
geometry/output is omitted. See [current rollback](native_rollback_current.md).

Each PC owns an arbitrary non-overlapping mask of the four controller ports.
This represents one player per PC, multiple local players sharing a PC, or any
other partition of the four active slots. Every active room peer must own at
least one human fighter; its mask may also cover CPU or empty ports so all four
input histories still have one deterministic sender. Every datagram carries
one through eight consecutive frames for all ports owned by its sender. Repeating recent
frames recovers ordinary loss without a reliable ordered stream. A maximum
four-port batch is 520 bytes.

The packet uses explicit little-endian fields rather than native structure
layout. Each controller sample serializes the original button, sticks,
triggers, analog face buttons, error value, and native extension buttons in 15
bytes. The receiver validates the session ID, sender identity, source IPv4
endpoint, and exact assigned port mask. A decoded batch is applied to a copy of
the fixed input history and committed only when every non-expired confirmation
is valid, so a contradictory sample cannot partially mutate the live session.
Packet encoding, decoding, ownership checks, atomic rejection, nonblocking
loopback exchange, redundant recovery, and checksum carriage have native tests.

Once all four controller samples for a simulated frame are confirmed, each
client publishes its newest semantic state checksum in subsequent input
packets. The receiver compares that frame only after its own inputs are also
confirmed, preventing ordinary prediction from being diagnosed as divergence.
The current checksum explicitly serializes portable gameplay fields rather than
hashing native structures. It covers match timing and respawns, player stocks
and staleness, fighter input/motion/collision/animation/hitbox/hurtbox/damage
state, common item motion/collision/hitbox/timer state, common ground-controller
state, stage and item joint animation, camera transforms/quakes/mode flags and
the active camera-subject list, and the shared RNG seed. A focused test
constructs the same state at different pointer addresses and requires an equal
digest, then mutates a fighter hitbox, item timer, ground controller, stage
animation, stale-move entry, camera target, and camera subject independently and
requires each change to be detected. Process-local camera GObj and callback
addresses are excluded. Persistent per-character variables and the common
walk/turn/dash/run/jump/fall/squat/landing motion families, jab and rapid-jab
control state, dash-attack/down-tilt flags, and the standard damage-state
timers have explicit schemas. Shield angle, analog amount, powershield windows,
and guard transition state are also covered. Other action-specific fighter
motion variables remain after knockdown timers, tech/air-dodge state, rebound,
platform drop-through, ledge state, item pickup, light/heavy throw interpolation,
parasol-open history, and heavy-item lift state were added. Item-specific unions
and stage-specific controller unions also remain. Live fighter, item, and ground
objects now receive
category-and-list-order IDs, so held-item,
owner, grab-victim, reflector, absorber, toucher, attack-victim, damage-source,
and hitbox-victim relationships are compared without hashing process addresses.
Allocator free-list and other object-pool topology remains outside the digest.

The first game frame acts as a short readiness barrier. Each client sends frame
zero and waits up to five seconds for confirmations from every remote-owned
port before simulation begins. Later missing inputs use rollback prediction,
but a client may advance at most six frames beyond the slowest remote-owned
port. At that boundary it repeatedly polls and resends its redundant batch for
up to five seconds. This keeps every unconfirmed frame inside both the rollback
history and the next eight-frame redundant packet; sustained loss therefore
becomes a bounded stall and then a match failure instead of expired state.
Windows UDP ICMP resets are disabled so a single unreachable response does not
bypass this timeout policy.

For two clients on one PC, launch identical matches and assign all four ports
between them. These commands use ports 47200 and 47201:

```powershell
$match = @('--iso', '..\Super Smash Bros. Melee (USA) (v1.02).iso', '--match', '--stage', 'battlefield', '--p1', 'human:ganondorf', '--p2', 'cpu:zelda:1', '--p3', 'cpu:ice-climbers:1', '--p4', 'cpu:mr-game-and-watch:1', '--mode', 'stock', '--stocks', '1', '--items', 'off', '--replay', '.\build\native\rollback-probe-input.fmr', '--netplay-session', '0xF00DCDEF')
& .\build\native\melee_native.exe @match --netplay-listen 47200 --netplay-local-peer 0 --netplay-local-mask 0x5 --netplay-peer 1,127.0.0.1,47201,0xA --netplay-delay 3
& .\build\native\melee_native.exe @match --netplay-listen 47201 --netplay-local-peer 1 --netplay-local-mask 0xA --netplay-peer 0,127.0.0.1,47200,0x5
```

`--netplay-delay` is a diagnostic outbound-frame delay. Frame zero still passes
the readiness barrier immediately; later input is held by zero through six
frames to force prediction and correction while remaining inside the redundant
delivery window. It also works on normal home boot for sidebar-created rooms:
launch each client with its own optional `--netplay-delay <frames>` before
creating or joining a room. The local value follows that client into every
room match, including a guest receiving the host's match proposal. A
same-machine host launched with `--netplay-delay 3` and a zero-delay guest
reached a custom Mario versus Falco Battlefield match and sudden death; the
guest corrected two timelines across six hidden sudden-death frames, compared
1,669 confirmed peer checksums in that scene and reported no rejected input
packets. Evidence is in `build/native/room-delay-flag-host.log` and
`build/native/room-delay-flag-guest.log`. The delay flag is diagnostic and
does not change the agreed match rules or content.

On the first delayed two-process run, peer 0 completed without correction and
peer 1 corrected two timelines across 12 hidden full-draw frames. Both clients
reached replay sample 2031 with fighter digest `7f75ed3fbf3113d6` and RNG state
`748ae5d7`. An earlier zero-delay run also completed in both processes with no
rejected packets. This proves the direct transport, mixed slot ownership,
late-input restore, replay-source rewind, and catch-up path on one machine. It
does not provide distributed-machine performance or determinism evidence, and
the fighter digest is narrower than the planned semantic gameplay checksum.
After checksum exchange was added, another three-frame-delay run compared
2,021 confirmed peer checksums on one client and 1,265 on the other without a
mismatch; peer 1 corrected two timelines across ten hidden frames and both
again ended at sample 2,031 with the same fighter digest and RNG state.

The expanded semantic checksum later exposed a protocol-ordering error: a late
input could make a predicted frame fully confirmed immediately before rollback,
allowing that speculative digest to be published or compared before the
corrected timeline replaced it. Checksum publication and comparison now pause
while a correction is pending. A clean repeat with a three-frame outbound delay
completed all 2,031 samples; peer 1 corrected four timelines across 22 hidden
frames, the clients compared 2,005 and 1,226 confirmed checksums without a
mismatch, and both ended with fighter digest `01b9fb327a404f30` and RNG state
`fd7642d8`.

Match transport/setup failure now takes the
original Versus `NO_CONTEST` cleanup path instead of setting the process-wide
exit flag. It skips results and rotation, shuts down rollback and transport,
and leaves online mode. Until the native room screen exists, the safe fallback
is the main menu and the launch-time netplay configuration is cleared; entering
ordinary character select cannot accidentally start a match using the stale
canonical proposal. The packet codec remains the match data plane, but an ICE
agent must own the socket: mapped addresses, TURN allocations, permissions,
keepalives, and consent checks all belong to that live agent.

The first room-signaling service now lives in `services/signaling`. It uses one
Cloudflare Durable Object per private room and hibernatable WebSockets for
presence, build identity, active/waiting/spectator roles, ready state, bounded
opaque per-PC selections, and directed connection-candidate messages. Its Workers-runtime integration
test connects two clients, transfers room ownership/presence state, updates a
spectator into the waiting queue, forwards a directed candidate, and restores a
live participant after forced Durable Object hibernation. The private Worker is
deployed at `https://freak-melee-signaling.sndrec32exe.workers.dev`; version
`089ff6bf-bb71-42fe-9ff5-802bd38aaafb` carries the per-PC selection payload.

The room now also stores one bounded opaque match proposal owned by the current
room owner, plus one bounded opaque local-selection payload per participant.
Proposal publication, replacement, or clearing invalidates every ready state.
A selection or active-player-topology change clears both the proposal and every
ready state, preventing a stale proposal from launching after a menu change.
Ready is rejected when no proposal exists. The proposal epoch and bytes survive Durable Object
hibernation in SQLite; an owner departure transfers control by join order and
clears the old proposal, while a same-peer reconnect replacement retains it.
The native wire/client can publish and receive these bytes. The native
`NativeLobbySelection` codec represents up to four local player choices with
stable character and costume identities plus team assignment in 78 bytes or
less; the Worker does not interpret it.

`NativeLobbyMatch` defines the canonical proposal payload without native struct
layout or allocation. Its bounded big-endian encoding includes the shared
session identity, exact rules and item set, stable stage identity, four stable
fighter and costume identities, human/CPU/empty slot state, CPU/team/handicap
settings, every active room peer's numeric rollback identity and controller-port
mask, and a 32-byte digest of the selected gameplay content. Validation rejects
missing content/session identities, duplicate peer identities, overlapping or
incomplete port ownership, invalid slots or rules, and team matches without two
occupied teams. Decode consumes the complete byte string and a second encode is
byte-identical. The focused native test covers those properties.

The direct-start room transition now constructs this descriptor from the
applied stock match and its configured peer ownership. The room owner publishes
the canonical bytes, and every peer verifies the proposal, epoch, active room
topology, exact build, per-PC human count, numeric rollback identity, and port
mask before setting ready. ICE topology cannot lock until every listed peer is
ready on the same proposal. A live local two-client probe passed this agreement,
closed signaling, and then exchanged validated shared-PC rollback input.

The room owner remains authoritative for the diagnostic direct-start match.
A non-owner now decodes and canonically re-encodes the owner's proposal,
validates its configured peer ownership, adopts its session and exchange
identity, resolves the stock fighter/costume/stage identities, and applies the
exact roster, rules, item mask, handicaps, and preload selection before the VS
scene is preloaded. The agreed descriptor also becomes the semantic launch
signature used for the shared RNG seed. The live connection probe deliberately
starts the guest with a different session, stage, and stock count; both peers
lock the owner's descriptor and exchange validated shared-PC inputs. Peer IDs,
port ownership, and per-PC human counts are still supplied by the diagnostic
CLI. The menu lobby must derive those assignments and let the owner construct
the proposal from actual selections.

The native Windows signaling client now connects to the room service through
WinHTTP WebSockets. The connection has two explicit I/O lanes because WinHTTP
permits WebSocket send and receive concurrently: the receive lane owns setup,
incoming frame assembly, and final handle closure; the send lane drains a
bounded command ring. A requested stop sends one final presence request so its
response wakes the blocked receive before both lanes join.
The game-facing side only writes immutable commands and reads immutable events
through fixed single-producer/single-consumer rings. Neither I/O lane mutates
lobby or simulation state, and signaling allocates no memory during
steady-state operation.

The room wire format is versioned binary rather than JSON. Protocol version 2
carries per-PC selection payloads. Native byte-level
tests cover client encoders and bounded server snapshot decoding. A live local
probe starts two independent native clients against `wrangler dev`, receives
welcome and ordered two-peer presence on both, forwards opaque candidate bytes,
receives the sender acknowledgement, and shuts down both blocked receive lanes
cleanly. Reproduce it with:

```powershell
cd services/signaling
npx wrangler dev --port 8799

# In another terminal from the repository root:
cmake --build build/native --target native_signaling_probe -j 8
.\build\native\native_signaling_probe.exe http://127.0.0.1:8799
```

This closes native signaling transport compatibility, not lobby UI.

The native ICE boundary uses pinned `libjuice` 1.7.2 source. Each remote PC gets
one agent configured in shared-poll-thread mode, with setup and candidate
exchange confined to the lobby transition. Receive callbacks only append opaque
datagrams to a fixed single-producer/single-consumer queue; the game thread owns
decoding and rollback-state mutation. A local two-agent test gathers host
candidates, exchanges complete descriptions, selects a pair, transfers packets
in both directions, and drains the fixed queues without drops. The same agent
accepts a STUN server and one short-lived TURN credential. Room startup now
fetches a bounded binary configuration from the signaling origin before
creating any agents. Normal clients use its STUN server and optional UDP TURN
credential; an explicit complete TURN configuration remains available for
development. The blocking fetch occurs only during the room transition, copies
the result into fixed connection storage, and then discards the response.

The connection coordinator turns compatible active room participants into at
most three remote ICE agents. It wraps complete descriptions in a bounded,
versioned binary envelope and forwards one distinct description to each peer.
Participant reconnects replace the corresponding agent by join order. Once the
lobby has agreed on the active topology and every route is connected, the game
can lock that topology and close signaling without affecting match traffic.
Room lifetime now has separate connect and match-configuration transitions.
The connection and its stable storage can therefore remain live while a future
menu displays changing presence; only after selections are final does the room
bind the canonical match, rollback identities, and controller-port ownership.
The diagnostic launch path uses the same two transitions back-to-back.

`NativeLobbySession` now owns that pre-match policy. It exposes current room
presence, queues role/local-player/ready updates, restricts match creation to
the current room owner, derives the peer and controller-port topology from live
presence, and automatically adopts a canonical owner proposal on every guest.
Its state moves through connecting, waiting, starting, and ready without UI
dependencies or runtime allocation. The live native probe now uses two lobby
sessions: the owner waits until the guest appears, publishes one match, the
guest adopts it, and both proceed through ICE setup and validated rollback
input after signaling closes.

`NativeOnlineSession` gives the menu flow one process-lifetime owner for that
lobby. It adopts the agreed proposal into the native match configuration and
retains the ready `NativeNetplayRoom` at a stable address while the combat loop
borrows it. The diagnostic command-line path continues to own its room
directly. A deployed two-client stock match exercised that shared adoption
code: both clients agreed on RNG seed `9815032c`, released signaling, entered
Battlefield, and remained connected without a checksum or transport failure
during the observation window.

The first in-game room screen now starts and pumps that owner. On native builds,
the old Tournament Melee selection opens create-room and join-room choices.
Create generates an eight-character shareable code; join provides an
eight-character controller-driven editor. The connected room view displays the
live connection state, room code, participants, roles, ownership, and failures,
and leaving it shuts down signaling before returning to Versus. Closing the
client also unconditionally stops the process-owned room. A live create-room
run against the deployed Worker displayed the assigned code and local owner,
then returned to the stock menu with all room text removed. This is functional
bring-up inside the existing right preview pane. The temporary Tournament label
and description remain, and profiles, local-player count, role/ready controls,
room rules, chat, and the final native side-rail presentation are not attached
yet.

Start on a connected waiting room now enters stock character select without
destroying that process-owned session. The screen exposes only the number of
local human slots advertised by that PC. It translates chosen fighters and
costumes into stable stock content IDs and publishes them as the participant's
bounded selection payload. Start confirms the local group only when every local
slot has selected a fighter; changing any choice clears confirmation. Once every
active participant has confirmed and the room contains two to four fighters,
the clients combine selections in stable room order and apply the same roster
before leaving character select. A deployed one-client run selected and
confirmed Ganondorf and correctly waited for another active participant. A
room now enables character select only when its active participants advertise
at least two and no more than four fighters, instead of allowing a one-person
room to enter an unexplained wait in character select.

The initial stock stage-select integration completed that visible setup path
with the owner choosing. The later [lobby stage-selection flow](native_lobby_stage_checkpoint.md)
assigns control to the previous loser or lowest-index active player, including
when the owner spectates. After that player chooses, the owner builds a canonical match from the combined
CSS roster, visible VS rules, item settings, and stable stock-stage identity.
The online-session owner assigns the fresh session identity and room topology;
every guest verifies and adopts those same bytes. Both clients remain on stage
select while match agreement and ICE finish, then apply the agreed descriptor
to the entered scene and start gameplay.

A deployed two-client run exercised the full room -> CSS -> stage select ->
match path. The host chose Captain Falcon and Battlefield, the guest chose
Ganondorf, and both reported session `337e943cc1098f95` with initial RNG seed
`ed9b0688` before releasing signaling. The two-minute match completed normally.
The host received 7,433 input packets with none rejected, compared 7,381
confirmed peer checksums, and needed no rollback correction; the guest received
7,485 with none rejected, compared 7,345 checksums, and corrected two timelines
across four hidden frames. Neither client reported a checksum mismatch. The
clients can still advance through results independently and the online session
is released after the match, so synchronized results and return-to-room/rotation
remain open.

`native_connection_probe` exercises that entire path against the local Worker:
two native clients join one room, gather and exchange their distinct ICE
descriptions, select a route, and bind room peer IDs to non-overlapping numeric
peer and controller-port ownership. Each client locks its verified topology
before sending a ready datagram over ICE. Receiving that datagram proves the
sender has also locked its routes, allowing both WebSockets to close without
the faster client removing a slower peer. The probe then exchanges validated
shared-PC input batches and resolves the same four controller samples on both
sides.

Room startup now treats the canonical proposal as the only source of match
session identity, numeric rollback peers, and controller-port ownership. The
room coordinator derives its local exchange and remote routes directly from the
proposal and local stable peer ID. Its former duplicate session, local-peer,
port-mask, peer-count, and remote-peer configuration was removed, so lobby code
cannot hand transport a topology that disagrees with the bytes every participant
accepted. The local Worker-to-ICE connection probe passes with this narrower
boundary and still exchanges all four shared-PC controller samples after
signaling closes.

The first presentation-independent lobby policy now derives match topology from
room presence. It sorts active participants by stable join order, assigns compact
numeric rollback identities, and maps each participant's advertised local-player
count onto the match's human controller ports. The first active peer owns the
remaining CPU or empty ports so every input history still has one sender.
Waiting participants and spectators stay outside combat topology. Build
mismatches, duplicate identities or join orders, more than four active PCs, and
human-count mismatches reject the proposal without partially modifying it. This
policy now drives the live connection probe instead of hard-coded peer masks.

With `wrangler dev` running as above, reproduce it with:

```powershell
cmake --build build/native --target native_connection_probe -j 8
.\build\native\native_connection_probe.exe http://127.0.0.1:8799
```

The live match loop now accepts this room transport through command-line setup.
Both clients must provide the same session and match settings plus explicit
numeric-peer, room-peer-ID, and controller-port assignments. Before the VS
scene creates stage or fighter state, the clients establish ICE and derive the
same Melee RNG seed from the shared session ID and semantic match signature.
This keeps scene-enter randomization deterministic. Once the ICE ready exchange
finishes, signaling closes and match input continues only over the selected
ICE route for direct CLI rooms. Home rooms retain signaling across matches so
the owner can clear the completed proposal, publish another, and keep the same
participants and ICE connection. Active match input still uses ICE.

A two-process, one-stock Final Destination run established the room route,
reported the same initial seed (`3a92004b`) in both processes, released both
signaling clients, and remained in the active match for more than 40 seconds
without a semantic checksum mismatch. The first run before seed agreement
aborted at frame zero with different checksums, confirming that the validator
detects divergent initialization. Running two clients from one installation
can make one Aurora process report a locked pipeline-cache database; this did
not interrupt the match transport and is not distributed-machine evidence.

A second live run applied a three-frame outbound delay to both clients and
changed one remote-owned human controller through the native control channel.
The predicting client corrected one timeline across four hidden frames and
compared 3,114 confirmed peer checksums without a mismatch; the other compared
2,465. Both clients then handled a normal window-close request and shut down
their match transports cleanly. This exercises actual late-input rollback over
room ICE rather than only steady neutral input.

Room setup currently accepts an optional `--netplay-stun host,port`. Both the
local Worker-to-native probe and a 2026-09-22 run against the private deployed
Worker exercise automatic credential retrieval in STUN-only mode before two
agents connect, signaling closes, and rollback input crosses the selected ICE
route. The deployed run completed despite an expected same-machine ICE role
conflict warning. Cloudflare TURN secrets, a forced-relay cross-network test,
and lobby recovery after a real transport failure remain open. Active transport
failure aborts through normal Melee cleanup rather than terminating the client
or continuing with neutral input. Visible character selection now publishes and
confirms each PC's local roster. Visible character and owner-authoritative stage
selection produce the canonical match proposal and launch room-backed play.
Rotation and synchronized results remain open.

Home-room match cleanup now returns each client to its CSS or practice flow
without leaving the room. The host clears the finished proposal; both clients
wait for its newer empty epoch before accepting another match. Setup discards
packets from an earlier session while still rejecting malformed setup for the
current session. Guest result input uses its mapped logical port. A local
two-client run completed an aborted first match, a full two-minute second match,
and returned both clients to practice under the same invite code, with
6360/6656 matching confirmed checksums in round two. Another local run failed
at frame 2059 of round two, so repeat-round determinism remains open.

Online matches retain the latest 64 semantic-state breakdowns in each process.
A client detecting a checksum mismatch prints the stored hashes after
controller, camera, player, object, ground, item, and fighter state for the
mismatched frame. Compare the first different category in the two error logs.
The fixed-size capture writes no per-frame text during normal play. A
subsequent two-round run reached results again with 7433/7378 matching
confirmed checksums; a separate disconnect/rejoin run diverged at frame 768
of the replacement match before this automatic capture was enabled.

The trace now separates each ground object's core and pose, and each item's
identity, motion, animation, individual hitbox slots, damage, commands,
relations, and pose. Repeat-round captures localized a frame-zero false
mismatch to uninitialized `Ground.x0` plus `Ground.xC0` while its material flag
was off. `x0` is excluded from the semantic checksum; `xC0` is checked only
when the original material code reads it. Later captures localized another
false mismatch to a disabled Food-item hitbox slot. Item hitbox payload is now
checked only when the slot is active; the disabled state itself remains in the
checksum. A further Bob-omb mismatch reached an item hurtbox: the checksum
now excludes the item-only uninitialized `bone_idx` and cached hurtbox positions
while collision is set to update them on demand. The next two-client run
completed a full first round, then failed at frame 1424 of round two in the
Capsule item's damage/timer/counter group. Its earlier item fields, hitboxes,
and hurtbox matched. Source-level collision behavior was not changed.
Repeated-room runtime acceptance remains open.

The next run retained 17 raw item damage/timer/counter scalars per item for
diagnosis. After a full first round and same-room return, a Crate produced the
first semantic difference at frame 555 in `xCAC_angle`. Its knockback was zero;
the original collision path writes angle alongside knockback, so the digest
ignores the angle while knockback is inactive and still detects changes while
active. A fresh two-round runtime check is still required.

The portable checksum also covers the common fighter tail after the
character-specific persistent union: bury stage and timers plus the two shared
integer pairs used by hammer state. It serializes those fields individually,
without copying adjacent union storage or native padding. Its per-kind manifest
now covers every named scalar and vector in the persistent character variables,
plus their fighter/item/ground references through stable object IDs. Native
resource pointers and unnamed filler remain excluded. Common locomotion,
standard attack-control, damage, guard, knockdown/tech, ledge, and item-handling
motion state plus standard grabs, throws, grabbed wait/damage, and thrown-victim
state is selected by the active motion ID, so inactive union bytes do not enter
the digest. Motion-owned fighter references use the same stable object IDs as
persistent fighter relationships. The same manifest covers carried victims and
cargo throws, Yoshi eggs, Koopa/Mewtwo/Kirby capture families, bury, Warp Star,
hammer transitions, mushroom transitions, entry platforms, freeze, Like Like,
and barrel state. All 61 native tests and the release client build pass. A
two-process localhost runtime attempt
remained in Aurora startup and shader preparation for the full 90-second
observation window, so it did not compare any gameplay frames. Cross-process
evidence above therefore still applies to the earlier fighter/RNG checksum. A
fresh two-client gameplay run must validate the broader checksum before it can
be treated as online determinism proof across fighters, items, stages, deaths,
match transitions, longer delays, or different machines.

## Game & Watch article layout regression (2026-09-22)

A four-fighter home match diverged when Mr. Game & Watch spawned a Chef item.
Both processes spawned variant 4, but the article helper wrote an eight-byte
native model-visibility pointer across the retail four-byte prefix and Chef's
following variant. The peers subsequently read invalid variants 527 and 439;
the first checksum difference was in the item joint's X rotation. The native
item union now places the Chef variant and Rescue owner after the full pointer.
The semantic digest covers the active Chef variant and Rescue owner relation;
the focused test verifies both survive the shared helper's pointer assignment.

`build/native/chef-verified-host.log` and `chef-verified-guest.log` record a
fresh local two-process, four-fighter Battlefield run with a three-frame
outbound delay. Chef, Zelda's Sheik transformation, Ice Climbers jump, and
Ganondorf movement ran through rollback corrections to the winner screen.
The clients compared 1423 and 2984 confirmed checksums, with no mismatch or
rejected packets. `chef-fixed-results.png` shows the result. All 354 native
tests pass. This validates the earlier action regression locally; other move
combinations, physical shared-PC input, and cross-network determinism remain
to be checked.

## Custom fighter room match (2026-09-23)

The room roster now carries installed fighter-bundle identities as well as
stock selections and costume identities. Stage select submits the custom
package ID in the canonical proposal. Match setup requests any missing bundle
from its owner's PC, checks its SHA-256 identity, and leaves the receiver's
collection uninstalled. Each peer resolves the package against native behavior
compiled into the matching client build, then registers its assets after the
fighter-system reset. Match adoption clears the previous practice scene's
custom registration before validating the new roster.

An isolated two-client run selected Rook and Falco in their respective CSS
screens, joined a room, and played Battlefield from stage select through a
two-minute round, sudden death, results, and return to CSS in the same room.
Both clients rendered Rook. The guest held Rook's 1,801,640-byte package only
in cache, without an installed marker, and did not offer Rook on its CSS.
The peers agreed on seed `1bfc65b5`, compared 7,436/7,437 confirmed checksums
in the first round and 1,613 on each side in sudden death, and reported no
checksum mismatch. This is same-machine, one-custom-fighter evidence; multiple
custom kinds, forced correction, relay, and cross-network runs remain open. A
second run started with an empty guest content root and again reached matching
Battlefield gameplay. Its newly cached bundle was 1,801,640 bytes, and the
file's SHA-256 matched package ID
`4fc6fb3ec1bfda9ebd2d939142d3a7e26389794b883ae6ec2d0beabf31257cc3`.

## Explicit room gifting (2026-09-23)

While waiting in a room, Collection can offer an installed package to one
connected participant. The recipient's sidebar presents Accept & install and
No thanks. A decline leaves the recipient's content root untouched. Acceptance
installs a verified cached package immediately or transfers the missing bundle
over the existing room ICE connection, checks its SHA-256 identity, and then
creates the per-install collection marker. Each offer has a separate serial so
declining a package does not suppress a later offer of that same package.
Gifts stop when the room configures a match; ordinary match-required downloads
remain cache-only.

Local two-process checks completed transfer and install of Rook's 1,801,640-byte
bundle into a fresh guest root, immediate acceptance from a cached-only root,
decline without a created content root, and decline followed by a fresh offer
and acceptance. The received file's digest matched its package ID. The build
and all 359 native tests pass. This is same-machine, one-recipient evidence;
costume gifting, simultaneous offers, and relay/cross-network routes remain
untested. A later boot from the gifted guest store added Rook's portrait to
the original CSS, selected it with the controller cursor, and entered practice
with Rook and Sandbag. The CSS editor also refreshed a newly installed bundle
while the home CSS remained open in a separate live run.

A separate local two-process check gifted an installed Mario costume to a
fresh guest store. The received 473,598-byte cache file matched ID
`1d9fb26d02038326c192f8f74a21d2a3baff2fae786c26172bc368aa1c412bcd`;
the guest gained an installed marker. On a subsequent boot, the guest selected
Mario and the received skin through COSTUMES, then entered practice. The
package contains original Mario art, so visual skin differences are unproven.
The costume picker now refreshes on in-session gift installation.

## Waiting participant match boundary (2026-09-23)

The room coordinator retains an active proposal if its owner leaves while
serving only as a spectator. A waiting participant can update their selection
without invalidating an active match; changes to active participants still
invalidate setup as before. A hibernatable-room integration test covers a
spectator owner, two active peers, a waiting peer joining and selecting during
the match, and transfer of room ownership while the match proposal remains.
The native client now keeps a room participant in the waiting state if the
published match does not include that participant or their presence role is
not active. The signaling tests and type checks pass; the release client and
all 359 native tests pass. This establishes non-playing room membership, not
spectator rendering, mid-match snapshot/catch-up, queue rotation, or a
cross-machine runtime demonstration.

## Direct custom-fighter checksum checkpoint (2026-09-24)

Direct raw-UDP matches can now select an installed custom fighter with
`--pN human:custom` and its `--pN-fighter` package ID. Both peers must have
that exact bundle installed locally and launch matching match options; this
path does not transfer packages. Sidebar rooms retain their verified cache-only
match transfer.

An isolated two-client direct match ran the same installed Mario clone against
Falco for a one-minute Battlefield round and its tied sudden death. Both peers
agreed on the initial RNG seed and showed matching results. The first run
revealed that a peer ahead by a few frames could replace its previous checksum
before the slower client compared it; the guest counted zero comparisons while
the host counted thousands. Each peer now retains pending checksums in the
fixed rollback-history window and fails explicitly if they overflow before
comparison. A focused UDP test sends two checksums before the receiver's
simulation catches up. On the repeated live run with three frames of outbound
host delay, the host/guest compared 3,834/3,831 confirmed checksums in the
timed round and 1,580/1,577 in sudden death. Both reported zero rejected
packets and reached the same result. Logs and UI captures are
`build/native/direct-clone-host3.log`,
`build/native/direct-clone-guest3.log`, and
`build/native/direct-clone-checksums-results-{host,guest}.png`.
The native executable builds and all 374 tests pass. This is a same-machine
raw-UDP check with one custom fighter; it does not establish cross-network
connectivity or prove correction through a specific authored article callback.

A later direct run exercised four instances of the same installed Mario clone:
two human slots on separate local clients and two CPU slots. The CPU profiles
had previously been empty for custom kinds; after copying Mario's imported
attack choices into clone-owned runtime storage, the CPUs moved, attacked,
dealt damage and completed a local match. The two-client run reached matching
results after a one-minute round and sudden death. With a three-frame host
outbound delay and one host B input, the guest corrected two timelines across
eight hidden frames. Host/guest confirmed-checksum comparisons were
3,823/3,828 in the round and 1,187/1,185 in sudden death, with zero rejected
packets. Logs: `build/native/four-clone-fixed-{host,guest}.log`.
This proves same-machine four-instance execution of one bundle, not a
correction specifically through article birth.

Four separately installed package IDs cloned from the same Mario draft then
occupied all four custom fighter slots in another two-client direct match.
Alpha and Beta were human slots; Gamma and Delta were CPUs. Both clients
showed Delta winning. With one host B input and three outbound-delay frames,
the guest corrected two timelines across eight hidden frames. The host/guest
compared 3,752/3,349 confirmed checksums and rejected no packets. The host
waited 14.9 seconds across 1,474 input stalls, a remaining performance issue
to profile. Package IDs, logs and both result captures are under
`build/native/four-distinct/`. These are four independent package identities
and module loads derived from one Mario implementation; unrelated custom
fighter behavior and cross-network synchronization remain untested.

## Direct-match disconnect checkpoint (2026-09-24)

Two local raw-UDP clients entered the same two-minute Battlefield match with
four fighter slots (Ganondorf human; Zelda, Ice Climbers, and Game & Watch CPU).
They agreed on seed `6666e092`. After both reached gameplay, the guest process
was terminated. The host declared `peer input timed out during the match` after
5,063 ms of input stalls, reported 1,406 received input packets and 1,405
confirmed peer checksum comparisons, and logged `Match aborted; returning
home.` A host screenshot showed character select rather than results; no winner
or result rotation was awarded. Evidence is in
`build/native/disconnect-host.log` and the corresponding host capture under
`screenshots/`.

This verifies the direct diagnostic route's timeout and No Contest return. That
route has no persistent signaling room, so its home screen correctly showed
the room as offline. Returning connected room participants to their lobby,
handling a spectator-only departure, and distinguishing short packet loss from
a disconnect still require separate live checks.

## Local packet-loss and jitter checkpoint (2026-09-24)

`tools/netplay_udp_fault_proxy.py` provides a repeatable single-PC network
fault point for direct matches. Bind the proxy to a third loopback port and
configure **both** clients' remote peer as that port; the native transport
requires the datagram's source port to match the configured remote endpoint.
For example, with clients listening on 47350 and 47351:

```powershell
python tools/netplay_udp_fault_proxy.py --listen 47352 --host-port 47350 --guest-port 47351 --seed 121 --drop-percent 5 --delay-ms 12 --jitter-ms 18
```

A one-minute four-slot Battlefield direct match ran through that proxy. Its
periodic counter reached 7,734 received datagrams, 411 intentionally dropped,
and 7,322 forwarded. The clients received 3,653 and 3,668 input packets,
rejected zero, compared 2,711 and 3,587 confirmed checksums without a logged
mismatch, and reached visually identical results: Game & Watch first, Zelda
and Ice Climbers tied second, Ganondorf fourth. Evidence is in
`build/native/fault-run/{proxy,host2,guest2}.log` and
`build/native/fault-run/{host,guest}-result.png`.

This covers moderate independent packet loss and 12–30 ms per-datagram delay
on loopback. Both clients reported zero rollback corrections because their
human input did not change during the fault window; previous artificial-delay
runs cover correction separately.

A second one-minute run combined the same loss/jitter profile, three frames of
host outbound delay, and five P1 virtual-pad changes (special, attack, jump,
horizontal stick, special). The proxy counter reached 9,258 datagrams received,
467 dropped, and 8,788 forwarded. The guest corrected five timelines across
14 hidden frames; the host corrected none. They received 4,348/4,439 input
packets, rejected none, and compared 2,861/2,811 confirmed checksums without
a logged mismatch. Both result captures show Game & Watch first, Ganondorf
second, Ice Climbers third, and Sheik fourth, with matching KOs and falls.
Evidence is in `build/native/fault-run/{proxy,host,guest}-correction.log` and
`build/native/fault-run/{host,guest}-correction-result.png`.

This establishes one local correction-through-loss path, not general reliability
under loss bursts, larger latency, or cross-network routes. The diagnostic
proxy is separate from room ICE/TURN and cannot validate those transports.
