# Fighter hitboxes

In the Workshop Action Editor, select a hitbox on the event timeline or in
the Event list. Use **Launch** to choose how the hit launches opponents, and
**Capsule** to give the hitbox two endpoints. Type a decimal in **Damage**
for fractional damage. **Grab target** controls whether the hitbox can hit
the fighter you are holding. Press Enter to confirm a typed value.

## Creating a hitbox

To adjust a hitbox created by an action event before it can hit, use an
accessory helper through `callbacks.accessory(HelperName)`. Accessory helpers
run after the action events and before hit detection. Animation callbacks run
before those events, so a hitbox created that frame may not be present yet.

Declare reusable immutable hitbox data at the root of a fighter script, then
create it in one of the four native capsule slots:

```fms
hitbox_definition Foot:
    bone 2
    damage 12
    radius 4.5
    offset_y 1.0
    angle 45
    growth 90
    base_knockback 25

func EnableFoot() -> bool:
    return hitbox.create(0, Foot)
```

`hitbox.create(slot, definition) -> bool` initializes a complete native fighter
capsule. `slot` must be integral and in 0 through 3. The definition name is a
closed selector belonging to the current fighter program: it cannot be made
from a number, cast, or used in arithmetic. Article callbacks can call a root
fighter helper through `attachment_owner.helpers`; they cannot create fighter
capsules directly.

| Field | Meaning | Default and range |
| --- | --- | --- |
| `bone` | Fighter [bone ID](bone_queries.md#bone-ids), or its declared live article reservation | Required; nonnegative signed 32-bit integer |
| `damage` | Damage before the common scale, charge and stale-move rules | Required; nonnegative Float |
| `radius` | Native capsule radius, in model units | Required; finite, nonnegative Float |
| `offset_x`, `offset_y`, `offset_z` | Local offset from the joint | 0; finite Float |
| `angle` | Knockback angle, including Sakurai angle 361 | 361; integer 0..2147483647 |
| `growth`, `weight_knockback`, `base_knockback` | Native knockback parameters | 100, 0, 0; integer 0..2147483647 |
| `group` | Hit group used by native activation and victim-history initialization | 0; integer 0..2147483647 |
| `element` | Native hit element | 0; integer 0..31 |
| `shield_damage` | Additional shield damage | 0; signed 32-bit integer |
| `sound_severity`, `sound_kind` | Native impact sound parameters | 0; integers 0..7 and 0..31 |
| `item_interaction`, `ignore_fighter_scale`, `rebound` | Native collision flags | false; Bool |
| `clank`, `grounded`, `aerial` | Clash and target eligibility flags | true; Bool |
| `victim_only` | Restrict contact to the captured victim | false; Bool |
| `exclude_grabbed` | Let the hitbox pass through the fighter you are holding | false; Bool |
| `require_throw_owner` | Refuse creation without the native captured-victim owner | false; Bool |

Definitions accept constant expressions, not mutable fighter variables or
native calls. Missing required fields, duplicate or unknown fields, nonfinite
geometry and fractional/out-of-range integer fields are errors. A program may
declare up to 64 definitions. Definitions allocate no storage during a tick.

Creation returns false without changing capsules when the joint is absent,
when a reserved article endpoint has retired or been replaced, or when the
requested captured-victim owner is absent. Invalid arguments or an unavailable
fighter program are script errors. Reserved joints are resolved through the
actual retained resource collection; raw joint pointers are not exposed.
Resource removal disables and detaches capsules borrowing that resource.

When the capsule was disabled or its group changes, creation enables it and
uses the common native victim-history initialization. Refreshing an active
capsule in the same group preserves its native state and victim history.
Damage and angle use the common engine setters. Radius and offsets use the
values declared here directly; imported packed command coefficients must be
converted explicitly in the declaration.

## Capsule shape

Enable **Capsule** in the hitbox inspector to add **End X**, **End Y** and
**End Z**. Both endpoints follow the selected bone. **Radius** applies along
the full segment, including the rounded ends. The game checks the posed
capsule each tick. Spherical hitboxes sweep from their previous position.

For agent action edits, set `capsule: true` and use `end_x`, `end_y`, and
`end_z` in the same coordinate system as the offsets. In a
`hitbox_definition`, provide all three `end_*` fields to create a capsule.

## Throw damage

Select **Define throw values** on a throw's timeline to edit damage, angle,
knockback, element and sound. **Damage** accepts decimals such as `7.5`.
The **Release throw** event controls when the throw applies those values.

## Launching toward a point or a moving hitbox

In the Workshop Action Editor, select a hitbox event and choose **Toward
point** or **Follow hitbox** under **Launch**. Edit the settings that appear
and press Enter to confirm a typed value. **Angle** uses the hitbox's angle.

In **Follow hitbox**, enable **Separate grounded knockback** to give grounded
opponents their own growth and base knockback. The main growth and base
knockback fields then apply to airborne opponents.

Use these calls after the hitbox is created. Each setting lasts until the
hitbox is cleared or replaced. Knockback attributes still determine hitstun.

| Call | Effect |
| --- | --- |
| `hitbox.set_launch_target(slot, forward, up, frames)` | A hit launches the opponent toward a point relative to the fighter. Positive `forward` follows the fighter's facing. The initial speed is the distance to the point divided by `frames`, which must be at least 1. |
| `hitbox.set_launch_follow(slot, momentum, attraction, min_up, ground_angle)` | An airborne opponent inherits the fighter's velocity multiplied by `momentum`. Each axis also gets the difference between the hitbox center and the opponent's position multiplied by `attraction`. `min_up` sets the lowest vertical speed. Grounded opponents use ordinary knockback at `ground_angle` degrees. |

The game calculates this velocity when the hit connects. Normal knockback
decay, gravity and directional influence affect the opponent after the hit.
Both calls work on slots 0 through 3.

For `actions.edit`, use `launch_mode` 0 for Angle, 1 for Toward point, or
2 for Follow hitbox. Toward point uses `launch_forward`, `launch_up` and
`launch_frames`. Follow hitbox uses `launch_momentum`, `launch_attraction`,
`launch_min_up` and `launch_ground_angle`.
Set `launch_override_ground_knockback` to true to enable `launch_ground_growth`
and `launch_ground_base`.

## Reading and updating a hitbox

Fighter `hitbox.state(index)` returns the exact native capsule state as an Int.
`Hitbox.disabled` is 0 and `Hitbox.enabled` is 1; native states 2, 3 and 4 remain
distinct. Equality with `Hitbox.enabled` is stricter than testing for a
non-disabled capsule.

`hitbox.damage(index) -> float` reads the selected capsule's current native
damage. This includes scaling, smash charge and staleness already applied by
the setter; it is not the original subaction's unmodified damage word. Reading
does not recompute damage or mutate the capsule, regardless of its state.
Float bits, including signed zero, NaN and infinity, are preserved. It uses
the same retained source Fighter and bounded integral index as `hitbox.state`.
For example, an authored charge move can save the initial damage as an
ordinary fighter variable, then set a scaled value on subsequent ticks.

`hitbox.set_damage(index, value)` returns Void and runs the actual common
`ftColl_8007ABD0` setter. Both operations accept a bounded integral index into
the four Fighter capsules. Invalid indices are errors before field access.
This permits dynamic indices; the historical reference emitter admitted only
literal indices 0 through 3 and rejected invalid indices during generation.

`hitbox.current(index).x/y/z` and `hitbox.previous(index).x/y/z` are readable
and writable Float coordinates in the selected capsule's native current
(`x4C`) and previous (`x58`) position vectors. The indexed selector accepts an
integral expression and captures it once before the scalar operation. Float
indices, omitted selector calls and Article contexts are rejected. Runtime
indices must be in 0 through 3 without narrowing a wider integer first.
Writing a coordinate changes only that scalar; it does not advance position
history, change capsule state or update its joint. Native Float bits, including
NaN and infinity, are preserved.

`hitbox.set_world_position(index, x, y, z)` returns Void and calls the common
`ftColl_8007B8A8` primitive. It clears the selected capsule's joint and stores
the three Float inputs in its offset vector. It leaves current and previous
position vectors unchanged. This is distinct from assigning the current
coordinates. Both interfaces are available only on the source Fighter and
retain its physical and interpreted lifetime; they expose no foreign private
hitbox state.

`hitbox.set_enabled(index, enabled)` takes a strict Bool and changes only the
selected capsule's enabled/disabled state. Enabling also sets the native
Fighter hitbox cleanup marker; disabling preserves that marker. It does not
clear victims or advance position history.

`hitbox.clear_victims(index)` calls `lbColl_80008440` to clear both native victim
lists and their counters independently of capsule state. Both methods accept
integral indices without narrowing first. Out-of-range integral indices are
healthy no-ops, matching these native operations; malformed types and unavailable
source ownership are errors. Both remain source-Fighter operations.

`hitbox.clear_all()` disables all four capsules and clears the Fighter cleanup
marker through `ftColl_8007AFF8`. It preserves victim lists/counters, current and
previous coordinates, joints, offsets and other capsule fields. It is distinct
from clearing each capsule's victims.

```fms
hitbox.previous(0).x = hitbox.current(0).x
hitbox.set_world_position(0, state.x, state.y, state.z)
```

Damage first converts to the native unsigned 32-bit word. Integral conversions
retain modulo-32-bit narrowing, including signed bit patterns. Float conversion
truncates toward zero and rejects undefined unsigned conversion domains. The
native helper then performs these operations in order:

1. Convert the unsigned word to Float.
2. Apply the common Y-scale damage formula when instance scale differs from 1.
3. Apply the smash-release multiplier, when applicable.
4. Store the truncated signed integer count, represented as an unsigned word.
5. Apply the native stale-move factor to the final Float damage.

The backend checks the exact post-scale/post-smash Float against the defined
signed 32-bit conversion domain before any setter writes. It does not clamp
damage. A high-bit or negative integral input can be valid after scale reduction;
the same word can be invalid at scale 1. Scale data, native player slot and stale
table bounds must also be valid on the branches that use them. DebugRom bypasses
the stale table, and common attack ID 1 bypasses stale weights.

The setter changes only the selected capsule's `unk_count` and `damage` fields.
It preserves capsule state, group, joint and victim links. Its scale, smash and
stale helpers perform math and read native state; they do not allocate, invoke
authored callbacks or advance attack/item counters. The current stale factor
compares move IDs and ignores the attack-instance argument.

Operations retain the actual Fighter pool/scene/native generation and, when
attached, its exact interpreted program/storage. Faults or changed identities
leave result outputs unpublished. The capsule fields remain native Fighter
state: packed interpreted actor snapshots require the native Fighter and player
staleness peers to be restored separately.

The focused `tests/native/script_fighter_hitboxes` fixture links the actual
compiler, preparation, VM, actor and backend, comparing common native math and
complete Fighter field changes with controlled player stale records and source
asset selection. It does not establish hit detection, geometry, rendering or
in-game parity.
