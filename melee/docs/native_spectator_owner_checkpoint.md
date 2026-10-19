# Starting matches as a spectating host

A lobby owner can publish a match for the active players while remaining outside
its simulation. The active players use the existing match agreement and input
exchange. The owner keeps the lobby connection and can continue local practice
or request viewing separately.

`nativeLobbySession_BeginMatch` assigns the active roster before deciding whether
the local client participates. A fighting owner enters match setup. A spectating
owner queues the proposal and stays in the lobby's Waiting state. A second start
request at the same proposal epoch is rejected. Publication still requires room
ownership, and an existing proposal must finish before a new one is published.

The signaling service already accepts proposals from a spectating owner and
clears them when a fighting peer reports the result. This change uses that
existing service behavior.

## Verification

The rebuilt `native_connection_probe` has an `--inactive-owner` case with three
independent native session instances and the real local signaling service:

- The owner joins first, gives up its active seat, and publishes the two players'
  match. A non-owner start and an immediate duplicate owner start are rejected.
- The players agree on the exact match bytes and establish ICE input exchange.
  Each resolves the other player's input and the authority's CPU-slot inputs.
- The owner stays in Waiting, with no configured match, exchange session,
  simulation peers or ready simulation.
- A fighting peer reports the result. Both players return to the lobby and the
  owner publishes a second session. Both players reach setup readiness again.
- The existing active-owner case also passes, including input delivery after
  signaling closes.

Commands, from the repository root:

```powershell
melee/build/native/native_connection_probe.exe http://127.0.0.1:8893
melee/build/native/native_connection_probe.exe http://127.0.0.1:8893 --inactive-owner
```

Evidence: `.local/native-inactive-owner-active-regression-1.log`,
`.local/native-inactive-owner-probe-1.log`,
`.local/build-native-inactive-owner-2.log` and
`.local/native-inactive-owner-receipt.json`. The probe and full native client
both build. The service ran locally with isolated storage; nothing was deployed.

The probe uses its existing connection-only build configuration. These results
prove session publication, agreement, input transport and return. They do not
exercise game scenes, package downloads or spectator playback.

## Remaining integration

The native menu flow still assumes the host is fighting when collecting the
start request and entering stage selection. Choosing who selects the stage for
a spectating host remains a UI decision. That scene flow and its full native
game acceptance remain open; this checkpoint does not complete the spectator
plan item.
