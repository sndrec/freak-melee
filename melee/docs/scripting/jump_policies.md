# Aerial jump and shield-break policies

These operations expose shared movement and damage rules. Scripts own motion
selection, turn counters and editable jump tuning; they do not call a fighter's
stock entry or motion handlers.

The multi-jump configuration/query, drift threshold and bounded drift described
below are ordinary interpreted operations. They use shared native engine
routines; they do not enter a generated-C fighter module. See the
[migration checkpoint](../native_script_multi_jump_checkpoint.md) for current
coverage and the remaining costume-attachment work.

## Owned multi-jump resource

`fighter.enable_multi_jump()` enables the common multi-jump input path and binds
its table to this fighter's owned special-attribute resource. The common input
path still handles held/tap jump, the used-jump limit, command0's repeat lock and
the worn Screw Attack alternative. Its ordinary entry dispatches
[`on aerial_jump`](common_policies.md#on-aerial_jump) when supplied; without an
authored hook, the existing stock entry remains available.

```text
jumps.configure_multi(turn_frames, reverse_threshold, horizontal_impulse,
    acceleration_multiplier, speed_multiplier,
    vertical_1, vertical_2, vertical_3, vertical_4, vertical_5,
    count, first_motion, helmet_first_motion)
```

Write the call on one source line. It returns a boolean and can also be used as
a statement when its result is not needed. `turn_frames`, `count` and both
motion IDs require integer expressions. The other arguments are floats.
`count` specifies one through five **aerial** jumps; the common total maximum
becomes `count + 1`, including the first grounded jump. The current used-jump
count is preserved.

Configuration requires an enabled, owned resource, positive turn frames,
finite floats and motion ranges within the active definition. Both motion
ranges must contain `count` states; `helmet_first_motion = -1` disables the
second range. Failure returns false and changes neither tuning nor jump counts.
There is no allocation or raw-offset script access.

An unenabled or unbound table returns false. A live table must belong to the
fighter's own attribute scratch; a borrowed table rejects the call. Invalid
argument types and nonfinite floats produce a script diagnostic before writing.

Configure after copying/reloading special attributes, before the engine's
common instance/item scaling. A reload repeats that sequence, so changing the
owned parameter file updates the shared input limit and the scaled impulse
table together. Stock `attribute` declarations are import metadata: normal
`import-parameters` turns them into named project values rather than retaining
aliases into a donor's memory.

`jumps.multi_vertical_speed(index)` reads one impulse from the owned table
**after common scaling**. Its index is an integer; an absent table, invalid
declared count or out-of-range index returns zero. Entry can use
`jumps.used - 1`, then call [`jump.begin`](movement.md#jumpbegin), which increments
the used count. This operation does not choose a motion or consume input.
As with configuration, a non-null table must be bound to owned instance scratch.

## Shared phase operations

| Surface | Behavior |
| --- | --- |
| `air.drift_threshold` | Read the common horizontal-stick drift threshold. |
| `physics.air_drift_with_limits(threshold, acceleration, speed)` | Run common fast-fall/gravity followed by horizontal acceleration and friction with the supplied limits. |
| `controls.aerial_jump_actions()` | Run the common aerial-jump interrupt chain, including its command0 checks and short-circuit order. |
| `state.fall_aerial()` | Enter common exhausted-jump fall and initialize that motion's fall-animation state. |
| `collision.basic_landing_velocity_threshold` | Read this fighter's current scaled threshold for immediate Wait versus BasicLanding. |

The aerial-jump interrupt chain differs from `controls.air_actions()`, which
uses common Fall input. The drift operation includes vertical movement;
`physics.air_accel_friction(...)` supplies only the horizontal portion.

For an authored turn, use an ordinary `state int` countdown and the existing
`bones.root.add_rotation_y(angle)`. Decrement before applying the incremental
rotation, then test the integer halfway point. This updates the primary root
joint directly; it differs from setting an absolute rotation on a secondary
animation joint. Motion selection, impulses and turn thresholds remain named
fighter parameters.

## `on shield_break:` and top-blast damage policy

`on shield_break` is a void fighter callback. It runs during common
ShieldBreakFly entry after motion/animation initialization and initial velocity,
before the shield-break effect, sound, rumble and hurt status. The ordinary
interpreter runs this callback synchronously and the engine revalidates the
retained actor and published program before continuing. An owned fighter
without this hook makes no extra shield-break decision; it never dispatches a
stock fighter handler. Unmodified stock fighters retain their original policy.

```text
action ShieldBreak:
    on shield_break:
        damage.force_top_blast_ko = true
```

`damage.force_top_blast_ko` is a readable/writable boolean damage flag. When set,
the common top-blast decision forces the KO independently of its usual damage
threshold. The flag is also used by other damage states; it is not a
fighter-specific helper. Common motion changes clear it, so set it at the
entry boundary that owns the policy rather than in a later polling callback.

The ordinary interpreter exposes this on the executing source-owned Fighter,
with exact actor/program/storage/data and scene-generation admission. Its
native flag is shared with other damage states; changing it does not cause
an immediate KO or bypass the common top-blast routine. The surrounding legacy
generated-module descriptions are reference behavior, not the default backend.

State changes and the common aerial-jump interrupt chain can invoke authored
callbacks synchronously. Generated callers and the retained-state portions of
these shared routines stop if their original source was removed or recycled;
they do not write through a cached fighter pointer after invalidation.
