# Online connection recovery

Players send a heartbeat every 10 seconds. After 45 seconds without activity,
the service treats the connection as uncertain. It reserves the player's place
for another two minutes before removing it. A lobby with uncertain connections
keeps its settings and history during that period. Connected participants keep
the lobby alive, even when its host disconnects.

The newest lobby connection takes over a player's previous membership. Signed-in
players use their account identity across PCs. Guests use a private installation
key, shared by that installation's chat and lobby transports. Display names do
not identify players; two different guests can both be named Player.

Replacement connections have distinct connection IDs. Heartbeats, departures,
and delayed close events from an old connection cannot remove or renew its
replacement. Quick Play's party-to-match transition follows this same rule.

Implemented behavior:

- [x] Persist guest identity and send it only to the configured service origin.
- [x] Enforce one lobby membership per player and one chat presence per player.
- [x] Send client heartbeats and detect missing replies.
- [x] Expire dead connections through persistent server alarms.
- [x] Preserve lobby state and seats through a recovery grace period.
- [x] Reconnect transient transport failures without reclaiming superseded sessions.
- [x] Verify lost heartbeats, recovery, late close events, concurrent joins,
  hibernation, host loss, guests with identical names, and Quick Play handoff.

Do not deploy heartbeat enforcement ahead of the corresponding client build.

## Verification

`services/signaling/test/connection-recovery.spec.ts` and `chat.spec.ts` cover
connection leases, guest and account identity, seat recovery, replacement,
hibernation, and concurrent joins. The existing Quick Play tests cover the
party-to-match handoff and rematches with the new membership rule.

`native_signaling_probe --hold 65` checks idle native clients through multiple
heartbeat intervals. With an isolated local service running, the real Windows
transport can also be checked through a proxy that drops traffic for 55 seconds:

```powershell
node melee/tools/review_native_connection_recovery.mjs `
  --exe melee/build/native/native_signaling_probe.exe `
  --endpoint http://localhost:8798 --output .local/connection-outage
```

The native outage check passed with two reconnections and fresh replies from
both clients. Replacement checks also passed for lobby and chat transports;
neither client attempted to reclaim a replaced connection. Guest-key persistence
and origin restrictions pass in `native_account_test`.

Run the service test suite after large native links finish. Running all test
workers during an LTO link caused unrelated five-second test timeouts. A serial
service run passed after the link finished. These local checks do not establish
behavior on every PC or during an actual ISP outage.
