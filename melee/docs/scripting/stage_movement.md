# Stage and movement queries

## Custom standard ground collision

`collision.ground_standard_box(top, bottom, left_x, left_y, right_x, right_y)`
runs Melee's standard ground response with an authored collision box. All six
arguments are finite Float expressions in fighter-local coordinates. The
engine mirrors the left/right points when facing left, updates collision
position/history, and enters ordinary Fall when floor contact is lost. This
is a response operation rather than a Bool-only contact query.

```text
collision.ground_standard_box(12, 0, -6, 6, 6, 6)
```

It requires the live owned Fighter. Fall entry may invoke authored hooks;
source retirement diagnoses before publishing success. Invalid arguments
reject before collision. The operation uses a stack-local box and allocates
no script storage. See also [collision history copy](collision_copy.md).

These shared operations let fighter scripts make movement decisions without
calling a stock fighter's move handlers. Article scripts can also use the
stage-segment query and global sound operation.

## Surface friction policy

Declare `surface_friction stage` or `surface_friction ignore` once at top level
before code blocks. `stage` uses the stage surface multiplier for common ground
movement. `ignore` uses a multiplier of 1, including common walking, running and
ground-acceleration projection. It does not disable the fighter's own friction,
change the floor normal, or alter collision. Omitting the declaration retains
the copied base's policy.

```text
surface_friction ignore
```

This is immutable package configuration. It adds no callbacks or per-frame
allocation. `floor.friction` remains a surface query; it does not
override this policy.

## Stage segments

`stage.segment_blocked(x0, y0, x1, y1)` returns Bool. It checks a world-space
2D segment through the existing stage remap solver, including moving stage
geometry. It requests no output point, normal, line ID or flags and skips no
joint. Coordinates are Float expressions. This is a collision query, not a
fighter movement update.
See [direct stage queries](stage_queries.md) for the connected interpreter
signature, finite-coordinate admission and retained source lifetime. That
reference also covers `stage.sweep_box`, which returns raw environment flags
from an independent ECB probe without updating the Fighter's own collision box.

## Air movement

| Surface | Type and meaning |
| --- | --- |
| `movement.used_tether` | Read/write Bool: whether the current airborne sequence has consumed its tether attempt. |
| `movement.fastfall` | Read-only Bool: the fighter's current fast-fall flag. |
| `movement.special_landing_lag` | Read-only Float: retained special landing lag. |
| `input.shield_held` | Read-only Bool: either shield button is held. |
| `acceleration.z` | Read/write Float: the Z component of self acceleration. |
| `position.delta.x`, `.y`, `.z` | Read/write Float: the frame's world-position delta. |
| `collision.position.x`, `.y`, `.z` | Read-only Float: the collision box's current position. |

`physics.check_fastfall()` returns the common fast-fall input decision;
`physics.fastfall()` runs the common fast-fall update. These remain separate
so scripts can preserve the original order around gravity, drift and other
decisions. `collision.landed` runs the native airborne collision step and
returns its surviving contact decision. It updates collision history and
position and applies environmental responses; it does not simply observe the
fighter's current ground/air state. `collision.air_catch_hit()` takes no arguments
and returns no value. It runs the common catch-hit collision path: a surviving
floor contact selects the neutral landing path when vertical self-velocity is
above the fighter's scaled landing threshold, or basic landing at or below it.
Basic landing retains the engine's held-hammer choice.
`state.damage_fall()` enters the common damage-fall state.

## Ledge probes

`collision.probe_ledge(snap_y, snap_height)` returns Bool. It copies the
fighter's current collision data, adds the two reach offsets, and probes the
side selected by facing. The selected live ledge ID receives the solver's
output, including any output written on a failed probe. Success sets the
corresponding ledge-grab environment flag. Other collision data is unchanged.
The script decides whether to stop velocity after success. Nonfinite offsets
are rejected.

`collision.ledge_available()` returns whether the common occupied-ledge
lookup finds no occupant. `Collision.left_wall_mask` and
`Collision.right_wall_mask` expose the native wall flag groups.

`state.cliff_jump_phase2(multiplier)` enters the second phase of the common
quick or slow ledge jump. It initializes that shared state and scales its
vertical velocity. As with other state transitions, generated code stops if a
synchronous callback invalidates the calling fighter.

## Optional tether input

```text
action Movement:
    on air_tether_input:
        if movement.used_tether: return false
        if not input.shield_held: return false
        # Add this fighter's remaining input and resource decisions here.
        return false
```

This optional predicate executes at the normal tether interrupt priority.
Providing it replaces the entire stock tether predicate, including its
fighter-kind, held-item, callback and used-tether gates. Return true to consume
the input or false to let the surrounding common interrupt chain continue.
False does not invoke stock tether behavior. An absent hook preserves the
original stock path. Removing or recycling the fighter, changing its kind, or
replacing its definition during the callback consumes the interrupt so the
caller cannot continue with stale fighter state.

## Global sound

`audio.play_global(sound, volume, pan)` plays through the shared global sound
engine and returns its Int handle. All three arguments are Int expressions.
Unlike fighter-local sound, this does not retain a sound handle in the calling
fighter's state.

## Runtime freefall and typed writes

`state.freefall(normal_air_limits, force_special_landing, allow_interrupt, mobility, landing_frames)` accepts five expressions: two Ints, a Bool and two numeric values. The runtime landing argument can read `movement.special_landing_lag`. This directly calls the existing common freefall initializer; it does not substitute the distinct blend initializer. Compile-time special metadata retains its separate owned-scalar validation.

Assignments to `movement.used_tether` require a Bool expression; numeric truth conversions must be explicit. Position-delta component writes use the normal Float assignability rule and preserve the other components. Fastfall, ledge probes, air collision and state-transition operations are fighter-only; authored articles share the segment and global-sound operations. Model-effect context does not support this batch's operations.

Stage and audio results keep their Bool and Int types in article local variables. Global audio rejects Float and Bool arguments. Source-invalidating freefall/phase-two transitions stop before subsequent generated state writes, including when called through a readable helper. Console generated-module fixtures control these Host boundaries; actual geometry, common lifecycle and gameplay require their separate evidence.
