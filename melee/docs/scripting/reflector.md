# Reflector API

A reflector is a fighter-owned collision bubble that changes a projectile's
velocity, damage and, optionally, owner. The engine resolves contact; the script
owns the move's state transitions and responses. The same operations work for
any custom fighter. See [fighter state](language.md#fighter-state) for timers
and [parameters](language.md#fighter-attributes) for editable tuning.

## Method summary

| Method or property | Purpose |
| --- | --- |
| [`reflector.active`](#reflectorretain-and-reflectordisable) | Read whether reflection is currently enabled. |
| [`reflector.keep_owner`](#reflectorretain-and-reflectordisable) | Read or change the live bubble's projectile ownership policy. |
| [`reflector.create(bone, max_damage, x, y, z, radius, damage_mul, speed_mul, keep_owner)`](#reflectorcreate) | Create or replace the reflection bubble. |
| [`reflector.update()`](#reflectorupdate) | Update bubble geometry from the current pose. |
| [`reflector.retain()` / `reflector.disable()`](#reflectorretain-and-reflectordisable) | Enable or disable the existing bubble. |
| [`callbacks.track_reflect_hit()`](#callbackstrack_reflect_hit) | Register the script's projectile-contact response. |
| [`reflector.hit_direction`](#reflectorhit_direction) | Read the direction provided by the most recent reflection. |

## `reflector.create()`

```text
reflector.create(attrs.bubble_bone, attrs.max_damage,
                 attrs.offset_x, attrs.offset_y, attrs.offset_z,
                 attrs.radius, attrs.damage_mul, attrs.speed_mul,
                 attrs.keep_owner)
```

The example is wrapped for reading; write this call on one source line.
Arguments can be numeric expressions or owned parameters. `bone` is an index
into this fighter's bone parts, rather than a common bone name. The three
coordinates specify an offset in the attached bone's space. `radius` is the
bubble's radius; `max_damage` controls the engine's damage threshold.
The two multipliers modify reflected projectile damage and speed.
`keep_owner = true` prevents the normal change of projectile ownership.

The engine copies these values immediately. Creation enables reflection and
clears the existing reflect-hit callback. If the move needs a scripted response,
call `callbacks.track_reflect_hit()` **after** creating the bubble. Changing a
parameter does not change a bubble that was already created; create it again
at the appropriate move transition.

## `reflector.update()`

Update the engine's bubble collision geometry using its current bone pose.
Call it in the phases where the move should maintain the bubble. This operation
does not create a descriptor, choose a move phase, or register a callback.

## `reflector.retain()` and `reflector.disable()`

`reflector.active` is a read-only Bool for the fighter's current reflection-active
flag, available in the ordinary interpreter. It accepts typed Bool locals and does not create a bubble or dispatch a
callback. Read it without parentheses; use the operations below to change the
flag. It belongs to fighter callbacks and helpers.

`reflector.keep_owner` is a readable/writable ownership policy for the live
bubble: when true, a reflected item's damage owner stays unchanged. It is
separate from the descriptor's behavior argument to `reflector.create`.
This Bool property is available in the ordinary interpreter. It changes only
the ownership flag; it does not create geometry, enable reflection or install
a callback. The source must be a live, independently owned Fighter.
`callbacks.clear_reflect_hit()` clears the fighter's reflection-hit callback;
`callbacks.track_reflect_hit()` installs its authored `on reflect_hit` callback.
Disabling a bubble does not automatically select either ownership policy.

Set or clear the fighter's reflection-active flag. Retaining a bubble reuses
its descriptor and attachment; it does not create missing geometry or restore
a callback cleared by a motion transition. Register that callback separately.

## `callbacks.track_reflect_hit()`

Install `on reflect_hit` in the fighter's projectile-reflection callback slot:

```text
action ReflectionResponse:
    on reflect_hit:
        facing.direction = reflector.hit_direction
        state.enter_at_speed(362, 0, 0, 1)
        reflector.retain()
        callbacks.track_reflect_hit()
```

The numbered motion above is an example, not a universal reflected-hit state.
Choose a motion belonging to your fighter. A matching handler is required,
including when registration occurs in a helper. The callback does not decide
whether a projectile contacts the bubble; it responds after the engine has
resolved reflection. Ownership and item callbacks remain engine/article work.

This differs from [`callbacks.track_reflect()`](combat_visuals.md#callbackstrack_reflect):
`on reflect` handles the fighter itself being reflected, as with a moving attack.
`on reflect_hit` handles a projectile hitting this fighter's reflector.

## `reflector.hit_direction`

Read the engine's reflection-hit direction, normally `-1` or `1`. It is useful
inside `on reflect_hit` when the fighter's facing or next animation should
follow the projectile contact. Reading it does not change facing automatically.
The property is a read-only Float. It observes the engine's last contact value;
it does not synthesize a direction from the current facing or normalize it.

## Stock translation example

[Fox's template](../../examples/fox_wip.fms) implements Reflector
entirely with scripts and shared engine operations. Its release lag, gravity
delay, turn timer and effect phase are ordinary fighter state. All fourteen
source constants, including the ownership flag, become owned parameters on
import. See its [translation ledger](../fox_falco_script_translation.md) for
the stock comparisons and remaining fighter-specific work.
