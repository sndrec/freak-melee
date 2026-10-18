# Combat, presentation and collision primitives

These operations are available to fighter callbacks and typed
[functions](functions.md). They expose shared engine operations; the script
owns the decisions, timing, and ordinary [fighter state](language.md).
The [Rollout translation](../jigglypuff_script_translation.md) combines them
without calling a stock Jigglypuff handler.

## Damage reaction observations

`damage.reaction_suppressed: bool` reads the common reaction suppression test.
It is true when pending knockback is zero, or when both the shared SDI and
reaction flags are set and pending knockback is below the stored reaction
threshold plus the common margin. It consumes no damage or flags.

`damage.knockback_level: int` classifies pending knockback as 0, 1, 2 or 3.
It first multiplies the pending knockback by the common reaction scale, then
compares the result against the three common level thresholds using strict
less-than comparisons. It does not classify the magnitude of current velocity
or set hitstun. Both reads use the original common damage functions and require
the current owned fighter and loaded common parameters.

These are observations, not a custom-fighter policy. A holding move can use
them to decide whether to keep an item or take another reaction; see
[heavy-item damage callbacks](fighter.md#on-heavy_item_damage).

## Common reaction entry

`damage.enter(motion, facing)` initializes the existing common damage reaction
with a chosen motion. Motion is an Int in the fighter's installed motion table;
`-1` asks that same initializer to select its normal motion. Facing is a finite
Float: zero leaves the common direction decision in place, while a nonzero
value is passed unchanged as the override.

`damage.default_reaction(facing)` runs the common default entry, including
ledge cooldown and flower/song reaction selection for non-sandbag fighters.
It takes one finite facing Float. This differs from explicitly calling
`damage.enter(-1, facing)`, which goes straight to the standard initializer.

Both operations use existing engine knockback, hitstun, velocity, hitlag
callbacks and initial animation processing. They neither invent a new damage
formula nor call a stock fighter-specific handler. The selected motion's
authored callbacks and the fighter's `on knockback_enter` remain script-owned.
The caller, program, storage and common parameters must be current. A callback
that retires or faults the caller stops native continuation and publication.

For the current interpreter's fighter visibility groups and color selection,
see [shared fighter presentation](common_fighter.md#visibility-and-color).

## Position and Article-root effects

`effects.spawn_at_position(id, x, y, z)` spawns a native effect at a copied world
position in a fighter callback. The ID must be integral and its native recipe
must consume exactly one synchronous position pointer; coordinates are finite
Floats. Joint/scalar effects and unknown or extra-payload recipes reject. The
native helper copies the temporary vector; the script passes no native address.
This operation does not automatically change `effects.active`.

Articles use `item.spawn_root_effect(id)` for an effect attached to their actual
runtime root, and `item.destroy_effects()` for normal native effect cleanup.
Creator presence is not required. IDs must have the source-verified one-joint ABI.
The native generator may retain/refcount that joint as part of its graph lifetime;
this is distinct from retaining a temporary scalar or position pointer. Model-less
Articles still have the runtime anchor allocated by the shared Item constructor.
Missing roots, wrong classes/owners, retired or replaced programs/storage and
incompatible argument signatures reject before further script continuation.

For joint-attached weapon ribbons, see the [trail reference](trail.md).
Its `on trail` callback supplies rendering data; it does not tick gameplay.

For model effects whose visual decisions depend on your fighter's state, use
[animated model effects](model_effects.md). A named effect installs an owned
update script instead of the recipe's original fighter-specific updater.

## Radial wind

`effects.wind_at_position(x, y, z, frames, strength, decay, frequency)` creates
the same radial wind field at an explicit world position. It works in fighter
and article callbacks. Coordinates, strength, decay and frequency are numeric;
frames must be an Int. Each expression runs once in written argument order.
The direct interpreter admits source-owned Articles without requiring their
creator or a model. The native routine copies the position into the shared wind
pool; it stores no script-local pointer. Coordinates and scalar inputs must be
finite. If the native pool is full, no new wind field is created. Source/package
retirement stops further script continuation.
For an article attachment point, use
[`item.joint_world_position()`](article_models.md#itemjoint_world_positionmodel-path-x-y-z).

`effects.wind(bone, offset, frames, strength, decay, frequency)` creates the
shared engine's temporary radial wind field at a semantic bone's world position.
`offset` shifts world X in the fighter's facing direction. It affects physics;
it does not draw a visual effect. `frames` is an integer lifetime. Strength
decreases by `decay` each engine update, while `frequency` controls the engine's
cosine modulation in radians per update.

All five values can be ordinary expressions, including owned
[fighter parameters](language.md#fighter-attributes). They are evaluated once
in argument order. The bone resolves through the fighter's own semantic map.
This operation is available in fighter callbacks and helper functions.

```text
parameter int wind_frames = 80
parameter float wind_strength = 1
parameter float wind_decay = 0.003
parameter float wind_frequency = 1.0471976

effects.wind(Top, 0, attrs.wind_frames, attrs.wind_strength, attrs.wind_decay, attrs.wind_frequency)
```

## Animation blending and root compensation

`bones.set_direct_animation_copy(part_or_joint, enabled)` selects direct
animation transform copying instead of blending that joint between the source
and destination poses. A semantic role such as `TransN` resolves through the
fighter's bone map; a numeric or parameter expression addresses a literal
runtime joint index. These are different address spaces: a stock load callback
may flag joint 1 even when its semantic TransN maps to joint 2. Use an owned
parameter for that rig-specific index. Invalid indices are ignored. The operation
does not replace the animation clip or disable descendants.

`animation.hip_root_compensation` is a read/write boolean. It enables the
existing animation policy that positions the rig's auxiliary root from HipN
and compensates its displacement during animation updates. The costume must
already define the auxiliary root; this setting creates no bones and is
separate from extracting TransN root motion into fighter movement.

```text
action RigPolicy:
    on load:
        bones.set_direct_animation_copy(TransN, true)
        animation.hip_root_compensation = true
```

These are generic rig policies. Use them only when the authored rig and its
animations require them. Mewtwo's stock load behavior enables direct copying
on literal joint 1 and also enables root compensation.

## Editable hurtbox capsules

`hurtboxes.define(index, bone, height, grabbable, ax, ay, az, bx, by, bz, radius)`
initializes an existing fighter hurtbox with the shared engine routine.
Endpoints are bone-local offsets. `bone` is an own-rig index, for example
`bones.index(XRotN)`; invalid capsule or joint indices are ignored. This does
not allocate additional capsules or change the saved costume definition.

This operation now runs directly in the interpreter for owned fighter packages.
`index` selects an existing capsule slot; `bone` is a raw index into that
fighter's imported costume joints, not a stock character's rig. `height` accepts
`Hurt.height_low`, `Hurt.height_mid` or `Hurt.height_high`; it classifies the
capsule for collision rules rather than moving it. `grabbable` is a Bool.
The seven endpoint/radius arguments are Floats and retain the native values
without clamping. The initializer sets that capsule to normal hurt status,
attaches its current costume joint and marks the fighter's hurtboxes for update.
It preserves cached world endpoints and update flags until the engine refreshes
them; it does not recalculate them during this call.

Invalid numeric slot or bone indices are ignored without changing the fighter.
Missing costume joint data, malformed scalar types and an inconsistent capsule
count are errors. The runtime checks the source's selected program, owned data,
joint table and capsule storage before calling the shared initializer. No stock
fighter behavior or additional capsule allocation is involved.

```text
hurtboxes.set_all(Hurt.intangible)
hurtboxes.define(0, bones.index(XRotN), Hurt.height_mid, false, 0, 0, 0, 0, 0, 0, 3)
```

The initializer enables the new capsule. `hurtboxes.status` reads the shared
body-hit status query; it is distinct from individual capsule status.
`damage.reset_callbacks()` runs shared damage cleanup callbacks before a
scripted reaction, matching engine callback ordering. It can call other
authored functions installed in those slots.

## Hitboxes

| Method | Result |
| --- | --- |
| `hitbox.enabled(index)` | Article-only query. For a fighter, compare `hitbox.state(index)` against `Hitbox.disabled`. |
| `hitbox.state(index)` | Read the integer capsule state without reducing it to a Boolean. |
| [`hitbox.damage(index)`](#hitboxdamageindex) | Read the live capsule's damage after subaction updates. |
| [`hitbox.set_world_position(index, x, y, z)`](#hitboxset_world_positionindex-x-y-z) | Place a live capsule in world space through the shared collision helper. |
| [`hitbox.current(index).x/y/z`, `hitbox.previous(index).x/y/z`](collision_capsules.md) | Read or write the collision sweep endpoints directly. |
| `hitbox.set_enabled(index, enabled)` | Set its enabled/disabled state and register enabled attacks for ordinary motion-change cleanup. |
| `hitbox.group(index)` | Read its unsigned 32-bit repeat-hit group tag. |
| `hitbox.set_group(index, group)` | Change that tag without clearing recorded victims or changing capsule state. |
| `hitbox.clear_all()` | Clear all attack capsules through the engine. |
| `hitbox.clear_victims(index)` | Clear the capsule's recorded victims through the shared collision helper. |

World-position placement, `set_enabled` and `clear_victims` accept integer expressions;
out-of-range indices do nothing. State, damage and group methods accept integer
expressions addressing slots `0` through `3`; invalid indices produce a script
error. These methods change live
capsules, not the action's saved subactions. Groups affect repeat-hit tracking;
Rollout alternates two groups after its configured interval:

```text
if hitbox.state(0) != Hitbox.disabled:
    hitbox.set_group(0, (hitbox.group(0) + 1) % 2)
```

Group tags use the engine's unsigned 32-bit assignment semantics. Integer
arguments narrow to that width; they are equality tags, not array indices.
The getter can observe any capsule state, including disabled slots.

[`hitbox.active()`](fighter.md#hitboxactive-and-hitboxset_damage) tests the
specific Enabled state. `enabled()` also accepts other non-disabled capsule
states, matching stock callbacks that use that broader test. Use
[`hitbox.set_damage()`](fighter.md#hitboxactive-and-hitboxset_damage) to
recalculate live damage through the engine.

Use `Hitbox.disabled` and `Hitbox.enabled` with `hitbox.state(index)` when a
move distinguishes the exact Enabled state from other non-disabled states.
Clearing victims and enabling a hitbox are independent operations; use
`hitbox.clear_victims(index)` followed by `hitbox.set_enabled(index, true)`
when a move requires both. Clearing victims does not enable the hitbox.
Neither operation changes its saved subactions.

Enabling an attack also marks the fighter as having attacks to clear during an
ordinary motion change. It does not clear victim history or bypass a transition's
explicit keep-hitboxes flag. Disabling one capsule does not clear that fighter-wide
cleanup marker; `hitbox.clear_all()` clears all capsules and the marker together.
This bookkeeping prevents a manually refreshed attack from surviving an unrelated
state, the cause of stock Ness's delayed yo-yo refresh glitch. No fighter-specific
behavior is built into this operation.

### `hitbox.damage(index)`

Read a capsule's current damage. `index` is a literal from 0 to 3. It does not
require the capsule to be enabled. When scaling damage over several animation
ticks, store the original value once in ordinary fighter state; repeatedly scaling
the previously scaled damage would compound it. Use `int(...)` if the original
move truncated that base damage to an integer.

### `hitbox.set_world_position(index, x, y, z)`

Set a capsule's world-space offset through Melee's collision helper. It detaches
the capsule from its joint so subsequent collision updates use that world position.
It does not create, enable or damage anything itself. All arguments are expressions;
indices outside 0–3 do nothing.
The call does not immediately write the capsule's current or previous endpoint.
Use [collision capsule coordinates](collision_capsules.md) when a script needs
to set those positions explicitly rather than waiting for the common update.
Call after the subaction creates the capsule. An [accessory callback](language.md#special-accessory-callbacks)
can consume the subaction's [cue](#cueconsumebit) and then use a
[floor query](stage.md#terrain-queries) for an attack that follows terrain.

## Motion and model

### `bones.set_scale(bone, x, y, z)`

Set a semantic bone's local XYZ scale through the engine's joint transform
operation. `bone` is a named role such as `XRotN`; each scale argument is an
expression. The role resolves against the current fighter's skeleton, so the
script need not hard-code the stock joint index. A missing joint does nothing.

```text
bones.set_scale(XRotN, attrs.travel_scale_x, attrs.travel_scale_y, attrs.travel_scale_z)
```

This changes the live pose, not the saved animation or rest skeleton. Animation
updates may overwrite it; reapply it in the appropriate callback when needed.
Use `model.set_scale(...)` for the model root rather than a named part.

### `state.enter_at_speed(motion, flags, start_frame, rate)`

Change to a numbered motion with an explicit animation rate and preservation
flags. Arguments can be expressions, including function arguments. Unlike
[`state.transition()`](fighter.md), this does not pick a special slot's flags
or automatically install a cue. Install callbacks after a change when the
engine clears them. `0` rate is useful for procedurally rotated charge poses.

### `state.landing_special(interruptible, lag)`

Enter the common special landing state. The script is responsible for any
preceding ground transition and cleanup. This is an explicit state change;
[`collision.air_special_landing()`](movement.md) instead performs the complete
standard collision/landing check.

### `animation.set_timer(frame)` and `animation.set_rate(rate)`

`set_timer()` writes the current motion timer. It does **not** sample a new
pose, execute commands up to a frame, or seek the Workshop timeline.
`set_rate()` changes playback through the engine animation system. To start
a newly entered motion, use [`animation.start()`](fighter.md).

### `bones.root.scale_x`, `bones.root.scale_y`, `bones.root.scale_z`

Read the current fighter model root scale. `bones.root.set_scale(x, y, z)` writes
the scale and marks the joint matrices dirty. These are model transforms;
they do not alter the fighter's gameplay position or costume data on disk.
Save a starting scale in ordinary `state` fields if an interruption must
restore it. See [Bone observations](bone_queries.md#rendered-root-scale)
for the distinction between the rendered root and mapped fighter bones.

### `ground.acceleration` and `position.x/y/z`

`ground.acceleration` is readable and writable alongside
[`ground.velocity`](movement.md). `position.x/y/z` are read-only world
coordinates for effects, collision decisions, and other calculations.

## Collision queries

| Expression | Engine check |
| --- | --- |
| `collision.ground_box(top, bottom, left_x, left_y, right_x, right_y)` | Ground collision with a custom box. |
| `collision.ground_box_edges(top, bottom, left_x, left_y, right_x, right_y)` | Ground collision with edge handling. |
| `collision.air_box(top, bottom, left_x, left_y, right_x, right_y)` | Air collision with a custom box. |
| `collision.air_box_ledge(direction, top, bottom, left_x, left_y, right_x, right_y)` | Air collision with a direction-sensitive ledge check. |

All return whether the engine found ground. A query updates collision state;
call it once, store its result, and then inspect contacts. The two additional
box queries work in helpers or collision callbacks. Ordinary ground/air box
queries retain their existing collision-callback scope.

`collision.left_wall` and `collision.right_wall` read the respective contact
masks. These names follow Melee's left/right wall conventions, not the
fighter's facing direction. `wall.left.normal.x/y` read the engine's
left-facing-wall normal. `ecb.top`, `ecb.bottom`, `ecb.left` and `ecb.right`
each expose read-only `x` and `y` components of the Fighter's current cached
collision-box offsets. These are offsets from the collision origin, not world
positions or a request to recalculate collision. `floor.friction`
returns the stage material's friction multiplier for the current Fighter or
owned Article collision cache, using the low-byte material index; upper collision
flags do not change the lookup. The property is read-only; malformed indices
outside the engine's twenty-entry table reject. `floor.normal.x/y` read its
normal. Contacts should be read after a collision query for that frame.

## Effects, sound and color

### `effects.spawn_async_on_bone(effect, bone)`

Spawn an asynchronous effect attached to a semantic bone, such as `Top` or `TransN`.
The effect ID is an expression. The existing three-argument form additionally
supplies an angle and uses the engine's directional-effect mode. Neither form
sets `effects.active`; set that ordinary effect marker explicitly when needed,
and install the shared effect hitlag callbacks separately.

### `cue.consume(bit)`

Consume one subaction signal bit, returning true only if it was set. `bit` is a
an Int expression from 0 to 7. Values outside that range fault without mutation.
This operation requires a current owned fighter. Consuming clears that bit, including when a later condition
fails. For example, `if cue.consume(3) and collision.floor_contact:` consumes the
signal before testing floor contact. `cue.article.consume()` selects bit 0 and
`cue.throw.consume()` selects bit 1. `cue.throw.reset()` clears all eight bits.

## Damage callback lifecycle

### `callbacks.track_secondary_damage()` and `callbacks.clear_secondary_damage()`

Install or clear the fighter's secondary damage callback. Registration requires
an authored `on secondary_damage:` handler, including when registered inside a
[function](functions.md). This is a separate engine slot from `on damage`, not a
second declaration of that handler. Shared damage processing determines which
slot runs and when. Motion transitions may clear the registration; reinstall it
where the original move did. Clearing it leaves the primary damage/death callbacks
intact. DK's punch uses this distinction to clear effects without clearing charge.

| Method | Behavior |
| --- | --- |
| [`effects.spawn_with_scalar_at_position(id, x, y, z, scalar)`](joint_feedback.md#effectsspawn_with_scalar_at_positioneffect-x-y-z-scalar) | World-position and one-scalar recipe; scalar meanings belong to the recipe. |
| [`effects.spawn_with_two_scalars_at_position(id, x, y, z, first, second)`](joint_feedback.md#effectsspawn_with_two_scalars_at_positioneffect-x-y-z-first-second) | Exact world-position and two-scalar recipe; recipe 1023 uses facing and rotation. |
| `effects.spawn_facing_at_position(id, x, y, z, facing)` | Effect expecting world position and facing, without a rotation argument. |
| `camera.quake(kind, x, y, z)` | Request an engine camera quake at a world position. |
| [`controller.rumble(pattern, duration)`](world_feedback.md#controllerrumblepattern-duration) | Request a loaded rumble pattern for the Fighter’s controller, using native eligibility and duration rules. |
| `sound.play(id, volume, pan)` | Play a fighter sound with normal engine bookkeeping. |
| `sound.loop(id, volume, pan)` | Play through the fighter's dedicated looping-sound channel. |
| `color.start(id, restart)` | Start a common color animation. |
| `color.clear_attack()` | Clear the attack color animation. |

Effect argument layouts must match the chosen effect definition. These
methods are not interchangeable; they pass different arguments to the engine.
Charge Shot's release effect uses `spawn_facing_at_position`; its definition
reads a position and facing pointer but no angle.
For example, stock floor dust `0x3FF` expects orientation, whereas wall impact
`0x406` expects rotation. Angles are radians. Quake kinds are `0` none, `1`
loop, `2` small, `3` medium and `4` large. Volume and pan use the engine's
byte values, normally `127` and `64`. `sound.loop()` selects a sound channel;
it does not schedule repeated script callbacks. See [fighter sound channels](sound_channels.md)
for the package-owned suppression policy used by scripts and subactions.

## Events and action statistics

### `callbacks.track_reflect()`

Install the script's `on reflect` event in the fighter's reflection callback
slot. A matching handler is required, including when registration happens
inside a helper. The engine performs its shared reflection bookkeeping;
the callback updates any move-owned velocity, facing, and state that also
need reversal. It is not a projectile-reflection event.

For a projectile hitting a fighter-owned bubble, use the separate
[`reflector` API and `on reflect_hit`](reflector.md).

### `callbacks.deal_damage(None)`

Clear the move's native and interpreted damage-dealt callback slots. This prevents a hit reaction from
running that move callback again. It does not remove the damage-taken or
death cleanup callbacks.

Use the ordinary [callback-slot API](callback_slots.md), rather than the legacy
generated-module `clear_deal_damage()` spelling.

### `stats.refresh_attack()` and `stats.record_attack()`

`stats.refresh_attack()` clears the current segment's statistics, captures its
starting damage and condition flags, allocates its statistics counter, and
updates the player's action statistics. It preserves the current action's
classification. It invokes the common native `ft_80089824` operation.

`stats.record_attack()` allocates a fresh attack-instance number while retaining
the current attack ID, through `ft_800892A0`. It does not add a move to the
staleness queue by itself. Both take no arguments and return Void.

A move that starts another attack segment can invoke these from its
[`on state_change`](callback_slots.md) callback. Neither method chooses an
attack or decides when a segment starts. The script owns their order; for
example, Marth and Roy refresh first and then allocate the attack instance,
matching their native side-special transition.

## Hurtboxes and attached-item visibility

| Method | Effect |
| --- | --- |
| `hurtboxes.set_all(status)` | Change every body capsule's hurt state. |
| `hurtboxes.set_bone(bone, status)` | Change the first body capsule attached to that costume joint. |
| `items.set_held_visible(visible)` | Update the held item's visibility through its ordinary visible/invisible callbacks. |
| `articles.attached_visible` | Read or set a Bool that gates drawing of the fighter's two engine-attached article slots. |

Use `Hurt.normal`, `Hurt.invincible` or `Hurt.intangible` as the status.
Invincible capsules receive contact without ordinary damage; intangible
capsules cannot be struck. `set_bone` requires two Int arguments; invalid statuses
or malformed tagged values fault before mutation. Out-of-range bones and bones
without a hurt capsule do nothing. `bone` is a raw costume joint index; use
[`bones.index(Name)`](guard.md#bonesindexname) when a semantic bone is appropriate.
The operation acts on the owned source fighter, not another fighter receiver.
Only the first matching capsule changes, its position-update flag is preserved,
and enabling that capsule does not clear the aggregate hurt-status flag. Use
`set_all(Hurt.normal)` when all capsules and that flag should return to normal.

```text
hurtboxes.set_bone(bones.index(RightShoulder), Hurt.intangible)
items.set_held_visible(false)
articles.attached_visible = false
```

These change live simulation/presentation state, not saved subactions or the
model workspace's editor-only eye buttons. The held-item method also keeps
the engine's visibility latch synchronized; repeated calls with the same
value do not repeatedly fire the item's callback. Article visibility does
not destroy articles or disable their collisions. It controls only the two
ordinary attached slots, rather than every owned article or effect. Normal
motion entry may reset this flag according to the native motion policy.

The held-item method takes a Bool and returns Void. A transition dispatches
the fighter's `on item_visible` or `on item_invisible` hook synchronously;
with no held item, it still updates the visibility latch. If that callback
retires the fighter, the call stops and does not publish a result or continue
the helper. `articles.attached_visible` does not dispatch those hooks.

`hurtboxes.set_all(Hurt.normal)` restores all capsules and clears the shared
hurt-state override flag. A bone-only change does not reset other capsules.

### `effects.spawn_at_position(effect, x, y, z)`

Spawn an engine effect with a world-position argument instead of a joint.
The effect must expect that argument layout; effect IDs are not interchangeable
between joint-attached and position-based effects. Arguments can be expressions:

```text
effects.spawn_at_position(1240, position.x, position.y, position.z)
effects.active = true
effects.pause_during_hitlag()
```

Spawning alone does not set the move's `effects.active` flag or install
hitlag callbacks. The script owns those decisions. Bowser Bomb uses a world
position for its landing dust and a joint for its diving effect.

The engine's numeric generator families (`0..591` and `30000..30999`) also
use this method. They immediately copy the world position and do not require
the creator's model root. For example, effect `27` is the original charge
flash. The selected effect still needs its particle resources; accepting an
argument layout does not make every effect asset available in every scene.

## Numeric conversion

`int(value)` truncates to a signed 32-bit integer; `float(value)` rounds to
binary32 at that point in the expression. `pi` is the double-precision
constant. `atan2_float(y, x)` calls the same native float operation used by
stock callbacks; [`atan2(y, x)`](movement.md) remains double precision.
Hexadecimal integer literals such as `0x40012` are supported through
`0xFFFFFFFF`. These tools preserve explicit stock rounding boundaries while
keeping custom parameters and state named and editable.

## Bone directions and instance scale

### `bones.segment_angle(bone, end_x, end_y, end_z, start_x, start_y, start_z)`

Transform two offsets from the same semantic bone into world space and return
the signed XY angle from the start point to the endpoint, in radians. The Z
coordinate can affect the transformed X/Y even though the returned direction
lies in the gameplay plane. Missing bones return zero. Arguments may be
expressions. The engine uses float `atan2` after transforming both points.

```text
let angle = bones.segment_angle(RightThumb, 0, 1.23, 4.26, 0, 1.23, 0.01)
articles.launch_projectile(attrs.projectile_kind, RightThumb, 0, 1.23, 4.26, angle, attrs.speed, 1)
```

The [launch operation](article.md) accepts that world-space angle directly;
do not multiply it by facing again.

### `model.effect_scale`

Read the fighter's common effect-size attribute as a Float. This is separate
from model scale, instance scale and the animated root's scale. A recipe that
expects this attribute can receive it as the final argument to
`effects.spawn_with_scalar_at_position(effect, x, y, z, scalar)`. The recipe
defines what its scalar means; it may instead represent rotation or facing.
Reading the attribute does not automatically resize existing effects.

### `model.instance_scale_y` and `model.base_scale`

Read the fighter's instance Y scale and common model scaling attribute,
respectively. These are distinct from the animated root joint's
[`bones.root.scale_y`](#bonesrootscale_x-bonesrootscale_y-bonesrootscale_z). For example, an
attachment can scale with the fighter's authored body size without inheriting
an animation's temporary squash. Both expressions are read-only.

## Named bone rotation and pickup control

`bones.rotate_x(bone, angle)`, `bones.rotate_y(bone, angle)`, and
`bones.rotate_z(bone, angle)` set a named engine bone's rotation in radians.
They resolve the semantic bone through the active fighter's skeleton mapping;
they do not assume that every costume uses the same numeric index. `TransN`,
`XRotN`, and `YRotN` are available alongside the existing attachment bones.
These calls preserve the engine's quaternion/secondary-joint handling.

```text
bones.rotate_x(XRotN, float(6.2831854820251465) - state.travel_angle)
```

`items.block_ground_pickup = enabled` controls the fighter's transient exclusion
from ordinary nearby-ground-item pickup detection. It does not remove a held
item, change the costume, or disable explicit article creation. Motion changes
normally reset this flag; reapply it after transitions when the move requires it.

`items.reject_healing_pickup(enabled)` instead controls the common pickup filter
for items whose engine healing query returns a nonzero amount. It applies to
ordinary nearby-item candidates, including the supported food, heart, tomato,
Lucky Egg and healing apple items. Non-healing items remain eligible. The operand
must be Bool. This is a fighter initialization flag; it neither prevents explicit
article creation nor substitutes for `items.block_ground_pickup`.

`family.retire_companion_on_sleep(enabled)` controls the shared Sleep-entry rule
that retires entity 1 in the player's family. The operand must be Bool. The engine
first performs its normal Sleep/invisibility/camera setup, then retires the
companion if present, then updates the player's match status. Set this during
fighter initialization when the primary member should retire its companion.
It does not send a special-move cleanup notification to that companion.

### `bones.index(Name)` and shoulder attachments

[`bones.index(Name)`](guard.md#bonesindexname) resolves a semantic bone to a
[bone ID](bone_queries.md#bone-ids). `LeftShoulder` and `RightShoulder` are also accepted by the
named-bone methods. For a joint-attached effect that receives facing, use:

```text
effects.spawn_with_facing(1265, bones.index(RightShoulder))
```

This preserves the engine effect's joint and facing argument layout.

### `effects.spawn_async_attached(effect, bone)`

Create an asynchronous effect attached to a semantic fighter bone, with the
engine's attachment argument mode. `effect` accepts an expression; `bone`
uses the named bone roles such as `Top` or `Hip`. The effect participates in
the fighter's asynchronous-effect list. This differs from
`effects.spawn_async_on_bone()`, whose two-argument form supplies a world
position sampled from that bone. Use attachment when the effect should follow
the joint rather than receiving that position argument.

## effects.spawn_between

`effects.spawn_between(effect, first_joint, second_joint)` creates a synchronous
effect spanning two joints in the fighter's own skeleton. All arguments accept
expressions. Joint arguments are the fighter's bone IDs. Get a role's ID with,
for example, `bones.index(TransN)`. Missing IDs are ignored. Scripts own duplicate suppression, cleanup
and [hitlag pause/resume](fighter.md).

## visuals.set_visible

`visuals.set_visible(visible: bool)` controls whether the engine draws the
fighter. It does not disable physics, collisions, or the held item. Moves that
temporarily hide a fighter must restore visibility on their relevant exit
paths. Capsule status is controlled separately.

## hurtboxes.set_body_status

`hurtboxes.set_body_status(status)` sets the fighter-wide hurt status and
its corresponding engine color animation. Use `Hurt.normal`,
`Hurt.invincible`, or `Hurt.intangible`. This is distinct from
`hurtboxes.set_all()`, which edits individual capsule statuses. The engine
combines both levels when resolving collisions.

## model.effect_scale and positional scalar effects

`model.effect_scale: float` reads the common size attribute used to size
effects. It differs from `model.base_scale`, the instance scale, and the
animated root scale. It is read-only.

[`effects.spawn_with_scalar_at_position(effect, x, y, z, scalar)`](joint_feedback.md#effectsspawn_with_scalar_at_positioneffect-x-y-z-scalar)
supplies a copied world position and Float to an admitted engine effect. All
arguments accept expressions. The native recipe decides whether the scalar
means size, facing, rotation, or another value.

```text
effects.spawn_with_scalar_at_position(1231, position.x, position.y, position.z, model.effect_scale)
```

## visuals.set_group_material_frame

`visuals.set_group_material_frame(table, group, variant, frame)` requests and
evaluates the material animation of every mesh in the selected costume
visibility group/variant. It does not change which variant is visible.
Tables and groups are zero-based; invalid selections are script errors. Table 2
uses the alternate model mesh list, while the other tables use the main list.
`frame` may be fractional and must be finite. A reset or animation policy
belongs in the script. An absent channel performs no work. The interpreter
checks the selected owned lookup and mesh indices before evaluation and retains
material identities across request/evaluation. If the fighter or selected
graph changes during either operation, the call stops before continuing to
another material. Already evaluated materials are not reverted by that error.

## Numeric joint rotation

`bones.count: int` reads the active fighter's runtime part count, including
parts inserted separately from the costume, such as an attached sword.
Costume/animation joint counts may be smaller. Bone IDs can have gaps and remain
attached to their bones when the hierarchy changes.

`bones.root.rotate_x(joint, angle)`, `bones.root.rotate_y(joint, angle)`, and
`bones.root.rotate_z(joint, angle)` also accept a bone ID.
The one-argument forms rotate the bone with ID 0. Angles use radians. Prefer
[`bones.rotate_x/y/z(Name, angle)`](#named-bone-rotation-and-pickup-control)
when a semantic bone role exists; numeric IDs are useful for authored
rig parts with no common role.

## Material animation initialization

`visuals.initialize_group_material_animation(table, group, variant, prior_end)`
returns the shared end frame of the selected visibility variant's material
animations and freezes each material animation's playback rate at zero. Pass
zero initially, then feed the returned value into subsequent calls to validate
that several groups use the same duration. Nonzero end frames must agree;
a mismatch or nonfinite length is a script error detected before any rates
are changed. An absent channel leaves `prior_end` unchanged; invalid selectors
and mesh indices are rejected. A missing mesh, material or material animation
is skipped. This performs no allocation or RNG work.

Store the result in an ordinary fighter variable, not in immutable imported
attributes. Then use
[`visuals.set_group_material_frame`](#visualsset_group_material_frame) to
sample it from an authored policy.

`visuals.current_group(group)` returns the current live visibility selector,
or `-1` for an invalid group. It reads the same selector changed by
[`visuals.set_current_group`](common_policies.md#visualsset_current_groupgroup-value),
which can differ from the saved selector used by a later visibility reset.

## Joint pose operations

`bones.set_translation(joint, x, y, z)` changes a bone's local translation.
The joint argument is its bone ID; a missing ID does nothing.
Resolve a named role with `bones.index(Name)` when needed.
This changes the live pose, not the fighter's gameplay position or saved keys.
Animation updates can overwrite it.

[`bones.set_scale`](#bonesset_scalebone-x-y-z) instead takes a semantic bone role
such as `XRotN`.

`bones.root.add_rotation_y(angle)` adds a radian angle directly to primary
joint 0's Y rotation. It does not perform semantic/secondary-joint quaternion
redirection; that distinction matters for accumulated root turns.

[`pose.copy_costume(Pose.live, joint)`](poses.md) restores the costume rest-pose
traversal starting at the supplied bone ID. It resolves the matching costume
descriptor and validates the descriptor path and target joints before copying.
Use `Pose.blend` for the blend rig. Resolve semantic roles with
`bones.index(Name)` when appropriate. This is a rest-pose copy, separate from
sampling the current animation.

`damage.jump_armor` reads or sets the shared jump armor amount. Move policy
and its named parameter belong to the script. `jump.update_animation()` runs
the common aerial-jump animation update, allowing a custom callback to add
its own turn/armor policy before or after the common update.

For a deferred effect at a joint's position with the common effect size, use
`effects.spawn_async_position_scalar_on_part(effect, joint, model.effect_scale)`.
The old native code passes the common effect-attribute block's address, but the
queue copies only its first Float (size), not the complete block. The interpreter
therefore exposes a generic scalar argument. Position is sampled at queue
processing time; the scalar is copied when queued. See
[deferred joint positions](joint_feedback.md#deferred-positions-on-raw-joints).
