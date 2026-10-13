# Aerial actions

Use these callbacks and functions to add airborne actions such as hovering,
choose aerial attacks, and respond to landings and body contact. Melee handles
input direction and collision detection; your script decides which move to use.
Store availability and timers in [fighter variables](language.md), and use
[parameters](language.md#fighter-attributes) for editable settings.

[Common policies](common_policies.md) | [Movement](movement.md) |
[Synchronous callback lifetime](synchronous_callbacks.md)

## Aerial input policies

| Handler | Result | Entry point |
| --- | --- | --- |
| `on aerial_action_manual:` | Bool | Called during input checks in Fall, Jump and JumpAerial when that motion allows a manual aerial action. |
| `on aerial_action_automatic:` | Bool | Called after the jump check when the current motion allows an automatic aerial action. |
| `on grounded_motion_setup:` | Void | Grounded motion entry, before the common grounded flag resets. |

Return `true` after handling an aerial input. Return `false` to let Melee
continue checking the other inputs for that motion. A fighter with no aerial
callback continues those checks automatically.

Use `grounded_motion_setup` to reset resources when a grounded motion begins,
such as making hover available again. Keep this callback to setup work: starting
another grounded motion here calls `grounded_motion_setup` again.

If a callback removes the fighter, changes its move or program, or encounters a
script error, the current input check ends immediately.

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

Define `BeginHover()` in your script. This helper can set `hover_available` to
`false` and start the hover motion with its animation, physics and collision
callbacks.

## Input and held items

| Value or call | Type | Meaning |
| --- | --- | --- |
| `input.jump_held` | Bool | Either logical jump button is held. |
| `controls.tap_jump_threshold` | Float | Shared upward stick threshold for tap jump. |
| `controls.horizontal_stick_deadzone` | Read Float | Current engine setting for ignoring small horizontal stick movements. |
| `controls.stick_smoothing` | Read Float | Engine smoothing factor for stick direction and magnitude. Use it in your script's interpolation calculation. |
| `controls.aerial_action_down_threshold` | Float | Shared magnitude used for downward aerial-action input. |
| `input.cstick_attack_edge()` | Bool | The engine's directional C-stick attack edge test. |
| `controls.aerial_attack_motion()` | Int | Return the aerial attack motion ID selected by the A button or C-stick, using the fighter's facing and the engine's direction thresholds. |
| `items.held_use_category` | Int | Held item's engine use category; -1 if there is no held item. |
| `controls.try_air_item_throw()` | Bool | Check and enter the ordinary airborne held-item throw. True consumes the input. |
| `state.air_gun()` | Void | Enter the shared airborne held-item shooting action. |
| `controls.try_air_special()` | Bool | Check B and dispatch the selected authored airborne special, with the normal directional priority and facing rules. |
| `physics.animation_vertical_velocity_adjusted()` | Void | Set vertical self-velocity from the animation's TransN displacement, including the shared bunny-item multiplier when present. |

The held item's use category is distinct from its hold kind and article ID.
Item throws and shooting can enter authored callbacks synchronously. The
interpreter stops after the call if the source's lifetime changed or its VM faulted.

`controls.aerial_attack_motion()` returns a motion ID. Your script can use that
ID to start a normal aerial attack or map it to a custom move.
`input.cstick_attack_edge()` returns whether either C-stick axis crossed its
direction threshold. Check held items, game mode and other buttons separately
when your move needs those conditions.

`state.air_gun()` starts the airborne shooting action for a held ray gun, Fire
Flower or Super Scope. It takes effect when one of those items is held.
`controls.try_air_item_throw()` applies Melee's throw/drop selection, stick
timing and smash-throw thresholds. A `true` result means the input was handled.

`controls.try_air_special()` checks up, down, side, then neutral special. It
starts the selected special with Melee's facing adjustments when that special
is defined in the fighter's scripts. It returns `false` if that entry is absent.

`physics.animation_vertical_velocity_adjusted()` sets vertical self-velocity
from the animation's TransN displacement. It applies the bunny item's vertical
multiplier when the fighter is wearing that item. Animation playback and
gravity are handled by their respective update functions.

## `collision.air_landing_callback(Function)`

Check airborne movement against the stage and call `Function` on an accepted
landing. Melee checks platforms, walls and ledges; your helper chooses the
landing motion and resets any move variables.

`Function` must name a fighter-level helper with no arguments and no return
value. The helper runs during this call. If the fighter is removed or its
script encounters an error, script execution stops.

When the fighter stays airborne, this function checks wall jumping and then
ledge grabbing. Use a separate input callback for aerial jumps.

```text
func LandHover():
    state.hover_available = true
    state.basic_landing()

action Hover:
    on state_341_collision:
        collision.air_landing_callback(LandHover)
```

## `collision.contact_rebound_allowed()`

Return whether the fighter currently touching this fighter allows a rebound.
If that fighter defines `on body_rebound:`, its callback decides. Otherwise,
Melee uses its usual counter-state rules. The result is `true` when there is no
body contact and `false` when the contacted fighter has been removed.

Use the result to decide whether to enter your rebound motion. The engine
checks that both fighters and their contact link remain valid while the
callback runs.
