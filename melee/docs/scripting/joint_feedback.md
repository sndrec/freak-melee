# Joint effects, position wind and motion playback

These direct Fighter operations take typed values. They expose shared engine
primitives; the script owns timing and move decisions.

| Method | Meaning |
| --- | --- |
| `effects.spawn(effect, part)` | Spawn an effect using a semantic bone role such as `LeftHand`. |
| `effects.spawn_on_part(effect, raw_part)` | Spawn using an exact index in this fighter's owned rig. |
| `effects.spawn_between(effect, first_part, second_part)` | Synchronously construct an admitted two-joint effect using two exact owned rig indices. |
| `effects.spawn_with_scalar(effect, part, value)` | Semantic bone role plus one copied Float consumed by the native effect. |
| `effects.spawn_on_joint_with_scalar(effect, joint, value)` | Exact rig index plus one copied Float. |
| `effects.spawn_async_attached(effect, part)` | Queue a joint-only effect on a semantic bone role, using the engine's immediate/deferred effect timing. |
| `effects.spawn_async_with_scalar(effect, part, value)` | Queue a joint plus one copied Float on a semantic bone role. |
| `effects.spawn_async_position_on_part(effect, raw_part)` | Queue a numeric engine effect or a declared position `model_effect` at the eventual world position of an exact owned rig joint. |
| `effects.spawn_async_position_scalar_on_part(effect, raw_part, value)` | Queue the same position sample with one Float copied immediately. |
| `effects.wind_at_position(x, y, z, frames, strength, decay, frequency)` | Create wind at the supplied world position; frequency is already radians. |
| `state.enter_at_speed(motion, flags, frame, rate)` | Enter an owned/common motion preserving all four values. |

Semantic roles are mapped through the owning rig. Raw joint indices are not
mapped again: part 104 means that exact joint, not semantic role 104.
`spawn_on_part` retains the integral effect ID until signature admission and takes
a signed Int raw part. Negative or out-of-range raw parts and absent joints are
healthy no-ops, matching the native raw-part operation. Incompatible effect
signatures or unavailable source ownership produce a diagnostic.
No operation borrows another fighter's model or behavior.

## Deferred positions on raw joints

The two `spawn_async_position` methods use a signed Int index in your fighter's
current costume. They admit native position-only or position-plus-scalar recipes,
respectively. Use `bones.index(Hip)` for the owning rig's mapped Hip joint.
The scalar's meaning belongs to the effect recipe; it must be a finite Float.

```text
effects.spawn_async_position_scalar_on_part(1231, bones.index(Hip), model.effect_scale)
```

The engine copies the scalar into the queue at this call. It samples the joint's
world position when it processes the queue, so later animation or movement before
processing affects the position. Changing the original scalar afterward does
not change the queued value. Native process links below 9 defer execution; other
contexts execute immediately. Queue order, allocation failure and clearing keep
their native behavior. A failed allocation produces no effect.

An absent raw joint index or unmapped joint is a healthy no-op. An incompatible
effect signature, malformed type, nonfinite scalar or unavailable owned source
is an error. These methods borrow the joint until native queue processing; they
do not promise safety for arbitrary rig replacement while effects are pending.
For an explicit world position copied immediately, use the position methods below.

See [two-joint effects](effect_between.md) for admitted signatures, creator-root
scaling, partial allocation and synchronous animation order.
For a copied world position and scalar, see
[`effects.spawn_with_scalar_at_position`](#effectsspawn_with_scalar_at_positioneffect-x-y-z-scalar).

Effect signatures differ. Joint-only methods admit native effects that consume
exactly a joint; scalar methods admit a joint followed by an immediately read
Float. Position, erased-pointer, forwarded argument lists and other signatures
are not guessed. Scripts never pass native pointers. For example, Mario's stock
fireball visual reads a facing Float in addition to its hand joint:

```text
effects.spawn_with_scalar(1146, LeftHand, facing.direction)
```

For a literal rig index, pass facing explicitly through the same scalar API:

```text
effects.spawn_on_joint_with_scalar(1170, 1, facing.direction)
```

This is the common joint-plus-Float contract used by Falcon/Ganondorf's
side-special feedback. A dedicated `spawn_with_facing` operation is unnecessary;
the script can choose any finite scalar expression. An incompatible native
effect signature still rejects rather than guessing its argument layout.

The Float is copied before the native call. An effect cannot retain a pointer
into the fighter's script variables through this API. Already completed native
side effects are not replayed if a later callback retires the source.

## `effects.spawn_async_with_scalar(effect, part, value)`

Queues an admitted joint/scalar effect with native attachment mode 3. The engine
copies `value` into the queue record immediately, so later script edits do not
alter a deferred effect. Delivery follows the ordinary immediate/deferred
effect scheduling, LIFO order and cancellation rules. The operation returns
Void and leaves `effects.active` to the script.

The effect ID must be integral, the semantic part must map to a live joint in
this fighter's owned rig, and `value` must be a finite Float. The native effect
signature must consume exactly a joint followed by one Float; no variadic
argument shape is guessed. Joint class/extent and source/program/storage/rig
identity are retained through queue publication.

```text
# The native effect determines the scalar's meaning; this recipe uses radians.
effects.spawn_async_with_scalar(1168, RightFoot, 0)
effects.spawn_async_with_scalar(1292, LeftFoot, radians(25))
```

See [model effects](model_effects.md) for authored visual update logic and
[effects](combat_visuals.md) for other spawn signatures. This operation is
available to any fighter; it does not install a stock fighter callback.

## `effects.spawn_with_scalar_at_position(effect, x, y, z, scalar)`

Owned Article callbacks can use the corresponding
`item.spawn_with_scalar_at_position(effect, x, y, z, scalar)`. The effect ID must
be integral and its native recipe must consume one copied Vec3 followed by one
copied Float. XYZ and scalar must be finite. The scalar's meaning is defined by
the effect recipe, such as rotation, facing or scale; the call does not choose a
fighter-specific behavior. Fighter-parent recipes are rejected for Items. Recipes
which read the creator's root require its admitted JObj; other recipes can spawn
from a rootless Item. Source retirement stops subsequent script statements and
result publication, without undoing effects already spawned.

For example, to emit a muzzle effect from the original creator's animated rig:

```text
let float x = 0
let float y = 0
let float z = 0
creator.bones.world_position_offset(creator.bones.index(RightThumb), x, y, z, 0, 1.2325, 4.2636)
item.spawn_with_scalar_at_position(1166, x, y, z, 0)
```

Guard the creator relation before using its bone query. The bone query samples
the current native matrix; the effect receives the resulting world position,
not a joint pointer or a local offset.

This Fighter operation synchronously supplies one copied world position and
one copied Float to an admitted native effect. The ID is integral; coordinates
and scalar are finite Floats evaluated once in written order. It returns Void
and does not change `effects.active`.

The native recipe determines the scalar's meaning. Link's effect `1009`
(`0x3F1`) negates the scalar and uses its sign for facing; effect `1231`
(`0x4CF`) uses the scalar to size its twelve separate models.

```text
effects.spawn_with_scalar_at_position(1009, position.x, position.y, position.z, facing.direction)
effects.spawn_with_scalar_at_position(1231, position.x, position.y, position.z, model.effect_scale)
```

The current source audit admits 34 exact position-plus-scalar recipes. Joint,
two-scalar, scalar-ignored and unresolved signatures reject. Effect `0x4D9`
also requires the retained Fighter's valid primary model root; `0x447` installs
a Fighter-parent callback. Fourteen recipes create native effect models and
require their ordinary imported effect assets. Native allocation failures keep
the engine's partial/no-op behavior; these calls do not promise allocation-free
construction.

The binding checks the same physical owned Fighter, program, storage and rig
around construction. Native helpers immediately copy the position and scalar
value rather than retaining pointers into script storage. Completed native
effects are not undone if a later check detects retirement. The ABI cohort
checks all 65,536 IDs and exact native branch, pointer-consumption and creator
requirements; the connected Item-terrain cohort also runs the actual Fighter
VM and native `0x3F1`/`0x4CF` branches with silent controlled resource leaves.
These checks do not establish graphics or gameplay acceptance.

The older `effects.spawn_scaled_at_position` spelling belongs to the legacy C
exporter. The interpreter uses the generic scalar method above.

## `effects.spawn_with_two_scalars_at_position(effect, x, y, z, first, second)`

Create a built-in effect whose recipe consumes a world position and two scalar
values, in that order. Their meanings belong to the selected recipe. All five
float arguments must be finite; the effect ID must be an integral value in the
native 16-bit range. Recipes with other signatures are rejected before spawning.

```text
# Recipe 1023 uses facing direction followed by Z rotation in radians.
effects.spawn_with_two_scalars_at_position(1023, position.x, position.y, position.z, facing.direction, radians(25))
```

The current native audit admits recipe `1023` (`0x3FF`). Its constructor copies
the position, dereferences the first scalar for facing and immediately copies
the second scalar into the new effect model's rotation. No pointer to script
storage or temporary arguments is retained. It requires ordinary imported
effect assets, but does not read the creator's model root. Owned fighter,
program, storage and rig identities are checked before and after construction;
completed effects are not undone if the source retires. This operation exposes
an argument shape, not a fighter-specific effect API.

## `effects.spawn_async_attached(effect, part)`

The effect ID must be an integral value identifying an engine effect that takes
exactly one joint. `part` is a semantic bone role, mapped once through the
fighter's owned rig; use the synchronous raw-joint methods for exact rig indices.
Scalar, position and other effect signatures are not accepted by this call.

The native effect queue determines timing. Calls made in its deferred update
phases prepend work to the fighter's queue and drain in the original order;
other phases dispatch immediately. A frozen fighter retains deferred work until
the native update can drain it. Allocation failure is the engine's no-op result,
not a script retry. Queue cancellation and fighter teardown use the existing
engine paths.

This method does not change `effects.active`. Set that flag explicitly if your
move uses it to request the normal effect cleanup policy. Queue ownership and
the flag are separate concerns. The binding validates the retained source and
mapped joint around dispatch, but does not replace native particle scheduling
or imply allocation-free queue growth.

Position wind uses the three coordinates as given. It neither samples a bone
again nor adds a facing offset. The existing `effects.wind(part, offset, ...)`
operation keeps its semantic-bone sampling and facing-offset behavior.

Motion entry preserves UInt flag bits and Float frame/rate values, including
native zero and negative playback rates. It does not implicitly start animation.
Use `animation.start()` only where the move's original sequence requires it.
Entering at the default rate can use `state.enter_at(motion, flags, frame)`.

See [motion callbacks](motion_callbacks.md), [movement](movement.md) and
[alternate forms](forms.md) for scheduling and arrival behavior.
