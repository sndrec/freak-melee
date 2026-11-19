# Catch policy hooks

These hooks run in the holder's Fighter program. They keep the common native
catch, pull, pummel, throw and caught-state setup; a declared policy replaces
the corresponding fighter-specific decision or cleanup. Missing hooks preserve
the existing common/native fallback. A false Bool is a policy result, not a
request to run a donor fighter's implementation.

| Event | Result | Timing and meaning |
| --- | --- | --- |
| `catch_allowed` | Bool | After held-item throw priority, before standing/dash catch input tests. False denies the catch; true permits the common input check. |
| `catch_start` | Void | After common acceleration/catch-counter initialization, before entering the catch motion and installing common grab callbacks. |
| `catch_dash_motion` | Int | Choose which motion plays when the fighter starts a dash grab. `event.motion` is the default motion. |
| `catch_frame` | Bool | Standing/dash catch animation update. True consumes the frame; false continues the common animation-end check. Both results replace fighter-specific frame logic. |
| `catch_exit` | Void | Replaces fighter-specific cleanup on the catch floor-loss path, before common Fall entry. |
| `catch_wait_release` | Void | After an accepted pummel or throw input leaves CatchWait. Replaces fighter-specific article cleanup. It is not an every-frame notification or a promise that the capture relation has already ended. |
| `catch_pull_anchor` | Bool | Before pull motion entry. True retains the anchor assigned by the script; false selects the ordinary common grab joint. |
| `catch_pull_follow` | Bool | After entering the pull motion. True installs the common caught-position follower; false clears that follower. |
| `catch_pull_ready` | Bool | Pull animation update. True enters common CatchWait; false keeps waiting, without applying native animation/tether readiness. |
| `capture_wait_enter` | Void | After the caught Fighter has entered its high/low wait motion and completed common setup; before its remaining common flags. `other` is the caught Fighter, while the executing source is the holder. |
| `air_tether_input` | Bool | Before stock air-tether eligibility. Its result returns directly, including false; the policy owns eligibility and entry. |
| `throw_victim_motion` | Int | Before common throw entry. Receives `event.motion` and the default `event.victim_motion`; returns the victim's common motion ID. An out-of-range result preserves the default. |
| `throw_animation` | Void | Each forward/back/up/down throw animation update, before the common throw tick. Replaces fighter-specific animation logic. |
| `throw_finish` | Bool | After the common throw tick when the animation ends. True means the script handled completion; false runs the common end-of-action transition. |

`capture_wait_enter` receives the caught fighter as the scoped `other` object.

For `catch_dash_motion`, return an owned motion that shares the dash-grab
motion policy. The new motion supplies the animation and hitboxes, and the
common policy supplies grab, pull and floor-contact behavior. Return
`event.motion` to use the normal dash grab. An invalid motion also falls back
to the normal dash grab.

`throw_victim_motion` accepts IDs in the common motion range, from zero through
the last common state. It does not choose a move from the holder's custom action
table. Reaching the end without an explicit return preserves
`event.victim_motion`. The common engine still computes weight-dependent throw
animation speed and establishes the captured pair's throw motion. The mapping
hook retains both participants and their reciprocal capture relation.

An absent `throw_animation` does nothing for a source-owned fighter. An absent
`throw_finish` returns false and uses common completion. Neither selects a
stock fighter's throw handler. These events do not change throw hitbox values
or release timing; those remain authored action events and capture operations.

`catch_pull_anchor` may synchronously signal an owned Article to bind its
resource anchor. Return true only when that assignment succeeded. The interpreter
does not select Link, Young Link or Samus storage unions for a declared policy.
For air tether occupancy, use identity checks such as
`callbacks.matches(Callback.accessory_before, None)`; native pointer nullness
alone does not describe an interpreted registration.

Dispatch retains the exact actor, program and storage lifetime. Pull/wait and
caught transitions also retain both physical participants and their established
reciprocal capture relation. A fault, retirement, slot reuse, replaced participant
or lost relation stops native continuation. Bool input arbitration consumes an
invalidated source so its caller does not continue the old input chain.
Completed writes are not rolled back. Normal motion changes may occur during a
hook; safety checks retain identity rather than requiring the old motion ID.

The pummel, throw and caught high/low motion continuations guard their own cached
participants before later common writes. The outer throw-animation hook retains
its executing actor; participant checks also occur inside common throw entry.
These hooks do not prove a complete tether move. The grouped console
fixture is `tests/native/script_catch_policy`; imported assets, game rendering
and full Article behavior remain separate acceptance work.
