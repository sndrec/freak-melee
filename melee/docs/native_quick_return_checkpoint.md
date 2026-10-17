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
the retained-P2P policy described in the earlier checkpoints.

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
