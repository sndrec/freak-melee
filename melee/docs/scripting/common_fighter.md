# Shared fighter movement and presentation

These interpreter operations work with the active fighter's native common data.
Move decisions and timing remain in your script; none selects a stock special
handler. See [movement](movement.md), [parameters](language.md#fighter-attributes)
and [callback slots](callback_slots.md).

## Movement methods

| Method | Behavior |
| --- | --- |
| `facing.from_input_threshold()` | Apply the shared stick-threshold facing rule. This differs from the existing `facing.from_input()` rule. |
| `physics.force_air_and_use_jumps()` | Run the native ground-to-air/jump-use lifecycle, including its existing shared decisions. |
| `physics.switch_to_ground()` | Transfer movement into ground space without selecting or restarting a motion. |
| `collision.skip_soft_floor() -> bool` | Run the actual stick/platform test and update its collision state when the native rule permits dropping through. |

Calls require a live source fighter with its owned program/storage. Reached
native callbacks can retire or fault it; execution stops before publishing a
result or continuing the script. The binding layer does not allocate or parse
during ticks. Setting `airborne` directly still changes classification alone and does
not perform these movement transitions.

`facing.sign` is the native facing convention: -1 when direction is negative,
+1 otherwise. Zero, negative zero and NaN follow the shared helper's comparison.
It is an Int observation, not a truncating numeric cast. For example:

```text
if collision.check_ground_ledge(facing.sign):
    state.basic_landing()
```

## Common observations and pickup policy

| Property | Meaning |
| --- | --- |
| `air.terminal_velocity` | Common fighter terminal velocity, Float. |
| `controls.stick_smoothing` | Shared stick smoothing, Float. |
| `model.instance_scale_y` | Instance Y scale, Float. |
| `model.scale` | Native instance Y scale multiplied by common model scaling, Float. Distinct from live joint scale. |
| `ceiling.normal.x/y` | Actual ceiling collision normal components. |
| `wall.left.normal.x/y` / `wall.right.normal.x/y` | Separate native wall surface normals. |
| `ecb.top.y` / `ecb.bottom.y` | Current ECB offsets, Float. |
| `fighter.creation_id` | Full unsigned native spawn identity; not converted through Float. |
| `items.block_ground_pickup` | Boolean read/write policy bit. |

Observations are read-only except the pickup policy. Set it with an assignment:

```text
items.block_ground_pickup = true
```

An Article can query an existing retained fighter through the normal receiver
surface, for example `creator.model.scale`. Guard `creator.exists` first.
The creator relation is not a fighter-self namespace.

## Visibility and color

`visuals.set_visible(bool)` controls the fighter's invisible flag. It does not
change held-item visibility or choose model alternatives.

`visuals.set_group(group, alternative)` selects an alternative in an owned model
visibility group. Group and alternative must be integral; their width/sign is
preserved before bounds checks. Alternative -1 hides the group. Native selections
are signed8, so selectable alternatives are 0..127 and must exist in at least one
active owned channel. An alternative absent from another channel hides that
channel, matching the native behavior. Selection marks the existing dirty state;
it does not force an immediate rendering update or alter immutable model data.

`color.start(id, bool)` invokes the shared color-animation selection and priority
rules. IDs use the actual common123/special6 tables (0..128). The operation returns
Void; a higher-priority existing animation can prevent replacement. Missing tables
or invalid current overlay indices reject before mutation. It is separate from
material editing and joint animation.

These three methods require the current source fighter, owned rig and model
tables. Invalid indices are rejected rather than clamped or truncated. Native
retirement/replacement prevents subsequent result publication.
