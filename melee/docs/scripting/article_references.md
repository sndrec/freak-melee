# Read-only article references

An article creation ID is an integer handle. `articles.ref(handle,
DeclaredArticle)` reads a live article with that exact declaration in the same
published script bank, from either a fighter callback or an article callback. The expected kind
must have an `article` declaration in the script. All declarations are cataloged
before callback compilation, so the target may appear later in the file.

```text
state int ball_id

# Within a fighter callback:
if articles.ref(state.ball_id, It_Kind_Ness_PKThunder).exists:
    let bool active = articles.ref(state.ball_id, It_Kind_Ness_PKThunder).state.active

article It_Kind_Ness_PKThunder states 1:
    state bool active
    state float trail_x[8]
    state int parent_id
    # Complete article callbacks/states follow.
```

This excerpt shows reference syntax; it is not a complete fighter script.
| Property | Type | Description |
| --- | --- | --- |
| [`exists`](#reference-validity) | bool | Whether the creation ID resolves to the declared article in this package. |
| [`current_owner`](#current-owner) | uint | Current live damage owner's fighter creation ID, or 0 when absent. |
| `state.field` | declared type | Read a declared scalar or indexed array element. |
| `position.x/y/z` | float | Read the article's physics position, independently of model transforms. |
| `surface_normal.x/y/z` | float | Read the stored collision normal of an ordinary authored article. |

`.state.field` returns the declared scalar type;
`.state.field[index]` returns an array element of its declared bool, int or float
type. Array reads require an index, and scalar reads reject one.

```text
# Within another article callback:
let int parent = item.state.parent_id
if articles.ref(parent, It_Kind_Ness_PKThunder).exists:
    let float x = articles.ref(parent, It_Kind_Ness_PKThunder).state.trail_x[2]
```

Handles and indices must be integer expressions. Keep creation IDs in `state int`
fields or integer locals so all 32 bits survive; float storage loses some large
IDs. An explicit integer conversion is accepted by expression typing, but cannot
restore bits already lost in a float. Negative or out-of-range integer indices
return zero at runtime.

## Reference validity

A missing or foreign-bank handle, or a handle for another declaration, captures
an absent reference. Its existence is false and scalar state/current-owner reads
return zero. Invalid array bounds also return the declared type's zero. Unknown
fields, unindexed arrays, and unsupported projections are load errors.

Each `articles.ref` expression captures the selected physical instance before
later selectors such as a helper-produced array index are evaluated. A selected
instance that retires, changes its program/schema, or is reused during that
expression causes a state/model read to stop; it cannot silently read a newly
created replacement. `.exists` reports false for retirement. References use
bounded invocation scratch and do not prevent removal or transfer ownership.
Keep persistent handles as integer state, not invocation-scoped references.

Two instances sharing the exact published author and script bank may reference
each other's articles. An article's current damage owner can change through
reflection without changing its package identity. Matching a fighter kind alone
does not grant access to another package's articles.

## Current owner

`articles.ref(handle, It_Kind_Declared).current_owner` samples the article's
current damage owner on each read. It is available in fighter and article
callbacks. Reflection or another ownership transfer is visible immediately;
this is not the original creator or a saved owner variable.

```text
let uint owner = articles.ref(state.projectile_id, It_Kind_Declared).current_owner
if owner != 0:
    let bool airborne_owner = targets.airborne(owner)
```

Replace `It_Kind_Declared` with an article kind declared in your script. The
query validates the article's program, declaration, storage and full creation ID, then
checks that its current owner belongs to the live fighter registry. Missing or
retiring articles and missing/removed owners return 0. The returned ID preserves
all 32 bits; store it in an integer variable. Later [target queries](targets.md#retained-fighter-references)
resolve that ID again, since this read does not retain the fighter.

References are read-only. Assigning to `.current_owner` or `.state.field` through a reference is a
compile error. Use an article's own `item.state` for its owned writes. These APIs
add no cross-article writes. For authored notifications, use the separately
checked [signal operation](fighter.md#articlessignalhandle-declaredarticle-name-value).

## Physics position and surface normal

```text
# From a fighter or another article callback:
if articles.ref(state.ball_id, Ball).exists:
    let float x = articles.ref(state.ball_id, Ball).position.x
    let float normal_y = articles.ref(state.ball_id, Ball).surface_normal.y
```

`Ball` must be an article declared by this script. These vectors are read-only
snapshots of native item fields; no model evaluation or collision query runs.
Missing references return Float zero. A stale captured reference faults rather
than following a reused address. Stored Float bits are preserved, including
signed zero. Surface normals require ordinary article storage; holdable recipes
use a different native union and are rejected. See [stage sweeps](item_observations.md)
for producing a normal and [links](article_links.md) for selecting a linked ID.

## Indexed bone world position

`articles.read_bone_position(handle, DeclaredArticle, bone, x, y, z) -> bool`

Read one joint from the target article's native indexed bone table and evaluate
its world position through Melee's existing joint-position helper. This is
different from the joint's local translation and the item's physics position.
The integer bone index must lie within the loaded model's bone count, at most
100 entries. The query works from fighter and article callbacks.

```text
let float x = 0
let float y = 0
let float z = 0
if articles.read_bone_position(item.link_id(0), Ball, 6, x, y, z):
    position.x = x
    position.y = y
```

This excerpt belongs in an article callback with a declared `Ball` article.
Use `item.creation_id` as the handle to query the calling article itself.
Outputs must be three distinct Float destinations. Missing/retiring articles,
missing tables/joints and out-of-range indices return false without changing
outputs. Invalid classes, parent cycles or changed actor/table/joint identities
stop publication. The query uses fixed invocation scratch, not heap allocation.
For local rotation, scale and translation of the calling article's indexed
joints, use [`item.dynamic_joint(index)`](article_models.md).

## Primary model translation

`articles.read_primary_translation(handle, It_Kind_Declared, x, y, z) -> bool`

Read the primary model root's **local translation** into three distinct Float
outputs. This is the translation authored on that root, before a parent transform
is applied. It can differ from the item's physics position or the root's world
position. Use it when behavior follows a model-driven object; use ordinary item
position or [model world queries](article_models.md) for those other coordinates.

```text
let float x = 0
let float y = 0
let float z = 0
if articles.read_primary_translation(state.projectile_id, It_Kind_Zelda_DinFire, x, y, z):
    let int enemy = cpu.nearest_enemy_at(x, y, z)
```

This excerpt belongs in a fighter callback with a declared DinFire article.
The query is also available to article callbacks. Outputs may be Float locals,
scalar fighter `state` fields in fighter callbacks, or scalar `item.state`
fields in article callbacks. Handles must be integers and the expected kind
must be declared in this script. Arrays, overlapping outputs and wrong types
are compile errors.

Missing or retiring articles, absent model roots, foreign script banks,
or a different declaration return false without changing any output. Both source
and recipient identity are checked before publication. If the source is retired
or recycled during the query, interpreter lifetime checks stop its continuation
before storing outputs. The query captures one physical instance and makes one
local-vector observation. It does not prevent removal or give permission to write
the model. A replaced root or invalid engine JObj class stops publication.

See [`cpu.nearest_enemy_at`](cpu_policies.md#cpunearest_enemy_atx-y-z) for choosing
an opponent near those coordinates. The query does not change the CPU's current
target.

Article and state declarations must fit the source package's fixed preparation
and actor-storage capacities. The compiler resolves each read to a constant
target declaration/field index; no field-name lookup or runtime heap allocation
occurs during gameplay. The retained receiver pool also has a fixed capacity;
exhausting it is an explicit script error.
