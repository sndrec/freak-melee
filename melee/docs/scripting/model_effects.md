# Animated model effects

An animated model effect is a visual object constructed from a fighter joint
or an explicit world position. Use
`model_effect` to give it an owned update script. The engine constructs and
animates the model; your update controls the visual decisions that depend on
the fighter, such as hiding a branch or aligning a swirl to a sloped floor.

This is separate from an [article](article.md). A model effect does not gain an
item's movement, hitboxes, ownership transfer, or article callbacks. Ordinary
particle spawns still use the [effect operations](combat_visuals.md).

## Declare and spawn an effect

Use a model from your fighter's resources or select a built-in recipe.

### Use your own model

```text
model_effect FeatherBurst "effects/feathers.dat" 60:
    pass

move Launch:
    on air_animation:
        if frame == 33:
            effects.spawn_root(FeatherBurst)
```

Import the DAT as a blob resource with the same path. The number after the
path is the lifetime in ticks. The game samples the fighter's world position
when the effect spawns. Use `effects.spawn_on_part` or
`effects.spawn_async_position_on_part` to sample a bone.

Add `follow` after the lifetime to keep the effect at the fighter's root or
selected bone as it moves. Use `effects.spawn_root` or `effects.spawn_on_part`
for a following effect. The effect keeps its own orientation and scale.

The DAT contains an `effect_joint` model and can include
`effect_animjoint` and `effect_matanim_joint` roots for joint and material
animation. Animate child joints to move the model relative to its spawn point.
The effect plays its animations automatically. Use `pass` for animation alone,
or add one `on update` callback for behavior driven by the game.

### Use a built-in recipe

```text
state bool touches_floor

model_effect FloorSwirl 0x47c:
    on update:
        effect.joint("1").visible = creator.motion == 349
        if creator.command3 and creator.state.touches_floor:
            effect.joint("").rotation.z = -atan2_float(creator.floor.normal.x, creator.floor.normal.y)
        else:
            effect.joint("").rotation.z = 0

move Spin:
    on special_lw:
        effects.spawn_root(FloorSwirl)
```

The recipe ID selects an existing engine model constructor, not a donor
fighter's updater. Named construction binds the interpreted update directly.
The immutable catalog declares each admitted constructor's argument family
and complete model count. Particle recipes and unsupported argument families
are rejected. Joint recipes accept a root or bone; position recipes accept
ordinary world coordinates. Recipe `0x4cc` constructs one positioned model;
`0x4da` constructs two linked models. Recipe `0x47c` is the stock Tornado model, whose second
child is its ground swirl. Its geometry and animation assets must be available
in the fighter package's effect bank.

`effects.spawn_root(Name)` constructs the declared effect from the fighter root.
`effects.spawn_on_part(Name, raw_part)` constructs a joint recipe attached to a
chosen raw joint slot, or a position recipe at that bone's current world
position, and runs the same declared update program. Use
`bones.index(BoneRole)` to find that slot in the fighter's own rig:

```text
effects.spawn_on_part(FloorSwirl, bones.index(Hip))
```

The raw index is signed; this operation does not remap it a second time.
A healthy owned rig with an absent optional slot produces no effect. A
mismatched rig, recipe family, class, program or scene is an execution error.
The live effect retains its creator root and selected bone separately, so bone
motion remains valid while bone replacement or scene retirement invalidates the
attachment. Packed save/load preserves this selection and checks native peers
before restoring script state.

`effects.spawn_async_position_on_part(Name, raw_part)` queues a named position
recipe, such as `0x4cc`, at a selected bone's eventual world position. It uses
the engine's normal deferred sampling phase and binds the declared update
program when the model is constructed. The queue stores program, parameter,
layout and creator identities instead of VM stack or program pointers. Reload,
bone replacement or scene retirement drops a stale request. Clearing the
engine queue releases the request without constructing a model.

```text
effects.spawn_async_position_on_part(HandSlapFacing, bones.index(Top))
```

Named explicit-position and extra-scalar construction remain migration work.
Unsupported numeric effect calls reject named declarations rather than using
their declaration indices as engine effect IDs. Numeric engine operations in
[Combat visuals](combat_visuals.md) remain available separately.

Numeric effect spawns retain their existing recipe behavior;
use a declared name when you want an authored model update.

Declare up to 64 interpreted effects with distinct names. Each can have one
`on update` body. Shared helper functions use the same
[statement and expression language](functions.md), compiled for the effect
context when called here.

## Spawn from an article

```text
model_effect ShellFlash "effects/shell-flash.dat" 12:
    pass

article ExampleProjectile states 1:
    on damage_dealt:
        item.spawn_effect_at_position(ShellFlash, position.x, position.y, position.z, 0)
        return false
```

Give the packaged effect a `pass` body and a lifetime. The last argument sets
its roll in radians around world Z. The effect plays at the supplied position
while the article moves on. It ends at its declared lifetime or when its
article is removed. `item.destroy_effects()` clears it early.

## Read the creator

Creator properties refer to the fighter that spawned the effect. The effect's
update callback can read these properties.

| Property | Meaning |
| --- | --- |
| `creator.motion` | Current animation motion ID. |
| `creator.facing` | Facing direction as a float, normally `-1` for left and `1` for right. |
| `creator.command0` through `creator.command3` | Current move command variables. |
| `creator.state.name` | Named script-owned fighter variable, including indexed arrays. |
| `creator.floor.normal.x`, `.y`, `.z` | Current floor collision normal. |
| `attrs.name` | A named [owned parameter](language.md#fighter-attributes). |

An effect update cannot change the fighter's velocity, enter a move, mutate its
state, or call a fighter-only helper. Keep those decisions in the fighter's
ordinary callbacks. For example, collision callbacks set `touches_floor`, and
the effect reads that result to choose its rotation.

## Control joints

`effect.joint("path")` selects a joint in the first model.
`effect.joint("path", model_index)` selects a linked model by an optional
literal index `0..3`; nonliteral indices are rejected. Each admitted recipe
has a fixed model count, and indices beyond that count resolve as missing. An empty path is
the root. Each slash-separated number selects a child by zero-based sibling
index: `"1"` is the root's second child and `"0/2"` is the first child's third
child. Paths support at most 12 levels and child ordinals `0..255`.

| Property | Access | Meaning |
| --- | --- | --- |
| `translation.x`, `.y`, `.z` | Read/write | Local translation in joint units. |
| `rotation.x`, `.y`, `.z` | Read/write | Local Euler angles in radians. |
| `scale.x`, `.y`, `.z` | Read/write | Local scale factors. |
| `visible` | Read/write | Read this joint's visibility; writing also changes its subtree. |

For a following effect, use a child joint to offset the artwork from its
attached bone. The attachment sets the root's world position.
The connected effect fixture exercises all three Euler
channels through source compilation, real effect scheduling and actor-state
save/load; imported model loading and drawing are controlled in that fixture.

Euler writes do not alter quaternion joints. Non-finite writes are ignored.
Missing-joint writes do nothing; reads return zero for translation/rotation,
one for scale and quaternion W, and false for existence/visibility. These are
the same HSD operations used by [article model controls](article_models.md).

## End the effect

`effect.cancel()` ends the current effect and immediately stops the callback.
For a linked group, cancelling or removing any member ends the whole group.
It also stops continuation through a helper that cancelled the effect. The
engine rejects later callbacks even if object deletion is deferred.

Construction is atomic: if any required linked model is missing, the newly
created members are removed. This deliberately replaces the stock partial
allocation case that could leave one member without its intended updater.
Each member retains its own animation/lifetime update; the authored policy
runs on the primary model after that model's animation.

The binding also ends when the original fighter is removed or recycled, its
active module changes, or its attachment no longer matches its rig. Scene
cleanup removes authored effects before releasing their modules. An effect
cannot accidentally attach to a new fighter occupying the same object address.

## Update timing

The engine retains the original model animation and effect scheduling. The
authored update runs at the stock update point, after model animation. It is
not a draw-only callback. Hitlag pause and effect cleanup still belong to the
ordinary effect system.

The [Mario template](../../examples/mario_full.fms),
[Dr. Mario template](../../examples/drmario_full.fms), and
[Luigi template](../../examples/luigi_wip.fms) use this to own their swirl's
visibility and slope decisions. Validation evidence and remaining boundaries
are recorded in [the backend ledger](../native_model_effects.md).
