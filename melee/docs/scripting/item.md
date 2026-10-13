# Item API

## `item.slide_velocity`

Read or assign the Float components `x`, `y` and `z` of the native ground-slide
vector, separately from ordinary `velocity`. Ground-slide routines consume
this vector while following a floor; assigning it does not move the item or
perform collision on its own.

```text
item.slide_velocity.x = item.slide_velocity.x * float(0.9)
```

These properties require the live script-owned Item in the current scene,
preserve Float values without clamping, and allocate nothing. They are only
available in article/item context, not on a Fighter or owner receiver.

`item` is the namespace available inside an [article](article.md) callback.
It refers to the article currently being updated. Its operations call shared
Melee item routines; the script decides which operations to call and when.

See [fighter presentation](presentation.md) for appearance baking, immediate
physics updates, spin reset and presentation article references.

## `item.pause()` and `item.resume()`

These zero-argument statements call the shared item engine's pause and resume
operations. They are available in article and ordinary scripted-item callbacks;
they return no value. Pausing marks the item paused. Resuming clears that mark
and requests processing to restart when the engine's saved restart condition
is set. It preserves an existing restart request and unrelated item flags.
Resume therefore performs more work than assigning a pause flag to false.

```text
on signal pause:
        item.pause()
on signal resume:
        item.resume()
```

Neither operation removes the item, changes its article state or dispatches an
authored callback. Scripts own when to call them, including any hitlag policy.

## Properties

### Ground and air state

`item.grounded` reads Bool and accepts Bool assignments in ordinary source-owned
Article callbacks. True calls `it_802762B0` to set `GA_Ground`; false calls
`it_802762BC` to set `GA_Air` and clear `xDC8_word.flags.x1F`. Neither helper
changes motion or velocity. A live Destroyed callback may assign while its
storage and retained native identity remain current. See [ground-state
assignment](item_ground_state.md) for the lifetime and type contract.

`item.airborne` reads Bool and accepts Bool assignments in source-owned Article
and Item callbacks. Assigning true writes the native `GA_Air` field; false writes
`GA_Ground`. It does not change motion state, perform landing logic or advance
animation. The script chooses those operations separately.

```text
item.airborne = targets.airborne(item.current_owner)
```

Workshop holdable items derive grounded and airborne getters from their motion
state, so assigning either property to them is rejected. This prevents writing a field
that their getter does not use. The source must retain its exact live package,
storage and owned Article attributes.

### Surface response properties

`item.surface_restitution` is the read-only Float coefficient from the item's
shared attributes. `item.stop_on_surface` is a writable Bool selecting the
shared ground response's stop branch instead of its continuing bounce. It is
separate from custom floor-motion policy, and setting it does not itself run a
collision response or stop an item immediately.

```text
item.stop_on_surface = true
let float bounce = item.surface_restitution
```

Surface checks and wall reflection are described in [article terrain](article_terrain.md#item-wall-and-ceiling-contacts).

`item.damage_received`, `item.damage_direction` and `item.scale` read the latest
incoming damage amount, its direction and the item's uniform scale. These are
read-only. `item.harm_owner`, `harm_teammates` and `harm_sibling_items` select
outgoing hit eligibility; `item.can_be_hit_by_owner`, `can_be_hit_by_teammates`
and `can_be_hit_by_sibling_items` select incoming hit eligibility. Each uses the
`item.` prefix and is readable/writable. These policies are independent.
`item.subaction_sounds_muted` suppresses subaction sounds, independently of
contact or damage hitlag. `item.attachment_active` compares the current
damage owner with the live revocable attachment owner. It is false after control
is cleared or its fighter retires; it does not compare immutable creator provenance.
`item.attachment_owner_is(other)` compares a captured object with the live
revocable attachment owner. It does not compare immutable creator provenance;
separately captured references to the same physical object still match.
Source Items keep immutable original creator
separate from their revocable attachment/control relation. Clearing attachment
does not discard original provenance or change current damage ownership.

Use `creator` for original provenance and `attachment_owner` for revocable
control. Both are retained Fighter references with `.exists`, owned state and
typed helper projections. For control-dependent calls, keep the damage-owner
eligibility check separate from attachment presence:

```text
if item.attachment_active and attachment_owner.exists:
    attachment_owner.helpers.restore_parts()
    let float common_scale = attachment_owner.model.scale
```

`model.scale` is the native instance Y scale multiplied by the common model
scaling attribute. `model.instance_scale_y` reads just the instance Y scale;
`model.root_scale_y` reads the live rendered root joint scale. Use the same
paths on `creator` when original provenance is intended. A nullable receiver
requires an explicit `.exists` check before these projections; write a desired
fallback in script. Presence alone does not establish current damage ownership.
After `item.clear_attachment_owner()` the attachment receiver is absent. `item.clear_current_owner()` only clears damage ownership;
`item.detach_owner()` clears damage ownership followed by the attachment link.
Neither operation physically releases the item or changes `item.owner_bound`.
`item.attach_to_owner(raw_part, register_auxiliary=false)` runs the actual pickup
path only while the attachment relation is live and matches current owner.
The optional Bool records the Item in the Fighter's auxiliary slot after pickup,
provided the participants survive. Ordinary out-of-range raw parts are no-ops;
malformed argument widths are errors.

| Property | Description |
| --- | --- |
| `item.damage_behavior` | Read or assign the engine damage-behavior field (`xC98`), separate from incoming damage amount. |
| `item.accumulated_damage` | Read-only accumulated integer damage (`xC9C`), separate from the latest incoming damage amount. |
| `item.base_scale` | Read-only uniform scale from the item's common attributes, before per-instance scale changes. |
| `item.dynamic_ecb` | Read or assign whether model changes update the item collision-box size. |
| `item.life` | Current life timer; can be read and assigned. |
| `item.half_life` | Read-only half-life timer captured by the engine; reflection callbacks decide when to use it. |
| `item.current_owner` | Read-only live fighter handle for the current damage owner, or zero when invalid. |
| `item.reflector` | Read-only U32 Fighter creation handle from the Item's current recorded reflecting object. Null, non-Fighter and physically retired objects return zero. |
| `item.reflection_owner_transfer_disabled` | Read-only reflection policy flag that suppresses replacing the damage owner with the reflector before the reflected callback. |
| `item.drop_speed` | Read-only initial vertical drop speed from common item attributes. |
| `item.animation_rate` | Read or assign the item's animation frame-speed multiplier. |
| [`item.slide_velocity.x/y/z`](#itemslide_velocity) | Read or assign the ground-slide vector, separately from ordinary `velocity`. |
| [`item.owner_bound`](#itemowner_bound) | Read or set the owner-bound item lifecycle flag. |
| `item.motion_state` | Read-only current article state index. |
| `item.kind` | Read-only engine item kind; `It_Kind_*` constants are accepted in article expressions. |
| `item.collision_group` | Read-only integer identifying the engine item-hit group (`xAC4_ignoreItemID`); distinct from the item creation ID and authored state. |
| [`item.surface_normal.x/y/z`](article_links.md#surface-normals) | Read or assign the normal stored by an explicit stage-segment sweep. |
| `item.remove_at_bounds` | Read or set the engine's out-of-bounds removal enable flag. |
| `owner.incapacitated` | Whether a valid attached owner satisfies the common incapacitated-state query. False without one. |
| `owner.action_detaches_items` | Read-only action flag used by stock weapon handlers to end attached-item behavior (`x2071_b6`). Scripts decide when to respond; reading it does not remove an item. |
| `owner.action_category` | Read-only four-bit action metadata (`x2071_b0_3`) of a valid attached fighter; zero without one. This legacy category is separate from the fighter's motion ID and move ID. |
| `item.creation_value` | Saved, read-only finite birth scalar supplied by [explicit creation](article.md#explicit-creation). Zero during `on spawned`, then initialized before `on created`. |
| `item.exit_category` | Read or assign the engine's four-bit item exit category (`xDCC_flag.b4567`); use values 0â€“15. |
| `item.damage_hitlag` | Read or assign whether the item's damage-dealt hitlag amount can initiate its own hitlag (`xDC8.flags.xD`). A separate queued hitlag request is unaffected. |
| `item.contact_hitlag` | Read or assign whether hits, clanks and shield contacts enqueue the item's contact hitlag (`xDC8.flags.xC`). This leaves hitboxes active; it is independent of `item.damage_hitlag`. |
| [`item.throw_speed_damage`](#itemrelease_attachmentx-y-z-preserve_held) | Read or assign whether ordinary throw velocity adds damage to the item's attacks. |
| [`item.state.NAME`](article.md#local-variables) | A declared per-instance bool, int, or float; readable and assignable. |
| `item.params.NAME` | Read-only float, int or bool from this article's scope in the owned [parameter file](article.md#parameters). |
| `item.attr.NAME` | Stock translation draft only: an imported DAT float, converted to `item.params.NAME` by parameter import. |
| `attrs.NAME` | Read-only float, int, or bool from the owning fighter package's [parameter file](language.md#fighter-attributes). This table stays with the item when it is reflected or changes holder. |
| `velocity.x`, `velocity.y`, `velocity.z` | Current item velocity; can be read and assigned. |
| `position.x`, `position.y`, `position.z` | Current world position; readable, and assignable in article callbacks. |
| [`collision.floor_contact`, `collision.ceiling_contact`, `collision.left_wall`, `collision.right_wall`](#stage-contact-and-collision-box) | Read the last stage sweep's contact flags. |
| [`ecb.up/down/front/back`](#stage-contact-and-collision-box) | Read or assign collision-box edge extents. |
| `facing` | Current item facing; readable, and assignable in article callbacks. |
| `item.reflect_facing` | Read-only facing supplied by a reflector contact. |
| [`item.launch_angle`, `item.launch_speed`, `item.launch_origin.x/y/z`](article.md#launch-initialization) | Read-only data captured by the authored projectile launch. |
| [`item.joint("PATH").translation/rotation/scale.AXIS`](#article-model-transforms) | Read or assign an article joint transform component. |
| [`item.joint("PATH").exists`](#article-model-transforms) | Read-only: whether that model joint is present. |
| [`item.joint("PATH").local_visible`](#article-model-transforms) | Read or assign only the selected joint's visibility. |
| [`item.joint("PATH").visible`](#article-model-transforms) | Read that joint's visibility; assign visibility to its subtree, leaving its own following siblings unchanged. |
| [`item.joint(slot, "PATH")`](#itemjointslot-path) | Address an independent article model instance in slot 1..3. |
| `owner.state.NAME` | A declared fighter-state value of a valid article owner. |
| [`item.grounded`, `item.airborne`](#ground-and-air-state) | Ordinary Article ground/air condition; Workshop holdables use their motion-state projection. |

Assign `item.grounded = true` or `false` to run the common ground or air marker
operation; the air marker also clears `xDC8_word.flags.x1F`. Assigning
`item.airborne` changes only the ground/air condition and preserves that flag. Use that direct assignment when the
script must control the condition independently of common landing bookkeeping.

## Dynamic joints and animation descriptors

`item.dynamic_joint(index).translation.x/y/z`, `.rotation.x/y/z` and
`.scale.x/y/z` read or assign a joint from the article's indexed dynamic-bone
table. Indices accept expressions. This table is distinct from root-relative
[`item.joint("PATH")`](#article-model-transforms); rotation is in radians.
Writes mark the transform dirty. Missing/out-of-range joints return zero for
translation/rotation and one for scale; invalid or non-finite writes do nothing.

`item.set_animation_descriptor(index)` selects a DAT animation descriptor and
installs its animation without changing the logical item state or resetting its
command stream. Descriptor indices can extend beyond the logical state count.
Use `item.advance_animation()` when the new animation needs immediate evaluation.

`item.add_animation_descriptor(index)` selects the descriptor and adds its
joint, material and shape animation graphs directly to the primary model.
It does not remove existing graphs, request or advance a frame, change rates,
reset commands or change the logical state. Use the separate model operations
in the required order. Missing models or out-of-range descriptors do nothing.

`item.clear_animation_commands()` clears only the animation-command pointer.
It leaves command variables, loop/timer storage and model animation alone.

## Additional stage operations

`item.hits_stage()` queries the common item's stage-contact condition without
changing state. `item.land_on_stage()` applies the common landing response
using this article's declared `on stage_landed` callback. The declaration is
required at compile time. The script owns the subsequent motion state and
velocity changes; neither operation delegates to fighter-specific item code.

`item.sweep_stage_flags()` performs the common item stage sweep and returns its
response flags. `item.reset_default_ecb()` loads the item-kind collision box
into the item's ordinary ECB storage. `item.set_collision_facing(direction)`
updates the collision-facing direction separately from visual facing.

`item.bounce_to_ground(1)` runs the contact-reset variant of the common landing
response; `item.bounce_to_ground()` retains the variant without that additional
reset. Both require `on stage_landed`. Scripts own the selected landing state.

`item.wind_at_position(kind, strength, radius, angle)` creates a common wind
effect centered on the current item position. The effect's interpretation of
the scalar arguments follows its engine definition.

Other implemented item properties include position, collision mode,
world-interaction mode, and animation command values. The compiler rejects an
unknown field instead of silently creating one.

`item.check_ground()` runs the engine's ground collision query and returns
whether it contacts ground. The query updates item collision data; it is not
a read of the previous contact flags.

`item.show_model()` and `item.hide_model()` take no arguments and return no
value. They run the native primary-model flag traversal over the root and its
descendants, stopping below instance joints; the root's following siblings are
unchanged. They do not change hitboxes, collision, state, lifetime or model
selection. For a single child, use `item.joint(0, "0").local_visible`.

`item.spawn_effect_at_position(effect, x, y, z)` creates an effect at an explicit
world position. Each argument accepts an expression. `owner.quake(kind)` and
`owner.rumble(pattern, reset)` request camera feedback and controller rumble
for a valid attached fighter, and do nothing without one. The script decides
when its action category permits that feedback.

### `creator.input.x`, `creator.input.y`

Read the original creator's current main-stick axes. These float properties
remain associated with the immutable creator after reflection changes damage
ownership. They read zero without a live matching creator. Assignment is rejected;
scripts may use the values to steer an article without changing player input.

### `camera.quake(kind, x, y, z)`

Request the common camera quake at an explicit world position. `kind` is an
integer engine quake selector. This article statement uses the position supplied
by the script, allowing feedback centered on the projectile rather than its
creator. It is distinct from `owner.quake(kind)`.

### `item.rumble(pattern, argument)`

Invoke the common item rumble operation for the item's current damage owner.
Both arguments are integers. Its engine owner/type gates remain intact; an absent
or ineligible damage owner receives no rumble. This differs from `owner.rumble`,
which operates on a validated attachment creator.

### `item.stop_loop_sound()`

Stop this item's looping sound through the engine item-audio lifecycle. This
statement does not stop motion or alter fighter audio.

Article callbacks can also use [`articles.signal`](fighter.md#articlessignal)
to communicate within the same immutable authored Definition and
[article resources](article_resources.md) for bounded model/collision collections.

For an article age or contact flag, declare and update a named local variable.
`item.age` and `item.ground_contact` belong to the separate holdable-item API;
reading that union storage from an article is rejected.

## `item.set_model_animation_rate(rate)`

Pass an F32 rate to the native primary-model joint-tree routine. It updates
descendant joint AObjs, skips null AObjs and stops below instance joints.
The native routine deliberately leaves the primary root's own AObj unchanged.
Material, texture and shape-animation AObjs are unchanged. Negative, signed-zero
and nonfinite rates retain the native direct-store behavior. This statement
returns no value and does not write `item.animation_rate`, the separate Item
frame-speed field, or resize the model.

`item.align_facing_to_velocity()` updates facing through the shared item routine;
scripts can then set a model rotation.

### `item.overlaps_owner(hitbox)`

Test an item attack capsule against all hurtbox capsules on its valid attached
fighter using the engine's geometry, scales and depth handling. The index is
a literal from 0 through 3. A missing/detached owner returns false. The query
does not filter hurtbox status or attack enable state: decide whether the
reaction is permitted in script. See [owner functions](functions.md#calling-the-owner-from-an-article).

`item.clear_attachment_owner()` clears the attachment relationship and saved
creator ID, preserving current damage ownership. `item.clear_current_owner()`
clears only current damage ownership, retaining the original creator context.
Neither operation updates the fighter's declared state or cleanup callbacks.
`item.detach_owner()` clears both relationships.

### Ground following and transitions

| Operation | Required callback | Behavior |
| --- | --- | --- |
| `item.follow_ground()` | `on stage_left` | Follow the floor and notify the script when ground is lost. |
| `item.bounce_to_ground()` | `on stage_landed` | Run common air collision/bouncing and notify when the routine settles on the floor. |
| `item.slide_on_ground()` | Both | Run common ground sliding and choose the landing or ground-loss callback. |

These routines update collision and movement state. Their callbacks run
synchronously and return no value. The script chooses the resulting article
state and animation flags; the compiler rejects missing callbacks. They use
shared item collision routines, without invoking a stock fighter's article
handler. See [article callbacks](article.md#stage-response-callback).

## Article model transforms

Read or assign `item.joint("PATH").translation.x`, `.rotation.y`, or
`.scale.z`, using any X/Y/Z component. Translation is local to the joint's
parent. Rotation is in radians; scale is a per-axis multiplier. Writes use
Melee's normal joint setters so matrices are invalidated correctly.

```text
item.joint("").scale.z = item.state.beam_length
item.joint("0/0/0/0").translation.y = -0.425
item.state.previous_rotation = item.joint("0/1").rotation.x
```

An empty path selects the model root. Each number selects a child ordinal
of the current joint: `"0"` is its first child, `"0/1"` is that child's second
child. Paths use the article's own model hierarchy and do not assume stock
fighter bone indices. The compiler accepts up to twelve steps, child indices
0â€“255, and forty bytes including the terminator.

For a missing joint, reads return zero for translation/rotation and one for
scale; writes do nothing. Non-finite writes are ignored. Rotation components
operate on Euler joints: quaternion joints return zero and ignore those
rotation writes. Their translation and scale are still accessible. Raw
`.quaternion.x/y/z/w` channels read and write HSD's stored rotation components
without normalization; see [article model channels](article_models.md#joint-addressing).

These operations change the live article, not its saved model or bind pose.
Use them for procedural recoil, stretching, and other model behavior from
callbacks. Use the Model workspace to alter the saved asset.

Use `.exists` to guard behavior that should only advance when a model branch
is present. `.visible` reads whether the selected joint's hidden flag is clear.
Assigning it runs HSD's recursive flag operation over the selected joint, its
children, stopping at instance joints. The selected joint's own following
siblings are unchanged. `.local_visible` changes only the selected joint's
hidden flag through HSD's single-joint helpers. Both getters read local flags,
not effective visibility through hidden ancestors. Use subtree visibility for
an entire model branch:

```text
if item.joint("0/1").exists: item.state.recoil_frame = item.state.recoil_frame + 1
item.joint("0").visible = false
```

Missing joints report false for both properties; visibility writes do nothing.
Existence cannot be assigned. The [gun model example](../../examples/fox_falco_blaster.fms)
combines these properties with [constant tables](article.md#constant-tables).

## Method summary

| Method | Return | Use |
| --- | --- | --- |
| [`item.gravity()`](#itemgravity) | â€” | Apply ordinary item gravity. |
| [`item.stage_collision(stay_airborne)`](#itemstage_collisionstay_airborne) | â€” | Update stage contact, optionally allowing landing. |
| [`item.bounce_stage()`](#itembounce_stage) | `bool` | Apply stage bounce and report whether it bounced. |
| [`item.bounce()`](#itembounce) | â€” | Apply common item victim-bounce velocity factors. |
| [`item.respond_stage_mask(mask)`](#itemrespond_stage_maskmask) | â€” | Apply the native masked stage-response sequence. |
| [`item.attachment_owner_is(other)`](#itemattachment_owner_isother) | `bool` | Compare a captured object with the live attachment owner. |
| [`item.shield_bounce()`](#itemshield_bounce) | `bool` | Use ordinary item bounce-off-shield behavior. |
| [`item.spawn_effect_on_bone(effect, bone, mode)`](#itemspawn_effect_on_bone) | â€” | Spawn an effect at an article bone. |
| [`item.spawn_effect_on_joint(effect, path)`](#itemspawn_effect_on_jointeffect-path) | â€” | Attach an effect immediately to a model joint. |
| [`item.attach_to_owner(bone_index)`](#itemattach_to_ownerbone_index) | â€” | Attach the live article to its recorded owner's skeleton joint. |
| [`item.release_attachment(x, y, z, preserve_held)`](#itemrelease_attachmentx-y-z-preserve_held) | â€” | Release attachment through the normal item engine, retaining damage ownership. |
| [`item.reflect()`](#itemreflect) | `bool` | Apply ordinary item reflection. |
| [`item.change_state(index)`](#itemchange_state) | â€” | Enter a declared article state. |
| [`item.set_lifetime(frames)`](#itemset_lifetime) | â€” | Set the live and half-life timers. |
| [`item.interact(other)`](#iteminteract) | â€” | Clear references to the object supplied by an interaction notification. |

## Method descriptions

### `item.gravity()`

Applies ordinary falling physics, including the shared spin update. Call it
from a state's `on physics` callback when that state should fall. Pass the
literal `false` as `item.gravity(false)` to apply fall acceleration and the
terminal-speed clamp without changing spin. This lets a released article
keep an authored rotation speed. Omitting both allows independently moving
or floating articles.

### `item.stage_collision(stay_airborne)`

Runs the shared stage-collision update. Use before `item.bounce_stage()` when
the article should respond to the stage geometry. Luigi's Fireball does both
in `on collision`.

The optional boolean defaults to `true`: update the collision box while
retaining airborne status. Pass `false` to use the shared sweep that allows
the item to land. Both paths update contacts, position and ground-line data;
neither chooses removal or bounce behavior. Read the
[contact flags](#stage-contact-and-collision-box) afterward to make those
decisions. Existing `item.stage_collision()` calls keep airborne status.

### `item.attach_to_owner(bone_index)`

Attach the current article to a valid recorded fighter owner. `bone_index`
is an index in that fighter package's skeleton, rather than a semantic part
name. Expressions are accepted; negative indices and indices outside the
package's joint count do nothing, as does a missing attached owner.

The optional second boolean, `item.attach_to_owner(bone_index, true)`, also
registers the article in its owner's held-special slot. It defaults to false
and is separate from the ordinary held-item slot.

This runs the normal item pickup and attachment sequence, including
`on picked_up`. It does not create another item. Use it after
[explicit article creation](article.md#explicit-creation) when the spawn
position and the later attachment point differ. For creation and attachment
in one operation, use [`articles.spawn_bone_attached()`](fighter.md#articlesspawn_bone_attached).

### `item.release_attachment(x, y, z, preserve_held)`

Release the live article using the shared item release operation and the
given initial world velocity multiplied once by the native Item attribute
`x4_throw_speed_mul`. Both three-argument and four-argument forms are ordinary
Item interpreter operations; XYZ must be finite Floats and the optional fourth
argument must be Bool. This reclassifies it as a released item and performs
normal release bookkeeping. The damage owner remains assigned; this differs
from `item.detach_owner()`, which clears owner identity.

The optional `preserve_held` boolean defaults to `false`. Set it to `true`
for a weapon article released independently of an ordinary held item. The
operation saves and restores the creator fighter's held-item slot around
the normal release call only when the Item's current damage owner equals its
creator before release. A reflected or reassigned Item still releases through
its native current owner, without restoring the creator's slot. It does not
equip the projectile in that slot.
The recorded creator and attachment relationships remain available to callbacks.
The actual release runs the owner's ordinary `item_drop_ext` policy. The source
Item, native owner, optional held-slot Fighter and saved held Item retain exact
physical identity and any interpreted program/storage across that callback.
Retirement or a fault stops cached native continuation and prevents held-slot
restoration or successful result publication. The native release still chooses
`x51C` when its existing transfer flag requests it.

```text
item.release_attachment(0, 0, 0, true)
item.throw_speed_damage = false
velocity.x = item.state.speed * cos(item.state.angle)
velocity.y = item.state.speed * sin(item.state.angle)
```

Normal release enables throw-speed damage. Set `item.throw_speed_damage`
to `false` when the article's attack states already specify its full damage,
as Charge Shot does. The script chooses lifetime, animation state, velocity,
contact behavior and when to forget its saved handle.

### `item.spawn_effect_on_joint(effect, path)`

Spawn an immediate synchronous effect attached to a joint in the primary
article model. `effect` accepts an expression; `path` uses the same
[child-ordinal syntax](#article-model-transforms) as `item.joint()`.
An empty path selects the root; a missing joint produces no effect.
Independent model slots are not accepted by this operation.

```text
item.spawn_effect_on_joint(1151, "0/0")
```

This effect consumes one attachment-joint argument. Select an effect whose
definition expects that layout. The queued bone operation below has a
different timing and argument layout.

### `item.bounce_stage()`

Applies the shared item stage-bounce operation and returns whether a bounce
occurred. Luigi uses the result to check remaining speed and optionally spawn
an effect. This operation does not choose the article's removal threshold;
the script does.

### `item.bounce()`

Applies the common victim-response factors: multiply horizontal velocity by the
common item X factor, and set vertical velocity to its common multiplier plus
the common Y addition. It does not choose a state, remove the item or apply a
shield/stage collision policy. Scripts may use it for whichever contact callback
requires this response, then choose their next state separately.

### `item.respond_stage_mask(mask)`

Runs the shared native stage-response sequence using an integral 32-bit collision
mask. It first applies stage bounce, checks the selected contact flags and, when
the native condition matches, runs contact presentation, masked velocity response
and damage attenuation in their original order. The contact check also retains
the native position and collision-box corrections on its non-response branches.
It returns no result and does not select an authored state.
Use the mask returned by `item.sweep_stage_flags()`;
the script decides which response or retirement flags matter to its behavior.

### `item.attachment_owner_is(other)`

Returns whether a captured object resolves to the live revocable attachment owner.
Use it to recognize interaction notifications:

```text
on interaction:
    if item.attachment_owner_is(other):
        item.clear_attachment_owner()
    item.interact(other)
```

Null objects, missing attachment owners and valid unrelated objects return false.
The original creator and current damage owner may differ from this relation.
Malformed or retired supplied object references are errors; a reused pool slot
does not become the old target. This query reads identity only, so foreign or
stock objects do not need this fighter's program or private state layout.

### `item.shield_bounce()`

Applies the engine's ordinary item bounce-off-shield operation. Return its
boolean result from `on shield_bounced` when that is the desired contact
behavior:

```text
on shield_bounced:
    return item.shield_bounce()
```

Luigi's translated Fireball declares this operation for its shield-bounce
callback. A recorded shield contact matches stock Luigi through its
`on hit_shield` removal path; it does **not** exercise
`item.shield_bounce()`. A bouncing contact needs its own parity recording.

`item.shield_bounce()` can also be called as a statement when the script owns
the callback's return decision and updates model rotation afterward.

### `item.reflect()`

Applies shared velocity reflection, facing flip, reflection multiplier and
lifetime adjustment without choosing the article callback's return value.
The script can then change speed or orientation and return its own decision.
`item.reflection_multiplier` is the current multiplier as a read-only float.
The existing `return item.reflect()` uses the same operation and returns its
boolean result.

### `item.respond_to_stage()`

`item.respond_to_stage(1)` selects the alternate shared contact policy: it
includes floor contacts alongside wall contacts and uses the common contact
response rather than the airborne wall-bounce response. The no-argument form
keeps the airborne policy. Both require `on stage_contact`; that callback owns
state transitions, effects and removal.


Updates stage contacts and applies the shared contact response. It invokes
the article's required [`on stage_contact`](article.md#stage-response-callback)
handler. The script owns the resulting state/effects/removal policy. Unlike a
plain stage sweep, this operation performs the stock stage-response work too.

### `item.begin_expiry()` / `item.expire_tick()`

`begin_expiry()` is a zero-argument Void operation on a live owned Article.
It calls the native common expiry helper: overwrite `item.life` with common
item data `xF8`, set `xDCF.b2` and `xDD1.b0`, then clear `xDC8.x1A`. The half-life,
world-interaction flag, state, model visibility and motion vectors are unchanged.
Missing common data is rejected before any writes or output publication. Native
float values are copied exactly; the operation adds no clamp or conversion.

`expire_tick()` subtracts one from `item.life` and returns whether the resulting
value is at most zero. It does not remove the Item itself. Returning that Bool
from an animation phase lets the normal scheduler perform removal. Calling
`use_manual_lifetime()` only clears world interaction `xDC8.x15`, disabling the
common automatic timer path; it neither resets the timer nor begins expiry.

### `item.stop_motion()` / `item.physics_disabled`

`stop_motion()` clears the shared item's motion vectors. It does not change
state or configure expiry. The writable boolean `physics_disabled` controls
the common item physics gate. It does not suppress an authored state's own
physics callback: omit that callback or branch on the flag when appropriate.

### `item.spawn_effect_on_bone(effect, bone, mode)`

Queues effect `effect` at article bone `bone`. Optional `mode` selects
attachment (`0`, the default) or the bone's position when the queue is processed
(`1`). These correspond to Melee's `EF_SPAWN_ATTACH` and `EF_SPAWN_POS`;
this argument is not an effect scheduler channel. Other spawn modes require
extra effect parameters and are not accepted by this method.

With an indexed bone table, `bone` selects that table's entry. Otherwise `0`
selects the model root and a positive index follows its child chain, matching
the engine's item attachment convention. A missing bone produces no effect.
Mario's Fireball uses `item.spawn_effect_on_bone(1147, 0, 1)` after a stage
bounce; Luigi's uses effect `1288`. The [Mario translation ledger](../mario_script_translation.md)
records the bounce comparison that checks this model-root path.

### `item.reflect()`

Applies the shared reflection operation and returns its boolean result. A
script can write `return item.reflect()` in `on reflected`. The result decides
whether the article is removed by that callback.

### `item.change_state(index)`

Change the live article state and update its animation/script through the
common item engine. `index` may be an expression, evaluated once, within the
[article's declared state count](article.md#states-and-callbacks). Invalid
constant indices fail compilation; invalid computed indices do nothing.
A successful transition clears the accessory callback, including when it
selects the current state.

### `item.change_state_flags(index, flags)`

Enter a declared article state with explicit common item transition flags.
Both arguments accept expressions; a computed state outside the declared
range does nothing. Flag `2` updates animation, and flag `16` preserves
existing hitbox history instead of clearing it. Combine them as `18` when
restarting animation while retaining that history. This runs the engine's
normal item-state transition, including its joint-animation evaluation. It
does not perform an additional `item.advance_animation()` call or physics step.

### `item.play_sound(sound, pan, volume)`

Play a common item sound. All three arguments accept expressions evaluated
once. Sound IDs are limited to 0â€“16777215; pan and volume to 0â€“255. Negative
or out-of-range values do nothing. Keep sound choices in ordinary
[owned parameters](article.md#parameters) when authors should tune them.

### `item.inherit_owner_debug_display()`

Apply the current damage owner's debug display settings using the shared
engine operation. This sets display configuration; it does not select a
state, advance animation or affect damage ownership.

### Links and indexed joints

See [linked articles](article_links.md) for `item.spawn_link()`,
`item.link()`, `item.link_reciprocal()`, `item.signal_link()`, `item.unlink()`,
`item.copy_link_collision()`, `item.bone()` and `item.link_bone()`.

### `item.set_lifetime(frames)`

Sets both the active life and half-life timers to `frames`. `item.life` can
then be read or adjusted in animation callbacks. The article's
[`auto_lifetime`](article.md#projectile-appearance) setting determines whether
the engine also applies its automatic-lifetime behavior.

### `item.interact(other)`

Call this inside `on interaction` to run the common item reference-cleanup
routine for the object supplied as `other`. Matching owner, reflector,
absorber, source-fighter and toucher references are cleared. If the source
fighter matched, its recorded player slot is reset too. This notification is
about object references; it does not create a hit or apply damage.

```text
on interaction:
    item.interact(other)
```

The Mario and Dr. Mario Cape translations use this instead of retaining the
stock article's callback. `other` is only available in this callback; callers
cannot invent a pointer to another game object.

## See also

- [Article declaration](article.md)
- [Script structure and callbacks](language.md)
- [Package-authored holdable items](../native_fighter_items.md)

## Trajectory operations

### `item.sweep_stage(previous_x, previous_y, previous_z)`

Sweep from a saved world position to the item's current position using the
normal engine stage-intersection routine. Returns a bool indicating a hit.
The engine can change `position` to the contact point. This call does not
choose expiry, bounce, or removal; the article script does that itself.

The laser saves its current position, performs the sweep, and on a hit sets
one frame of lifetime and restores that position. See the complete
[translation](../../examples/fox_falco_laser.fms). A different custom article
can instead retain the contact point and choose another state.

### `item.mirror_velocity()`

Reflect velocity about the shield-contact normal supplied by the engine.
This runs the shared vector mirror operation on the item's velocity and
contact normal. The three-argument form `item.mirror_velocity(x, y, z)` uses
an explicit normal instead; its components are expressions evaluated for this
call. Both forms use the same shared vector operation. They do not change
speed variables, model scale, angle, facing, or lifetime; those decisions
remain in the article callback.

## Attachment operations

### `item.spawn_directional_effect(effect, bone, x, y, z, angle)`

Create a directional effect at an offset from the attached **fighter's**
semantic bone. `bone` uses names such as `RightThumb`; the offset is local to
that bone and `angle` is in radians. This method uses the engine effect
signature taking a world position and angle pointer, such as effect 1166.
Use other effect operations for IDs with different argument signatures.

The engine consumes the temporary position and angle during creation. Nothing
is retained in script state. A missing/mismatched owner, missing bone, or
nonfinite geometry produces no effect.

### `item.track_accessory()`

Install this article's [`on accessory`](article.md#accessory-callback) callback.
State changes clear it; tracking again is an explicit script decision.

The direct interpreter requires a matching declared `on accessory` Void hook
in this Item's pinned source program. It registers that hook's code ID in the
saved Item actor slot and clears the native accessory pointer. A missing or
mismatched declaration is an error; it does not select a stock handler.
Declaring the hook alone leaves the slot inactive. The native Item accessory
tick runs an active hook outside hitlag.

### `item.clear_accessory()`

Clear this Item's accessory code-ID slot and native accessory pointer. This is
valid without an accessory declaration. Both operations require the current
owned Item, source program, storage and physical lifetime. Faulted or retired
actors cannot register or clear a callback. A later shared native assignment
supersedes the source slot; an explicit `track_accessory()` selects source code
again. Packed Item actor snapshots preserve the active code ID alongside their
required native object/model peers.

### `item.advance_animation()`

Advance the item animation and command script through the common engine.
Use this when a transition must immediately evaluate the new animation.

### `item.scale_by_attribute(multiplier)`

Multiply the common DAT scale by this expression, store the live item scale,
and apply it to the primary model. The multiplier is Float and is rounded to
F32 before the native multiply. This uses the exact owned common attributes.
It preserves native negative, signed-zero and nonfinite scale behavior and
does not refresh hitbox, grab or collision ranges. `item.set_scale(value)`
instead sets only the primary model's uniform scale, without changing Item scale.

### `item.set_instance_scale(value)`

Assign the F32 value to the live Item scale and primary root scale, then run
native hitbox-scale, grab-range and collision-range refresh in that order.
Active hitbox scales use the engine's existing scale input; range fields
multiply by the new Item scale. Repeated calls can therefore multiply already
scaled ranges again. The operation preserves native nonfinite values.

### `item.reset_spin()`

Clear the engine's calculated spin speed and zero the selected joint's native
rotation axis. The owned model descriptor chooses child depth 0-3; the live
spin-axis field chooses Z, X or Y. This does not zero every rotation component,
change the base spin speed or substitute the primary root when a child is needed.
A missing descriptor/joint or quaternion joint is an error before mutation,
because the native Euler setter requires a present Euler joint.

### `item.detach_owner()`

Clear the item's owner and attachment references. Update named fighter
variables and fighter callbacks before detaching if needed; this operation
does not clear them. Later attached-owner operations have no target.

### `item.destroy_effects()`

Destroy effects attached to the live article. Useful when returning `true`
to let the engine remove the item later in its callback loop.

### `item.remove()`

`item.remove()` is a zero-argument Void operation on an owned Article. It runs
normal native Item removal and terminates the calling VM with `SourceRetired`;
later statements and helper continuations do not execute, and no return value
is published. Returning `true` from a native Bool phase remains a separate way
to request removal after that phase returns.

`on destroyed` runs while its owned storage is writable. Removal then retires
that storage, keeping the program pin until actual userdata cleanup. Calling
`item.remove()` again from `destroyed` terminates that callback too, without
repeating native teardown. If GObj deletion is deferred until its current proc
returns, physical existence does not make the retired source callable.

Removal during `spawned` can make construction return NULL; factories preserve
that actual constructor boundary. Completed native side effects are retained.

## Common motion constants

Fighter and article expressions accept `Motion.catch_dash`,
`Motion.throw_forward`, `Motion.throw_back`, `Motion.throw_up`, and
`Motion.throw_down`. These are common engine motion IDs, not fighter-specific
states. Compare the attached fighter's current state with `owner.motion`.
Reading a constant never invokes stock fighter behavior.

## Math in article callbacks

| Expression | Result |
| --- | --- |
| `atan2(y, x)` | 32-bit angle in radians, using the same native float operation as the stock item callbacks. |
| `wrap_angle(angle)` | Nonnegative radians with whole turns removed, preserving stock float assignment rounding for ordinary angles. |
| `pi`, `tau` | Mathematical constants for a half-turn and full turn. |
| `float(value)` | Explicit conversion to 32-bit float, useful before division or other arithmetic. |
| `int(value)` | Explicit conversion to signed 32-bit integer. |

`sin`, `cos`, `abs`, `sqrt`, `min` and `max` remain available. Article
`atan2` uses float precision; the existing fighter expression uses double
precision. `wrap_angle` reduces very large finite values in a bounded
operation and returns zero for nonfinite input. These defined choices prevent
a stalled subtraction loop without changing the ordinary stock angle path.

## Additional article models

### `item.load_model(slot, name) -> Bool`

Create an independent instance of a declared `model_root` in literal slot 1â€“3.
The name belongs to this exact Article; preparation imports its immutable graph.
Returns false on bounded capacity failure, preserving the occupied slot. A
successful load replaces that slot with a fresh instance. Scene storage is
reserved before gameplay. See [Article models](article_models.md) for declaration
syntax, transforms and ownership. `clone_model` is not a direct-interpreter API.

### `item.destroy_model(slot)`

Release the instance in slot 1..3. Empty slots do nothing. Item removal also
releases every additional instance; scripts need not remember cleanup.

### `item.joint(slot, "path")`

The optional first argument selects model 0 (the main GObj model) or an
additional slot 1..3. Child paths and `translation`, `rotation`, `scale`,
`exists`, and `visible` have the same meaning for every slot. Extra instances
use world-space root transforms, independent of the main model.

### `item.inherit_owner_scale()`

Apply the common engine owner-scale operation to the item, its primary model,
collision and grab ranges and hitboxes. This uses the current damage owner's
live Fighter instance scale Y, including a foreign reflected owner; immutable
creator provenance and revocable attachment ownership are separate relations.
It requires a live Fighter in the same scene and rejects absent, non-Fighter or
stale ownership before mutation. It multiplies the current Item scale, so call
it once during creation unless repeated scaling is intentional. Range refresh
follows the same order as `item.set_instance_scale`. Neither method selects or
creates an additional model.

### `item.use_manual_lifetime()`

Disable the common automatic lifetime/world-interaction flag. The script then
owns changes to `item.life` and its expiry decision. Setting `item.life`
directly preserves the separate engine half-life value; `item.set_lifetime()`
updates the common lifetime fields together.

### `item.disable_surface_response()`

Disable the shared velocity-based stage response. This does not disable
hitboxes or prevent an explicitly scripted stage sweep/collision query.

### `item.cancel_hitlag_request()`

Clear this item's pending damage-derived hitlag request. This does not clear
an already-running fighter hitlag timer. Call it from an article's damage
callback when that article must continue updating through contact.

### `item.owner_bound`

Read/write bool for the engine's owner-bound item lifecycle flag. It is
separate from the creation-owner metadata used by `owner.state.NAME`.
`item.detach_owner()` clears both owner references; it does not change this
flag, so scripts can reproduce the engine's cleanup order explicitly.

## Stage contact and collision box

Call `item.stage_collision()` to update the article's engine collision state.
Then read `collision.floor_contact`, `collision.ceiling_contact`,
`collision.left_wall` and `collision.right_wall`. These report contact masks;
they do not decide how to bounce, slide or expire.

`floor.normal.x/y`, `ceiling.normal.x/y`, `wall.left.normal.x/y` and
`wall.right.normal.x/y` read the corresponding contact normals. Only use a
surface's normal when its contact flag is set. `ecb.up`, `ecb.down`, `ecb.front`
and `ecb.back` read or assign the source collision-box extents before a sweep.
These are article-only Float properties of the native source box. They do not
replace the fighter's read-only world-space `ecb.top/bottom/left/right` points,
and changing them does not run a collision sweep. The interpreter writes the
same `ecb_source` fields used by the shared item collision code.
They do not move the article. Use [vector math](math.md) to implement responses.

```text
ecb.up = 3
ecb.down = 3
ecb.front = 3
ecb.back = 3
item.stage_collision()
if collision.floor_contact: item.state.direction_y = item.state.direction_y + floor.normal.y
```

## Article hitboxes

`hitbox.enabled(index: integer) -> bool` reads whether the indexed
article hitbox is active. `hitbox.radius(index: integer) -> float`
reads its current scale/radius, including a disabled capsule. The index may be
an expression, evaluated once, and must be 0..3.

`hitbox.set_radius(index: integer, radius: float)` changes that radius;
negative or nonfinite values are ignored. It does not create a hitbox or replace
its subaction definition. `hitboxes.disable_all()` disables all article hitboxes.
Scripts own any desired lifetime or per-instance scaling policy.

```text
if hitbox.enabled(0): hitbox.set_radius(0, item.state.base_radius * item.state.size)
if item.state.age > item.params.hitbox_lifetime: hitboxes.disable_all()
```

## item.spawn_root_effect

`item.spawn_root_effect(effect: int)` spawns a synchronous effect attached to
the article's current model root. The effect ID can be an expression, allowing
an owned per-instance variant without a fighter-specific host operation.
This differs from the asynchronous [bone effect](#itemspawn_effect_on_bone).
Use `item.destroy_effects()` for explicit cleanup.

```text
item.spawn_root_effect(item.params.effect_base + item.state.appearance)
```

For a child joint rather than the root, use
[`item.spawn_joint_effect(effect, "path")`](article_models.md#synchronous-joint-effects).
It has synchronous timing and an immutable primary-model path, distinct from
the deferred `item.spawn_effect_on_bone` and `item.spawn_async_joint_effect` calls.

## Model animation and spin

| Property | Access | Meaning |
| --- | --- | --- |
| `item.animation_finished` | Read | No model animation is still running. |
| `item.base_spin_speed` | Read | Initial article spin speed. |
| `item.spin_speed` | Read/write | Current common rotation speed. |
| `item.reverse_spin` | Read/write | Shared reflected/reversed spin flag. |
| `owner.facing` | Read | Attached owner's facing; zero when detached. |
| `owner.instance_scale_y` | Read | Owner's instance Y scale; one when detached. |

The instance scale is distinct from the model-root scale and the fighter's
common model-size calculation. Choose the quantity your behavior actually
uses. [`Attachment ownership`](article.md) controls these owner queries.

`item.sweep_floor()` updates floor-sweep and line data without choosing a
state transition. `item.clear_accessory()` removes the article's accessory
callback; it does not hide the model or destroy effects.

## item.spawn_scaled_root_effect

`item.spawn_scaled_root_effect(effect: int, scale: float)` creates an
asynchronous effect attached to the article root with the supplied scale.
Use `item.destroy_effects()` for cleanup. Both arguments accept expressions.

## item.loop_sound

`item.loop_sound(sound: int, pan: int, volume: int)` replaces the article's
looping sound channel and stops its previous voice. Sound IDs must fit 24 bits;
pan and volume must be 0..255. This differs from `item.play_sound()`, which
uses the separate ordinary item sound channel.

## `item.advance_model_animation()`

Advance the current model's joint, material and shape animations without
advancing the item's `x5CC_currentAnimFrame` or executing item subaction
commands. Use `item.advance_animation()` when both the item frame/commands and
model need their ordinary update. Model-only advancement is useful for animated
article effects whose command stream has separate timing.

## Animation frames, hitbox damage and effects

`item.advance_animation()` evaluates the primary model, stores its current frame,
then executes the native Item DATA commands. Those commands can update hitboxes,
effects and timers. `item.advance_model_animation()` only evaluates the model;
it leaves the stored Item frame and DATA command state unchanged. Both require
the live source-owned primary graph and check lifetime after native stages.

`item.stage_collision()` uses the standard true-mode algorithm.
`item.stage_collision(true)` selects the same path; `false` selects the alternate
native map-collision algorithm. Both update position and successful floor state;
these statements return no value. Contact flags are readable separately.

`item.inherit_owner_debug_display()` copies the native debug-display policy from
the current damage owner. Null/non-Fighter ownership uses the global Item debug
mode. This operation does not change creator or attachment ownership.

`item.rumble(pattern, duration)` uses the current Fighter damage owner's native
eligibility and loaded rumble data. Integer expressions are accepted. Duration
zero selects the native continuous value (-2); invalid eligible patterns produce
a diagnostic. Missing or ineligible owners remain ordinary no-ops.

`camera.quake(kind, x, y, z)` is shared by Fighter and Item callbacks. Kinds 1â€“4
are Loop, Small, Medium and Large. Positions accept Float expressions; native
quake behavior and stage-model lifecycle are retained. Kind zero and undefined
kinds are rejected.

`item.animation_frame` reads the item's stored animation frame.
`item.set_animation_frame(frame)` both stores the frame and requests it on the
model; assigning a counter alone would not update that animation.

`hitbox.set_damage(index, damage)` in an article callback sets outgoing damage
through the shared item hitbox routine, including its damage-owner scaling and
stale-move calculations. The integer index may be an expression and must be
0â€“3. Damage converts to an unsigned 32-bit integer before that operation;
integral values preserve native wrapping, and floating values truncate.
Undefined floating conversions produce a diagnostic. This is separate from
a fighter callback's hitbox helper.

`item.spawn_effect(effect, bone, ox, oy, oz, vx, vy, vz, synchronous, argument)`
uses the shared item effect routine with explicit offset and random variance.
The routine's random branch consumes three random draws even when variance
is zero. Use the synchronous selector and argument that the desired engine
effect requires; this is different from spawning an effect directly on a joint.

`item.spawn_joint_effect_at(effect, "path", ox, oy, oz)` spawns a synchronous
effect on the selected primary-model joint, with an offset and no randomization.
The root path is the empty string.

`item.suppress_destruction_effects` is a readable/writable bool suppressing the
ordinary item-destruction graphics and sound. It does not prevent destruction
or suppress effects explicitly spawned by the script. Model operations are
listed in [Article models](article_models.md).

## Creator context

`creator.exists` reports whether the original creator is a live fighter whose
active authored definition matches the article's immutable package. Validation
uses the saved fighter creation ID, so a recycled object address cannot become
the original creator. Reflection and pickup leave that creator ID unchanged.
`item.detach_owner()` clears both owner references and the saved creator ID.

The validated context supports `creator.state.NAME`, indexed fighter arrays,
and `creator.call(FunctionName, arguments...)`. Function names belong to this
script's authored fighter functions. These accesses remain valid after damage
ownership transfers; they do not dispatch into the current owner's state.
Absent or mismatched creators produce zero/false state reads and harmless
writes/calls.

`creator.position.x/y/z` reads the fighter's current world position;
`creator.motion` reads its motion ID, or -1 without a valid creator.
`creator.action.category` reads its four-bit action category, or zero without
one. `creator.command0..3` read its integer animation-command values, or zero
without a valid creator. `creator.model_scale` reads the common model-scale
product (instance Y scale multiplied by its model-scaling attribute), or one
without a valid creator. `creator.hitlag` reads the creator's common hitlag flag and returns false
without a valid creator. `creator.incapacitated` reads the creator's shared
incapacitation condition, or false without a valid creator.
`creator.root_scale_y` reads the root joint's Y scale, or one without a valid
creator; it differs from the common model-scale product above.
`creator.instance_scale_y` is a read-only Float containing the creator's
per-instance Y scale (`x34_scale.y`), or one without a valid creator. It excludes
the model-scaling attribute and root joint scale. Use `creator.exists` when a
missing creator must take a different branch. This article-only property uses
the same live creator validation and is independent of current damage ownership.
Each read uses the same creator validation. The `owner` context additionally
requires current damage ownership to remain with the original creator.

For a safe current-owner or reflector aim point, use
`targets.read_camera_target_position(handle, x, y, z, valid)` with three declared
article float destinations and an optional bool destination. The query uses
the fighter's configured camera-target bone and offset, matching
`ftLib_800866DC`; it differs from `creator.position`. Invalid handles produce
zero coordinates and false. [Target handles](targets.md) remain zero when invalid.

### `item.release_camera()`

Release this item's retained camera box through the common item engine. Takes
no arguments and returns no value. Use it when an article stops contributing
to camera tracking; this does not remove its model, hitboxes or authored state.
The engine clears the camera-box pointer after release.

## `item.custom_floor_motion`

Read/write boolean for the shared item engine's custom floor-motion flag.
While set, the common floor update suppresses its usual displacement step;
item callbacks can own that movement. This is separate from
`item.physics_disabled` and surface-response settings. It does not disable
all physics or remove hitboxes.

## `item.disable_hitbox(index)`

Disable one of this article's four collision capsules through the common item
operation. The integer index is evaluated once; indices outside 0..3 do
nothing. This also clears the engine's associated contact flag and refreshes
capsule attachments as the common operation requires. Other capsules retain
their enabled state. Use `hitboxes.disable_all()` when all four should stop.

See [Colors](colors.md#article-controls) for article material and layer controls.

## item.spawn_effect_at_position

`item.spawn_effect_at_position(effect: int, x: float, y: float, z: float)`
spawns a native effect at a world position. Use it in an article or held-item
callback; the effect belongs to that item, so `item.destroy_effects()` cleans up
its attached effects. It does not require a fighter creator.

```text
item.spawn_effect_at_position(item.params.effect, item.position.x, item.position.y, item.position.z)
```

Effect IDs must use the engine's exact single-position argument format. The
runtime copies the XYZ argument through the native dispatch; it never retains
a pointer to a script temporary. Coordinates must be finite. Most admitted
effects work without an item model. Effects that read source-root scale require
a valid model, and effects whose later callbacks require a fighter parent are
rejected for items. An ID using joint or scalar arguments is not interchangeable
with a position effect.

The source, owned script state, and model identity are retained across the
native call. If it retires or replaces the source, subsequent script statements
stop rather than writing into a replacement item. Model-root class identity is
also retained for effects that actually read the root.


## Loop sound and camera ownership

`item.stop_loop_sound()` calls the shared `Item_8026B074` sound operation. If the
Item owns a looping SFX voice, the engine stops it; the stored loop handle then
becomes `SFX_NONE`. It does not stop music or other actors' sounds.

`item.release_camera()` calls the shared `Item_80267454` operation for the live
owned Article. If the camera mode is nonzero and a camera subject is attached,
the engine releases that subject and clears the Item's camera reference/mode.
A missing subject preserves the native no-op behavior, including its existing
mode bits. This operation does not detach the creator or change the fighter's
camera settings.

The loop-stop no-voice and camera no-subject cases run against the real linked
native library. Active-subject release also has native-body comparisons with a
controlled camera-release leaf. These checks do not prove audible output or
full camera-manager behavior in gameplay.
