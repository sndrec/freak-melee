# Source-owned Chain resource closure proposal

Initial read-only audit, 2026-10-08, followed by the allocator foundation described
below. The collection/model integration remains a proposed contract, not evidence
that Sheik's full source package is accepted.

## Implemented allocator foundation

[native_hsd_reservation.c/h](../src/sysdolphin/baselib/native_hsd_reservation.h)
now binds caller-owned backing at an explicit transition: up to16 pinned owners,
96 distinct bins per owner and16384 slots per bin. Class pieces use their actual
32-byte rounded size; satellite bins retain the exact initialized ObjAlloc
descriptor, size and alignment. Payload, occupancy/generation and free order are
carved from the supplied region; no graph objects are preconstructed. Invalid
binding leaves supplied storage unchanged.

`Bytes` computes backing size/alignment; `Bind` establishes its nonzero ownership
epoch. `Begin` checks a complete construction demand against available slots and
native number limits before publishing a single active claim. Nested claims fail.
The direct `hsdAllocMemPiece` and `HSD_ObjAlloc` sites consume that claim without
bin splitting, pool growth or ordinary free-list fallback. Unknown size/descriptor,
quota exhaustion, direct `HSD_MemAlloc` and explicit `HSD_ObjAllocAddFree` attempts
make failure sticky until `End`. ObjAlloc heap-limit mode is explicitly unsupported
for external backing rather than bypassed. Native satellite used/peak counters
remain meaningful; reserved slots do not enter its unrelated ordinary free list.
Normal unclaimed allocation retains its existing paths.

Native class and ObjAlloc frees recognize reserved addresses after the claim has
ended. Interior, duplicate and wrong-bin frees are rejected; `Release` requires
zero occupied slots and no active claim. Construction quotas count total allocation
calls, including temporary objects freed during construction. A footprint must
therefore supply total calls as well as its simultaneous backing budget.

[native_article_model_demand.c/h](../src/melee/it/native_article_model_demand.h)
provides an allocation-free bounded census of already imported descriptors. It
counts JObj/DObj/PObj/MObj/Material/TObj/Tlut/TObjTev class pieces, construction ID
and explicit-matrix satellites, and a separate potential lazy Vec demand. Borrowed
ImageDesc/LOD data is not cloned by the native loader. All relevant native allocator
descriptors must already be initialized at the transition. The result is an
inventory, not graph admission. The initial foundation marked all TEV compilation
uncovered; the subsequent bounded extension below counts supported default
combiner demand while preserving the unresolved material environment. Unsupported
shape/envelope, RObj references, instances, particle/spline arms and custom classes
also report explicit gaps. Animation loading and renderer allocations remain outside
this census. Actual default class selection must be verified when admission lands.

The scoped [fixture](../tests/native/hsd_reservation/README.md) passed86 checks at
`A:/FreakMelee-test-archives/hsd-reservation-final/receipt.json`: real class,
ObjAlloc, memory and ID units; exact Vec/Mtx/RObj allocator bodies and Aurora OS
allocation bodies; ordinary heap reuse, fixed reservation routing, native limits,
sticky failures, frees and local slot-generation restoration. Only unrelated
diagnostic `OSDumpHeap` logging was omitted from the native OS body extraction.
The integrated build separately passed38 tests, including ordinary native heap
and ObjAlloc tests. Neither proof constructs HSD graphs or activates Chain.

Snapshot ownership remains explicit and incomplete for new backing: the API does
not register caller memory with the existing broad snapshot owner. Real integration
must capture/pin reservation structs and backing, claim/registry state, native ID
tables, class/ObjAlloc counters, live graphs and applicable renderer state. The
fixture copies only its exact local reservation metadata/backing to demonstrate
slot resurrection; Item actor snapshots alone do not cover native graph state.
No secondary-model source load, resource collection or animation admission has
been enabled by this foundation.

### Bounded default material/TEV demand extension

Actual `MObjLoad` copies Material and optional PE, then sets RENDER_TOON.
`HSD_MObjCompileTev` can append global shadow textures or prepend a global toon
texture. It assigns native texture resources, runs MObj/TObj expression builders,
then the actual compiler/simplifier/register scheduler. The compiler uses fixed32
DAG/order entries and maps stages through a16-stage native mapping. Its only heap
allocation sites are class pieces for `HSD_TETev`, `HSD_TECnst` and
`HSD_TExpTevDesc`; optimization and scheduling allocate no additional objects.
Compiled descriptors and referenced constants persist; transient TEV expressions
and unreferenced constants are freed by the real compiler.

Census now supports a conservative allocation budget for unnamed default material
and texture combiners without custom TEV, bump/toon descriptors or explicit shadow.
It allows at most8 textures and requires the conservative expression count<=16.
The bound counts base1, optional diffuse1/specular3, and at most one final merge;
each applicable diffuse/ambient, specular and EXT texture pass adds1 expression and
up to2 blending constants. Native duplicate-constant sharing may reduce demand.
Compiled descriptor calls are bounded by the expression count. Equal rounded
allocation sizes coalesce into the actual reservation bin. No optimizer-dependent
stage reduction is assumed for acceptance of this budget.

Every material still reports `MaterialEnvironment` uncovered. Descriptor data
cannot establish empty global toon/shadow state, actual default class method
selection, shader register legality or renderer resource ownership. Custom TEV,
invalid texture modes, named materials/textures and excessive budgets retain the
separate `TevCompiler` flag. These bounds do not enable graph loading or source
metadata admission; environment/default-method capture and construction cleanup
must be integrated before using them as a construction claim.

The hidden allocator fixture compares actual expression builders, material compile,
the complete native DAG optimizer/scheduler and cleanup against ordinary allocation
for289 materials. It covers all9 color/8 alpha modes with four base lighting/vertex
combinations, a multi-lightmap blending case, and recompilation of live outputs.
Native stage configurations/retained constants match, no heap allocation occurs
inside the reserved claims, and native cleanup releases all reserved shader objects.
The final receipt is
`A:/FreakMelee-test-archives/hsd-tev-final/receipt.json` (9215 checks/289 materials).
Only core material/texture setup and an explicitly empty toon/shadow environment
are controlled. No whole graph loader, DAT admission, draw or snapshot integration
is claimed; actor-only storage still cannot restore these native objects.

## Native behavior and present boundary

[Sheik source](../examples/sheik_full.fms:1543) declares capacity30 and roots
`Link 100`/`Tip 104`. Construction reserves a runtime link count, creates nodes
sequentially, loads the declared Link/Tip graph into each independent node, then attaches the Article. Per-frame
source moves actual nodes, updates collision history, performs point sweeps,
updates Fighter endpoints and builds world matrices. Cleanup releases the pool.
The retail count comment20 is not a generic hardcoded simulation count.

The original [Chain implementation](../src/melee/it/kinds/itseakchain.c:73)
loads `itSeakChain_Attrs.x64_joint` and `.x68_joint`, allocates real ItemLink GObjs
and payloads, links neighbors, initializes fixed ECBs and gives the tip a distinct
model/draw callback. Its `it_802BCA30` copies the attachment matrix plus the local
depth translation into the tip's world matrix; `it_802BCB88` positions/orients the
other real models. This behavior must remain authored over generic primitives.

[native_article_resource.c](../src/melee/it/native_article_resource.c) already
implements the substantive bounded collection, ItemLink/model, collision and
matrix operations. It currently admits owners through `article_definition()`:
Definition + matching physical kind + native `xB8_itemLogicTable`; Begin additionally
uses `nativeArticle_Creator`. Collection and creator validation retain Definition
identity. Source Items deliberately lack that legacy authored logic. Calling this
backend from a new binding would therefore remain a failing compatibility route.
Replace its ownership/lifecycle contracts directly; do not add a second wrapper.

The bundle in [native_source_resources.h](../src/melee/ft/native_source_resources.h)
is a bounded name/blob/PNG codec. It does not import a Chain model root or allocate
runtime nodes. Current ItemProgram preparation has no resource-capacity/root
metadata. Primary `item.joint` selectors do not provide resource or secondary roots.

## Immutable preparation contract

Add a data-only `NativeScriptItemResourceSpec` to each applicable prepared Item
program: exact source owner, declared capacity, owned model-root rows, prepared
ordinal joint selectors and immutable identity. Store rows/selectors in aggregate
fixed pools and rebase pointers during ItemPrograms_Rebind, as with states/current
model selectors. Do not embed a large table in every program or allocate on stack.

`resources capacity N` admits the existing engine range2..90; Chain's authored
3..30/endpoint/history constraints remain its source logic. Preserve the current
six global collection slots initially; exhaustion returns false without clamping.
`reserve_part` is optional existing generic metadata, not needed by current Chain;
it must stay rejected until actual owned rig reservation/borrower cleanup is wired.

`model_root Name byte_offset` is a DAT special-attribute pointer-field recipe,
not a joint index. Roots100/104 mean offsets0x64/0x68 in the actual Chain attribute
block. Prepare against the exact pinned original Article DAT and
`Article.native_special_attribute_source`: aligned/in-bounds slot, real local
relocation, bounded DAT_JOINT import, no external/donor fallback. The existing
[itArticle_ImportModelRoots](../src/melee/it/itarticle_data.c:30) performs this
substantive import but takes legacy NativeFighterArticleModelRoot/hash metadata.
Change that input to a data-only root recipe and adjust actual import callers;
do not synthesize a NativeFighterDefinition. Preserve complete-table publication.
Existing root limit8 is a usable explicit bound, not a reason to silently truncate.

Resolve `load_model(Link|Tip)` at load to a closed source-owner/root-row literal.
Reject unknown, duplicate, foreign-owner or numeric roots. No runtime name hash
or source scan. Required roots must be nonnull; a NULL model must not become the
primary graph. Validate literal joint paths against every root reachable by that
resource operation; `joint("")` is valid for both Chain roots.

Secondary Item model slots are a separate real collection of owned instantiated
graphs. A resource node's own model is neither `item.joint(0,...)` nor a fabricated
secondary Item slot. Keep Item slots1..3 rejected until their prepare, instantiation,
teardown and snapshot ownership are implemented; never alias them to primary.

## Scene storage, owner and captured node contracts

Use the existing six-collection/90-node organization, but replace Definition,
logic-pointer and live-list-only identity checks with:

* owning Item full nativeGObjPool reference/scene epoch, native generation/data;
* exact published source ItemProgram/bank/owner/layout/parameter identities;
* collection generation, node-slot generation, real ItemLink GObj reference;
* retained attachment Fighter reference/generation/program and actual raw bone
  chosen at node creation. Original creator/current damage owner remain distinct.

The source Item actor initializes attachment_owner from creator and can revoke it
independently ([actor relations](../src/melee/it/native_script_item_actor.c:177)).
CreateNode uses that explicit current attachment relation, then retains its
actual bone and Fighter identity. Later matrix queries must not reread a changed
owner after reflection/nested work. Detach may revoke future attachment operations
without preventing release of existing nodes. Stale attachment reads fail/zero;
release uses retained identities, not a new live-owner lookup.

Proposed substantive APIs, with caller execution scope supplied explicitly:

```c
NativeScriptVMStatus Begin(scope, caller, count, bool* accepted, diagnostic);
NativeScriptVMStatus Capture(scope, caller, integral_index, ScriptValue* token, diagnostic);
NativeScriptVMStatus ValidateTarget(scope, caller, token, operation, diagnostic);
NativeScriptVMStatus Read/Write/Invoke(scope, caller, semantic, token, operands, diagnostic);
void CancelOwner(full_object_ref, native_generation);
void CancelProgram(exact_program_identity);
bool Boundary(void); // no running native operations or invocation captures
```

`item.resource(index)` captures once before any RHS/native argument effects.
Capture must represent a reserved uncreated slot so `.create(...)` can run;
after creation it retains that node's real generation/reference. `exists` is
nullable. A released/reused collection or previously captured node cannot retarget
to its replacement. Use a recognized invocation-only Object tag in the existing
receiver scratch, no pointer ScriptValue or persistent Object state. Resource
domain routing must occur before generic Object export; ItemLinks are not Items
and must never pass NativeCallbackSource_Item or receive a fake Item context.
Reserve a distinct domain (candidate0x07000000, subject to root approval), with
SourceOnly Item-owner descriptors and prepared identities.

For valid source operations preserve native contracts: float geometry missing
reads0, environment flags0, surface/line identifiers-1, missing joint primitive
defaults. Invalid writes/captured retirement diagnose; uncreated allocation/model
failure returns false with unchanged published outputs. Captures for creation
must validate collection/index before evaluating effectful constructor operands.

## Real allocation, deferred release and snapshots

[ItemLink](../src/melee/it/itCharItems.h:310) includes neighbor pointers, position,
velocity, CollData, surface line, GObj and attachment JObj. The current resource
backend allocates it with `HSD_ObjAlloc(item_link_alloc_data)`; Item_80266FCC only
initializes that allocator. [HSD_ObjAlloc](../src/sysdolphin/baselib/objalloc.c:88)
can lazily grow through HSD_MemAlloc when empty. A six-by90 metadata array does
not make payload/model allocation fixed.

Reserve the source resource ItemLink payload/metadata/free order with a real
NativeMatchPool at scene transition; capacity540 covers the existing bound.
Allocate actual ItemLink payloads/GObjs from fixed pools, and return them through
their actual userdata ownership callback. Do not retain an HSD allocator wrapper
or a pointer-list facade. Stock-native ItemLink users share the engine type but
are separate call sites; migrate their allocation directly if consolidating the
allocator, otherwise do not mix/free their payloads through the source pool.

Real HSD graphs are also required, but the refined recommendation is to reserve
native allocator storage at the scene transition and instantiate actual graphs
only during explicit Create/LoadModel. Do not preconstruct540 instances for each
root/declaration. See the allocator evidence and ownership contract below. Draw
remains actual HSD; roots are chosen by their prepared literal, never by an inferred
Link/Tip slot sequence. No static Chain substitution or lazy host-heap growth.

Retain `active_operations`/`releasing`: release immediately revokes access, then
defers physical teardown until the outermost sweep/matrix/model/effect operation
returns. Balance entry/exit even on retirement/error. Capture all identities before
native dispatch; guard source/node/attachment/model before output publication.
Detach every external part, capture anchor and hitbox borrower before freeing any
graph. Current sweeps guard only deferred storage, not new source generations;
those checks must become substantive. Current FollowFloor/matrix/model calls need
the same operation protection, not just the two sweep functions.

Current resource metadata is `.mstate`; HSD payloads are tracked heap memory.
There is no typed resource snapshot record in Item actor snapshots. Add occupied
collection/node records plus actual ItemLink collision/position/velocity/flags,
surface line, neighbor indices, pool generations/free order and model-slot identity
and mutable graph state. Reuse NativeMatchSnapshotPool for actual native pool
payloads; validate/rebuild pointer relations only after their real pools/graphs
are restored. Include JObj world/SRT/flags and relevant native animation/material
state; saving only script arrays or host pointer lists cannot resurrect nodes.
Reject snapshots during active operations/running invocation scopes, validate all
records before mutation, restore without running constructors or callbacks.
Item removal, Fighter attachment retirement, program release and scene reset must
cancel retained collections before dependent native graph/storage is reclaimed.

## Typed geometry and matrix calls

Keep original source spellings and channels: create(raw_part,radius,category,x,y,z),
load_model(root), set_draw(bool), position/velocity.xyz, collision.position.xyz,
collision.previous.xyz, surface_line, collision.env_flags, joint(path).translation.xyz,
sweep_point, read_attachment_point, read_attachment_matrix, set_world_matrix.
`collision.previous` is CollData.last_pos, not prev_pos. Create initializes the
fixed ECB before later position writes; point sweep calls mpColl_80048844 and
does not substitute a floor-only sweep. Root/bone selection uses actual bounded
owned rig raw-part mappings; retain native radius/category/finite validation.

Matrix codec is exactly twelve F32 scalars in row-major Mtx[3][4] order. Attachment
read runs HSD_JObjSetupMatrix then C_MTXCopy; point read runs C_MTXMultVec.
World write runs HSD_JObjCopyMtx then USER_DEF_MTX|MTX_INDEP_PARENT|MTX_INDEP_SRT
and dirty semantics, preserving original Chain behavior. Do not rebuild the cached
world matrix from SRT. Existing world write rejects nonfinite input before mutation.

`state float matrix[12]` currently needs a generic bounded-owned-array argument
lowerer: accept only the exact owner/declaration/type/count and expand the bare
array argument to twelve typed Index operands/lvalues at load. Use normal native
scalar input/output marshalling and existing guards; no ArrayRef/native pointer
value exemption. Read output destinations are captured before invocation; input
elements are copied before native dispatch. Preserve source span diagnostics.

The actual current limits all fit: expression/descriptor/Execution arguments32,
bytecode and VM native args32, independent selector ancestry32, uint32 output and
input-output masks, uint8 counts, uint16 frame register addressing (shared runtime
65536 registers). Matrix calls need12 args plus a separately captured resource
selector, point calls6. No width increase is needed. Native call layout arity must
match the expanded semantic Call node; reject an expansion exceeding32 rather
than introducing a resource-only exception. Any later larger aggregate must
extend these shared limits/masks/verification together.

Old matrix readers zero the local matrix even on false, and the old emitter copies
all12 entries unconditionally. Preserve that ordinary-absence zero publication
with AlwaysAfterGuard; retirement/error leaves authored destinations unchanged.
This differs from existing SuccessOnly position queries and must be tested rather
than silently changed. World writes copy all12 inputs and validate them before any
native write. Output alias/type/index validation applies to every expanded element.

## Coherent implementation and acceptance boundary

First agree spec/root recipes, capture token variants, bounded native allocator
reservation/destruction ownership and array lowering. Then land metadata/import + fixed runtime collection
+ typed resolver/lowerer + cleanup/snapshot integration together. Suggested split:
root owns package/import/publication and pool/snapshot routing; one worker owns
substantive collection/lifecycle/model/geometry operations; one owns source
selectors/catalogue/bounded-array lowering. No production edits are authorized
by this read-only proposal itself.

Acceptance requires real full Chain body compilation, actual owned DAT roots and
raw attachment bone, live source Item actor + scene pools + genuine HSD graph,
native point-sweep/matrix comparisons, nested release/retirement and snapshot
resurrection/free-order tests. Pool/model failures must reach authored cleanup.
Chain still also needs generic Fighter capsule/endpoints/terrain queries,
targets.call("chain_lost") migration and successful item.remove unwind semantics;
resource admission alone must not be presented as complete Sheik activation.

## Model storage refinement: reserve storage, construct only live graphs

The current owned dataset is
`A:/FreakMelee-test-archives/source-family-project-cli-root/drafts/stock-0/partner/fighter.dat`,
166907 bytes, SHA256
`aaf8e3a4037bd211df4117f5ce8e26d9ebb1f122129862d6f5dfd17d03c7ea4e`.
Read-only bounded DAT traversal followed the current dat_model/dat_material
descriptor layouts and verified each nonnull pointer slot is relocated/in bounds.
Chain Article is at38004, special attributes15388, count20. Slots100/104 both point
to joint73416. These are two named recipes for the same descriptor in this dataset,
not permission to share a mutable runtime graph between nodes.

| Per instantiated Link or Tip occurrence | Count | Native Win64 bytes | 32-byte class piece |
|---|---:|---:|---:|
| JObj | 1 | 184 | 192 |
| DObj | 1 | 48 | 64 |
| PObj | 1 | 48 | 64 |
| MObj | 1 | 64 | 64 |
| Material | 1 | 20 | 32 |
| TObj | 1 | 368 | 384 |
| Core subtotal | 6 | 732 | 800 |

Sizes were checked with the current MSVC-target Clang's read-only native record
layout dump. Neither root has descriptor RObj/Rvalue, envelope/envelope matrix,
shape set, TLUT, optional PE, custom class name or animation controller. PObj is
skin type without an envelope list; MObj render mode28 and one TObj are real.
Vertex/display/image descriptors borrow pinned imported data, not a per-instance
DAT clone. These counts exclude generated TEV state, ID entries and lazy scale
vectors. HSD_JObjMakeMatrix can allocate a Vec3 even without an animation.

Thus a count20 Chain has20 core graphs:14640 live object bytes/16000 rounded
class-piece bytes before those extras. Even at the engine maximum540 live nodes,
the same graph shape has395280/432000 core bytes. Preconstructing540 for each of
the two root names duplicates this despite their equal descriptor, and multiplying
again by every source declaration is unjustified. Reserve against the selected
scene's simultaneous node budget and maximum admitted per-root footprint, not
the sum of capacity times every possible root name. Preserve the capacity limits;
do not hardcode a19-Link/1-Tip distribution.

### Actual allocator routes and required extensions

The native [Aurora OS allocator](../../aurora/lib/dolphin/os/OSAlloc.cpp:276)
already creates a heap over a fixed start/end range. OSAllocFromHeap splits that
range or returns NULL; it does not extend host memory. HSD_SetHeap selects the
heap, and HSD_MemAlloc asserts on NULL. A reserved OS arena therefore avoids
host-heap growth, but alone does not guarantee recoverable graph allocation,
fixed size-class supply or complete snapshot ownership.

| Route | Current behavior | Concrete bounded reservation work |
|---|---|---|
| JObj/DObj/PObj/MObj/TObj, Material, TEV descriptors/constants | `_hsdClassAlloc`/`hsdAllocMemPiece` use 32-byte size classes. GetMemoryEntry lazily allocates/resizes metadata; an empty bin splits a larger bin or obtains another HSD_MemAlloc chunk. | Prime all required class-size metadata at transition; reserve actual free blocks per class including transient TEV peak, add explicit sealed/no-growth reservation admission in the actual allocator. Account split remainders or disable bin splitting for a claimed constructor reservation. No wrapper around a failed allocation. |
| ItemLink, Vec, Mtx, ID, RObj/Rvalue, SList and other satellite records | HSD_ObjAllocData free list, with lazy AddFree fallback. | Existing AddFree plus SetNumLimit **and EnableNumLimit** can prepopulate/hard-bound a descriptor. Its backing must be the actual pinned scene region; total used+reserved capacity matters, not merely the current free count. NativeMatchPool remains preferable for the resource ItemLink generations/snapshots. |
| AObj/FObj | Actual native paths already use nativeAObjPool/nativeFObjPool. | Reserve their existing capacities and include future declared animation bundles in the model budget; initial Chain has none. No controller preconstruction. |
| Shape blend weights, RObj expression args, animated texture tables; shape draw scratch | Some paths call HSD_MemAlloc directly. | A generic root budget must account for block size/alignment/lifetime and native free routing, or explicitly reject such unsupported recipe features at load until covered. Reserving JObj slots alone is insufficient. Current Chain does not exercise these features. |
| Model ID lookup and material shader state | JObjLoad inserts IDs; MObjLoadDesc immediately calls HSD_MObjCompileTev. Generated TEV/constant lists allocate and free during compile. | Include persistent and transient allocations and global ID table state. The absence of a DAT TEV descriptor does not mean zero generated shader objects. |

Full graph preconstruction pays native loader and TEV compilation for all spare
graphs and requires a bespoke reset of JObj matrices/flags, material mutations,
TObj/native render handles, animations and references before reuse. It does not
magically avoid future Vec/animation/draw allocations. Reserving empty blocks
instead pays storage initialization at transition and executes the genuine loader
only for nodes actually created. Use normal native release/destruction to return
those blocks, so new instances get genuine native initialization rather than a
hand-written partial reset.

Recommended generic sequence:

1. Prepare a bounded allocation recipe from each owned descriptor occurrence;
   distinguish class-size blocks, satellite pools, persistent blocks and temporary
   constructor peak. Root names sharing a descriptor may share the immutable
   allocation recipe, not live objects. For generated TEV costs, trace at most one
   genuine temporary construction per unique admitted recipe at the transition,
   then release it normally; no capacity-wide constructor loop. That trace must
   include the actual material compile and lazy matrix stage. The six-object DAT
   subtotal is not a complete budget or a boot-time benchmark.
2. Reserve the selected scene's shared capacity with actual bounded native storage.
   Prime class metadata/ID/Vec supplies without constructing all graphs. The new
   reservation API belongs in the actual generic HSD allocator/scene ownership,
   not in a resource binding shim. `nativeHsdReservation_Bind/Begin/End/Release`
   now supplies this allocation foundation; complete graph recipes, scene binding
   and actual snapshot-region ownership are still required.
3. Before LoadModel, atomically claim its complete footprint and stage the native
   graph off-GObj. HSD_JObjLoadJoint runs against that bounded claim; it must not
   steal another live collection's reservation or silently grow allocator supply.
   Unknown classes/direct-allocation features are load-time admission errors.
   Native loaders currently assert in several NULL paths, so either a complete
   non-reentrant guaranteed claim must make exhaustion impossible within the
   constructor, or those actual loaders need checked failure cleanup. Do not turn
   a native assertion into an authored Bool success.
4. Guard the owning Item/collection again, then attach the finished real root and
   draw callback. Failure releases the staged graph through genuine native
   destruction and returns false; release/final teardown uses JObjRemoveAll and
   satellite destructors after borrower/deferred-operation barriers.

Do not implement this by temporarily switching HSD's global current heap around
one call. Class freelists can contain blocks from another heap and native frees
need their original ownership; such switching is not an isolated graph allocator.
Reservation and free routing must be substantive in the actual allocator. Shared
bin consumption by other engine actors and nested native work must be accounted
for; available-heap-byte guessing is not an atomic allocation claim.

Snapshots must save allocator occupancy/free order/class metadata and all live
graph objects, material/TEV state and native pointer relations in addition to
ItemLink/CollData. Immutable descriptors/vertices/images remain pinned. AObj/FObj
use their existing typed pool snapshots. JObj/DObj/PObj/MObj/TObj/Material and
generated TEV allocations are not currently a typed graph snapshot just because
the page snapshot covers their heap. Native TObj first draw initializes GX texture
handles; TObjRelease destroys those handles. Renderer resources need the existing
renderer ownership/invalidation contract rather than copying resurrected GPU
handles as gameplay payload. Destruction returns CPU blocks to the same reserved
allocator; restore resurrects live allocations without replaying constructors,
material compilation or source callbacks.

This replaces the earlier full-graph-spare-pool recommendation. Bounded reservation
plus genuine lifecycle construction is the preferred direction; actual allocation
recipes and ownership/failure paths are still implementation work. No measured
full renderer/constructor boot timing or completed resource runtime is claimed.

## Next functional secondary-model slice

The smallest activation target is Link/Young Link's static arrow trails: two
owned model roots at attribute offsets 36/40, installed in secondary slots 1/2.
The importer already supports owned DAT joint roots, the actual model loader
constructs independent HSD graphs, and Item draw/destruction visit those slots.
This is a proposed next slice, not newly admitted source metadata.

First census the actual four imported descriptors. Restrict initial admission to
the supported default classes/materials, no shadow flag, and no effective global
toon image. Do not assume these real roots fit the current bounded subset.
Boomerang additionally requires animation bundles; Hookshot remains a separate
resource collection. Active toon/shadow and all unsupported graph features stay
rejected without preventing a verified simpler static-root implementation.

Backing must live in the existing captured native scene heap, including allocator
reservation metadata and headroom for constructing a replacement before releasing
the old graph. Reserve the later secondary draw/matrix scale-vector allocation
too: construction reservation alone does not cover lazy `HSD_VecAlloc`.
Descriptors stay pinned through rollback history. The acceptance fixture must
use actual imported DAT roots and prove load, transform, draw-slot selection,
replacement, destruction and broad-page restoration; actor-only snapshots are
insufficient. No retail renderer or restoration acceptance is claimed here.
