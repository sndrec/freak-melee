# Quick Play cancellation and party return

Updated: 2026-10-17. Tested locally with hidden, muted native clients. Quick Play
remains undeployed while its player-facing stage-choice controls await review.

## Countdown cancellation

B during the countdown requests an unready decision from the service. The client
keeps practicing until that answer arrives. If cancellation reaches the service
before the match commits, the service clears that PC's readiness and preparation,
cancels the countdown, and publishes the unlocked selection. The client then
returns to CSS. This applies to both players on a shared PC.

If the match commits first, the service returns the committed proposal and phase.
The client continues into that match with the other players. A late cancellation
does not close its signaling connection or strand it on CSS. The same decision
applies to the Return to selection button during preparation and countdown.

The request carries the last completed epoch, so a delayed cancellation from an
earlier round cannot unready a later one. The first round uses epoch zero. The
client waits for a newer presence revision showing its own unlocked selection,
and suppresses preparation and selection updates while the request is pending.

Client message 16, `UnreadyQuick`, carries one big-endian u64 completed epoch.
Presence and Quick State carry the answer. The current native build identity is
`freak-melee-native-dev-quick-return`.

## Opponent exit

The native checks terminate only a test-owned opponent process during a live
match. The survivor reaches its existing five-second missing-input timeout,
submits a no-contest report, and exits gameplay. Because the assigned group is
incomplete, the service retires it. The survivor reconnects to its original party
with the same peer ID and can enter and cancel another search.

The timeout is an expected failure in this test. Passing requires recovery to a
usable party and a subsequent search, rather than merely keeping the process
alive. Losing only signaling while gameplay remains viable is still governed by
the retained-P2P policy described below.

## Signaling loss during a viable match

An established P2P match keeps running when its signaling connection closes.
After gameplay releases its borrowed room, the affected client reconnects to its
original party. It does not wait for a result acknowledgement on the closed
socket. The other client reports the completed result, allowing the service to
retire the incomplete assigned group. Both clients can search again.

The native online session performs the reconnect on its next menu pump. It
preserves the room storage until the match releases it, keeping the live gameplay
transport and its borrowed pointers valid through shutdown.

`review_native_quick_signal_loss.py` places a local TCP forwarder in front of one
test client's signaling service. It closes that active TCP connection during
gameplay while leaving the native ICE transport untouched. It waits longer than
the missing-input timeout, then uses ordinary controller input to finish the
four-stock match. The test checks matching results, both original party codes,
unchanged peer identities, and a second actual netplay session after requeueing.

Singles evidence is in `.local/native-quick-signal-return-singles-1`: sessions
`2c2ffff68431a846` then `d3637f8f15cbeb9d`, with 1,877 confirmed checksum
comparisons in the first match. Shared-PC doubles evidence is in
`.local/native-quick-signal-return-doubles-1`: sessions `afa85312ff927f33` then
`e1851ff8a9ab9f69`, with 1,737 comparisons. Both complete without a gameplay
failure or reported checksum mismatch. The disconnected client cannot queue its
result, as expected; party recovery and the second match establish that it moves
past that boundary. Reviewed PNGs show the second matches running.

The native build and six focused regression checks pass after this change.

## Evidence

- `npm run check`: 44 service tests pass. The new cases force cancellation before
  commit, cancellation after commit, and an obsolete round request.
- Native build, six signaling/chat/selection/topology/CSS checks, and Wrangler
  deployment dry-run pass.
- `review_native_quick_return.py`: singles evidence is in
  `.local/native-quick-return-singles-2`; shared-PC doubles evidence is in
  `.local/native-quick-return-doubles-1`.
- Both native runs request B near the countdown deadline, observe the acknowledged
  cancellation, ready again, and enter one shared session. The doubles request
  comes from physical controller port 1. Both then exercise opponent exit,
  original-party return, identity retention, requeue and cancellation.
- Native PNGs show the usable CSS after recovery. The service tests establish
  the post-commit cancellation outcome; the native runs observed the pre-commit
  outcome. These are separate pieces of evidence.

The first native harness attempt timed out its UI inspection during the game's
missing-input wait. The harness now tolerates that bounded wait; no gameplay
timeout was changed. All test-owned clients and the local service are stopped
after verification. No production deployment or upstream push was made.
