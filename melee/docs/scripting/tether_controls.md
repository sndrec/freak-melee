# Common tether prerequisites

These Fighter operations use common native capture and ledge behavior. The
script selects the timing, motion policy and resource ownership.

| Operation | Result | Native behavior |
| --- | --- | --- |
| `capture.mask(mask)` | Void | Assigns the current Fighter's capture exclusion mask. Accepts Int values from 0 through 32767. Zero clears the exclusions; 511 is the common mask used during captured and air-tether motions. |
| `state.damage_fall()` | Void | Switches a grounded Fighter to air, selects the common parasol branch when applicable, or enters DamageFall with common drift and damage flags. |
| `collision.probe_ledge(snap_y, snap_height)` | Bool | Probes a copy of the current collision box with the two finite Float offsets added to its ledge reach. Success stores the selected ledge ID and grab flag. Velocities remain script-controlled. |
| `collision.ledge_available()` | Bool | Tests whether the selected ledge has an occupant using the common Fighter registry lookup. It neither probes geometry nor enters a ledge state. |
| `state.cliff_jump_phase2(multiplier)` | Void | Enters Quick2 when the current motion is CliffJumpQuick1, otherwise Slow2. Starts its animation, resets the common phase flag, and sets vertical velocity to the Fighter's common ledge-jump velocity times the finite Float multiplier. |

`capture.mask` controls whether another Fighter or Item grab category can catch
this Fighter. It is separate from `capture.arm`, which installs grab callbacks
and selects categories for this Fighter's own grab detector. Motion changes
reset the mask according to common engine behavior; set it after entering a
motion when that motion needs exclusions.

```text
state.enter_at(360, 0, 0)
capture.mask(511)

let bool ledge = collision.probe_ledge(5, 5)
if ledge:
    velocity.x = 0
    velocity.y = 0
if ledge and collision.ledge_available():
    state.catch_ledge()
else:
    velocity.x = 0
    state.cliff_jump_phase2(attrs.pull_jump_multiplier)
```

Keep probe, occupancy and entry separate when reproducing this ordering.
`collision.cliff_catch()` also applies input eligibility and statistics, so it
does not replace these explicit operations. `state.enter` alone does not replace
DamageFall or phase-two ledge jump initialization.

All five calls require the executing owned Fighter. The interpreter retains its
physical actor, program and storage identity before publishing a result. The
two common state kernels also guard cached Fighter data after motion and
animation callbacks. Retirement or a script fault stops later continuation;
completed writes are retained. No call invokes a stock fighter special handler.

Article resource creation and the capture anchor remain in
`item.resource(index).bind_capture_anchor()` and the related resource API. The
Fighter's `catch_pull_anchor` hook can synchronously signal its Article and
return whether the anchor was bound. These common prerequisites do not prove
complete tether gameplay or imported asset acceptance.
