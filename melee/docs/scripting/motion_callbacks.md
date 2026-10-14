# Motion callbacks

Motion callbacks let a script replace a shared phase with one of its own helpers.
Each tick advances the animation, calls its animation helper and processes
action events. Accessory helpers run after those events and before hit
detection. Use an accessory helper to adjust the damage of a hitbox created
on that tick; see [fighter hitboxes](fighter_hitboxes.md).

Use a named, root-level function with no arguments and no return value, or `None`
to clear the phase. Numeric IDs, other fighters' helpers and event callback
declarations are not callback values.

| Method | When the helper runs |
| --- | --- |
| `callbacks.animation(helper)` | The current motion's animation phase. |
| `callbacks.interrupt(helper)` | Its input/interrupt phase. |
| `callbacks.physics(helper)` | Its physics phase. |
| `callbacks.collision(helper)` | Its collision phase. |
| `callbacks.accessory(helper)` | Its slot 4 accessory phase each eligible nonhitlag frame. |
| `callbacks.accessory_cue(helper)` | Its slot 4 accessory phase when the normal article/subaction cue is set; consumes that cue before calling the helper. |

Each method returns `Void`. An unavailable actor, foreign function or wrong
signature produces a script diagnostic; it does not install a native callback.
Only Fighter scripts can register these phases. The helper remains part of the
same interpreted program and can use ordinary fighter variables.

```text
state int pulses

func Pulse():
    state.pulses = state.pulses + 1
    if state.pulses == 3:
        callbacks.accessory(None)

action Example:
    on special_n:
        state.pulses = 0
        callbacks.accessory(Pulse)
```

This is a callback example, not a complete special move. Normally enter the
move's state first, then register its helper. Selecting another motion replaces
these bindings with that motion's authored phases. A helper can clear itself or
install another helper; the change applies to the next invocation. A shared
engine callback installed in the same native slot takes priority.

Cue registration differs from checking a cue inside an every-frame helper.
`accessory_cue` calls the helper only when the cue exists and clears it first;
`accessory` leaves any cue decision to the helper. Clear either policy with
`callbacks.accessory(None)`.

`callbacks.accessory_before` and `callbacks.accessory_hitlag` are separate
[instance callback slots](callback_slots.md). They run in the earlier slot 2/3
native scheduler and survive ordinary motion changes; neither uses slot 4's
cue policy. They cannot be substituted for `accessory` or `accessory_cue`.

Bindings, cue policy and fighter variables are part of the interpreted actor
snapshot. Restore them alongside the native Fighter/GObj state. Snapshots retain
the exact program identity; they cannot transfer helper IDs to another package.
A new native generation preserves persistent variables, while normal motion
selection resets the phases. No source parsing or allocation occurs per call.

See [alternate forms](forms.md) for destination activation and
[script structure](language.md) for helper declarations.
