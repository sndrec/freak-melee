# Fighter presentation

Presentation policy belongs to the fighter package, just like special-move
decisions. These operations use shared engine routines and work with any
authored article. They do not select a stock fighter's constructor.

See [Colors and model presentation](colors.md) for diffuse/outline colors,
instance width and owned secondary visibility tables.

## `on match_entrance:`

Return `true` to play the fighter's **Entry** action during Ready. The
game plays its animation and timeline cues, then enters standing idle or
falling when the clip ends. The end of Ready also ends a clip still
playing, so the player can act on the normal frame.

Edit **Entry** in the Action Editor to set the timing of sounds and
temporary mesh selections. Its animation can be edited in the Animation
workspace. A fighter can choose the usual trophy entrance by returning
`false`.

```text
action MatchPresentation:
    on match_entrance:
        return true
```

## `on demo_enter:`

The shared engine enters the selected demo state first, then calls this fighter
event. Integer `event.mode` identifies the scene variant. Versus results use
variants 0, 1 and 2; a package can add accessories for the variants it needs.
An absent handler leaves the ordinary shared presentation unchanged.

Dr. Mario's [script](../../examples/drmario_full.fms) uses variant 1 to create
two presentation articles with separately chosen appearances. His move scripts
and article callbacks own the decisions, including initialization order and
when to follow their original creator. Classic and video-specific scenes are
outside the current versus milestone.

## `on victory_fanfare:`

The fighter chosen for the victory music receives this event when the results
screen opens. In a team match the game chooses one winner to play the fanfare.
Play a clip from the fighter's sound bank to give that fighter a victory theme.

```text
action VictoryTheme:
    on victory_fanfare:
        sound.clip(0, 50, 110, 64)
```

## Temporary visibility selections

`visuals.set_current_group(group: int, variant: int)` changes the current model
selection without changing its saved selection. Use `-1` to hide the group's
alternatives. The engine immediately invalidates channel-zero draw visibility
when the selection changes. This differs from `visuals.set_group(group, variant)`,
which sets both current and saved selections and participates in normal resets.

Both operations use the fighter's owned visibility tables; group indices are
not bone indices. Temporary equipment can save `visuals.previous_group(group)`
and restore it later without overwriting the saved default. These controls also
work through a retained fighter receiver using that receiver's own package.

## Visibility group queries

`visuals.current_group(group) -> int` reads the currently selected visibility
variant. `visuals.previous_group(group) -> int` reads the engine's saved previous
variant for the same group. These values can differ during temporary equipment
or accessory changes. Both take an integer group index and return `-1` for an
invalid index; neither changes visibility or evaluates a material animation.

Use the saved value when restoring presentation that must respect the previous
variant. Use the current value when querying what is selected now. These are
group indices, separate from costume variants and skeletal joint indices.

## Partial skeletal animations

These controls operate on the fighter's owned part-animation groups, separately
from visibility groups and the fighter's main action animation.

| Operation | Behavior |
| --- | --- |
| `visuals.set_part_animation(group: int, variant: int)` | Choose the variant to show later. This does not play it or replace an already active variant. `-1` clears the choice. |
| `visuals.show_part_animation(group: int)` | Apply the chosen variant immediately with zero blend duration. With no chosen variant, leave the current pose unchanged. |
| `visuals.hide_part_animation(group: int)` | Remove the active partial animation and restore the underlying main animation at its current frame/rate, or the costume pose if no main animation exists. Keep the chosen variant available for a later show. |
| `visuals.apply_part_animation(group: int, variant: int, blend_duration: float)` | Apply an explicit variant with the native blend duration, without changing the saved choice. |

For example, a held-item callback can choose a hand pose when an item is picked
up, then show or hide that pose as the item becomes visible or invisible:

```text
visuals.set_part_animation(0, 1)
visuals.show_part_animation(0)
visuals.hide_part_animation(0)
```

Groups range from 0 through 4; variants range from 0 through 7 and must exist in
the package. Showing an unselected group or hiding an inactive group is a no-op.
Restoration validates the selected costume, current clip, rig, omission mapping
and animation-family remapping before changing the active layer. These are
ordinary interpreter operations; none calls a stock fighter handler.

## Article references

### `articles.bind_held(handle) -> bool`

Assign an existing authored article to the fighter's held-item reference.
This does not pick the item up, attach it to a bone, change its animation, or
run pickup callbacks. Use ordinary pickup operations when those effects are
intended. The article must still exist and have been created by this fighter
from this package. Invalid or removed handles return `false`.

### `articles.bind_auxiliary(handle) -> bool`

Assign the same kind of validated reference to the fighter's auxiliary-item
slot. This is useful for a second presentation object. It has the same
validation and absence of pickup side effects as `bind_held`.

```text
let int ornament = articles.create_with_velocity(It_Kind_DrMario_Vitamin,
    position.x, position.y, 0, 0, 0, 0, facing.direction, true, 0, 1, true)
let bool bound = articles.bind_auxiliary(ornament)
```

The example is line-wrapped for explanation; write the call on one line in
the current script editor. See [Articles](article.md) for creation arguments
and [Item API](item.md) for the item's own callbacks.

## Article appearance and initial update

### `item.set_appearance(descriptor, frame)`

Bake a frame from an owned animation descriptor into the article model's
appearance. `descriptor` is an integer index into the article's state/animation
table. Invalid indices do nothing. This operation is separate from choosing
the live article state; scripts can restore that state after baking appearance.

The direct interpreter uses the current source program's owned descriptor
table and primary model. It removes existing animation, resets child transforms
from the source joint graph when present, loads the selected descriptor's joint,
material and shape animation, applies the Item animation rate, requests the
frame, evaluates it, and removes animation again. It retains the selected
descriptor and clears the DATA command pointer. It does not change the Item's
script state, native motion ID or recorded animation frame.

An animation end callback can retire the Item or replace its state or model.
The operation checks the retained source, program, storage, primary model and
descriptor between stages. It stops before further cleanup or output when
those identities change; completed animation effects remain. A missing owned
state table is an error, while an index outside an existing table is a no-op.
The operation has no fighter-kind or stock asset lookup.

### `item.reset_spin()`

Reset accumulated spin and the appropriate model-joint rotation using the
article model's own spin-axis metadata. It preserves the other rotation axes.
`item.reverse_spin` is a readable/writable flag controlling reversed spin.

### `item.refresh_physics()`

`item.refresh_physics()` is a zero-argument Void operation on an owned Article.
It runs the real immediate physics update followed by collision, clearing the
boundary-removal enable bit before either call and setting it to one only after
both finish with the retained Item still healthy. It does not restore the
previous bit value, tick animation, or advance the match.

Each callback return validates the pool epoch/generation, native generation,
Item userdata/class/kind, Article/attribute/model peers and owned program/view/
storage schema. Logical faults, removal and identity changes stop cached native
tails and the caller before publishing output. Completed native effects remain;
the suppressed boundary bit remains zero on interruption. Ordinary Item state
and ownership changes preserve the native continuation order.

## Original creator versus current owner

Reflection can change damage ownership. Presentation attachments can instead
follow their original creator via `creator.position.x/y/z`, `creator.exists`,
and `creator.model.root_scale_y`. The latter reads the actual root model's Y
scale, which can differ from common model scale or instance scale. Check
`creator.exists` before accessing it; an absent or retired receiver is an error.
A live Fighter whose root is legitimately absent yields scale one.

Keep behavior predicates in the script. For the original held-presentation
policy, test `creator.motion == ftCo_MS_DeadUpFallHitCameraIce or
creator.motion == ftCo_MS_Sleep`; this is the exact two-motion native predicate.
Use `attachment_owner` when following a revocable controller rather than the
original creator. See [Item ownership](item.md) for the roles.

Use existing `item.joint(0, "").translation.x/y/z` and `.rotation.x/y/z`
properties to update only the needed model channels. A saved script state
variable can retain spawn facing independently of later reflections or turns.
See [Article models](article_models.md) for transforms and hierarchy paths.

## `on presentation_item:`

A void callback owning held-item presentation after the common death or sleep
entry sets its motion and flags. It runs for `DeadDown`, `DeadRight`,
`DeadUpStarIce`, `DeadUpFallHitCameraIce` and `Sleep`; inspect `motion.id` to
choose a presentation. A declared hook replaces the fighter-kind item branch,
including a deliberate no-op for modes that need no item. It is separate from
`demo_enter`, which controls results and other demo motion presentation.

Use the ordinary article constructor, item binding, typed article state and
signal operations. For example, an article signal can apply an authored scale
using `item.scale_by_attribute` and its creator's model/instance scale. The
common caller validates the retained object, fighter data, generation, pinned
program and script storage after the motion change before invoking the hook;
logical VM faults stop delivery as well. It does not read the fighter after
the hook returns. This callback owns presentation policy, not stockfighter
behavior delegation.

## `animation.hip_root_compensation`

Read or set the executing Fighter's common hip/root compensation flag as Bool.
The ordinary native animation code uses it to compensate the root from the
animated hip position. Mewtwo uses this policy, but the property is available
to any source-owned fighter; it does not select Mewtwo code or require his rig.
The rig still needs the semantic joints used by the shared animation code.

The actor, program, storage and native data must belong to the current source
in its pinned scene. This operation changes the shared policy flag; it does
not evaluate an animation, move the fighter, or create missing joints.
