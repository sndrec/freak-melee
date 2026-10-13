# Explicit native match snapshot: ownership inventory and acceptance contract

Source audit, 2026-10-07. This is an implementation design and fixture inventory,
not a complete aggregate serializer or a claim of complete rollback acceptance.
No game/build was launched for this audit. Existing page snapshots remain the
correctness oracle during migration; the production representation must stop
copying/protecting entire game heaps or the reserved MEM arena.

The coordinated eight-actor benchmark already demonstrates that both generated
C and interpreted behavior can reproduce the existing page oracle. Its large
snapshot boundary cost is a native-engine floor, shared by both backends. The
reported approximately 193MB tracked range and 427 dirty pages describe the
current oracle/workload, not the proposed snapshot size. Less than 100us per
save/load is an aspirational measured target. Neither a sub-1MB representation
nor a 60Hz worst-case deadline is established by this audit.

2026-10-09 owner update: source secondary static models now have a concrete
scene reservation/registry and main-HSD-heap backing. Their immutable named
descriptor tables stay leased in the package through scene/history retirement.
See [the secondary owner checkpoint](native_source_secondary_owner_checkpoint.md)
for implementation and bounded evidence. This is still broad-page coverage,
not a compact secondary-graph serializer or a completed aggregate snapshot.
Primary fighter/item graphs, animated secondary graphs and resource collections
retain their separate ownership/serialization work.

2026-10-11 owner update: both stage Ground constructors now allocate actual
userdata from stable scene-owned storage, with occupied-record save/load and
ordinary HSD removal callbacks. See [Ground ownership](native_ground_snapshot_ownership.md)
for initialization, root-validation boundaries and verification. This covers
Ground records and their stage-specific unions; stage globals, collision buffers,
graphs and other satellites remain on the broad oracle. It does not complete the
aggregate snapshot or establish whole-match save/load timing.

2026-10-11 Item owner update: actual Item userdata and optional 100-pointer
model bone tables now occupy stable scene-owned slots, with occupied-record
save/load and original cancellation/detach ordering. See
[Item ownership](native_item_snapshot_ownership.md) for storage, verification
and the resolved field-level allocation differences with seven passing final
CPU comparisons. ItemLinks, graph/dynamics
allocations, resource/script owners and global counters remain separate work;
the production broad oracle has not been replaced.

2026-10-11 chain owner update: stock ItemLinks and authored resource nodes now
share one actual fixed scene pool, with occupied-record save/load and preserved
source borrower/deferred-retirement rules. See
[ItemLink ownership](native_item_link_snapshot_ownership.md) for storage and
bounded acceptance. The old stock allocator and separate authored heap backing
are removed. Graphs, resource registries, script state and module globals still
need aggregate closure; no whole-match timing improvement is claimed yet.

2026-10-11 reserved graph update: admitted authored HSD reservations now have
explicit occupied-record and allocation-metadata save/load. See
[reserved HSD ownership](native_hsd_reservation_snapshot_ownership.md).
Ordinary stock graphs, shared HSD allocator/class state and source registries
remain companion ownership work. Fighter/item script-state serializers already
exist and must participate in the aggregate restore; their records must not be
dropped when switching away from broad pages. Production still uses the oracle.

2026-10-11 shared accounting update: HSD heap recreation now retains the actual
initialized ObjAlloc registry. Shared descriptors and the object-arena cursor
have an explicit component serializer, with transition epochs and complete
candidate prevalidation. See [shared ObjAlloc ownership](native_objalloc_snapshot_ownership.md).
Ordinary payload/free-link backing, OS heap state, class counts and ID roots
remain separate closure work; this component complements reserved graph records.

## Boundary and representation

2026-10-11 main-camera update: the actual camera scalar/debug/coefficient globals
and auxiliary CObj root now have a 1,424-byte explicit component. Class backing
references and GObj generations connect it to existing payload owners without
copying the graphs twice. See [main camera ownership](native_camera_snapshot_ownership.md)
for exact owner restoration, native descriptor/projection corrections and limits.
All 26 components capture/validate in stock and interpreted eight-actor clients,
with exact previous/current traces. Stage/world/HUD/effect and remaining GM
state, cross-component closure and full aggregate restoration remain open.

2026-10-11 camera-target update: the actual requested target prefix and its
active/free topology now have an explicit component. Versus still requests 70
targets; its component is 8,456 bytes, including free-record history. See
[camera target ownership](native_camera_subject_snapshot_ownership.md) for
source-preserving storage, scene retirement, malformed-list refusals and exact
sixteen-tick restoration. All 25 components capture/validate in stock and
interpreted eight-actor clients. The main camera and stage/world state remain
separate work; the broad oracle has not been replaced.

2026-10-11 player update: all six actual player records, stale/extended statistics,
stocks/spawn/transform controls and both attack-instance counters now have one
23,080-byte component. See [player ownership](native_player_snapshot_ownership.md)
for exact sixteen-tick owner restoration, original-body source split, generation
resurrection and candidate dependencies. All 24 components capture/validate in
live stock and interpreted eight-actor matches. Other world/stage/camera/HUD/
effect/dynamics globals and complete aggregate restoration remain open.

2026-10-11 GM update: actual loop counters, exit/pause/frame-step controls,
process masks/configuration and scene/GObj roots now have an explicit component;
GM mapped/combined-controller and accelerated-repeat state has another. See
[GM ownership](native_gm_snapshot_ownership.md) for real match-loop 16-tick
re-execution, exact generation resurrection ordering, resident identity guards
and the remaining shared-mask candidate consistency requirement. All 23 owners
capture/validate in live stock and interpreted eight-actor matches. Other GM
modules and world/effect/player/camera globals, multi-heap payloads and full
aggregate restoration/performance still remain open.

2026-10-11 clock/RNG update: the complete native input scheduler's logical
cadence, phases and flags now have direct scalar Save/Load, paired with the
actual shared RNG seed. See [clock and RNG ownership](native_clock_random_snapshot_ownership.md)
for source binding, callback/validation boundaries and exact 24-tick component
re-execution. All 21 owners capture/validate in live stock and interpreted
eight-actor matches. GM counters/pause/process masks, other simulation globals,
multiple heaps and aggregate restoration remain open; production uses the oracle.

2026-10-11 ordinary class update: actual class-piece OS chunks, size-class
tables/entries, free links and initialized class live/peak counts now have an
explicit serializer. See [ordinary class ownership](native_class_snapshot_ownership.md).
Its fixed physical-block directory adds constant work only when acquiring a
new chunk. Reserved pieces remain separate. OS Cell metadata, ordinary ObjAlloc
satellites, direct HSD allocations, class preparation and aggregate closure
remain required; production still uses the broad oracle.

2026-10-11 ordinary ObjAlloc update: the existing shared descriptor/cursor
component now also restores actual allocation blocks and their payload/free
links from either the fixed object arena or OS heap. It excludes reserved
payload and unused arena capacity. See [ObjAlloc storage](native_objalloc_snapshot_ownership.md).
OS Cell metadata, ID/global roots, direct HSD allocations and aggregate closure
remain open. Source graph fixtures pair this with reserved/class components.

2026-10-11 allocator-control update: Aurora now saves actual heap descriptors,
Cell headers/list topology and OS current selection separately from payloads.
Melee now saves the default ID bucket roots, paired with entry payload ownership.
See [heap and ID control ownership](native_hsd_control_snapshot_ownership.md).
The class/ObjAlloc growth fixtures no longer use copied heap companions; the
connected source fixture pairs OS/ID/class/ObjAlloc/reservation components with
exact re-save. HSD current selection, other globals/registries, direct mutable
HSD buffers, class preparation and aggregate orchestration remain required.

2026-10-11 scheduler update: actual GObj/GObjProc occupied pools, configured
scheduler head/tail/priority tables, invocation/deferred controls, render kinds
and the pointed process mask now have one explicit component. See
[scheduler ownership](native_gobj_snapshot_ownership.md). Serialized topology
validates before writes, including resurrection of currently free/reused slots.
A connected fixture restores real interpreted Fighter/Item variables and
callback bindings alongside this component, then repeats actual HSD dispatch.
Other userdata/graph/global owners and the full production aggregate remain
required; the broad oracle has not been replaced.

2026-10-11 variable graph payload update: texture palette pointer tables and
additive shape blend weights now use actual class/reserved storage, with exact
ordinary and reserved restore/reuse tests. Shape evaluation uses fixed host
scratch outside `.mstate`, with native display-list bounds and bit-identical
prior/new interpolation and emitted-value comparisons. See
[graph buffer ownership](native_graph_buffer_snapshot_ownership.md). Constraint
argument scratch, other direct HSD buffers, globals and the aggregate remain open.

2026-10-11 source resource registry update: occupied collection prefixes and
created nodes, retained identities/binding flags, and the collection generation
counter now have an explicit serializer. Same-epoch scene/model re-preparation
invalidates old records. A connected fixture restores eleven actual owners after
animated graph destruction and reuse, including interpreted Item state and
Fighter borrowers. See [resource registry ownership](native_article_resource_snapshot_ownership.md).
Cross-owner candidate closure and production aggregate integration remain open;
the broad page oracle is still active.

2026-10-11 collision owner update: runtime vertex/line/joint arrays now use
stable actual storage, with active-prefix serialization of both runtime and
writable source descriptors, joint/island roots, attributes, query state and
counters. Island payloads use the existing class owner. A connected fixture
restores real moving-platform queries after graph destruction and reuse. See
[collision ownership](native_collision_snapshot_ownership.md) for contracts
and the controlled attribution of Brinstar's old/new difference to the native
line-query stride correction. Aggregate
peer-graph validation and production integration remain open.

2026-10-11 follow-up control audit: the direct production `HSD_SetHeap` calls
are heap initialization/recreation and the `lbHeap_80015BD0`/`lbHeap_80015CA8`
allocation/free helpers, which restore the prior HSD selector before returning.
The OS selector is already saved separately. The final aggregate must capture
or enforce the HSD selector at its idle boundary; this audit alone does not
establish that check. RObj expression arguments remain a distinct mutable owner:
`expEvaluate` counts all flag bits but does not write values for several of
them, so its retained argument buffer cannot be omitted as regenerated scratch.

The supported first boundary is the start of a complete simulation tick in
`gm_801A4D34` (`src/melee/gm/gm_1A45.c`), before input consumption, with the previous
tick's HSD drawing and audio-intent finish complete. Restore must happen on the
game thread outside any GObj/native-script callback, allocator operation or draw
callback. Keep all normal simulation and HSD draw callbacks during resimulation;
Aurora presentation alone follows the existing suppression path.

2026-10-11 controller update: the actual HSD raw queue, Master/Copy/Game status
history, PadLib configuration and all rumble channel/pool/free-order state now
have an explicit 1,896-byte component for the normal layout. See
[controller ownership](native_controller_snapshot_ownership.md). Physical
device mapping and output suppression remain host state; Load emits no motor
commands. Twelve queued-input/rumble ticks reproduce exactly in the controlled
owner fixture; all nineteen components save/validate in both live eight-actor
scenarios. GM/LB clocks and scheduling, RNG, other global families, borrowed
definition closure and complete aggregate restoration/performance remain open.

The smallest coherent first representation is conservative typed runtime state:

1. Fixed match pools at stable addresses for each runtime object family, with
   occupancy, ordered free-slot metadata, generation counters and used/high-water
   counts stored separately from payload. Reserve their backing storage and the
   snapshot ring on the match transition. Normal match allocation/free then
   changes these pools; it must not grow an OS heap or reclaim backing memory.
2. Packed records for the occupied slots of each mutable pool, plus the complete
   logical free-slot order/occupancy metadata needed to reproduce the next
   allocation. Copy full actual runtime records initially. Reduce fields only
   when their reconstruction dependency is proven by the owner fixture.
3. Explicit module-owned global records for match/controller/player/CPU/stage/
   collision/camera/effect/scheduler state, including mutable arrays pointed to
   by globals. Initial .mstate capture can remain an oracle, but final ownership
   must be named rather than a PE section sweep.
4. Resident immutable descriptors, source model/animation/texture bytes, function
   tables and authored programs remain outside the ring. Mutable instances that
   point to them are included. Freeze asset/module lifetime for the match.

Stable pool addresses let current native pointers and callback slots be restored
coherently without a wholesale pointer-ID rewrite. The snapshot's pool/slot
identity is private native ownership metadata. It is not an authored pointer
API, a NativeCall escape hatch, a persistent save format, or a versioned
compatibility schema. On restore, write slot payload and logical metadata in
place before exposing restored roots to callbacks. No constructors, destructors,
effect creation or ordinary removal callbacks should run during the restore.
Objects created after the boundary vanish logically; objects freed since the
boundary resurrect in their original slots. Normal callbacks resume afterward.

The capacities are not inferred from today's live count. They must be selected
from declared game limits and measured creation churn, including all supported
articles/effects and stages. Pool exhaustion must produce the actual documented
allocation policy and deterministic cleanup; it cannot silently drop effects or
clamp article counts to make a test fit. Initial implementation can reject an
unsupported match before starting its simulation.

## Concrete mutable owners

| Owner | Necessary snapshot state and pointed storage | Source evidence / dependency |
| --- | --- | --- |
| Fighter | Full `Fighter`, including CPU state, motion/animation/command cursors, hit/hurt/capture state, velocities, collision, timers, union state, all installed callbacks, relationship pointers and generation. `native_module_state[256]`, native hitlag generations, attachments and dynamic masks participate. | `src/melee/ft/types.h`; `Fighter_Create`, `Fighter_NewSpawn_80068E40`, `Fighter_ChangeMotionState`, update/physics/collision callbacks and destruction in `fighter.c`. Lifetime validation reads actual registry membership plus `x8_spawnNum`. |
| Fighter satellites | `parts[MAX_FT_PARTS]`, per-instance attribute copy/backup, DObj list and `x203C` pointer list, actual live attachment graphs and dynamics descriptors. Rig entries above the physical bone count remain relevant when reserved parts borrow article nodes. | `fighter.c: Fighter_800679B0`, allocation/free paths; `native_fighter_attachment.c`; `native_article_resource.c: detach_borrowers`. Copy every owned reservation, not only drawing bones. |
| Player/pair | Full `StaticPlayer` match state, stale-move data, stock/state counters and `player_entity[2]`, both paired actors' states and relationship links. Restore slot-family mapping when mutable; keep registered immutable family definitions resident. | `src/melee/pl/player.h/.c`; `Player_GetEntityAtIndex`, `Player_80032610`, `ftpopospecialhi.c` and `ftnanaspecialhi.c` directly follow the primary/secondary pointers. |
| Item/article | Full `Item`, including `x1C` creation number, current owner, immutable creator identity/generation, author kind/Definition/logic, attachment and models, lifetime/collision/hitbox/victim state, callback slots and union or `native_article.state[512]`. Include article-runtime counters/global lists and dynamically allocated bones. | `src/melee/it/types.h`, `item.c` alloc/free/state changes, `native_article.c`, `native_held_article.c`. Creator identity and current owner are different contracts. |
| Article resource collection | `resources.generation`, each collection's owner/creator/full generations/Definition, count/created, reserved part/capture bindings and nodes; corresponding `ItemLink`, GObj and runtime model graphs. | `native_article_resource.c` defines the bounded .mstate pool. Cancellation/free detaches all borrowers before graphs disappear. Save the borrower and the borrowed graph together. |
| GObj and scheduler | All GObjs, their p/gx list links and priorities, user-data and HSD-object references, removal callbacks, render callback and GObjProc records. All head/tail tables and priority proc tables, per-proc flags and the rotating `HSD_GObj_804D783C` invocation marker. | `gobj.c`, `gobjplink.c`, `gobjgxlink.c`, `gobjproc.c`, `gobjuserdata.c`; `HSD_GObj_80390CFC` mutates invocation flags and supports delayed free/repriority/remove. Array pointer roots alone do not restore array contents. |
| Runtime joints/constraints | Complete live `HSD_JObj` tree and links/refcounts, flags, SRT, matrix, scale-vector/envelope-matrix storage, AObj/RObj pointers and mutable constraint/Rvalue records. Include any effect/particle joint roots. | `jobj.h/.c`, `robj.h/.c`. `HSD_JObjSetupMatrixSub` clears dirty flags and performs IK/RObj updates; `ftdynamics.c` consumes JObj matrices. A matrix is not automatically disposable draw scratch. |
| Animation | Every live `HSD_AObj` flags/frame/rate/rewind/end state, FObj list and owner; every FObj opcode/packet/cursor/interpolation/time/value/slope state. Include fighter command-info/event state separately. | `aobj.h/.c`, `fobj.h/.c`. FObj `ad` advances through resident bytes, while `ad_head` refers to the immutable start. Restoring only the current frame loses decoder state. |
| Materials/textures/geometry instances | Mutable DObj/MObj/TObj/POBJ instances, refcounts, attached AObjs, animation values, visibility/texture transforms and any writable morph/envelope runtime data. Resident image/TLUT/display-list/vertex descriptor bytes can be excluded only after checking writes. | `dobj.c`, `mobj.c`, `tobj.c`, `pobj.c`; TObj loads allocate TLUT pointer tables and animation changes texture/material fields. Distinguish descriptors from instances, not directories or pointer residency. |
| Dynamics | Entire active `DynamicsData` records and owning descriptor positions/bone indices, plus pool free order and global active/free chains. Do not reconstruct spring/history state from present joint transforms. | `ftdynamics.c`; `lb_00F9.c` manages `lb_804D63A0->entries[0x140]`, `cur_data`, effect/free structures; `lb/types.h` defines `DynamicsData`, `DynamicsDesc`, `BoneDynamicsDesc`. |
| Stage/world | Live Ground records and their full stage-specific unions, stage globals, dynamic attributes, hazard state/callbacks, stage GObjs/model graphs, moving-object bindings and counters. Stage descriptors can remain resident if immutable during the match. | `gr/types.h`, `ground.c`, concrete `gr*.c`; `grdynamicattr.c` is a four-entry mutable active/free pool. Battlefield alone does not establish all-stage closure. |
| Stage collision | Mutable runtime/source vertex, line and joint prefixes; active chain, flags/bounds/callbacks, island roots, query controls, dynamic attributes and counters. Per-fighter/item CollData remains in their records. | `mp/native_collision_state.c` owns stable capacities 2048/1536/256; `native_collision_snapshot.c` serializes active prefixes. `mpLibLoad` publishes/initializes them. Island payloads belong to class/OS companions; cross-owner closure remains open. |
| Effects/particles | EF_Effect records, live lists, allocation serials, native model-group member relations, generators, particles, AppSRT, user-data allocations and callbacks. Include particle/generator ID counters, linked heads, bank bindings if mutable and eight particle-attached JObj slots. | `eflib.c`, `native_model_effect.c`, `particle.c`, `generator.c`, `psappsrt.c`, `psstructs.h`. Particles execute command cursors, spawn children, alter joints and use callbacks; they are not uniformly cosmetic scratch. |
| Camera/HUD | Camera smoothing/history/mode state and subjects, CObj/WObj/LObj/fog/animation instances; mutable match HUD timers/counters and their GObjs/graphs. | `cm/camera.c: game_camera`, subject allocation; `cm/types.h`; `sysdolphin` CObj/WObj/LObj; `if/*`. Camera influences draw and some world queries; excluding it requires a concrete dependency proof. |
| RNG/input/tick scheduling | `seed` and the target of `seed_ptr`, HSD raw queue contents and queue metadata, master/game/copy pads, repeat/history state, `lb_0195.c` input-clock fields, match controller flags/time/pause/update masks, GM frame counters. | `random.c` modifies `*seed_ptr`; `controller.c/.h`; `lb_0195.c`; `gm_1A45.c`; `gmmain_lib`, VS controller and player code. Current OS wall time is external; deterministic sampled inputs and clock phases are state. |
| Audio/rumble | Preserve the existing audio-intent and rumble reconciliation protocol and deterministic simulation state. Restore may not restart actual audio devices or play speculative sounds. | `native_rollback_session.c` surrounds each tick with `HSD_AudioRollbackBegin/EndFrame` and resimulation rumble suppression. `.maudio` is separate from `.mstate` by the forced-section header; preserve its actual current protocol rather than bulk-copying host devices. |
| Allocation identities | Fighter spawn counter, Item creation counters, EF serials, resource generation, pool occupancy/free order, class instance counters/refcounts and texture/TLUT generation counters. | `fighter.c: Fighter_NewSpawn_80068E40`; `item.c`; `eflib.c`; `native_article_resource.c`; `class.c`; native `../aurora/lib/dolphin/gx/GXTexture.cpp: GXGetTextureStateRegions`. Wrong restored IDs break retained reference and lifetime checks even if positions match. |

This table is the minimum set of owner families to close, not a claim that every
private global in every stage/character has already been enumerated. A complete
registered-owner manifest and live write/creation census are required before
activating a production replacement. `native_semantic_state` is useful for
cross-backend differences, but its selected fields are not a complete serializer.

## Allocator closure is required before resurrection

There are three source-visible layers; saving an OS heap descriptor alone is
insufficient:

* `HSD_ObjAllocData` (`objalloc.h/.c`) tracks `freehead`, used/free/peak and limits.
  `HSD_ObjFree` writes its next-free pointer into the freed object's first word.
  `HSD_ObjAllocAddFree` grows through `HSD_MemAlloc` when the free list empties.
  Copying live payload without free-slot order/contents loses the allocator.
* `class.c` has a second size-class allocator: `memory_list`, `HSD_MemoryEntry`
  counts/links and free pieces. `hsdAllocMemPiece` can split a larger free piece;
  `hsdFreeMemPiece` writes a free-list pointer into released storage. JObj and
  material/texture/camera class instances use this class allocation path; they
  are not all covered by the GObj/Fighter/Item ObjAlloc descriptors. Class
  `nb_exist`/`nb_peak` and object refcounts also change with ownership.
* The actual native OS allocator is `../aurora/lib/dolphin/os/OSAlloc.cpp`, selected
  by `MELEE_AURORA_SOURCE` in the build. Its `HeapDesc` contains free/allocated
  heads and size; its 32-byte `Cell` headers live in the managed arena and carry
  prev/next/size/owner. Freeing coalesces cells. `OSGetHeapStateRegion` exports
  only the descriptor, and `OSGetCurrentHeapStateRegion` only the current heap
  selector. The existing page oracle restores the in-arena headers as well.
  The stock `extern/dolphin/src/dolphin/os/OSAlloc.c` is not the native runtime
  implementation and must not be used as the sole proof.

For a production fixed-pool migration, change these real match allocation paths
and call sites directly. Allocate every supported mutable family and variable
satellite from declared match pools; keep immutable import allocations in their
resident arenas. Pool metadata must not be overwritten by freed object payload.
Free-slot order must restore deterministically. During the match no pool can
fall through to arbitrary OS growth. Do not layer an allocator adapter over the
old arena and call that ownership closure.

Until that migration is complete, an experimental exact restore must include
all mutated backing allocator metadata/free-piece links as well as live data.
That is an oracle/transition requirement, not the proposed final representation.
Do not re-run ordinary destructors to recreate the old topology: destructors
detach borrowers, update global lists/counts and submit effects, producing a
different timeline while it is supposedly being restored.

## Copy, residency and recomputation decisions

Copy conservatively: full typed Fighter/Item/Ground records, live graph runtime
nodes, animation decoder state, dynamics history, particle/generator state,
collision runtime arrays, semantic globals, topology, allocator identities and
ordered free lists. Native C pointers to fixed pool slots and frozen resident
assets are valid private in-process references; code/module unload and content
replacement are explicit match transitions, not rollback operations.

Keep resident only with source provenance: DAT/FT animation descriptors, authored
FMP/bytecode/program/type tables, immutable textures and original vertex/display
lists. Shared imported article attributes are **not automatically immutable**:
`itsamusgrapple.c: it_802B75FC` writes scaled attributes/count into the article's
special-attribute storage. Such storage is necessary mutable state until its
instance ownership is correctly changed. Fighter `dat_attrs`/backup copies also
must be distinguished from their source descriptors.

One concrete exclusion candidate exists: `Fighter.x59C/x5A0` are two allocated
0x8000 animation scratch blocks per actor, but native branches of
`ftData_80085CD8` and `ftData_80085E50` read imported FigaTree pointers directly;
the large copy/relocate paths are under `#else`. Before eliminating their payload
from the match snapshot, enumerate all native readers/writers and preserve their
allocation/free identity or remove the unnecessary native allocation directly.
This evidence does not establish that every similar buffer is scratch.

Recomputation is deferred for these first snapshots:

* JObj matrices/envelopes/scale helpers: matrix setup includes IK/RObj evaluation,
  flags/refcounts and allocation of helper storage; dynamics/collision read the
  results. A dependency proof must establish the same invocation order and bits.
* Transformed collision arrays: moving lines have prior/current position and
  velocity/attachment history, not just a pure function of the current stage pose.
* Animation cursors: seeking to a frame may invoke callbacks or mutate state;
  FObj packet/interpolation state is explicit necessary data.
* Particle/effect state: particle commands spawn objects and alter attached JObjs;
  recreation consumes RNG and changes allocation/ID order.

GPU command/upload buffers, asynchronous presentation jobs, OS handles and
benchmark/VM coverage counters are host state outside snapshots. But HSD mutable
material/joint/camera/texture-generation changes caused while drawing remain
required. `GXGetTextureStateRegions` exports only the two TexObj/Tlut ID counters;
it is not a promise that all other renderer caches are stateless. Excluding a
cache requires proof that it is regenerated before any dependent read and does
not influence object lifetime or later gameplay. Preserve the current cache
invalidation/render sequencing while measuring.

## Fixture plan and implementation handoffs

Use the actual current native headers and owner bodies. Tests have fixed bounded
storage, fresh A: output, input/source hashes, hidden/muted client launches only
when coordinated. Fixture groups can be split by owner, but acceptance joins the
actual complete match and draw loop rather than calling eight fake actor updates.

1. **Registry and owner census (read-only first).** At match entry, enumerate every
   mutable pool/type, owner and satellite, .mstate globals and renderer exact
   state. Record sizeof/count/used/high-water/creation counts and each allocation's
   owner and mutability. Enumerate unclassified OS allocations separately. An
   unknown allocation reached by a callback must fail the ownership gate, not be
   silently treated as immutable. Audit both ObjAlloc and class-piece allocation.
2. **Save/load idempotence.** Save at a real tick boundary, perturb every record
   family, load, compare exact bytes of owned state plus semantic digests. Include
   inactive slots, non-head free slots, invocation marker and priority tables.
   A snapshot may omit inactive payload only if restored deterministic allocation
   initializes it before any read; expose partial-initialization violations.
3. **Creation/destruction/resurrection.** Save; create several objects, change
   priorities/links, free the running GObj through deferred removal, reuse its
   slot, create/free animation/material satellites; load. Assert original object
   identity/topology/refcounts/free order/counters. Repeat the same sequence and
   compare every allocation address/generation and allocator occupancy. Include
   model-root removal, shared reference ownership, class-piece split/coalesce
   transition evidence and explicit capacity exhaustion.
4. **References and borrowers.** Retire/reuse Item/Fighter/Definition identities,
   paired actors, captured victim, held items and article endpoint reservations.
   Save before each transition, load and verify all lifetime guards, user-data
   remove callbacks and capture/hitbox/part borrowers. No dangling restored graph
   or new-generation alias is acceptable.
5. **Animation/draw closure.** Save with active joint/material/texture animations,
   nontrivial FObj decoder cursors, constraints/IK, dirty matrices, dynamics,
   particles/generators/model effects and cameras. Run complete simulation and
   HSD draw, restore, repeat and compare next-tick outputs and ownership. Test a
   non-present replay and the final presented frame. Visual cache bytes may
   differ only through an explicitly proven rebuild contract.
6. **World/input/audio closure.** Exercise moving platforms and dynamic collision,
   stage hazards, RNG consumption, pads/repeat phases/pause, hitlag, projectile
   creation/removal, paired synchronization, audio starts/reuse/cancellation and
   rumble output suppression. Include stage/round transition refusal while a
   snapshot window exists.
7. **Page-oracle differential run.** In an untimed diagnostic batch, start from
   the same complete state, execute the same 15-tick window once from page restore
   and once from explicit restore. Compare semantic state each tick, owned raw
   state, live objects, allocation identities/order, callback coverage and final
   state after drawing. Page-level disagreement in unused/free padding is not
   automatically a gameplay bug, but every difference needs an owner-specific
   explanation and a next-allocation test. Do not mask arbitrary bytes to pass.
8. **Actual performance run.** Use all four level9 CPU Ice Climbers slots/eight
   live paired actors, the existing 600-warmup/64-window harness and the
   interpreted runtime with identical seed/input/rules. Measure explicit save,
   load, simulation, draw, audio/input and residual separately. Disable page
   protection in this timed mode once oracle parity passes. Fixed snapshot bytes,
   high-water counts and save/load p50/p95/p99/max must accompany timings. Hashes
   stay outside primary work windows. A sub100us target is not acceptance until
   measured, and passing snapshots does not imply total work is under16.667ms.

Suggested disjoint implementation owners: allocator/pool lifetime and slot metadata;
Fighter/Item satellites and retained references; HSD graph/animation/dynamics/effects;
world/collision/camera/input globals; root integration/oracle/benchmark. Implement
direct owner save/load operations and a central explicit aggregate with bounded
buffers. Avoid one monolithic serializer that discovers arbitrary pointers at
runtime or guesses ownership from the OS allocation list.

Open evidence required before choosing capacities/field compaction: per-family
native sizeof and live/high-water/churn census, complete stage/character global
manifest, mutable shared imported attribute inventory, variable particle userdata
ownership, class-piece allocation coverage, and dependency proof for any cache
or padding omission. This snapshot inventory does not establish independent
fighter behavior. The historical Samus constructor mismatch has since been
resolved in the separate [source Samus gameplay checkpoint](native_source_samus_gameplay_checkpoint.md);
it is no longer a blocker for that fighter's source-package preparation.

The [live component census](native_snapshot_live_census.md) now measures all
existing owner Save/Validate paths together in stock and eight-actor interpreted
matches. The retained shadow-render context is now owned separately from actual
callback activity, removing the previous three idle-boundary rejections. HSD
heap selection and persistent constraint-expression storage also have explicit
owners. This remains partial measurement and an integration handoff, not
aggregate acceptance; remaining globals, multi-heap coverage, candidate graph
closure and complete restore/resimulation performance are still required.


2026-10-11 shared-stage update: `stage_state.c` now owns the actual fixed
joint-update flags/counter and spawn cache alongside StageInfo. Ground/grLib
callsites use those records directly; the old 64-byte transition heap allocation
is removed. `native_stage_snapshot.c` adds explicit shared-state capture and
restore with generation/backing claims for changing roots, resident resource
identity checks and mutable ground scale/lighting colors. Its lease begins after the actual
stage startup queue drains, not at the earlier GM loop entry. See
[shared-stage ownership](native_stage_snapshot_ownership.md) for companion
ordering, class-span limitations and fixture/live evidence. Stage-private
records, mutable attributes outside Ground, hazard globals and aggregate graph
closure remain required; this is not completion of the whole Stage/world row.


2026-10-11 stage-private batch: the 28th component captures actual Corneria,
Venom, Green Greens and Brinstar Depths control records with paired actor
references and the indirect cannon-position cache. All 225 donor function bodies
are preserved except direct storage names. See
[stage-private hazard ownership](native_stage_hazard_snapshot_ownership.md) for
fixture, 2,400-sample stage and interpreted-eight-actor evidence and limits.
Other private globals, borrowed/variable payloads and aggregate restoration
remain unfinished; the 286-declaration audit is not an ownership-completion claim.


2026-10-11 Brinstar expansion: the existing stage-hazard component now owns the
actual cooldown, debris count, base positions and twenty bubble entries. Live
actor/joint borrowers have generation/class backing claims; dormant merged
joint aliases are preserved without dereference. The component is 2,800 bytes,
with no allocation/callbacks/draw during capture/restore. See
[Brinstar ownership](native_brinstar_snapshot_ownership.md) for the 76 added
restore checks, six 4,800-sample cohorts and interpreted-eight-actor evidence.
All 28 components still capture/validate; this does not complete stage-private
or aggregate restoration ownership.


2026-10-11 moving-stage expansion: the existing hazard component now includes
Kongo's two bank/fifteen motion-joint borrowers and Icicle Mountain's row table/
extra-row handles. Actual storage and initializer values are preserved; the
private Kongo parameter definition is removed from its included static header.
The component remains 2,800 bytes. See
[moving-stage ownership](native_moving_stage_snapshot_ownership.md) for the 113
additional restore checks, five live cohorts and interpreted-eight-actor proof.
All 28 components capture/validate; aggregate restoration remains unfinished.


2026-10-11 Green Greens satellite update: the 29th explicit component owns the
actual position/block arrays through optional Ground-slot storage, replacing
two heap allocation/free pairs. It serializes occupied records only: one grid
is 2,112 bytes; empty scenes are sixteen bytes. See
[grid ownership](native_green_grid_snapshot_ownership.md) for lifecycle,
controlled/live evidence, reservation versus payload size, and cell-borrower/
aggregate closure limits. This replaces the array payload gap; it does not
establish whole-stage graph restoration or complete-match performance.


2026-10-11 Green Greens active-cell borrowers: the existing 29th component now
includes GObj/Ground and GObj/Item generation pairs plus retained class spans
and known type identities for live cells. Capture/load check actor roles,
material ownership and joint agreement; inactive stale aliases are never
followed. One grid is now 6,192 bytes, empty scenes remain sixteen bytes.
See [grid ownership](native_green_grid_snapshot_ownership.md) for controlled
companion-reuse/restoration and 4,800-sample live evidence. This closes local
cell claims, not candidate-wide graph validation or whole-match restoration.


2026-10-11 secondary dynamics/wind owner: the 30th component captures the actual
320 bone-dynamics nodes, eight wind fields, internal free/active heads and
control values in 54,312 bytes. Direct heap allocations and the redundant old
initialization entry are removed; scene callers use `lbDynamics_Reset` directly.
Source arithmetic/update decisions are preserved. Candidate topology validates
before writes, without following raw model joints or external descriptor heads.
See [dynamics ownership](native_dynamics_snapshot_ownership.md) for exact owner
and live evidence; external descriptor/joint/wind-borrower agreement and complete
aggregate restoration remain open. The broad page oracle is still active.


2026-10-11 car/wind stage expansion: the existing hazard component now owns
Mute City's thirty car records/order/collision control and Castle's wind
pointer/angle/mode. Mute materials use Item generation/role claims; Castle
borrows a generic retained wind-slot reference from the dynamics owner. The
component is now 4,488 bytes, still thirty census components overall. The old
private mutable car header is removed and the sort's order[-1] access is fixed.
See [car/wind ownership](native_car_wind_snapshot_ownership.md) for connected
restoration, source-preserving changes and bounded live evidence. Candidate-
wide graph/collision agreement, other private/header state and full aggregate
restore/history/performance acceptance remain open.


2026-10-11 Poké Floats/Battlefield expansion: the existing hazard component owns
actual forty-four collision-map records with unchanged initial values and the
original forty-two-entry updater, plus Battlefield's mode/parameter pointer.
Scene identities are frozen; used joint borrowers have class span/type claims.
Unused tail pointer bits stay dormant. Safe generic class identity admission
replaces unchecked type-size reads. The component is 5,720 bytes, still thirty
census owners. See [Poké Floats ownership](native_pura_snapshot_ownership.md) for
connected restoration and bounded live evidence; class occupancy/model/collision
agreement and complete aggregate restoration/performance remain unfinished.


2026-10-11 Big Blue/coverage-audit update: the existing stage-private component
owns twelve actual track/offset records and captures the actual loaded x18 mode
flag without a live mirror. Indices/padding remain frozen by the scene lease;
other parameter bytes are not implicitly claimed. Component size is 5,728 bytes.
The compiler-backed stage audit now includes private headers/static locals and
conservative pointer aliases, finding 241 eligible declarations/115 candidate
roots across 32 units with no reported eligible location gaps. See
[Big Blue ownership](native_big_blue_snapshot_ownership.md) for evidence/limits;
this is not an interprocedural, pointee or complete snapshot coverage proof.
Shared generator backing and other world/variable/HUD/effect state, candidate-
wide closure and full aggregate rollback acceptance remain unfinished.


2026-10-11 shared stage generator: the 31st explicit component captures the actual
81-entry table, control/driver state and occupied-entry GObj/Item generation
claims in 5,896 bytes. The allocation-only helper/private header are removed;
source scanning/retirement decisions and constructor reset loops are preserved.
See [generator ownership](native_stage_generator_snapshot_ownership.md) for
controlled save/load, slot resurrection, source/body/section evidence and four
4,800-sample old/current rendered stage comparisons. Stock and interpreted
eight-actor scenarios pass all 31 Save/Validate components at ten boundaries;
previous/current and diagnostic-on/off traces match. No live aggregate Load is
performed. Corneria/Brinstar variable generator configurations and transformed
spawn buffers remain outside this table owner, alongside other world/HSD/HUD/
effect state, candidate-wide closure and full restore/history/performance work.
Production retains the broad page oracle.


2026-10-11 spawn configuration owner: the 32nd component captures the actual
configuration prefix and transformed segments used by Corneria and Brinstar,
including the constructor cursor. Configurations now use fixed scene storage;
original interpolation/RNG/callback decisions remain. Serialized sizes are
sixteen bytes empty, 200 for Corneria and 464 for Brinstar. See
[spawn ownership](native_stage_spawn_snapshot_ownership.md) for controlled source
update/restore and malformed/lease/occupancy evidence. Three 4,800-sample stage
comparisons pass, plus stock/interpreted eight-actor capture/validation of all
32 components. Previous/current and diagnostic-on/off traces match. Persistent
particle-generator configurations, other world/HSD/HUD/effect state and complete
candidate-wide restore/resimulation/performance remain open. Production retains
the broad page oracle; no live aggregate Load or total latency claim is made.


2026-10-11 Rainbow Cruise satellite update: the existing stage-hazard component
now owns actual seventeen moving-platform and twenty disappearing-platform
records, their owning GObj and parameter binding. Two direct HSD allocations are
removed; original initializers/state machines/collision callbacks remain with
direct storage binding. All 37 joints have class span/type claims, and the actual
Ground borrows wind through the dynamics owner. The component remains 5,728 bytes,
still 32 census components. See [Rainbow ownership](native_rainbow_snapshot_ownership.md)
for source-update/restoration, stale companion and bounded live evidence. Three
4,800-sample comparisons and stock/interpreted eight-actor Save/Validate pass with
previous/current and diagnostic-on/off trace equality. Remaining world/attribute/
HSD/HUD/effect state, reflection userdata, candidate-wide closure and complete
aggregate restore/resimulation/performance are unfinished. The broad page oracle
remains active; no live aggregate Load or complete latency claim is made.


2026-10-11 Fountain reflection owner: the existing stage-private component now
captures actual fixed reflection userdata, its matrix/image/parameter bindings,
camera GObj generation and retained CObj class span/type. The userdata heap
allocation is removed; source reflection rendering/matrix calculation remains.
The component is 5,768 bytes, still 32 census owners. GPU framebuffer pixels are
native renderer cache output keyed by the destination address; normal drawing
remains enabled. See [Fountain ownership](native_fountain_snapshot_ownership.md)
for source classification and local restoration guards. Three 4,800-sample
rendered comparisons and stock/interpreted eight-actor Save/Validate pass with
previous/current and diagnostic-on/off trace equality. Other world/HSD/HUD/effect
state, aggregate graph/floor/camera agreement, graphics regeneration during full
rollback and complete restore/performance remain open. The broad page oracle
remains in production; no live aggregate Load or full timing bound is claimed.


2026-10-11 effect controls: the 33rd component captures actual load mode,
asynchronous effect count, allocation serial, animation count/queue and eight
alpha/ID rows in 1,880 bytes. Table definitions moved to efdata.c; source consumers
and effect-library bodies remain unchanged. Active animation joints have class
claims; dormant queue tails retain raw pointer bits without being followed.
Parameter objects have GObj generation claims. See
[effect control ownership](native_effect_control_snapshot_ownership.md) for
controlled restoration and source setter evidence. Stock/interpreted eight-actor
capture/validation passes all 33 components with previous/current and diagnostic
on/off trace equality. Effect/queue payloads retain ObjAlloc ownership; particle/
generator/AppSRT globals, asset registries, candidate-wide graph agreement and
complete aggregate restore/performance remain unfinished. Production retains the
broad page oracle; no live aggregate Load or full timing bound is claimed.


2026-10-11 particle controls: the 34th component captures actual particle/
generator/AppSRT counters, scratch bits/ID cursor, sixteen particle heads,
pending/active generator heads, deferred SList root, camera/joint attachments and
prepared bank/configuration/hooks in 4,128 bytes. Actual globals reside in
particle_state.c, with direct source access and no live mirror. Ordinary physical
slot and class span/type claims connect to existing payload companions. AppSRT
now uses sizeof(HSD_psAppSRT), correcting the x64 184-byte record versus the old
164-byte request/168-byte slot. See
[particle ownership](native_particle_control_snapshot_ownership.md) for limits.
Three 4,800-sample comparisons and final stock/interpreted eight-actor Save/Validate
pass all 34 owners, with exact prior/current and diagnostic-on/off traces. List/
ID/callback/graph agreement, asset/resident/world/HSD/HUD closure and complete
aggregate restore/resimulation/graphics/performance remain open; broad pages
remain the production oracle. No live aggregate Load or full latency claim.


2026-10-11 retained-reference optimization: class and ordinary ObjAlloc queries
return spans by value, removing repeated output-buffer directory scans. Class
epoch zero remains valid at startup. All 34 components pass stock/interpreted
eight-actor observation checks; Fountain and Green Greens pass rendered old/new
comparisons. Sampled particle-control Save means fall to approximately 14 us.
See [reference cost checkpoint](native_snapshot_reference_cost.md) for exact
measurements, startup regression evidence and unchanged validation boundaries.
This is not aggregate restore or worst-case rollback acceptance.


2026-10-11 live self-restore update: the offline census can now retain distinct
captures for all 34 owners, invoke their actual Loads at the same boundary, and
compare every recapture byte for byte. Stock/interpreted eight-actor runs and
Fountain/Green Greens/Corneria pass 4,760 Load/recapture pairs, with unchanged
rendered traces. See [self-restore checkpoint](native_snapshot_self_restore.md)
for exact capture sizes, Load timings, host scratch and scope. Observation-only
mode remains the default. Same-boundary restoration does not establish history
rewind, changed-input resimulation, complete candidate graph closure or rollback
latency; production retains the broad oracle.


2026-10-11 candidate inspection update: generic pool validation now returns a
read-only view of saved generations and ordered occupied records. Candidate
lookup can resolve an older saved life while its live slot is free/reused,
without writing the pool or reading its current payload. See
[pool candidate views](native_pool_snapshot_view.md) for the replaced API, generic
lookup contract and verified boundaries. Actual pool/companion fixtures and all
34 live same-boundary components pass; full aggregate graph preflight and history
rewind remain open.


2026-10-11 actor preflight update: saved fighter/Ground/item records and saved
GObjs now have a bidirectional ownership check before diagnostic Loads. It reads
only candidate payloads and uses saved occupancy, so reused live slots do not
invalidate an older coherent graph. Other stage userdata-tag-3 roles remain
separate owners. See [actor graph preflight](native_snapshot_actor_graph.md) for
malformed/reuse tests and 140 passing live preflights with 4,760 Load/recapture
pairs. This adds one required aggregate relationship check; class/model/VM/other
candidate closure and genuine history rewind remain unfinished.


2026-10-12 class candidate update: caller-reserved indexes now expose saved class
payloads and candidate-prefix membership without per-tick allocation. Saved type
headers for main camera, stage/hazards/grid, effect queue and particle attachments
are checked before diagnostic Loads. See [class candidate views](native_class_snapshot_view.md)
for lease/index rules, later-block exclusion and 140 passing live preflights.
This does not prove class-piece occupancy or complete allocation/graph closure;
full history rewind/resimulation and rollback performance remain open.


2026-10-12 bounded older-capture update: the census can restore its own capture
after eight normal ticks, compare all 34 records, report raw target-oracle gaps,
and recover the complete current endpoint before continuing. Stock/interpreted
eight-actor and three stage cases pass 170 older Load/recapture pairs, including
creation/destruction/reuse, with unchanged continuing gameplay traces. See
[rewind diagnostic](native_snapshot_rewind_diagnostic.md) for the recovery-buffer
contract, raw coverage findings and limitations. This supersedes the earlier
current-boundary-only diagnostic precondition for this explicitly safeguarded
experiment; it does not add arbitrary candidate loading or run resimulation.


2026-10-12 VS/item/HUD update: component 35 captures actual VS controller, ambient
item-pick state/payloads and damage-HUD state in 14,088 bytes. Prepared callback/
descriptor/table bindings are leased; HUD GObj/class references are checked before
Load, with dormant pointer bits preserved. See
[versus control ownership](native_versus_control_snapshot_ownership.md) for actual
storage, source-body verification, fixtures and live old/current restoration.
All 35 owners pass stock/interpreted eight-actor and three-stage checks; the raw
comparison no longer identifies the three named control records. Remaining state
classification/ownership, full graph preflight and resimulation/performance remain
open; the production broad oracle is unchanged.


2026-10-12 particle display update: actual display serial, sixteen per-link sort
serials and seventeen cached non-edge heads now reside in psdisp_state.c and are
captured by the existing particle control component (4,824 bytes, +696). The
source sorter mutates particle chains, and AppSRT matrix caching depends on this
serial, so restoring only particle payloads was incomplete. Actual source sorter
replay, malformed/overlap checks, 35-owner stock/interpreted eight-actor and three
stage cohorts pass; display function bodies are unchanged except direct storage
names. See [particle ownership](native_particle_control_snapshot_ownership.md)
for the source-backed distinction between persistent caches and draw-call scratch.
The final particle-display-oct12 independent receipt checks source/exe pins,
sections and unchanged traces. Remaining gaps, complete candidate closure,
rendered/changed-input resimulation and total timing remain open. Production
continues to use the broad oracle.


2026-10-12 exact pool coverage update: the safeguarded older-capture report now
classifies every differing raw byte using restored current GObj/GObjProc/fighter/
Ground/item/ItemLink/AObj/FObj pool geometry. Full raw ranges/totals remain intact;
occupied payload, metadata and active free-order prefix are required state, while
free payload, stride padding and inactive free-order tail are counted separately.
All eight pool bindings/spans are validated before reporting; malformed/overlapping
registrations refuse. No state writes, callbacks, simulation or allocation occur.
Stock/interpreted eight-actor and Fountain/Green Greens/Corneria cases pass all35
restoration checks and exact continuing traces, with zero required-pool state
differences. Only 152/154/178/190/214 bytes remain unclassified in these intervals;
this is not a global completeness bound or permission to ignore those bytes.
See [pool coverage](native_snapshot_pool_coverage.md) and the final
pool-coverage-oct12-independent-verification receipt for precise counts/evidence.
Remaining state ownership, full candidate topology, compact resimulation and total
performance remain open. Production broad snapshots and endpoint recovery are
unchanged.


2026-10-12 texture identity update: component 36 captures/restores the actual
Aurora texture/TLUT identity counters in eight bytes, using its existing region
query at the GM/script idle boundary. No Aurora allocation/render decisions change;
all counter bit patterns, including pending overflow at zero, are preserved.
See [texture identity ownership](native_texture_identity_snapshot.md) and the
texture-identity-oct12 receipts. Source-counter repeat fixture (162 assertions),
stock/interpreted eight-actor and three-stage rendered cohorts pass all36 owners,
with final source/executable pins and unchanged traces. Required registered pool
state has zero raw differences; unclassified residuals are 149/151/175/188/212 bytes
in these intervals. These are not universal completeness or performance bounds.
Next extract a shared compact aggregate capture/load path from census and exercise
rendered/changed-input resimulation. Complete candidate closure, residual ownership,
other resource lifecycles and final timing remain open; production broad oracle
and the bounded diagnostic's endpoint recovery remain unchanged.


2026-10-12 shared compact API update: moved the direct 36-owner Capture/Load
implementation into native_match_snapshot_capture.c / native_match_snapshot_load.c
with native_match_snapshot.h. Removed old self-restore implementation/header and
updated callers, graph/build/test dependencies directly; no forwarding aliases.
The census now only owns reporting/comparison and uses shared calls for all captures
and Loads. Optional measurements/reporting are outside engine state, with null
capture mode avoiding timing/size-query instrumentation. Structured class preflight
replaces diagnostic printing. Source-equivalence verifies all36 expressions and
load order; real argument-span canaries, two eight-actor and three selectable-stage
cohorts pass 5,040 normal and 180 older pairs with unchanged traces and final pins.
See [shared core](native_match_snapshot_shared_core.md) for independent host-buffer
requirements and limits. Private own immutable captures still require resident
bindings; late Load failure is not atomic. Production remains broad, and the
compact history/resimulation driver plus full closure/timing are next required work.


2026-10-12 compact rendered resimulation checkpoint: the exclusive offline replay
experiment now restores the earlier compact state, rewinds the replay cursor and
runs eight original input/simulation/HSD draw ticks before comparing the catch-up
endpoint. Both stock/interpreted eight-actor and Fountain/Green Greens/Corneria
final cohorts pass all37 owner records, every intermediate traced entry and all
six broad oracle regions with ZERO byte differences. Broad recovery happens only
after these comparisons. Existing audio journaling preserves instance handles;
host action-transition observer history is checkpointed separately, without trace
masks. Component37 restores actual lighting roots/array/count/masks in 504 bytes.
Confirmed Euler W initialization and full-width temporary corrections preserve
all XYZ arithmetic; source body and final pin checks pass. See
[compact resimulation](native_compact_resimulation.md) for failure evidence,
contracts, exact receipts and limits. This supersedes the earlier no-old-world-tick
diagnostic limit only for this safeguarded same-input experiment. Production
remains broad; changed input, longer history, complete candidate/lifecycle closure
and final eight-actor / 15-tick timing remain required before replacement.
