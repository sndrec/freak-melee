# Stage selection in player-created lobbies

When all active fighters are ready, a three-second countdown leads to stage
selection. START pauses practice and unreadies; START resumes and readies.
Waiting players and spectators do not count. Rules are shared by the lobby and
edited by its host in the original Rules screen.

Stage Selection defaults to **Loser**. The previous game's loser chooses its
stage. In teams, the lowest match-port index on the losing team chooses. Before the first
game, or when that player leaves or stops fighting, the lowest-index active
player chooses. Match ports follow lobby join order and then local seat order;
the CSS's local-first card display does not change this order.

For free-for-all results, the lowest-index player tied for the worst result
chooses. A draw or cancelled match clears the previous loser and uses the active
roster fallback. These rules apply to player-created lobbies. Quick Play keeps
its separate random-first-stage and losing-side rematch flow.

The other Stage Selection settings work as follows:

- **On:** any active player can choose; the first accepted choice wins.
- **Random:** the service chooses from the Random Stage Switch settings, with
  the stock recent-stage and paired-stage cooldowns.
- **Ordered:** stages cycle through the stock stage-select order.
- **Turns:** active players take turns in canonical match-port order.

Random and Ordered choose their stage when the countdown finishes. Their stage
history advances when the match is committed, so cancelling a countdown does
not consume a turn or stage.

## Match flow

The countdown opens the original stage-select scene on the fighting clients.
For Loser and Turns, only the chosen player's assigned physical controller can
move its cursor or select a stage, including the second or later controller on
a shared PC. The service publishes that choice; the owner constructs the match
with its rules and the active roster.

A spectating owner can stay on CSS or keep practicing while the active players
choose and play. A later game requires the active fighters to ready again.
Watching remains an explicit choice.

The service saves the losing player's peer identity and local seat. It resolves
the next chooser against the current active roster. Stage choices carry the
current match epoch. Changing the active roster, active character selection or
the shared rules cancels the countdown and clears the pending
stage. Stale choices and repeated result reports cannot change a newer round.
The first accepted stage choice wins until that selection is cancelled.

The native and service wire definitions change together. Use matching client
and service builds.

## Automatic-readiness verification (2026-11-15)

The full signaling suite passes 82 tests in 12 files. Native wire, match and
selection tests pass (3/3), including shared-rule records, SD penalties and empty
inactive seats. The release client and control executable build successfully.

Hidden, muted native reviews completed actual matches and returned through
Results for every stage policy. The Loser review used three clients: the owner
stayed on CSS, a shared PC used only its second controller, and the other PC
lost the match and received the next stage pick. The inactive controller could
not select a stage. All reviews cancelled and restarted a countdown with START,
checked guest rule values, and preserved the guest's settings file. Ordered
advanced from stage 4 to stage 11; Random respected the FD-only switch mask in
the live test. Its full 29-stage selection and cooldown behavior is covered by
the service tests.

Evidence: `.local/native-ui-round2-lobby-flow-review-3/review.json`,
`.local/native-ui-round2-lobby-flow-review-other-policies-1/review.json`,
`.local/native-ui-round2-lobby-flow-full-tests.log`,
`.local/native-ui-round2-lobby-flow-codec-tests.log` and
`.local/native-ui-round2-lobby-flow-build-3.log`. These reviews use virtual
controllers on one PC with local signaling.

## Earlier verification, before automatic lobby readiness

The native wire, roster and result tests pass. Result checks cover individual
and team ranks, tied last place, vacant slots, draws and cancellations. The
signaling tests cover an observing owner, a shared-PC chooser on local seat 1,
unauthorized input, fallback after spectating, stale choices/results, and saved
chooser state across Durable Object eviction.

The connection-only native probe also passes two consecutive matches with an
inactive owner. Evidence: `.local/native-lobby-stage-owner-probe.log`.

Three hidden, muted game clients also pass the original stage-select and match
scenes. The owner has no active seat and keeps practicing. The first active peer
has two local controllers, with only seat 1 fighting. Controller 0 cannot move
its stage cursor; controller 1 can. The other fighting PC cannot move its own
cursor while waiting. The selected stage starts on both fighting clients, and
the owner remains outside the simulation.

The second fighter loses its stock. Both match endpoints agree at frame 451:
fighter digest `68555f0753d0b04f`, RNG `cd1383f8`. The actual result assigns the
next stage choice to that losing peer. The owner stays in practice throughout.

Driver: `tools/review_native_lobby_stage.py`. Initial live evidence:
`.local/native-lobby-stage-live-1/receipt.json`, with full screenshots and
per-client logs. Native tests: `.local/native-lobby-stage-tests-1.log`.
Service tests: `.local/native-lobby-stage-service-final-3.log` (45/45).
Build: `.local/build-native-lobby-stage-3.log`.

The final extended run, `.local/native-lobby-stage-live-4/receipt.json`, also
returns both players through Results to CSS and ready practice. Both remain
outside stage select until the owner presses Start match again. The losing peer
then controls the next stage scene and starts a second, distinct shared session.
Its first match ends at frame 462 with matching fighter digest
`b9d4252af5258659` and RNG `99a584f7`; both peers compare 460 checksums.
Earlier extended drivers stopped at the return-to-practice controls; their
failed receipts remain in the `live-2` and `live-3` folders. The final driver
waits for the CSS selection screen and uses the assigned controller's START.

The regular native signaling probe passes as well:
`.local/native-lobby-stage-signaling-probe.log`. Source and binary hashes are in
`.local/native-lobby-stage-receipt.json`.

One parallel service-test run during native linking observed the existing
Quick Play countdown deadline winning an unready race. The service suite passes
with files run sequentially; the result is preserved in
`.local/native-lobby-stage-service-final.log` rather than counted as a pass.

These checks use virtual controllers and one PC with localhost signaling.
Physical controllers, separate PCs and relay routes remain separate acceptance.
