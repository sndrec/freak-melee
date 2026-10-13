# Article models and transforms

## Joint properties

Use `item.joint(0, "path")` or `item.joint("path")` for the primary model.
Models `1` through `3` select independent secondary instances in fixed slots.
An empty path selects that model's root.
Each child ordinal is 0–255; paths contain at most twelve steps. The model
index and path must be literals. Package preparation decodes each path and
pins it to the owning imported Article before publication. A structurally valid
path may be absent: reads return the native missing-joint defaults and writes
do nothing. Descriptor topology is not assumed to be identical to live instance
links or a particle Article's synthetic root.

| Property | Access | Meaning |
| --- | --- | --- |
| `exists` | Bool read | Whether the prepared joint is present in the live graph. |
| `visible` | Bool read/write | Read local visibility; assign to the selected subtree. |
| `local_visible` | Bool read/write | Read or assign only the selected joint's hidden flag. |
| `translation.x/y/z` | Float read/write | Local translation relative to the parent. |
| `rotation.x/y/z` | Float read/write | Local Euler rotation in radians. |
| `quaternion.x/y/z/w` | Float read/write | Raw native four-component rotation storage, without conversion or normalization. |
| `scale.x/y/z` | Float read/write | Local joint scale. |

Missing translation/Euler/XYZ quaternion reads return zero; missing quaternion W
and scale reads return one and
missing visibility reads return false. Visibility getters read local flags,
not effective visibility through hidden parents. `local_visible` leaves child
and sibling flags unchanged; recursive `visible` also changes descendants,
stopping at instance joints, but leaves the selected joint's following siblings
unchanged.

Euler operations retain the native quaternion-joint contract. Transform writes
ignore nonfinite values through the shared model-joint routine.
`item.set_scale(value)` sets uniform primary-root scale using the native HSD
setter; it does not use that finite-value filtering. `item.set_animation_frame`
executes the native rate/request/evaluate/frame-read sequence, checking source
and root lifetime between stages.

The direct Item model operations have separate scale contracts:

| Operation | Resulting behavior |
| --- | --- |
| `item.scale_by_attribute(Float)` | Multiply the exact owned common scale by the F32 argument, write Item scale, and set the primary root uniformly. |
| `item.set_instance_scale(Float)` | Write Item/root scale, then refresh active hitbox scales, grab ranges and collision ranges in native order. |
| `item.inherit_owner_scale()` | Multiply Item scale by the live current Fighter damage owner's instance scale Y, then perform the same native refresh. This is distinct from creator or attachment ownership. |
| `item.reset_spin()` | Clear calculated spin and zero the descriptor-selected depth/axis on its live Euler joint. Missing/invalid/quaternion joints reject before mutation. |

These Article operations return no value. Their uniform native scale setters
preserve signed-zero, negative and nonfinite F32 behavior; individual transform
channels retain their finite-write/no-op rule. Inherited scaling requires a live
Fighter current owner and does not invent a default when that relation is absent.
Repeated range refresh multiplies existing ranges again. Source, primary model,
owned attributes and captured owner identities remain checked before completion.

`item.hide_model()` and `item.show_model()` run the native whole-primary-model
flag traversal. They change the root and descendants, stop below instance joints,
and leave the root's following siblings unchanged. They are separate from
single-joint `local_visible` and do not choose another model.

`item.set_model_animation_rate(Float)` runs the native joint-tree rate routine.
It leaves the root AObj unchanged and sets descendant joint AObj rates, ignoring
null AObjs and stopping below instance joints. Material, texture and shape rates
are unaffected. It preserves the F32 rate, including nonfinite values, without
writing the Item animation-speed field. All three operations return no value and
validate the same live owned primary model before completion.

An empty secondary slot returns the same missing-joint defaults. It is never an
alias to the primary model. Secondary animation bundles remain unavailable.

## `item.spawn_root_effect_with_offset(effect, x, y, z)`

Creates a synchronous effect attached to the article's primary root, with a
copied local XYZ offset. The effect ID must use the native Joint/Vec3 recipe;
currently recipe `1096` supports this family. XYZ are finite Floats. The native
constructor retains the root attachment and copies the offset into its effect
state; it never retains a pointer to script storage. This is distinct from a
world-position effect or a scale/facing scalar. The operation returns no value.
For example, `item.spawn_root_effect_with_offset(1096, 0, 0, 5)` creates the
same local offset effect used by a fire arrow embedding in stage geometry.

## `item.spawn_async_joint_effect(effect, path, mode[, scalar])`

Queues a stock visual effect on a joint of the article's primary model:

```text
item.spawn_async_joint_effect(1257, "0/1", 0)
item.spawn_async_joint_effect(1259, "", 3)
item.spawn_async_joint_effect(1230, "", 1)
item.spawn_async_joint_effect(1231, "", 4, 1.0)
```

`effect` must be an exact signed 32-bit Int expression. `path` is a literal child-ordinal path with
the same limits as other joint operations: up to 12 child steps, each ordinal
0 through 255. An empty path selects the model root. Missing models/joints and
negative effect IDs produce no effect. This statement returns no value.

`mode` is a literal 0, 1, 3 or 4:

| Mode | Effect arguments | Sampling |
| --- | --- | --- |
| 0 | Joint | Borrows the selected joint. |
| 1 | World position | Samples the selected joint when the queue processes the effect. |
| 3 | Joint and facing | Copies the article's facing direction at enqueue time. |
| 4 | World position and scalar | Samples the joint when processed; copies your finite Float scalar at enqueue time. |

Supply the fourth argument only for mode 4. Neither scalar mode retains a
pointer into script storage. Deferred effects can therefore use the joint's
later position with the scalar supplied when you made the call.
The interpreter admits source-verified native argument families: mode 0 needs
one joint; mode 3 permits one joint or a joint plus one Float pointer. Native
one-joint effects ignore the extra scalar, as Ice effect `0x4EA` does. Facing
effects `0x48D`, `0x49A`, `0x4B0`, `0x4EB`, `0x4F1` and `0x510` consume the copied scalar to select their
particle generator rotation. Other payload families are rejected before enqueue.
Mode 1 requires a position recipe; mode 4 requires a position-plus-Float recipe.
Recipes that require a Fighter parent are rejected for both position modes.
The older C exporter does not perform this argument-family admission.
The shared effect queue preserves its ordinary deferred/immediate processing
rules. Additional model slots and arbitrary payload formats are not selected by
this operation. The interpreter supports synchronous primary-root effects with
`item.spawn_root_effect`; synchronous arbitrary-path `item.spawn_effect_on_joint`
currently belongs to the older C exporter and is not an interpreter operation.

Preparation uses the same immutable child path table as `item.joint`; the
interpreter requires its current owned Item program/storage and primary graph.
It checks source and selected-path continuity around native enqueue. The ordinary
queue stores a borrowed joint pointer: those checks do not protect arbitrary
later graph replacement before flush. Item state changes and accessory processing
flush the queue; Item removal clears it. `item.destroy_effects()` destroys existing
effects and generators without cancelling pending queue entries.

Native queue allocation failure silently produces no effect. Its ObjAlloc pool
can grow backing memory when empty, so this operation does not promise
allocation-free ticks. Numeric IDs select built-in dispatcher recipes and require
their imported particle assets; Ice uses the prepared `EfIcData.dat` particle
bank and the primary model's first child. Named `model_effect` declarations
construct model effects and do not replace these particle recipes.

The focused [Item async fixture](../../tests/native/script_item_async_effect/README.md)
executes the actual queue, selected native dispatcher cases and facing helper
with controlled particle resource/allocation leaves. Position-mode tests use a
controlled geometry sampler and typed recipe captures to check sampling time,
not world-matrix calculations or particle construction. It does not prove rendered
particles, imported DAT correctness or arbitrary delayed joint lifetime safety.

## `item.joint_world_position(model, path, x, y, z)`

Reads one joint's world position into three distinct Float local variables or
scalar `item.state` fields. Literal model `0` selects the primary graph; literal
models `1` through `3` select their independently owned secondary graphs.
`""` selects its root, `"0"` its first child,
and `"0/1"` the second child of that child. These paths use child ordinals,
not dynamic-bone-table indices or fighter part names.

```text
let float x = 0
let float y = 0
let float z = 0
item.joint_world_position(0, "0", x, y, z)
effects.wind_at_position(x, y, z, 120, 3, 0.1, 1.0471976)
```

The model and path are literals, prepared with the exact Article owner before
execution. The shared HSD world-position calculation runs once for all three coordinates.
Missing models or joints yield three zeros. Check the joint's existing
`exists` property first if your behavior should skip the operation when absent.
The query does not substitute the item's position or a different graph.
See [radial wind](combat_visuals.md#radial-wind) for the connected effect.

Outputs publish together after the source, program, storage, root and selected
joint identities are checked. Retirement or graph replacement stops the call
without changing any output. Native world-coordinate values are preserved,
including signed zero and nonfinite values; this query does not sanitize them.

Secondary world-position queries also claim the scene's preallocated lazy matrix
storage. The claim is balanced before publishing the three outputs. Scalar
transform reads and writes do not require a matrix claim.

An [article](article.md) can load immutable model roots from its own package
and create independent model instances in three fixed extra slots. Models
use the same HSD joint and material renderer as the game. Scripts own history,
visibility and motion; the model operations contain no fighter-specific logic.

## Declaring a model root

```text
article It_Kind_Link_Arrow states 1:
    model_root trail 36
    on created:
        item.load_model(1, trail)
    state 0 anim -1:
        on animation:
            return false
```

`model_root name offset` declares a relocated joint descriptor at a byte offset
within that article's special-attribute DAT data. The offset addresses the
serialized GameCube data, not a native C pointer layout. At package activation,
the importer checks relocation and descriptor bounds and imports the joint
into the character's existing immutable arena. Gameplay never reparses the DAT.
Up to eight distinct names are supported per entity. Declarations precede the
first state; names cannot collide with other visible source/native symbols.
Offsets are aligned unsigned 32-bit byte offsets within the actual descriptor.
A declared null descriptor is rejected. Each named graph belongs to this exact
source owner, including when two entities use the same underlying Article.

### `item.load_model(slot, name) -> Bool`

Accepts a literal slot 1–3 and a root name declared by this entity. The compiler
resolves the name once; runtime code uses its exact prepared root ordinal.
Constructs a separate replacement before releasing the previous instance.
Returns false when bounded construction capacity is unavailable, preserving
the old slot. Instances use scene storage reserved before gameplay, with no
ordinary heap growth during construction. Unsupported graph/material features
are rejected during scene preparation.

### `item.clone_model(slot) -> Bool`

Constructs an independent model in a vacant secondary slot (literal 1 through
3) from this Article's primary model descriptor. Copies the live primary root's
rotation and uses the Item's uniform scale. Child poses, translation, materials
and animations start from the descriptor; this is not a snapshot of the live
primary model. It returns `false` if the slot is occupied, the live primary is
absent, or construction cannot proceed, leaving the existing slot intact.

Preparation detects this operation in the Article's own bytecode and reserves
bounded model storage at the scene transition. Construction and lazy matrices
use that pool, not an unbounded gameplay heap allocation. The graph is tracked
with the same owner, allocation generation and package lease as named roots;
`item.joint(slot, "")`, rendering and destruction use its actual independent
instance. No borrowed primary pointer or fighter-specific trail operation is used.

The scene permits 128 simultaneous secondary model instances across all
Articles, independently of its item-actor capacity. Named-root loads and primary
clones share this budget. Replacement of an occupied slot is permitted at
capacity; a new graph in a vacant slot fails without changing it. Descriptor
admission still rejects unsupported HSD features rather than approximating them.

```text
if not item.joint(1, "").exists:
    item.clone_model(1)
```

### `item.destroy_model(slot)`

Removes the independent instance in a literal slot 1–3. A vacant slot is a no-op.
Item cleanup also removes its remaining instances. These operations retain the
exact actor, source program and graph allocation generation; they cannot remove
a model borrowed from another slot or Item. Scene storage and package leases
remain resident while rollback history exists.

### `item.set_model_draw_enabled(slot, enabled)`

Sets whether a literal secondary slot 1–3 participates in the normal Item draw
pass. `enabled` is Bool. It does not change joint hidden flags or destroy a model.
The draw flag may be set while the slot is empty. Scripts can separately control
the selected joint/subtree through `visible` or `local_visible`.

Use `render standard` on the Article when its stock renderer expects a private
item-variable layout replaced by your script. See the [Item API](item.md) for
animation, effects and hitbox operations.

### `item.set_joint_transform(model, path, tx, ty, tz, qx, qy, qz, qw, sx, sy, sz, dirty)`

Assigns translation, raw rotation and scale in that order. Model 0–3 and the
bounded child-ordinal path are literals. The ten components accept Float
expressions; `dirty` is Bool. True uses the native explicit matrix-dirty setters;
false uses ordinary setters, retaining their matrix-independence behavior.
Missing models/joints or any nonfinite component cause no transform change.
Rotation values copy native storage without normalization or Euler conversion.

```text
if item.joint(1, "").exists:
    item.set_joint_transform(1, "", 0, 4, 0, 0, 0, 0, 1, 1, 1, 1, true)
```

## Animation bundles

Immutable preparation now imports exact owner-scoped joint/material/shape bundle
triplets using the character or Item blueprint's existing DAT cursor. Names and
descriptors retain the owning package, and null components remain null. This
preparation does not attach runtime controllers. Before publication, the compiler
prepares each literal model/path/bundle application, counts its native controller
and track demand, and records permitted controller origins. Playback checks the
whole live graph, including controllers retained by earlier sparse additions.
Palette, shape, referenced-object and callback channel families are not admitted
by these operations yet; see [article resources](article_resources.md).

Declare a named `model_animation` before the first article state or callback.
Each article can name eight animation bundles. These refer to immutable typed
DAT graphs imported into the same owned arena as model roots; declaring them
does not attach or advance any live animation.

`model_animation NAME OFFSET` selects three consecutive relocated pointer words:
joint, material and shape animation. The aligned unsigned byte offset must leave
all twelve bytes inside the actual special-attribute descriptor.

Use explicit components when the pointers are separate or some are absent:

```text
model_animation spin material 88
model_animation scattered shape 24 joint 8 material None
model_animation indexed joint 116[0] material 120[2] shape 124[1]
```

Components may appear in any order, at most once each. Omitted components and
`None` both mean no graph for that channel. Without brackets the offset selects
a direct graph pointer; `[index]` selects a pointer from the array referenced at
that offset. Offsets and indices accept decimal or hexadecimal integers. All
pointers, array entries, relocations, extents and graph types are checked against
the owned DAT before activation. Malformed declarations prevent publication;
they are never reinterpreted as an adjacent triplet. Names, pointer offsets and
array indices are part of the exact source/asset identity.

```text
article It_Kind_Link_Boomerang states 4:
    model_root trail 68
    model_animation trail_motion 76
    on created:
        item.load_model(1, trail)
        item.add_model_animation(1, trail_motion)
        item.request_model_animation(1, 0)
    state 0 anim -1:
        on animation:
            item.advance_model_animation(1)
            return false
```

| Operation | Behavior |
| --- | --- |
| `item.add_model_animation(model, NAME)` | Returns Bool after adding an admitted named bundle with `HSD_JObjAddAnimAll`. Checks fixed AObj/FObj pool capacity before changing the graph. |
| `item.request_model_animation(model, frame)` | Requests an animation frame throughout the model. The frame must be finite. |
| `item.advance_model_animation(model)` | Advances only the selected model's animation. The zero-argument form advances the primary model. |
| `item.remove_model_animation(model)` | Removes the selected model's animation graphs. |
| `item.set_model_animation_rate(model, "path", rate)` | Sets descendant joint AObj rates with `lb_8000BA0C`, excluding the selected root; material and texture rates are unaffected. The rate must be finite. |

These operations accept model 0 for the primary root and 1 through 3 for extra
instances. They preserve Add/Request/Advance/Remove call order chosen by the
script. Add never automatically requests or advances a frame. The existing one-argument
`item.set_model_animation_rate(rate)` continues to operate on the primary
model through its existing shared animation-rate routine. The two-argument
`item.set_model_animation_rate(model, rate)` selects the named model's root
and has the same root-exclusion behavior as the three-argument form.

Add, Request, Advance and Remove also accept a literal subtree path after the
model index. Omitting that path selects the root, as does `""`; `"0"` selects
its first child. The path has the same child-ordinal bounds as joint addressing.
Missing models/joints cause no animation change. Add also accepts expression
form and returns Bool: false for a missing joint, vacant secondary slot, or
insufficient pool capacity; true after installing the bundle (including a
declared empty bundle). The bundle must be an exact same-owner declaration;
unknown names are compile errors, not runtime misses. Capacity admission
requires the full new demand conservatively, without credit for old controllers
that the operation will replace. No new allocation is made by request or rate.
The named animation bundle is aligned with the selected joint before the shared
HSD operation runs. Scripts choose the subtree and the operation order:

```text
if not item.add_model_animation(0, "0", Primary):
    return
item.request_model_animation(0, "0", 0)
item.advance_model_animation(0, "0")
item.remove_model_animation(0, "0")
```

Operations retain the exact Item, scene, package and graph generations and
revalidate them before publishing their result. Unknown controller provenance,
unsupported graph classes, exhausted allocation generations, end-callback
registrations and callback reentry are rejected before mutation. Descriptor arenas
and compressed streams remain pinned until scene history drains. Request only
changes the requested frame and rejects non-finite frames.

## Native Article animation descriptors

`item.set_animation_descriptor(index)` and `item.add_animation_descriptor(index)`
accept one exact signed 32-bit Int and return Void. The index selects the owned
Article's imported native `ItemStateArray`, independently of the authored
state number. Missing tables and out-of-range indices (including negative
indices) make no change. These operations do not select an authored state or
alter its callbacks, command stream, command variables or Item animation-frame
field.

Set assigns `xD0_itemStateDesc` and calls the native `Item_80268D34` kernel. It
removes animation from the primary subtree, restores descendant transforms
from the Item's pinned shadow descriptor when present, installs the selected
triplet with native dynamic-bone subtree skips, applies the Item animation rate
and requests frame zero. Its installation visits the primary root's next
siblings too. It does not advance animation; scripts can call
`item.advance_model_animation()` separately.

Add assigns the same descriptor pointer and calls `HSD_JObjAddAnimAll` on the
primary subtree. It preserves sparse existing controllers according to native
Add semantics and does not restore transforms, apply the Item rate, request a
frame, advance animation or clear commands.

Preparation pins each state triplet and prepares separate Set/Add controller
demand and complete controller origins. Runtime calls retain the actual Item,
program, storage, package and graph. Malformed or unsupported selected channels,
foreign controller provenance, native animation callback context, mismatched
shadow descriptors, oversized dynamic-bone extents and insufficient fixed
AObj/FObj capacity produce a diagnostic before mutation. Capacity admission
conservatively requires the full prepared traversal demand without credit for
controllers removed by Set or subtrees skipped by dynamic bones. The native
controller attachments allocate from the existing fixed pools.

Native hand attachments may add unanimated translation, rotation or scale
references to the primary model. Live admission accepts these references only
when they target a bone of the current attachment owner and retain the native
joint-reference flags. Animated references, expressions, duplicate channels
and foreign targets remain unsupported. Descriptor provenance is unchanged.

Texture animation matching uses the effective texture IDs assigned by native
`HSD_MObjCompileTev`/`HSD_TObjAssignResources`, rather than the IDs originally
stored in texture descriptors. Preparation, origin matching and live admission
share the same bounded plan, including the last bump/toon texture ordering and
native slot limits. Active material contexts that import external shadow or toon
textures selected by `RENDER_SHADOW`/`RENDER_TOON` remain explicitly unsupported; local
toon-coordinate and bump textures are planned. This correction also applies to
the existing named model-animation operations.

The focused [descriptor fixture](../../tests/native/script_item_descriptor_animation/README.md)
compares interpreter calls with the unchanged native kernels on owned imported
assets, and tests texture planning against the actual native resource-ID kernel.

Texture image-switch tracks (`TIMG`) can select images from the imported
animation's image table. The current implementation admits complete constant
key streams whose values remain within that table after native integer
truncation. It retains the exact borrowed table and all permitted images across
animation removal and replacement. Palette-switch tracks (`TCLT`) use the same
bounded constant-stream rule, selecting entries 0 through 254 from an imported
table of at most 255 palettes. The native engine owns the live palette copies;
admission checks their data, format, entry count, terminal null and selected
index against the prepared descriptor table. Renderer-assigned palette names
are allowed to change. Removing an animation retains its palette table, as in
the original HSD implementation.

Interpolated image/palette-index curves and authored palette value 255 remain
unsupported. The latter is HSD's default-palette sentinel, rather than entry 255
of the animated table. A newly installed controller may start at that native
default; this does not admit a script-authored sentinel curve. Unsupported
streams produce an explicit diagnostic. See the
[real-graph constructor checkpoint](../native_source_samus_resource_constructor_checkpoint.md)
for native-controller comparisons and malformed-table rejection evidence.

## Synchronous joint effects

`item.spawn_joint_effect(effect, "path")` creates a built-in effect immediately
on a joint of the Item's primary model. `effect` is an Int; the native recipe
must consume exactly one Joint argument. The child path must be a literal string,
using the same bounded child/sibling indexing as `item.joint()`:

```text
item.spawn_joint_effect(1151, "0/0")
```

Here `"0/0"` selects the first child's first child. An empty string selects the
root; `item.spawn_root_effect(effect)` is the existing direct root operation.
Preparation records the exact source-owned path once. Dispatch checks that
prepared selector, the live owned Item, its program/storage and the engine joint
class before and after native construction. An absent joint is a no-op; a foreign
or changed selector/graph is an error. No runtime path parsing or allocation is
added by this operation.

This calls the synchronous effect path. It does not enqueue
[`item.spawn_async_joint_effect`](#itemspawn_async_joint_effecteffect-path-mode-scalar), which has separate
argument recipes and deferred timing. Position, scalar and other incompatible
native effect signatures are rejected before construction. Effect IDs are
validated in the native 16-bit range; negative IDs are rejected.

## Dynamic joints

`item.dynamic_joint(index)` selects an entry in the Item's native dynamic-bone
table. Use a literal Int from 0 through 99. This is a table index, separate from
the child paths used by `item.joint()`; it has no child-path fallback.

The selected joint exposes the same translation, rotation, quaternion, scale,
visibility and existence properties as `item.joint()`. A missing table or entry
uses the usual missing-joint defaults and ignores writes. A foreign or changed
graph produces a diagnostic.

## Matrix rotation reads

`item.read_joint_matrix_rotation(model, "path", x, y, z)` writes the Euler XYZ
rotation decomposed from the joint's existing world matrix into three distinct
writable Float destinations. Use a literal model index (0 through 3) and child
path. A healthy missing joint or empty registered model slot writes zero.

`item.read_attachment_matrix_rotation(x, y, z)` reads the original creator's
recorded raw attachment part. The creator must still be the attachment owner;
detached or reassigned Items write zero. The physical creator can be a stock
fighter or a separate custom fighter. Current damage ownership does not select
the rig.

Both calls return no value and preserve the cached matrix without evaluating
dirty transforms, animation or constraints. They do not replace local Euler or
quaternion reads. Native decomposition preserves its handling of degenerate
and nonfinite matrices. No quaternion fourth component is produced; if you copy
XYZ into raw quaternion storage, choose W explicitly.
