# Freak Melee signaling service

This Worker is the low-frequency rendezvous path for native rooms. One
Cloudflare Durable Object coordinates each room ID. It keeps hibernatable
WebSockets for presence and directed connection-setup messages; gameplay input
continues over the native client's UDP data plane and does not depend on this
service after peers establish a direct or relayed route.

Repository builds do not deploy the service automatically. The private deployment
is `https://freak-melee-signaling.sndrec32exe.workers.dev`. The 2026-10-19
account-key/social/guest-lobby deployment is `8e1f76a3-9a77-4ade-8efe-7eee190a196a`. Native room protocol v5 includes seat
assignments, owner overrides, match rotation, content controls and lobby chat.

`/chat/connect?peer=<32 hex digits>` serves the separate global conversation.
Its WebSocket carries recent chat history and the connected-player directory.
Lobby chat uses each room's existing WebSocket and retains messages while that
room remains occupied. See [chat](../../docs/native_chat.md) for controls,
delivery semantics and current test evidence.

The account service provides persistent identities, generated Ed25519 keys,
portable account-key files, recovery codes and device-session management.
The service verifies signed challenges using built-in Node crypto. The native
client generates/signs keys with Monocypher and protects its local record.
See the [account contract](../../docs/native_account_auth.md) for controls,
protocol, storage and verification scope. The old Windows WebAuthn flow has
been removed at the user's request.

The local service also implements friends, offline letters, verified chat/lobby
profiles and expiring lobby invitations. Its account socket upgrade uses
`Authorization: Bearer <session>` and `X-Freak-Account: <account_name>`.
The service supplies public account metadata in presence and chat records.
Signing in from a guest lobby uses authenticated `POST /v1/accounts/<name>/lobby/attach`
with its room code and private Welcome connection ID. The service preserves the
live socket and lobby state. Replacing an active guest socket requires its private
Welcome ID in `X-Freak-Connection`; a public peer ID alone is insufficient.
Update the service and client together: the new presence records require both
ends of the updated wire format. Local native checks set both
`FREAK_MELEE_SERVICE_ENDPOINT` and `FREAK_MELEE_CHAT_ENDPOINT` to the same
localhost service. The [social guide](../../docs/native_social_accounts.md)
records social behavior and native verification. The account-key suite now
passes 71 service tests in ten files. Two hidden/muted native clients also pass
the hosted account-key/profile/invitation/join/sign-out review:
`.local/account-key-hosted-1/receipt.json`.

Local checks:

```powershell
cd services/signaling
npm install
npm run check
npm run deploy:dry
```

`npm run types` regenerates `src/worker-configuration.d.ts` from
`wrangler.jsonc`; this generated runtime file is intentionally ignored. The
Worker uses Cloudflare's current hibernation API so idle room connections do not
keep the Durable Object active.

## NAT traversal

Before opening a room WebSocket, the native client requests
`GET /v1/turn-credentials`. The response is a bounded binary configuration
containing the public STUN endpoint and, when configured, one short-lived UDP
TURN credential. The long-lived Cloudflare key remains in Worker secrets and
is never shipped in the game.

Local development deliberately returns STUN-only configuration when the TURN
secrets are absent. For a deployed Worker, create a Cloudflare TURN key and set
both secrets interactively:

```powershell
npx wrangler secret put TURN_KEY_ID
npx wrangler secret put TURN_KEY_API_TOKEN
```

The Worker requests credentials with a 24-hour lifetime. Every room startup
fetches a fresh configuration, and clients reject malformed or oversized
responses before starting ICE. Direct candidates remain preferred by ICE; TURN
is available when the peers cannot establish a direct route.

The recorded September 2026 deployment returned STUN-only configuration.
A live two-client `native_connection_probe` against the
deployed origin joined a Durable Object room, agreed on an owner proposal,
closed both signaling sockets, and exchanged validated shared-PC rollback input
over ICE. Forced-relay validation remains pending TURN provisioning.

## Connection

The native client generates a private uppercase room code and connects to:

```text
wss://<deployment>/v1/rooms/<ROOM>/connect
  ?peer=<stable-session-peer-id>
  &name=<display-name>
  &build=<exact-client-build-id>
  &players=0..4
  [&active=<seat-bit-mask>&waiting=<seat-bit-mask>]
```

Room IDs contain 6–12 uppercase letters or digits. Peer IDs contain 8–64
letters, digits, `_`, or `-`. A later connection with the same peer ID replaces
the earlier socket, which provides a simple reconnect path. Each bit in the
optional masks identifies a local player seat (bit 0 is seat 0); omitted masks
fill available fighter spots up to four and place overflow seats in waiting.
Seats in neither mask spectate. The room code controls lobby admission.
Signed-in sockets also carry a verified account identity for profiles and
invitations; local guests can join without an account.

The server sends:

- `welcome`: connection ID, peer ID, and current room revision;
- `presence`: ordered participant snapshot, current owner, build IDs, active
  and waiting seat masks,
  local-player counts, bounded opaque per-PC selections, ready state, and the
  current match-proposal epoch and opaque payload;
- `signal`: opaque connection candidate or negotiation payload from a named
  peer;
- `signal_sent`: acknowledgement that the target was connected;
- `chat_begin`, `chat_entry`, `chat_end`: a subscribed history response and live
  messages, with sequence, nonce, sender peer, name and text;
- `chat_rejected`: a chat error and its nonce, independent of room errors; and
- `error`: rejected client message with a human-readable reason.

Clients send:

- `update` with any of `name`, `activeMask`, `waitingMask`, `localPlayers`,
  `selection`, and `ready`;
- `signal` with a connected `to` peer ID and bounded opaque payload;
- `request_presence` for a fresh snapshot; or
- `set_match` from the current room owner with a 1–4096-byte opaque proposal,
  or an empty payload to clear it; or
- `assign_seats` from the current room owner, naming a connected peer and its
  new active and waiting seat masks. The named peer may subsequently send its
  own `update`; the last accepted assignment wins;
- `set_rotation` selects Manual, Winner stays or Winner out;
- `report_match` reports a completed eligible 1v1 result for rotation;
- `request_chat` subscribes to lobby messages and requests recent history;
- `post_chat` supplies a nonzero, increasing 32-bit nonce and up to 512 UTF-8
  bytes of text. The server gets the sender's peer and name from its socket.

Lobby history contains at most 64 messages. SQLite writes precede broadcasts.
Retries reuse their nonce and are deduplicated while retained; expired retries
are rejected. Posting is limited to one new message every 500 ms per connection;
history requests are limited to one per second. Subscription and rate-limit
state live in connection attachments. The last socket leaving clears the
history. Room chat includes waiting players and spectators.


A PC with an active seat can become ready only while a proposal exists. Publishing,
replacing, or clearing the proposal resets every participant's ready state.
Before publication, changing active-player topology or an active participant's
selection clears readiness. Once a proposal exists, those changes are rejected
until the owner clears it. Waiting-only changes preserve the proposal. An
assignment cannot exceed four active fighters across the room. An owner's
active-seat override clears readiness just like the peer's own update. An
active PC disconnect clears the proposal; a spectator-only owner can leave
without ending the match. Replacing a connection with the same peer ID retains
its assignment.

WebSocket messages use a versioned binary protocol with a fixed eight-byte
header: ASCII `FMS1`, protocol version 5, message type, and a big-endian payload
length. Strings use length-prefixed UTF-8, revisions and join order use
big-endian 64-bit integers, and signaling and selection payloads remain opaque
bytes. Per-PC selections are limited to 512 bytes. The protocol has no legacy
JSON path; both sides change together while the client is pre-release.

Messages are bounded to 16 KiB and rooms to 32 connected PCs. The service
forwards negotiation data only to
the requested room peer and never relays gameplay packets, content packages,
or replays. Presence lives in serialized WebSocket attachments so it survives
Durable Object hibernation. SQLite stores the monotonically increasing room
revision and join order used for deterministic ownership transfer, plus the
proposal epoch and bytes that must survive hibernation independently of any
one connection.

Current tests run in the Workers runtime and cover route validation, TURN
credential exchange and binary encoding, binary room protocol exchange, two-client
presence, owner selection, seat-mask/ready/selection updates, directed candidate delivery,
owner-only match publication, readiness invalidation, and proposal cleanup on
ownership transfer. A separate eviction test hibernates the room object with a
live socket and verifies that its serialized participant state and stored
proposal are restored. The server deliberately does not interpret either
gameplay payload; the native client owns their bounded codecs and agreement.
External relay validation, automated waiting-order policy, and explicit lobby
recovery after an active disconnect
remain separate work.

The native end-to-end probe uses the same room route to exchange one complete,
versioned ICE description per compatible active peer. It locks the selected
topology, closes both signaling WebSockets, and only then transfers real
rollback input packets over ICE, demonstrating that established match traffic
does not rely on the Worker:

```powershell
# Run from this directory in one terminal.
npx wrangler dev --port 8799

# Run from the melee repository root in another terminal.
cmake --build build/native --target native_connection_probe -j 8
.\build\native\native_connection_probe.exe http://127.0.0.1:8799

# Or use the current private deployment.
.\build\native\native_connection_probe.exe https://freak-melee-signaling.sndrec32exe.workers.dev
```

## Quick Play service work

The local source now includes `QuickPlayQueue`, partitioned by mode and exact
build. Party leaders can configure a solo or doubles party and enter/cancel a
search through the room WebSocket. Whole parties are reserved together, so a
shared-PC pair or two teammates on separate PCs retain their team assignment.
Each assigned PC receives its own match-room admission token.

The Quick Play Durable Object is deployed. Local type/test and native
checks cover the following behavior. Native queue controls, assigned-room handoff,
ready practice, B-to-unready and the three-second countdown now reach live
singles and shared-PC doubles matches in local checks. PCs prepare against the
current roster revision with matching content digests. The service persists the
countdown and constructs the fixed-rule native proposal after its deadline.
The ordinary owner start command is rejected inside a Quick Play match. Every
PC reports a completed result; agreement gives stage choice to the losing side.
Each losing seat must vote for the same stage before the next ready countdown.
Stale reports cannot clear a newer match. Native singles and shared-PC doubles
round/rematch checks pass using stage-vote automation. The player-facing stage
confirmation UI remains open. Countdown cancellation now receives a service
decision before the client exits practice. A committed match wins a late request;
an earlier round's request cannot cancel a later one. Native singles and doubles
checks also recover to the original party and requeue after an opponent exits.
The current service suite has 44 passing tests;
its socket cleanup waits for closure before teardown. See the
[rematch checkpoint](../../docs/native_quick_rematch_checkpoint.md).
The [return checkpoint](../../docs/native_quick_return_checkpoint.md) records
the cancellation protocol and native recovery evidence.
The native client also preserves viable P2P gameplay after a signaling loss,
then reconnects to its original party after the match releases the room. Singles
and shared-PC doubles checks finish that match and requeue into a second one.

See [the Quick Play checkpoint](../../docs/native_quick_play_checkpoint.md) for
the wire records, state ownership, recovery model and implementation sequence.
