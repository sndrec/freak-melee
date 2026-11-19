# Target queries

Target operations find fighters or sample positions. The script chooses
priority, steering, speed and behavior when nothing is found. No operation
depends on a fighter or move name.

## targets.nearest_available_fighter

`targets.nearest_available_fighter() -> int`

Find the nearest other available fighter to the calling fighter. This operation
is available in Fighter callbacks and helpers, including a helper invoked
through a retained fighter receiver. Article callbacks must ask their fighter
receiver to perform the search; they cannot call it as an Article operation.

The result is a full 32-bit fighter creation ID, or `0` if nothing is found.
Store it in an ordinary `int` variable and pass it to
[`targets.read_position`](cpu_history.md#native-contracts) or another target operation.
IDs with the high bit set have a negative signed representation; their identity
bits are preserved. Do not interpret the sign as an error or a player slot.

```text
state float target_x
state float target_y
state float target_z

func FindTarget() -> bool:
    let int target = targets.nearest_available_fighter()
    return targets.read_position(target, state.target_x, state.target_y, state.target_z)
```

The search includes teammates. It excludes the caller and candidates outside
the blast rectangle inset by the caller's CPU body half-width and half-height;
points exactly on that boundary remain eligible. It applies the common native
availability and death-state filters. Distance is measured in X/Y, ignoring Z,
and equal distances retain the first eligible fighter in registry order.
It searches from the fighter's position, even if an article requested the helper.

This is different from [`cpu.nearest_enemy_at`](cpu_policies.md#nearest-opponent), which uses enemy and
distance rules for CPU planning. The returned ID alone does not steer, deal
damage or reserve a fighter. Query its position and define that behavior in
your script. Existing output queries leave their destinations unchanged if the
selected fighter is no longer available. Invalid callers or malformed registry
links fail before publishing a result; the operation allocates no runtime storage.

## targets.read_opponent_ahead

`targets.read_opponent_ahead(x, y, z) -> bool`

Arguments are three distinct writable Float outputs (locals or `item.state`
fields). The result is a Bool. The search starts from the article's
current position and facing, excluding its current damage owner and applying
stock eligibility and team rules. It also excludes another member belonging
to the same player as the owner and fighters with the native hidden flag.
The closest eligible fighter supplies its camera-bone target position.
Distance uses squared XY; a strict comparison preserves registry-order ties.
Facing -1 or +1 restricts the half-plane; other values do not. The rules are those of
[`ftLib_80086368`](../../src/melee/ft/ftlib.c), not an unrestricted roster query.

If no target exists, the result is false and all three outputs stay unchanged.
The current damage owner must be absent or a live physical Fighter. The query
admits borrowed rigs before native camera sampling and checks source/target
identity before publication. It uses bounded stack storage and retains no
object pointer across calls. Existing HSD matrix work can use its normal lazy
scale/expression storage; this is not a claim that all HSD traversal allocates
nothing. Invalid contexts, stale identities and malformed rigs fault the call
without publishing outputs.

## targets.read_lockable_item_ahead

`targets.read_lockable_item_ahead(x, y, z) -> bool`

Arguments and missing-target behavior are the same as above. The native lock-on
query searches eligible item categories: target, lock-on, enemy and category 7.
It excludes captured items and certain owned items, checks the facing
half-plane and chooses the smallest squared XY distance. It returns the
collision-center position, rather than the model origin. Ordinary pickups
are not automatically eligible. Exact filters are in
[`it_8026C258`](../../src/melee/it/it_26B1.c).

## targets.overlaps_hitbox

`targets.overlaps_hitbox(fighter_id, hitbox_slot) -> bool`

Article-only geometry query. Both arguments must be integral. It checks the
article's selected native hit capsule against the fighter's hurt capsules,
using their current swept endpoints, joint matrices, radius scaling and depth.
It preserves native endpoint-cache and contact-position/distance writes and
stops at the first overlap. No target or an out-of-range slot returns false.

Malformed capsule counts or bones outside the retained rig fail the call.

This does not apply damage, invoke a move, or filter combat immunity, hurtbox
status or the hitbox enabled flag. Those decisions belong to the caller.
Use the control owner when an article's control relationship matters; its
current damage owner may differ after reflection or other interactions:

```text
if attachment_owner.exists:
    let fighter_id = attachment_owner.fighter.creation_id
    if targets.overlaps_hitbox(fighter_id, 0):
        targets.call(fighter_id, "react_to_contact", position.x, hitbox.radius(0))
```

`react_to_contact` is an [exported helper](functions.md), defined by that
fighter. The engine query knows nothing about the helper or fighter behavior.

## Search for attacks in a capsule

`targets.attack_overlaps_capsule(ax, ay, az, bx, by, bz, radius) -> bool`

`targets.article_attack_overlaps_capsule(article, ax, ay, az, bx, by, bz, radius) -> bool`

Call these from a fighter script. Give the capsule both endpoints in world
coordinates and a nonnegative radius. The first call checks for an active
fighter or item attack. The second checks attacks from the named article in
your fighter's script. It includes articles spawned by another instance of the
same fighter and articles whose damage owner changed through reflection.

The search uses each attack's swept hitbox and checks whether the attack can
hit your grounded or airborne state. Team and item owner settings apply.
Both calls return true on the first match. Use the result to time an
animation, open a reflector, or trigger another scripted response.

## Example

Declare these variables inside the article before its callbacks:

```text
state float target_x
state float target_y
state float target_z
state bool target_found
```

Then choose a fighter first and an eligible item second:

```text
item.state.target_found = targets.read_opponent_ahead(item.state.target_x, item.state.target_y, item.state.target_z)
if not item.state.target_found:
    item.state.target_found = targets.read_lockable_item_ahead(item.state.target_x, item.state.target_y, item.state.target_z)
```

See [vector operations](math.md) for steering and
[article variables](article.md#local-variables) for instance-local storage.

## Retained fighter references

`item.contact_fighter` returns the current contact fighter's creation number,
or 0 if the contact is not a live fighter. Store it in an ordinary `state int`.
The reference preserves all 32 bits; it never stores a game-object address.
A removed fighter is absent, even if another object later reuses its address.
Creation numbers and script state are restored together by rollback.

### Current position

`targets.read_position(reference, x, y, z) -> bool`

Read the live fighter's current gameplay position into three distinct Float
outputs. This is `cur_pos`, independent of animation-bone transforms, the camera
target and the CPU planner's selected opponent. Use this query when a behavior
has retained a fighter ID and needs that actor's actual current position.

```text
let float x = 0
let float y = 0
let float z = 0
let int enemy = cpu.nearest_enemy_at(position.x, position.y, position.z)
if targets.read_position(enemy, x, y, z):
    # Choose behavior using this live enemy's position.
    let float horizontal_distance = abs(x - position.x)
```

The query is available in fighter and article callbacks. Outputs must be three
distinct writable Float destinations supported by the current source context,
including Float locals and state fields: `state.name` for a fighter,
`item.state.name` for an article. The ID must be integral. Expressions without
writable destinations, overlapping outputs and wrong types are rejected at
compile time. See [fighter IDs](paired_fighters.md#member-identity) for keeping
the full creation identity in integer state.

The query returns false for an initially absent target and leaves all three
values unchanged. Wrong output types/overlap are load errors. During the query,
the interpreter retains one physical Fighter identity and samples `cur_pos`
without sanitizing NaN, infinity or signed-zero values. Caller retirement or a
captured target's generation, kind, userdata or physical slot change stops the
query before publication; it cannot select a replacement through the same ID.
No persistent pointer or heap allocation is exposed. Later separate queries
resolve the ID again. Targets may belong to a foreign or stock Fighter program;
the script caller still requires its exact owned source bank and storage.

### Stored position delta

`targets.read_position_delta(reference, x, y, z) -> bool`

Reads the live fighter's stored `pos_delta` through the actual shared
`ftLib_80086BEC` kernel. This is the engine's saved movement delta, rather than
a subtraction of target and caller positions. Fighter and Article callbacks can
use three distinct writable Float destinations, including Float locals and
current source state fields. The integral reference preserves its full native
32-bit creation identity; signed Int references may carry its high bit.

Missing or zero references return false and preserve all three destinations.
On success the interpreter publishes XYZ together after verifying its owned
caller and the captured target's physical slot, scene, class, native creation ID,
userdata and kind. Native Float bits, including NaNs, infinities and signed zero,
are preserved. Stock and foreign targets are accepted. The read allocates nothing
and invokes no callbacks. Malformed registry links, cycles or matched Fighter
layout failures stop the query without publishing outputs. Valid native Fighter
userdata remains an engine registry prerequisite; these checks do not prove the
allocation extent of an arbitrary non-null userdata pointer.

Use the [target checks](authoring_checks.md#stage-queries-and-targets) to test
selection, observation and removal of the selected actor.

| Expression | Result |
| --- | --- |
| `targets.is_fighter(reference)` | Whether that creation number resolves to a live fighter. Available in fighter and article callbacks; requires an Int reference and returns Bool. |
| `targets.guard_active(reference)` | Whether the fighter is in a shared GuardOn, Guard or GuardSetOff state. False for an absent target. |
| `targets.model_scale(reference)` | Shared effective model scale, or 0 for an absent target. |
| `targets.motion(reference)` | Current fighter motion ID, or -1 for an absent target. Available in fighter and article callbacks. |
| `targets.airborne(reference)` | Bool: the live fighter is airborne. False for an absent target. Available in fighter and article callbacks. |
| [`targets.smash_charge_active(reference)`](#smash-charge) | Bool: the live fighter is in the active common smash-charge phase. False for an absent target. |

### Shield and camera geometry

`targets.shield(reference, x, y, z, radius) -> bool` samples the native shield
joint's world center and unscaled shield radius. Multiply the radius by
`targets.model_scale(reference)` when gameplay needs the effective model scale.
The radius uses the shared shield-health/lightshield formula and the native
constant-size branch where applicable; it does not clamp or sanitize Float values.
`targets.guard_active(reference)` tests the three common GuardOn, Guard and
GuardSetOff motion IDs, independently of this geometry query.

`targets.read_camera_target_position(reference, x, y, z) -> bool` transforms the
fighter's native camera-target offset through its configured camera joint. This
position can differ from both `position` and the shield center.

Both queries accept integral creation references and are available to Fighter
and Article callbacks. Outputs must be distinct writable Float destinations.
An absent reference or unavailable configured joint returns false and preserves
every output. Initialize outputs explicitly before the call when the behavior
requires a zero fallback. Native matrix evaluation uses the captured target's
own rig and package, including foreign and stock Fighters; it cannot follow a
replacement through a reused object address. Caller, target, metadata and graph
identity are checked before publication. These queries use the actual native
shield/camera kernels and the same bounded matrix admission as
[bone queries](target_bone_queries.md); unsupported native matrix dependencies
are errors. No new heap allocation is introduced by the binding.

`targets.model_scale(reference)` returns the native product of instance Y scale
and common base scale, preserving native Float arithmetic. It returns zero for
an absent target, and does not sample the rendered root's scale.

### Current motion

`targets.motion(reference) -> int` returns the retained fighter's native
`motion_id`, or -1 when the creation reference is absent. The value is the
target's current ID, including character-specific IDs; it is not translated
through the caller's move layout. The same -1 can be a live fighter's actual
motion value, so this result alone does not distinguish absence.

Fighter and article callbacks and their helpers can query a foreign stock
fighter or one running a different source package. The direct interpreter
requires the caller's current owned program/storage and fixed scene lifetime.
It samples a retained physical target once, then checks caller and target
identity before publishing the signed 32-bit result. Generation, kind,
userdata, class or physical pool-slot changes stop publication; it never
reselects a replacement through the same reference. The query allocates nothing
and does not invoke a fighter handler or authored callback.

Integral arguments preserve the full native 32-bit creation reference,
including high-bit values represented by signed Int state. Float arguments
and values outside that width are rejected. The direct interpreter also
accepts canonical Bool as the integral values 0/1, consistent with its other
retained target queries; use integer state to retain an actual fighter ID.

### Smash charge

`targets.smash_charge_active(reference)` reads the common smash-charge phase
(`smash_attrs.state == 2`), with the same meaning as the engine's ordinary
charge query. It does not infer charge from an animation, elapsed time or held
button. Fighter and article callbacks can use it with an integer creation ID;
missing or recycled IDs return false. The operation allocates nothing.
The direct interpreter allows a live foreign or stock Fighter target. It retains
the physical target and caller through publication, rather than looking up the
same ID again after a generation, kind, userdata or pool-slot change. It reads
the shared smash state without requiring the target to run the caller's package.

For a retained article's current owner, use
[`articles.ref(...).current_owner`](article_references.md#current-owner). Scripts
choose how their model animation responds to charging.

`targets.shield(reference, x, y, z, radius)` writes the shared shield-bone
world position and shield radius. The four outputs must be distinct declared
`item.state` floats. An absent target writes four zeros. Querying a shield
neither changes state nor chooses an attachment policy; scripts decide when
and how to follow it. A live target must have its normal initialized fighter
rig and shield attributes.

These operations use the same live-fighter registry and shield calculations
as the stock engine. They consume no RNG and allocate no memory. They are
available in article callbacks; a reference to a fighter does not grant a
reference to its native pointers or private move-variable union.

## Calling a recipient's named event

`targets.call(reference, "event_name", arguments...)` runs an explicitly exported function
belonging to the live recipient fighter. It is available in fighter callbacks,
article callbacks and their helpers. The recipient's own module supplies the
function, state layout and parameter values, even when it belongs to another
package. Nothing falls back to a stock handler or writes the caller's layout
into the recipient.

For example, a fighter can offer a resource-release event:

```text
state int bow_handle

export func release_bow():
    state.bow_handle = -1
```

Its article can notify the current damage owner:

```text
on destroyed:
    targets.call(item.current_owner, "release_bow")
```

The call returns true when the function was invoked, including when that
function removes its recipient and the caller survives. A dead/zero target,
unattached stock recipient or missing export returns false. Event names are
quoted compile-time identifiers; exported functions may declare typed arguments.
`targets.call` discards a typed export's result and still reports dispatch success.
See [function exports](functions.md) for declaration rules.

Each prepared program supports up to 128 exported functions and 128 distinct
recipient-call names. Names contain 1–127 identifier characters. Lookup uses
prepared keys and exact name comparisons, so hash collisions cannot select a
different function. Calls execute the recipient's own local function ID and
owned state; the caller does not need to declare the same helper.

## Typed recipient results

| Expression | Required export | Missing result |
| --- | --- | --- |
| `targets.call_bool(reference, "Name", arguments...)` | `export func Name(...) -> bool:` | `false` |
| `targets.call_int(reference, "Name", arguments...)` | `export func Name(...) -> int:` | `0` |
| `targets.call_float(reference, "Name", arguments...)` | `export func Name(...) -> float:` | `0.0` |

These calls return the helper's value, rather than dispatch success. A zero/dead
reference, missing export or mismatched result type
returns the neutral value without invoking the export. A legitimate neutral
result therefore cannot be distinguished from a missing recipient through this
value alone. Avoid calling `targets.call` first to probe: that would execute the
helper twice.

All four dispatch forms accept up to 30 numeric argument values after the
reference and literal name. A matching export validates the supplied arity
and applies its declared Bool/Int/UInt/Float parameter types before execution.
Mismatched argument types or counts fault without invoking the helper; a
wrong return type retains the missing-result behavior. See
[exported function arguments](functions.md#exporting-a-recipient-callback)
for conversion rules and examples.

```text
export func restore_held_resource() -> bool:
    if state.saved_item == -1: return false
    let bool restored = items.bind_existing_held(state.saved_item)
    return restored
```

An article can use the result directly:

```text
on destroyed:
    let bool restored = targets.call_bool(item.current_owner, "restore_held_resource")
    if restored: item.clear_current_owner()
```

Available in fighter and article callbacks and helpers, with the same live-ID,
recipient-owned state and source-lifetime checks as `targets.call`. Typed calls
can appear in assignments, conditions, expressions and helper arguments.
Arguments run once in written order; Boolean `and`/`or` retains short-circuit
evaluation. Use separate locals when you want to make a sequence of side effects
obvious. Model-effect updates do not call recipient exports.

Handles must be integer expressions. Use a `state int`, `let int`, integer
function argument or an integer target query; all 32 bits remain intact.
Converting a previously rounded float back to an integer cannot recover an ID.

Self-calls and nested calls use the interpreter's shared fixed scratch: 64 call
frames and 65,536 registers. Exhausting either capacity is a terminal script
error, not a missing-export result. Dispatch allocates no memory and restores
the shared stack when a call exits or fails.

If a delivered helper retires its recipient, `call` reports true while typed
calls return their neutral value: the retired VM does not publish a return value.
If the caller retires, later caller writes stop. A recipient VM fault or changed
live program/storage identity is an error; no result is published. Dispatch
never reacquires a replacement object after removal or pool reuse.

## targets.read_bone_position

See [recipient bone queries](target_bone_queries.md) for a current-world vector
query with typed outputs, native rig sampling and retained target validation.
[Paired fighters](paired_fighters.md#target-bone-position) provides a usage example.

## Native target observations

These queries accept a live Fighter creation ID in fighter or article callbacks
and helpers. All 32 handle bits are retained. They can observe stock Fighters
and Fighters running another source package without reading private script state.

| Query | Result | Missing target |
| --- | --- | --- |
| `targets.damage(reference)` | Float damage percent (`dmg.x1830_percent`) | `0.0` |
| `targets.facing(reference)` | Float native facing direction | `0.0` |
| `targets.cpu_body_half_width(reference)` | Float cached CPU body half-width (`cpu.x564`) | `0.0` |
| `targets.sleeping(reference)` | Bool native incapacitated flag (`x221F_b3`) | `false` |
| `targets.floor_line(reference)` | Int retained floor-contact index | `-1` |

Float observations preserve the native bits, including signed zero and nonfinite
values. A floor index can remain from an earlier contact while airborne. These
calls neither recompute geometry nor change target state. The caller and selected
physical target must retain their scene, generation, class and payload through
publication; a retired caller or substituted target rejects the read.
