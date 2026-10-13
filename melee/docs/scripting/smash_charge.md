# Common smash charge

Scripts can use the engine's existing charge record instead of duplicating
its timer in fighter variables. These properties are available in fighter
callbacks and helpers. They are not model-effect properties.

| Property | Type | Meaning |
| --- | --- | --- |
| `smash.charge_mode` | Int | Shared charge-update mode. |
| `smash.charge_frames` | Float | Frames accumulated by the shared update. |
| `smash.charge_limit` | Float | Authored maximum charge duration. |
| `smash.saved_animation_rate` | Float | Animation rate retained for release. |
| `smash.charge_color` | Int | Charge color-animation selector. |
| `smash.shake_index` | Int | Current index in the common shake table. |
| `smash.shake_period` | Int | Shake period used by the common update. |
| `smash.shake_table_count` | Int | Read-only size of the engine's common shake table. |

The script owns entry, interrupt and release decisions. Writing these fields
does not enter a motion, pause an animation, or install callbacks. The ordinary
fighter update advances charge frames, shake and sound according to the mode.
Do not increment `smash.charge_frames` again in a per-frame callback unless
you intentionally want a different charge mechanic.

## Charge modes

| Constant | Value | Meaning |
| --- | --- | --- |
| `Smash.none` | 0 | No common charge update. |
| `Smash.precharge` | 1 | Holding A enters charging; otherwise clear the charge mode. |
| `Smash.charging` | 2 | Advance charge time and presentation; release at the limit or when A is released. |
| `Smash.release` | 3 | No common timer update; the common charge damage multiplier applies. |
| `Smash.manual_charge` | 4 | Advance time, shake and sound without clamping at the limit or handling input release. |

For a move that freezes its animation while charging, set the animation rate
and saved rate in the order that move requires; the properties do not infer
one from the other. Shared motion transitions clear the charge mode but can
retain accumulated frames for release damage.

```text
smash.charge_mode = Smash.manual_charge
smash.charge_frames = 0
smash.charge_limit = 60
smash.shake_index = 0
smash.shake_period = smash.shake_table_count
```

## Validation

Mode, color, index and period assignments require Int expressions. Frame,
limit and saved-rate assignments accept numeric expressions. A mode outside
0–4 leaves the current mode unchanged. The shake index must be within the
table, and the period must be from 1 through the table count. The table count
must itself fit the byte-sized engine storage; invalid assignments do nothing.
Right-hand expressions are evaluated once.

Use [live hitbox operations](combat_visuals.md#hitboxes) to apply charged
damage or reset repeat-hit bookkeeping, and [input and motion
operations](fighter.md) to implement the move's control flow.
