# Luigi Cyclone translation

Luigi's down special in [`luigi_wip.fms`](../../examples/luigi_wip.fms)
owns its entry, animation, physics, and collision callbacks. It calls shared
engine operations for acceleration and stage collision; no callback delegates
to a stock Luigi Cyclone handler. The move follows the stock source in
[`ftluigispeciallw.c`](../../src/melee/ft/kinds/ftLuigi/ftluigispeciallw.c).

## Script state and attributes

The move keeps its changing horizontal speed, latest collision result, and
persistent lift charge in [fighter state](language.md#fighter-state).

| Script value | Stock source | Purpose |
| --- | --- | --- |
| `state.cyclone_ground_vel` | `SpecialLw.groundVelX` | Reduces horizontal speed after command 0. |
| `state.cyclone_collision` | `SpecialLw.isUnkColl` | Allows the root to follow floor angle while command 3 is active. |
| `state.cyclone_charge` | `x222C_cycloneCharge` | Prevents another lift until Cyclone lands. |

The script declares named attributes for DAT offsets `0x70`–`0x94`; `0x88`
is omitted because the stock source writes it to a motion field that has no
native reader. `0x94` is declared with [`attribute int`](language.md#fighter-attributes)
because the source layout stores landing lag as an integer. The other
declared Cyclone fields are floats. The script uses the active fighter's own
DAT block, so a clone can tune these values without changing shared code.

## Airborne taps and charge

The airborne entry checks whether a previous lift is charged when it sets
initial vertical speed. Later, while command 2 permits a tap, a fresh B press
calls [`physics.ascend()`](movement.md#physicsascend) only if charge is clear:

```text
if not state.cyclone_charge and command(2) and input.special_pressed:
    physics.ascend(attrs.cyclone_tap_speed, attrs.cyclone_tap_gravity)
```

When subaction command 1 fires, the animation callback marks the charge.
The grounded version handles a permitted fresh B press by adding upward
speed, changing to the airborne variant, and clamping fall and horizontal
speed. These are decisions in the script; the engine methods only perform
the requested movement steps. See
[`physics.ground_accel()` and `physics.air_accel()`](movement.md#physicsground_accel-and-physicsair_accel)
for the horizontal movement primitives.

## Landing and transition flags

The move tests the stock six-value collision box through
[`collision.air_box()`](movement.md#collisionground_box-and-collisionair_box).
On landing it clears charge, switches to the grounded motion at the current
animation frame, clamps ground speed, and reinstalls effect hitlag handling.

Cyclone's ground/air transition uses motion flags `0x0C4C508A` in the stock
source. The Luigi clone definition supplies that same value to
[`state.to_ground()` and `state.to_air()`](fighter.md#stateto_air-and-stateto_ground).
The generic Luigi special flag set is different: using it stalled the
grounded animation for two frames and moved the contact hit. This was a
transition-data mismatch, not a change in the Cyclone script's attack data.

## Fresh-life charge rule

The stock source reads `x222C_cycloneCharge` but does not explicitly set it
on fighter load. The clone writes `false` on load and death, so a new life
starts uncharged. This intentional fix is separate from the stock-parity
claim below; the compared recordings all begin with clear charge. Landing
still clears charge as stock does. The declaration and lifecycle callbacks
are in [`luigi_wip.fms`](../../examples/luigi_wip.fms).

## Validation boundary

Stock and scripted Luigi matched across 520 samples each for grounded
contact, airborne contact, and a repeated-tap recording. The comparison
covered fighter state, action, damage, item, and RNG traces. Six previously
recorded Green Missile/Fireball paths and two Super Jump Punch paths also
matched after this translation. The recordings do not yet cover every ledge,
sloped-floor, interruption, charge-across-lives, or shield interaction.

For the broader language, start at the
[fighter scripting reference](index.md); for the source recordings, see the
[parity input catalog](../../tests/native/parity_inputs/README.md).
