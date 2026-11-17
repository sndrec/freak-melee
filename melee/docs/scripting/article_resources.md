# Article-owned model and collision resources

Indexed resources are owned by a source Item or Article. Declare a bounded
collection, construct its collision nodes, and optionally load a declared model
on each node. These are actual engine objects; a resource selector never becomes
an Item pointer. For the separate secondary-model slots, see
[Article models](article_models.md).

The interpreter prepares joint paths and model ordinals before execution.
Matrix operations require a twelve-float state array owned by the same entity.
The compiler passes its prepared declaration identity; it does not expose a raw
pointer or treat the array as a scalar. Index expressions are evaluated once.

Storage is reserved at scene transitions. Payloads, graphs, collection identities
and allocator generations are tracked by the current rollback page owner.
Package assets are immutable and retained until the scene has released every
graph. This is still part of the page-based rollback implementation; compact
whole-match save states remain separate work.

New attachment reads require the collection's original fighter attachment to
remain current. Teardown retains the physical fighter identity independently so
it can remove borrowed part, capture and hitbox references after logical detach.
Removing or replacing a model never frees an object through a stale generation.
Callbacks that release a collection defer payload destruction until the outer
resource operation returns.

Resource joint animations are available in the interpreter. The compiler
prepares each literal path and same-owner bundle; scene initialization derives
the compatible model/subtree/bundle applications from those source calls.
No path parsing, string lookup or new recipe allocation occurs during playback.
See [animated resource joints](#animated-resource-joints).

This API does not register Hookshot or Grapple, run a native tether solver, or
close complete Link/Samus lifecycle behavior. Whole fighter source checks and
gameplay acceptance remain separate work.

## Historical generated-code acceptance, baseline23

The complete native build and 31 selected compiler, resource, identity and
lifetime tests pass. Resource source checks cover 477,371 assertions, 270
allocation-failure sites, 540 live nodes and six teardown cases. Geometry and
animation checks compare 56 original helper cases across 2,415 assertions.
The isolated emitter has 168 checks; the full compiler also compiles emitted
resource C, so the isolated fixture is not the only compiler evidence.

Same-author article signaling passes 10,444 assertions across 651 cases,
including protected-page removal/reuse, exact caller/recipient logic identity,
full-width handles, reflection-independent authorship and the real dispatch
depth limit. This signal fixture uses the actual resolver and lifetime code;
package lookup, live-list setup and authored recipients are controlled boundaries.

Evidence is in `.local/article-resource-integration23/receipt.json`, with the
immutable runtime/compiler/SDK snapshot in `.local/baseline23`. Baseline22
source receipts retain their input hashes; baseline23 includes the final signal
caller check and the separately documented native item collision-mask fix.
Rook's installed package was replaced once for this SDK without changing its
authored source. These checks do not accept rendered tether gameplay, active
resource heap rollback, or complete Link/Samus behavior.

## Script declaration

Declare the maximum collection size before an article's states and callbacks:

```text
article It_Kind_Link_HShot states 1:
    resources capacity 90 reserve_part 139
    model_root Segment 84
    model_animation Extend 96
    state int segment
    state bool allocated
    state float matrix[12]
    state float x
    state float y
    state float z
    on created:
        item.state.allocated = item.resources.begin(2)
    state 0 anim -1:
        on physics:
            pass
```

The kind/model offsets in this example illustrate an imported article; they do
not select a built-in solver. `capacity` must be 2–90. Omit `reserve_part` when
the collection does not publish a model into an extra fighter part. Its immutable
value is then -1. A reservation uses a fighter [bone ID](bone_queries.md#bone-ids)
assigned to an available attachment slot outside the body rig. Module loading validates the declaration;
runtime registration validates it again after all rig omissions are accounted
for. `begin(count)` resolves the exact matching authored article declaration
itself. Neither capacity nor the reservation can be overridden by a script call.

All resource expressions require a declaration. A second declaration, unknown
model/bundle name, fractional node/line/part ID, unknown property, or assignment
to a collision result is a compile error.

## Script reference

| Member | Type / result | Meaning |
| --- | --- | --- |
| `item.resources.begin(count)` | bool | Reserve a collection for this live article; does not construct nodes. |
| `item.resources.count` | int, read only | Requested collection count, or zero without a live collection. |
| `item.resources.release()` | void | Detach recorded borrowers and release the collection. |
| `item.resource(index).exists` | bool, read only | Whether this node has been constructed. |
| `.create(part, radius, category)` | bool | Construct the next node at the world origin with a real attachment joint and ECB. |
| `.create(part, radius, category, x, y, z)` | bool | Construct the next node at the supplied world position; initialize all ECB position history there. |
| `.load_model(Name)` | bool | Load a declared model root into the node. |
| `.set_draw(enabled)` | bool | Set the resource draw gate independently of authored solver state. |
| `.bind_part(part)` | bool | Bind only the exact declared reservation. |
| `.bind_capture_anchor()` | bool | Publish the model for a live common capture state. |
| `.position.x/y/z`, `.velocity.x/y/z` | float, read/write | Actual node position and velocity. |
| `.collision.position.x/y/z` | float, read/write | Current ECB position. |
| `.collision.previous.x/y/z` | float, read/write | ECB `last_pos`, rather than the separate `prev_pos`. |
| `.surface_line` | int, read/write | Authored persistent line ID. |
| `.collision.environment` | int, read only | Full-width engine environment flag bits. |
| `.collision.floor_line`, `.left_wall_line`, `.right_wall_line`, `.ceiling_line` | int, read only | Cached collision line IDs. |
| `.collision.floor_normal.x/y/z` | float, read only | Floor normal; corresponding wall/ceiling normal properties are also available. |
| `.sweep_stage()` | bool | Sweep the node's ECB through stage geometry; returns the floor-contact result. |
| `.sweep_point()` | bool | Sweep the node's point segment through stage geometry; returns contact with any admitted surface. |
| `.follow_floor()` | bool | Update the node's position along its current floor. |

Point sweeps use collision previous/current positions without reloading an ECB.
They preserve the engine's final step result, resolved positions, surface flags
and normals. ECB sweeps reload the node's collision shape and use a different
floor-contact policy. Both operations can run stage contact callbacks; generated
scripts stop if those callbacks invalidate the article. Scripts still decide
how to respond to contact and whether to move the resource model.

The optional constructor position seeds the node position and collision current
position **before** the engine initializes collision history. This matters for
rope, tether, and other resources whose first sweep must start at their spawn
location. Writing `.position` afterward moves an existing node and does not
reinitialize its collision history. All three coordinates must be numeric;
non-finite positions fail construction before allocation. Radius must be finite
and positive; category is an integer in 0–15.

## Animated resource joints

`item.resource(index).joint("path")` exposes translation, Euler rotation, scale,
quaternion, existence and visibility using the [shared model-joint contract](#resource-geometry-and-model-controls).
`exists` is read-only. Empty path selects the resource model root; child ordinals
use paths such as `"0/1"`. Resource joint calls `add_animation(Name)`,
`request_animation(frame)`, `advance_animation()`, `remove_animation()` and
`set_animation_rate(rate)` use the same actual HSD operations described below.
Animation bundles are declared by `model_animation`, separate from model roots.

| Method | Result | Behavior |
| --- | --- | --- |
| `.add_animation(Name)` | bool | Attach the declared joint/material/shape bundle to this subtree. `Name` must be a same-owner bundle identifier. |
| `.request_animation(frame)` | bool | Seek the subtree's existing animation controllers to a finite frame. |
| `.advance_animation()` | bool | Advance and apply the subtree's current native animation controllers. |
| `.remove_animation()` | bool | Remove the subtree's animation controllers and tracks. |
| `.set_animation_rate(rate)` | bool | Set the subtree's native controller rates to a finite value. |

Each call retains the selected node, graph generation, exact package and
controller provenance through completion. A missing node/path, incompatible
loaded root, exhausted controller/track pool or rejected animation operation
returns false without admitting a different graph. Invalid source recipes or
retired ownership fail the interpreter call rather than publishing an output.

One resource may load different declared roots. Scene preparation admits each
compatible root for an authored Add path/bundle; incompatible roots remain
unavailable to that Add call. If no declared root supports it, scene preparation
fails. Identical Add tuples share one prepared entry. All five operations use
the original HSD animation kernels. They do not call stock fighter behavior.

```text
    model_animation Travel 80
    model_animation Spin material 88
    model_animation Mixed joint 16 shape 40 material None
    model_animation Chain joint 116[0] material 120[0] shape None
```

The bundle form `Travel 80` reads three consecutive serialized pointer words:
joint animation at 80, material animation at 84, shape animation at 88.
The component form specifies each pointer word independently, in any order.
Omitted components and `None` are null and cause no DAT read. `Spin` therefore
reads exactly four bytes at 88, which may be the final word of the article's
attribute block. It does not read a twelve-byte bundle starting there.

An indexed component such as `joint 116[0]` reads the pointer word at116 as a
pointer to an immutable array, then imports the joint animation referenced by
element0. Each component can independently select a direct graph or a constant
array index. Indexing is confined to that array's serialized extent; it cannot
read the following object. The index must be a nonnegative integer literal,
not a runtime expression. `None` omits the component entirely.

Each supplied offset is an aligned nonnegative byte offset into the article's
source attributes. Import validates each word's bounds and relocation, then
imports the graph using its actual joint/material/shape type into the owned
character arena. Null source words and null array elements are allowed;
external or malformed pointers at either level are rejected. A known absent
component should use `None`, rather than interpreting an external relocation
chain as animation data. Duplicate component names or declaration names are rejected.
No table is published until all components import successfully. Runtime code
uses the imported immutable graphs and never reads these DAT offsets.

Matrix statements `read_attachment_matrix(item.state.matrix)`,
`read_world_matrix(item.state.matrix)` and `set_world_matrix(item.state.matrix)`
require a float array of exactly 12 entries. Reads/writes copy a 3×4 matrix;
they never expose pointers into module state. Setting a world matrix installs
the native complete matrix-ownership override on the model root; animation can
continue on that root and its children. This does not change the graph's
topology or controller provenance. `read_attachment_point(local_x,
local_y, local_z, item.state.x, item.state.y, item.state.z)` and
`read_line_speed(line, item.state.x, item.state.y, item.state.z)` require three
distinct scalar float output fields. Failed read outputs are zeroed. Scripts own
all solver decisions, state transitions, contact masks and application of line
speed; these queries do not implement such policy implicitly.

Each collection belongs to a live Item creation number and its exact immutable
fighter Definition. It also retains the original creator's creation number and
Definition. Current pickup/reflection ownership does not change the creator.
Lookups validate live registry membership and full generation before reading
retained objects. Creator-dependent operations additionally require that the
live creator still uses the collection's original Definition.

## Bounds and stock census

The engine has six global collections, each with up to 90 nodes. These are
editable engine limits, not fighter selectors or one collection per fighter.
Several collections may share a creator until global capacity is exhausted.
Each live Item owns at most one collection. Separate Items can own distinct
collections; the pool is not indexed by fighter slot.
There is no dynamic collection allocation, eviction, guessed count 128, or silent
count clamp. Begin rejects declared capacity outside 2..90, count outside 2..capacity,
and invalid part reservations. Node construction rejects invalid/nonfinite ECB
radius and attachment indices outside the actual runtime rig table.

Retail Link has ground 15/air 22 nodes, Young Link ground 10/air 15, and Samus
ground 30/air 45, doubled to 60/90 for its modified mode. Source count arithmetic is
in `itlinkhookshot.c:169` and `it_802B75FC` in `itsamusgrapple.c:317`. For finite positive scales
at or below one, the segment length cancels mathematically; it does not imply an
inverse-scale increase in node count. Rounded-f32 sampling includes subnormal
positive scales and scales below/above one. Maximum sampled counts remain 22,
15 and 90 respectively. Extreme overflow/nonfinite derived values fail explicitly;
custom attributes can exceed the ceiling and must fail rather than clamp.

`tests/native/article_resource_census.py` extracts attributes from the supplied
retail ISO without copying assets. Its JSON receipt records DAT/source hashes,
arithmetic inputs, scales and counts. This census is evidence for the chosen
retail capacity, not parity for arbitrary custom attributes or undefined retail
allocation failure behavior.

## Construction and direct model controls

`nativeArticleResource_Begin(item, capacity, count, reserved_part)` reserves a
descriptor only. Capacity and the optional reservation come from immutable
authored metadata at the eventual caller. A reservation of -1 disables external
part publication. Shared metadata integration must validate reservations against
the authored rig. Begin exclusively claims a reservation for that creator
generation and exact Definition, even before model publication. A second live
collection declaring the same part fails; no-reservation collections may share
the creator freely. A new generation/Definition cannot make an old collection
write the new life's part table.

`CreateNode(item,index,attachment_part,radius,category)` accepts the next index
only. It allocates an actual ITEMLINK GObj and pooled ItemLink userdata, initializes
CollData and a symmetric fixed ECB, and retains the creator's attachment joint.
`LoadModel(item,index,root_hash)` directly loads a declared immutable model root
and attaches it to that node's GObj. These calls interleave allocation and model
load in stock constructor order. A failed call leaves the already constructed
prefix owned by the collection; the authored failure branch must call Release.
Begin itself allocates no GObj or model.

Indices run 0..count-1. Neighbor links preserve the original convention: next is
index-1 and prev is index+1. Model and attachment are distinct: ItemLink.jobj is a
borrowed creator joint; GObj.hsd_obj is the collection-owned model. Resource joint
Read/Write use `nativeModelJoint_Resolve/Read/Write` directly. They preserve existing
ordinal-path bounds, finite values, Quaternion channels, visibility and matrix
dirty semantics. SetWorldMatrix validates all 12 values, copies the actual matrix
and sets the existing model matrix flags. SetDraw controls each node's GX callback;
endpoint drawing is an authored choice. No fake Item context or stock-kind dispatch
is involved. Geometry/animation operations for the nine-state solver remain a
later bounded batch.

## Publication, borrowers and release

`BindExternalPart(item,index,part)` requires the exact declared reservation and a
part outside the creator's actual rig count but inside MAX_FT_PARTS. It publishes
only into an empty slot or the identical owned model. The backend does not name
part 139; retail Link/Samus metadata chooses 139. Tests also publish part 100.

`SetCaptureAnchor(item,index)` assigns the common captor capture anchor only in
common Catch/CatchDash/CatchPull/CatchDashPull/CatchWait/CatchAttack states. The
collection records the specific binding. Release inspects that common union only
in those states and only for a recorded, exact pointer match. It restores the
captor's current owned common anchor `parts[ft_data->x8->x11].joint`; missing valid
anchor becomes NULL. It never restores an old borrowed endpoint or scans arbitrary
motion union bytes. Common consumer integration must handle this nullable fallback.

Release detaches all exact known external-part, common capture and fighter hitbox
borrowers before freeing any model graph. Newer pointers in reused slots/hitboxes
are preserved. Matching hitboxes are disabled and their joint pointers cleared.
The thrown hitbox is checked as well. If the creator has died, changed generation
or changed Definition, its storage is not touched; the Item's resources still
release. Creator/model teardown integration must therefore release resources
before changing the live creator's Definition or destroying its rig.

Release frees the constructed prefix in reverse index order using retained array
entries, never a freed neighbor. It handles partial construction and repeated
release. Ordinary script operations require a live Item and exact Definition.
Engine teardown uses retained identity keys instead: `CancelGeneration` compares
the Item address and creation number without dereferencing old Item storage or
retired metadata. A delayed cleanup for an old creation cannot remove a newer
collection at the same address. `CancelCreator` compares the fighter address and
creation number; `CancelDefinition` compares the immutable Definition address.
All routes detach valid borrowers before freeing models.

`Item_OnUserDataRemove` cancels that Item generation before freeing its userdata.
`Fighter_Unload_8006DABC` cancels the creator's collections before destroying its
rig. Runtime slot retirement cancels the Definition before freeing asset arenas
or unloading the module. Detached validation/staging slots own no live resources;
their Close must preserve a live scene that shares the same DLL/Definition.
Runtime Clear cancels all collections before clearing rig tables. HSD's normal destruction calls userdata removal before unlinking the
GObj or clearing its userdata. The retained-key route also handles explicitly
unlinked owners without relying on that order.

## Rollback and validation boundary

All mutable collection descriptors, nodes and generation counters are game-state
statics. The melee_game forced state-section include and
`-fno-zero-initialized-in-bss` place them in `.mstate$m`. Existing GetGlobalRegions
already registers the full section; no duplicate region or alternate save/load
route is added. HSD GObj/model allocator bookkeeping and active heap are already
tracked. Build20's production COFF symbol table confirms `resources` resides in
`.mstate$m`; `game-sources.txt` includes the new unit. An active collection rollback
test remains pending. The scoped console matrix does not accept heap/renderer
or gameplay rollback integration by simulating it.

Run the coherent console matrix using an immutable matching SDK:

```text
python -B tests/native/article_resource_source_test.py --snapshot .local/baseline19 --output .local/article-resource-source19
python -B tests/native/article_resource_census.py --iso "../Super Smash Bros. Melee (USA) (v1.02).iso" --out-dir .local/article-resource-census
```

The matrix compiles production resource, shared joint and callback-lifetime bodies.
Its HSD allocation/draw/ECB and creator-query boundary is deterministic fixture
code. It covers 270 allocation failure sites, 540 simultaneously live nodes, generic
reservations 100/139, stale generations, Definition changes, exact borrower
invalidation, and six teardown cases including unlinked/poisoned Item storage,
a delayed old generation, and detached validation versus bound slot retirement.
The latter compiles the actual `release_slot` body extracted from the runtime;
only candidate slot storage and module/held-visual removal are fixture boundaries.
Results record source/snapshot hashes and the
remaining acceptance limits. The normal CMake target `native_article_resource`
runs this same production-body matrix. Full authored tether source comparisons
and hidden/muted tether gameplay remain pending.

## Resource geometry and model controls

The following C backend operations are bound directly in the Host and emitted
by the script compiler. The historical baseline21 component receipt below
predates schema integration. `Begin(owner,count)` now resolves its immutable
capacity and reservation from the actual authored article logic. This API still
does not constitute a registered or gameplay-accepted tether article.

Every operation validates the live Item generation, exact author Definition and
actual article logic, then the constructed node index. No mutable resource
pointer is exported. Failed writes and model/probe operations return false;
missing float reads return zero. Missing integer line reads return -1 and missing
environment flags return zero. Line IDs and flag bits remain full-width integers,
including their high bits, throughout the API.

`NativeArticleResourceFloat` names the following actual storage channels, with
separate X/Y/Z members for each vector:

| Channels | Storage | Access |
| --- | --- | --- |
| Position | ItemLink.pos | read / finite write |
| Velocity | ItemLink.vel | read / finite write |
| CollisionPosition | CollData.cur_pos | read / finite write |
| CollisionPrevious | CollData.last_pos | read / finite write |
| FloorNormal | CollData.floor.normal | read only |
| LeftWallNormal | CollData.left_facing_wall.normal | read only |
| RightWallNormal | CollData.right_facing_wall.normal | read only |
| CeilingNormal | CollData.ceiling.normal | read only |

`CollisionPrevious` deliberately means `last_pos`, as used by the stock tether
helpers; the separate engine `prev_pos` is untouched. `ReadFloat` reads the
selected channel. `WriteFloat` rejects unknown channels, read-only normals and
NaN/infinite values before mutation. `NativeArticleResourceInt` exposes
`SurfaceLine` as read/write ItemLink.x1CC; `EnvironmentFlags`, `FloorLine`,
`LeftWallLine`, `RightWallLine` and `CeilingLine` are read-only collision results.
`ReadInt`/`WriteInt` never convert those values through float.

`SweepStage` calls `mpColl_800471F8` on the node's actual CollData and returns its
boolean result. `FollowFloor` calls `mpColl_8004B108` and returns its boolean
result. Neither function copies positions, selects tether state or records an
authored contact flag. The script owns those decisions and their order. In the
stock Link sweep, it copies collision current to last, node position to collision
current, probes, then copies the corrected current back to the node position.
`ReadLineSpeed` calls `mpGetSpeed(line,&node.pos,output)` without applying the
result. Its output is cleared before a failed lookup; the script decides whether
to test line existence and how to apply the returned vector.

`ReadAttachmentMatrix` and `ReadAttachmentPoint` additionally validate the retained
original creator's generation and Definition before reading its joint. They
request the attachment joint's actual HSD setup matrix. The matrix operation
copies the 3x4 world matrix; the point operation transforms a finite local point
through it. This preserves Link's local `(0,0,6)` offset and Samus's local origin
without a fighter-specific primitive. Failed outputs are zeroed, and point input
and output may alias. An absent creator cannot invalidate reads of an independently
owned resource model, but prevents all retained attachment reads.

`ReadWorldMatrix` copies the resource model's existing cached matrix. It does not
request HSD setup or derive a new matrix from SRT, matching the existing
`SetWorldMatrix` contract. Matrix construction and solver transforms remain
authored. Existing `ReadJoint`/`WriteJoint` use the shared HSD ordinal-path/channel
implementation on the actual resource model, with its finite, quaternion,
visibility and dirty-matrix semantics.

`AddAnimation` resolves a declared/imported animation bundle by immutable name
hash from the owning article and invokes `HSD_JObjAddAnimAll` on the selected
resource joint. `RequestAnimation`, `AdvanceAnimation` and `RemoveAnimation`
invoke `HSD_JObjReqAnimAll`, `HSD_JObjAnimAll` and `HSD_JObjRemoveAnimAll` on that
joint. Paths use the shared resolver: the empty path selects the root and `"0"`
selects its first child. Missing paths or bundles fail without dispatch; imported
bundle triples may contain null animation/material/shape members. Request frames
must be finite. No callback or phase is installed implicitly by these operations.

`SetAnimationRate` requires a finite rate and calls the actual generic
`lb_8000BA0C`. That helper walks descendants of the selected root; it does not
change the selected root's AObj itself, and INSTANCE joints stop descent.
Animation loop behavior remains in the imported descriptors. Stock Hookshot and
Samus grapple construction do not explicitly mutate loop flags, so this batch
adds no speculative loop setter.

Test the resulting item on flat ground, slopes and near an edge. Follow the
[article and resource checks](authoring_checks.md#articles-and-resources)
when verifying position, movement and cleanup.

The following describes historical generated-backend evidence. The old
snapshot-overlay runner uses retired ownership and animation-path interfaces;
it is not a current runtime check.

That historical matrix compiled the resource backend, shared joint implementation and
live-list generation checks against frozen baseline21. A single owned-header
overlay plus a byte-identical backend source copy prevents neighboring live
headers from bypassing the frozen SDK. Its receipt records the snapshot manifest,
owned inputs and original source hashes. Unchanged Link movement/probe/cache/
moving-line/attachment helpers and Samus attachment/animation setup bodies are
compared against sequences of the new operations. Actual lb rate, HSD joint
tree-walk and AObj rate setter bodies verify the traversal behavior.

HSD allocation, matrix setup/arithmetic, Add/Req/Anim/Remove dispatch, collision
responses and moving-line speed remain explicit deterministic fixture boundaries.
The matrix proves operation order/data/result preservation at those boundaries,
including invalid indices, read-only/nonfinite writes, full-width IDs, unlinked
poisoned owners/creators and changed generations/Definitions. It does not accept
the real collision solver, renderer, tether state machine, callback phases or
gameplay. Those checks follow the coordinated schema/Host/compiler integration.

## Resource effects

```text
item.resource(endpoint).spawn_effect_at_position(1011, x, y, z)
item.resource(endpoint).spawn_effect_facing_at_position(1009, x, y, z, facing)
```

These article-only statements dispatch `efSync_Spawn` with the collection's
actual node GObj. They require a declared resource collection. The node index
and effect ID are strict Int expressions; position and facing accept Float or
Int, excluding Bool. Operands evaluate once in written order, starting with the
node index. The existing generated lifetime checks stop continuation after any
operand or effect call that retires/recycles the article.

The backend validates the article creation ID, exact immutable author and
article logic, collection/node generation, and live original creator. Position
and facing must be finite. It copies the position, retains the node through
synchronous dispatch, and defers resource freeing until the outermost collision
or effect operation returns. Retiring collections immediately become unavailable.
No particle/model pointer or retained `EF_SpawnResult` is exposed. Backend true
means dispatch occurred; it does not promise successful effect allocation.

The supported payloads create independent generators without reading node
userdata as a fighter or retaining the endpoint as an effect parent:

- Position: IDs 0..0x24F and 30000..30999, plus 0x3E9,0x3EA,0x3F3,0x3F4,
  0x405,0x407,0x40C,0x40E..0x411,0x416,0x41C,0x41F..0x421,
  0x425,0x426,0x42D,0x434,0x43A..0x43C,0x441,0x442,0x444,0x445,
  and 0x474..0x477.
- Position plus facing: IDs 0x3EF..0x3F2. The native dispatcher owns its original
  sign choice: 0x3EF / 0x3F1 negate facing; 0x3F0 / 0x3F2 preserve it.

Unknown IDs, wrong payload families, joint/model attachment IDs and negative
IDs fail before variadic dispatch. This is a bounded source-backed signature
contract, not permission to pass arbitrary IDs to `efSync_Spawn`. Author scripts
still own effect selection, RNG, timing and effect ordering. The Samus calls
0x3F3 and 0x41C consume position only; 0x3F1 consumes position plus facing. The
extra ItemLink argument in the old 0x41C call is unused by its actual branch.

The focused `tests/native/article_resource_effects` batch links the production
resource/joint/lifetime bodies, actual dispatcher branches and generated modules
from both compiler routes. HSD/particle allocation and ECB responses are controlled;
GX pixels, particle evaluation and complete tether gameplay remain separate gates.
