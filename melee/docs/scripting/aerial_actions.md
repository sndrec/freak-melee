# Aerial actions

These operations let a fighter own hovering, aerial attacks and contact reactions
while using the shared input selector and landing solver. Store move availability,
timers and tuning in ordinary [fighter variables and parameters](language.md).
No operation selects Peach's Float or another stock special handler.

[Common policies](common_policies.md) | [Movement](movement.md) |
[Synchronous callback lifetime](synchronous_callbacks.md)

## Aerial input policies

| Handler | Result | Entry point |
| --- | --- | --- |
| `on aerial_action_manual:` | Bool | The manual aerial-action position in common Fall, Jump and JumpAerial interrupts, with their original per-state eligibility gates. |
| `on aerial_action_automatic:` | Bool | The automatic aerial-action position after the jump check, preserving each caller's original gate and priority. |
| `on grounded_motion_setup:` | Void | Grounded motion entry, before the common grounded flag resets. |

Returning true from an aerial policy consumes the input chain. Returning false
continues common interrupts; it does not then run a stock Float decision. If the
source is removed, recycled or faults its VM, or its selected program/state
changes, the input chain stops even when the callback returned false. Without
a declared policy, an attached scripted fighter continues common interrupts;
it does not test a stock fighter's Float eligibility. The grounded policy replaces the stock per-kind
availability reset, so it can replenish any script-owned aerial resource.
Keep it to setup work: entering another grounded motion would recursively invoke
the same hook.

```text
state bool hover_available = true

action Hover:
    on grounded_motion_setup:
        state.hover_available = true
    on aerial_action_manual:
        if state.hover_available and input.jump_held and input.y <= -controls.aerial_action_down_threshold:
            BeginHover()
            return true
        return false
```

`BeginHover()` is an authored helper, not an engine primitive. It can consume the
availability variable, enter an authored motion and install its callbacks.

## Input and held items

| Value or call | Type | Meaning |
| --- | --- | --- |
| `input.jump_held` | Bool | Either logical jump button is held. |
| `controls.tap_jump_threshold` | Float | Shared upward stick threshold for tap jump. |
| `controls.horizontal_stick_deadzone` | Read Float | Current common horizontal stick deadzone. This reads shared engine tuning each time; it is not a fighter parameter. |
| `controls.stick_smoothing` | Read Float | Shared interpolation factor for stick direction and magnitude. Reading it does not smooth input; scripts choose their interpolation and state. |
| `controls.aerial_action_down_threshold` | Float | Shared magnitude used for downward aerial-action input. |
| `input.cstick_attack_edge()` | Bool | The engine's directional C-stick attack edge test. |
| `controls.aerial_attack_motion()` | Int | Shared A/C-stick aerial attack motion selection, including facing and directional thresholds. It does not enter the motion. |
| `items.held_use_category` | Int | Held item's engine use category; -1 if there is no held item. |
| `controls.try_air_item_throw()` | Bool | Check and enter the ordinary airborne held-item throw. True consumes the input. |
| `state.air_gun()` | Void | Enter the shared airborne held-item shooting action. |
| `controls.try_air_special()` | Bool | Check B and dispatch the selected authored airborne special, with the normal directional priority and facing rules. |
| `physics.animation_vertical_velocity_adjusted()` | Void | Set vertical self-velocity from the animation's TransN displacement, including the shared bunny-item multiplier when present. |

The held item's use category is distinct from its hold kind and article ID.
Item throws and shooting can enter authored callbacks synchronously. The
interpreter stops after the call if the source's lifetime changed or its VM faulted.

`controls.aerial_attack_motion()` returns the common aerial motion ID selected
from current input. It does not apply the fighter's `aerial_attack_motion` hook
or enter an attack. A hovering move can map the returned common ID to its own
motions in script. `input.cstick_attack_edge()` only observes whether either
C-stick axis crossed its shared directional threshold; it does not check the
game mode, held item, or attack button. Input consumption remains your policy.

`state.air_gun()` selects the shared airborne shooting entry for a held ray gun,
Fire Flower, or Super Scope. With no held item or another kind, it does nothing.
`controls.try_air_item_throw()` retains native throw/drop selection, stick ages,
and smash-throw thresholds. Its result means the input was handled, rather than
that an item was successfully removed. A dropped item may legitimately remove
itself while the fighter continues its input handling.

`controls.try_air_special()` checks up, down, side, then neutral. If the selected
authored special is absent it returns false without reversing facing or trying
another direction. If present, its source hook runs with the shared facing
adjustments. It never delegates an attached fighter to a stock special handler.

`physics.animation_vertical_velocity_adjusted()` writes only vertical
self-velocity. It neither advances the animation nor adds gravity. The bunny
item relation is a presence test; the engine applies its vertical multiplier
once on that branch. There is no additional facing factor or value clamp.

## `collision.air_landing_callback(Function)`

Run the shared airborne map and environmental response with the normal
special-fall platform predicate and an authored landing callback. `Function`
must name a root helper in this fighter's program with no arguments and no
return value. `None`, event callbacks, article-owned helpers and typed-return
helpers are rejected.
The callback decides which landing action to enter and which variables to reset;
the solver owns collision and ledge acceptance. It may execute the helper during
this call, so source lifetime is checked before the script continues.
On accepted ground contact it runs the helper. Otherwise it tries the common
wall-jump response, then cliff handling; it does not add an aerial-jump check.
An environmental interruption retains the native solver's false result and
continuation order. This is not a general grounded-state predicate.

```text
func LandHover():
    state.hover_available = true
    state.basic_landing()

action Hover:
    on state_341_collision:
        collision.air_landing_callback(LandHover)
```

## `collision.contact_rebound_allowed()`

Query whether the current body-contact recipient permits a rebound. With no body
contact, it returns true. A recipient with `on body_rebound:` owns the decision;
otherwise original counter-state eligibility applies. The operation exposes no
recipient pointer and enters no recoil motion itself.

The native dispatcher validates source and recipient registration before reads,
then rechecks their generations, kinds, Definitions and retained contact link
after an authored predicate. An absent or retired recipient fails safely. This
validation covers the current call; the original contact field itself does not
retain a historical generation from when it was written.
