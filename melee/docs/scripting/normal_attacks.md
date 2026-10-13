# Normal attack policies

Normal attacks combine shared input, pickup, motion and collision routines with
fighter-owned decisions. These callbacks replace the relevant fighter decision;
they do not call a stock fighter's handler. Without a callback, an attached
scripted fighter uses the common entry. Stock fighter dispatch is reserved for
stock actors.

## Shared motion phases

`motion_policy 341 common 44` gives owned numeric motion 341 the canonical
common motion 44's animation, interrupt, physics and collision phases. Its
animation selection, flags, attack metadata and camera remain owned assets.
The custom motion must be in the imported custom range, and the common motion
must belong to Melee's common table. Duplicate or malformed policies reject the
package before any callback row is changed.

This is reuse of shared engine behavior, with no donor fighter identity or
fighter-specific handler. An authored phase on that motion takes precedence;
other phases keep the shared policy. Explicit callback binding or clearing
retains its ordinary runtime semantics. Policy rows are prepared once at the
loading transition and revalidated against the exact owned callback table.

## Entry callbacks

| Handler | Result | Responsibility |
| --- | --- | --- |
| `on jab_enter` | Void | Own first-jab entry and the common rapid-jab restart path. |
| `on rapid_jab_enter` | Void | Own rapid-jab startup. |
| `on down_tilt_enter` | Void | Own down-tilt entry, including pickup priority. |
| `on aerial_attack_motion` | Int | Map the common aerial selected in `event.motion` to an owned motion. |
| `on aerial_attack_enter` | Void | Run after shared aerial motion entry and animation startup. |
| `on forward_smash_enter` | Void | Replace forward-smash entry; `event.angle` is the input angle. Facing is already selected. |
| `on up_smash_enter` / `on down_smash_enter` | Void | Replace accepted up/down-smash entry after held-item throw priority; no event arguments. |
| `on forward_smash_start` | Void | Run after common forward-smash startup when no replacement entry is declared. |
| `on wait_enter` / `on crouch_wait_enter` | Void | Apply owned cleanup after the common neutral or crouching setup. |

`aerial_attack_motion` runs before ordinary aerial initialization. Return the
incoming motion to keep it, or a valid common or owned motion in your package's
declared motion layout.
An out-of-range result falls back to the incoming motion. The shared initializer
then runs, including any existing `on aerial_attack_enter` setup hook. An omitted
return keeps the incoming motion.

```text
action AerialSelection:
    on aerial_attack_motion:
        if event.motion == 65:
            return 347
        return event.motion
```

The numeric motions above illustrate a package layout, not universal custom
motion identifiers. Declare and use the motions belonging to your fighter.

## `attack.enter_jab(motion: int) -> bool`

Runs shared first-jab startup with the supplied motion. Pickup gets priority.
Returns true after complete startup and false if pickup consumed the entry.
A VM fault or lifetime change stops the calling script instead of reporting a
healthy false result. Shared interrupt flags, animation startup,
jab input window and attack counters retain their ordinary order.

## `attack.enter_rapid_jab(motion: int)`

Runs shared rapid-jab startup with the supplied motion, retaining pickup priority
and the shared rapid-jab counters. It does not install an accessory callback.
A script owns any subsequent setup, including whether setup still happens when
pickup consumed the entry.

## `attack.update_rapid_jab(finish)`

Runs the shared rapid-jab continuation. `finish` must name a helper with no
arguments and no return value. The helper supplies the end-state decision when
the shared routine requests it. The engine retains its loop/counter behavior.

## `items.try_ground_pickup() -> bool`

Attempts the normal grounded pickup and returns whether it consumed the action.
Use it before a custom normal-attack entry when that move retains pickup priority.

## `collision.air_aerial_landing_callback(land)`

Runs the shared aerial collision path with an owned landing helper. The helper
must take no arguments and return no value. It decides which landing motion to
enter; the shared collision solver determines when landing is reached.

## `state.landing_aerial(motion: int, lag: float)`

Enters a landing aerial through the shared grounded setup, animation transition
and playback-rate calculation. This preserves the original `(length + 0.1) / lag`
calculation. Select an appropriate finite, positive lag in your move's policy.

The following read-only Float properties expose the fighter's ordinary landing
attributes. They are neither custom motion IDs nor script state variables.

| Property | Landing lag |
| --- | --- |
| `stats.landing_air_neutral_lag` | Neutral aerial |
| `stats.landing_air_forward_lag` | Forward aerial |
| `stats.landing_air_back_lag` | Back aerial |
| `stats.landing_air_up_lag` | Up aerial |
| `stats.landing_air_down_lag` | Down aerial |

## Callback lifetime

Pickup, motion changes and animation startup may execute nested callbacks.
Interpreted continuations stop if those operations remove or recycle the source,
or fault its VM. Shared startup also checks fighter data, kind, pinned program
and live state storage before reading its cached fighter again.
See [synchronous callbacks](synchronous_callbacks.md).
These guarantees do not mean a requested transition necessarily completed;
observe the result or resulting motion where your policy requires it.
