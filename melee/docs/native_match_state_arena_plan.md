# Match state and simulation-only rollback

> Arena migration and copy-cost evidence. Production cutover is complete for the admitted paths and uses the confirmed-checkpoint policy. Current architecture/limits are in [current rollback](native_rollback_current.md); further optimization is deferred.

The production target is a compact, contiguous mutable match arena and rollback
ticks that do not render. The current 39-component serializer is an ownership
inventory and diagnostic implementation, not the desired permanent hot path.
State-layout work comes first; simulation/render separation follows it.
The selected production policy is one checkpoint at the contiguous confirmed
input boundary, not snapshots of every predicted/resimulated frame. See
`native_confirmed_checkpoint_plan.md`. Earlier per-frame ring experiments below
remain diagnostic evidence, not the production storage policy.

## State layout

- Reserve backing and history storage at the scene transition. Allocate runtime
  objects within that fixed backing; do not allocate from the operating system
  during a tick. Allocation bookkeeping and free lists are mutable match state.
- Put actual live fighter/item/GObj/procedure/animation records, runtime HSD
  objects, script VM state, particles and match controls in the arena. Change
  allocation and state accesses directly; do not maintain mirrored live records,
  compatibility adapters or a second authoritative world.
- Keep immutable asset definitions, vertex/texture source data, script programs,
  package catalogs and backend GPU resources outside the copied state. Their
  addresses must remain resident throughout the match/history lifetime.
- Consolidate remaining game globals into a small explicit match header or a
  few contiguous spans. Snapshot copies should not discover object graphs,
  rebuild typed views or repeatedly validate ownership.
- Restore to the same live addresses. Internal pointers then retain their
  meaning; external roots must be included in the header. Backend/audio resource
  lifetime and presentation observers remain separate from game state.
- Measure the real mutable high-water footprint before choosing arena/history
  sizes. Copying all existing oversized static capacities would recreate the
  problem: the item pool alone reserves 4,096 actor slots. Backing capacity and
  bytes copied need not be identical. Preserve stock allocation limits and give
  custom content explicit bounded allocation behavior.

Constructors, allocation admission and scene transitions establish ownership.
Keep exhaustive graph, alias, allocator and counter checks in diagnostic tests;
production own-history copies should not rediscover those invariants every tick.
This is a layout/ownership change, not simply deleting current checks and hoping
the same scattered storage remains correct.

Migration must cover all mutable allocation/root families, including stock and
script-driven articles, transformations and paired fighters. Continue using the
existing ownership census and broad oracle as diagnostic comparison evidence
until coverage is proven. Remove obsolete production snapshot/page-tracking and
serialization paths after their replacement is verified; no persistent snapshot
format, version conversion or compatibility layer is required.

## Simulation/render separation

Each fixed tick must advance gameplay, scripts, animation/state updates,
collision-relevant transforms, camera state and gameplay-relevant effects without
invoking mesh emission, GX command recording, graphics submission or presentation.
Render the resulting current state once for a displayed frame. Rendering must not
advance gameplay or consume gameplay RNG differently according to replay depth.

There is a confirmed source coupling to separate: the star-KO fighter callback
`ftDrawCommon_80080E18_inline2` camera-projects a position into `fp->cur_pos` and
updates the joint translation. Controlled original experiments diverged first at
sample 4037 when that drawing was skipped; shared RNG differed later at 4126,
with the seed changes occurring in simulation, not the preceding draw call.
See `native_match_launch.md`. This is evidence of state mutation inside drawing,
not proof that GPU rendering is required for RNG parity.

Move such updates into a deterministic simulation phase, preserving their
ordering relative to camera/animation/physics where it affects behavior. Audit
other draw callbacks and lazy state updates rather than special-casing only the
first observed star KO. Derived visual caches may be rebuilt for presentation;
state needed by future gameplay must advance on every tick, including hidden
replay ticks. Stock and custom fighter paths obey the same rule.

## Acceptance

- Arena save/load reproduces allocation continuation, globals, VM variables,
  partner state, actor/item lifetimes and gameplay-visible state. Exercise cold
  startup, ordinary gameplay, transformations, articles, effects and match end.
- Same-input and corrected-input arena replay agree with independent references;
  audio/resource side effects remain correctly journaled. Reject stale history
  at scene/content transitions.
- Compare render-every-tick and simulation-only replay repeatedly with identical
  seeds and inputs. Include star KOs, particles, camera changes, paired fighters
  and scripted articles. Require matching gameplay state and RNG, and verify
  that differing render frequency does not alter the result.
- Time actual saves, restores and fifteen simulation-only replay ticks with four
  Ice Climbers pairs. The complete fifteen-tick replay target is 16.67 ms.
  Sub-100-us capture/restore is not a
  requirement; minimize the actual cost and include checkpoint advancement in
  the full replay budget. Do not substitute warm copy-only probes
  or exclusion of required state work for this acceptance.

## First storage migration (2026-10-12)

`native_match_arena.c` implements live contiguous storage with an in-arena
allocation cursor, scene-lifetime admission and prebound private history slots.
Backing and first-touch costs are paid at scene construction. Saves copy the
used prefix and admitted external root spans; loads copy them back to the same
addresses. These operations do not enumerate objects, validate graphs or rebuild
references. Alignment/capacity checks remain on allocations. Root/history
alias checks occur at admission. These private slots are not an import format.

Actual GObj/procedure payloads, slot generations, free lists and scheduler tables
now reside in the scene arena. Scheduler dimensions use the scene configuration,
preserving the full public maxima when a scene requests them. The two pool
descriptors and changing scheduler controls are small admitted external roots.
Immutable root/table addresses and config remain fixed during the scene.

This is partial migration. The GObj/procedure pools still claim their full
4,096/16,384 capacities, unlike future demand-based actor payload allocation.
The small fixture copies 1,460,780 bytes including its external roots; that is
neither a complete match footprint nor a performance result. Do not copy the
4,096-item or 512-script-actor slabs wholesale as the next migration step.
Fighter/item/HSD/animation/VM/control owners still need migration and footprint
reduction before arena history can replace production rollback.

The broad migration oracle now covers the external arena as well as the old
heaps and global span. Production rollback remains broad; compact replay still
uses the existing serializers and original drawing. The new copy-only history
has been exercised against the actual GObj/procedure/scheduler implementation,
including same-address resurrection, repeated load, allocation continuation,
cursor rewind, initialized reclaimed bytes and stale-scene rejection.

Evidence: `A:/FreakMelee-test-archives/arena-oct12-gobj-final/receipt.json` passes
18 scenarios / 41,554 assertions. Five rebuilt CTests (arena, pool, scheduler,
history, stage-start) pass. `arena-oct12-replay-a/receipt.json` passes both stock
and interpreted eight-fighter cases, two repeated 15-tick intervals each,
78 component comparisons per case, exact fighter traces and zero raw differing
bytes. This is preservation evidence, not whole-match arena acceptance.
The final-source cold-start cohort `arena-oct12-cold-final/receipt.json` repeats
that comparison from input zero (including match-start state) and passes both
eight-fighter cases. `arena-oct12-correction-a/receipt.json` also passes fresh
reference and repeated corrected-input replay for stock and interpreted cases;
that cohort precedes the final diagnostic buffer-alias guard adjustment.

## HSD backing migration (2026-10-12)

Ordinary class-piece chunks, size-class tables and ordinary ObjAlloc chunks now
allocate directly from `native_hsd_arena.c`. Free lists still run the source
allocation/reuse/splitting behavior; new chunks consume the fixed contiguous
backing rather than the OS heap. Reserved article/construction pools remain
separate owners. No copied mirror or fallback heap allocation was introduced.

The HSD arena follows main-heap lifetime, not GObj scene lifetime. Class-library
amnesia retires class identity and private history, but retains backing needed
by other ordinary allocators. `HSD_CreateMainHeap` first forgets both families,
then resets backing. Scene changes retire admission before preload can change
class definitions/allocator registrations. Class initialization and ObjAlloc
registration reject changes while copy-only history is sealed; allocation of
objects using already admitted definitions remains legal during gameplay.

`nativeHsdArena_AdmitHistory` checks ordinary ownership/definitions once and
admits class counters, size-class roots, allocator descriptors and initialized
directory prefixes. Generic counted roots restore their captured lengths and
bytes, avoiding copies of 65,536 unused directory entries. Their count must be
captured state and their append writers enforce the declared capacity. Snapshot
save/load does no graph walk or per-object reconstruction. Explicit fixed
ObjSetHeap backing is admitted as a separate root when used; unclaimed external
chunks reject admission. Arena allocations preserve requested alignment above
64 bytes using the absolute backing address.

The retained diagnostic class/ObjAlloc components now include the shared cursor
and verify that restored cursors cover their owned blocks. Their candidates
must describe the same boundary. They are preservation oracles, not the final
production history path. The broad page oracle includes HSD backing during
migration, so it temporarily tracks reserved capacity as well as old state.

The ownership fixture exercises copy-only restore across both actual allocation
families, reclaimed/free bytes, metadata prefixes, counters, same-address
continuation, retained ordinary objects across class amnesia, physical heap reset
and stale scene/heap history. `hsd-span-oct12-owners-final/receipt.json` passes
88,570 checks, including malformed diagnostic cursor rejection. Arena, actual
GObj/scheduler and heap CTests also pass.

The census admits HSD history after real scene construction and measures 32 warm
single-span saves/loads before reusing its scratch for the component oracle.
`hsd-span-oct12-correction-final/receipt.json` passes stock and interpreted
eight-fighter fresh-reference and repeated corrected replay. Its constructed HSD
prefix is 1,178,432 bytes, 1,265,564 including 49 root spans; isolated saves are
44.538/46.741 us and loads 37.878/38.737 us respectively. These single-buffer
construction probes are NOT complete match/ring-history timing. The replay loop
still uses the 39-component serializers and rendering. Full-match integration,
actor/animation/VM/control migration, footprint reduction and render separation
remain required.
The renamed-source cold-start cohort `hsd-span-oct12-cold-final/receipt.json`
also passes both eight-fighter cases and two repeated fifteen-tick intervals.
`hsd-span-oct12-cold-final-verification.json` independently verifies both final
cold/corrected receipts, input/output hashes, exact traces and zero raw endpoint
differences against the same executable hash.


## Actor payload prefix migration (2026-10-12)

Fighter, Ground, Item and ItemLink payload arrays now occupy fixed windows in
one process-reserved backing created and first-touched at scene construction.
Stable base + slot * stride addressing, capacity, free order and generation
behavior remain unchanged. Metadata and descriptors are admitted as fixed roots
of scene history; each payload window is a counted root whose length is its
initialized high-water count. Free holes below that count remain copied. Unused
capacity above it is omitted, while full capacity remains available to gameplay.
Allocation updates the count and zeroes the payload; history restores the count
and allocator metadata, so slots first reached in a discarded future are safely
reinitialized on the same address and with the same generation on continuation.
The diagnostic packed pool header also restores that count. The broad page
oracle includes the new actor reservation during migration.

Source ownership evidence: actor-prefix-oct12-owners/receipt.json passes 19 cases
and 148,593 checks, including repeated actual typed-pool prefix restores and
allocation continuation. actor-prefix-oct12-pool/receipt.json passes 589,527
checks and 19 malformed-candidate cases. Heap, arena and actual scheduler CTests
pass. Updated standalone animation and GM fixtures construct the scene backing
explicitly: animation-final passes 73 cases / 668 checks; gm-final passes 2,019
checks plus 31,797 camera checks. All receipts are under
A:/FreakMelee-test-archives with the actor-prefix-oct12- prefix.

actor-prefix-oct12-cold and actor-prefix-oct12-correction both pass stock and
interpreted eight-fighter cases: exact traced replay, all 39 component endpoints
and zero raw differing bytes, including input-zero and corrected-input replay.
actor-prefix-oct12-cold-verification.json independently verifies input/output
hashes and the shared executable a195155a5ad0f061262e21281d4569e9e5669cf59c219440e302bca7d59fb4a9.

The census now probes both admitted HSD and scene spans after real construction.
The cold cohort's scene span copies 1,783,068 bytes (1,475,352 backing prefix plus
28 roots), saving in 62.509-64.981 us and loading in 50.462-54.309 us. HSD copies
1,265,564 bytes separately, saving in 40.413-41.247 us and loading in
35.816-38.159 us. These are 32 warm same-buffer operations at construction, NOT
a complete match, production history or a cold ring-buffer timing result.
The replay loop still uses the legacy serializers and rendered ticks; total
15-tick intervals remain roughly 33-41 ms in this cohort. No overall rollback
speedup is claimed yet. Animation storage, script VM/control owners and the
remaining mutable roots still need migration before final copy-only history
integration; render-state separation follows. Full pool reservations also
remain, including GObj/procedure capacities, though unused actor payload
capacity is no longer part of copy-only scene capture.


## Animation and fighter-VM copy history (2026-10-12)

AObj controllers and FObj curve decoders now occupy one process-reserved,
first-touched backing created at heap initialization. It retains main-heap
lifetime rather than being reset by scene binding. Descriptor/generation/free
order metadata and animation globals remain mutable; their initialized payload
prefixes are counted roots admitted to HSD copy history at scene construction.
The broad page oracle also includes the new backing during migration.

Fighter interpreter actors now track their initialized prefix. All lookups,
retirement and reporting honor that frontier, so actors first reached in a
rolled-back future cannot stay visible. Attach clears new/reused slots and keeps
the existing first-free allocation order. The legacy diagnostic header captures
the frontier; its restore clears the relevant old/saved extent before rebuilding
its active records. Copy-only admission validates live storage and execution
self pointers once, then captures inline variables, fault/tick state, installed
callbacks, cached bindings and identities directly with no rebuild on load.
Immutable programs and idle interpreter call/register scratch stay resident.
The item/effect interpreter owners have not yet been migrated.

The animation source fixture now exercises the copy-only path using actual
AObj/FObj interpretation, target transforms, callback counters and RNG: repeated
8-tick replay is exact, future slots restore their allocator frontier and
continue at the same addresses/generations, and heap recreation expires history.
Its receipt is A:/FreakMelee-test-archives/animation-span-oct12-owners/receipt.json
(92 cases, 1,144 checks). Focused rebuilt CTests pass for heap, arena, scheduler,
animation ownership, fighter scripts and animation/pose editors. The fighter
script fixture also copies a real bound actor, runs its counter behavior, loads
without rebuilding bindings, and verifies its packed recapture and next callback.
A stale fixture OSReport prototype and oversized stack-local prepared program
were corrected while rebuilding that target; no gameplay change was needed.

The real construction census verifies interpreter coverage independently: it
perturbs each actor's inline variables/storage identity and clears its state
pointer, then restores scene copy history before running callbacks. All eight
interpreted actors (608 bytes of authored variable state) recapture exactly.
Stock has no interpreted actors. animation-vm-span-oct12-cold/receipt.json and
animation-vm-span-oct12-correction/receipt.json both pass stock and interpreted
eight-fighter exact traces, all 39 owner endpoints and zero raw differing bytes.
Their inputs/outputs/shared executable are independently verified by
animation-vm-span-oct12-cold-verification.json. Executable SHA256:
00db4743309b61776d961260ffd26c3b5d4f98dbfeda214bbe6e174c5d51acd0.

The cold cohort's constructed HSD span is 1,566,004 copied bytes (58 roots),
including animation. Its 32 warm same-buffer saves take 47.775-50.744 us and
loads 44.709-46.909 us. Scene capture is 1,783,080 bytes for stock and 1,851,112
for interpreted fighters (31 roots); saves take 56.750-57.300 us, loads
51.053-53.069 us. These remain isolated construction probes, not whole-match
cold/ring history measurements. The full replay path still uses diagnostic
serializers and rendering, taking roughly 32-38 ms per 15-tick interval here.
No full rollback deadline or overall performance improvement is claimed.

Next: migrate item/effect script owners and remaining control roots, reduce
unused GObj/procedure payload copying, integrate complete prebound copy history,
and validate forward/corrected replay against the retained oracle. Simulation
and render-state separation still follow snapshot migration. The complete
PROJECT_PLAN objective remains active and incomplete.


## Item/effect VM and GObj prefix history (2026-10-12)

GObj and procedure payloads now occupy two more fixed windows in the shared
actor reservation, preserving base/slot/stride addresses, full capacity,
allocation order and generations. Their initialized prefixes are counted scene
roots rather than full-capacity arena allocations. Scheduler tables and slot/free
metadata remain in the small physical scene prefix. Broad migration regions
already track the complete actor backing; diagnostic buffer alias checks now
exclude that backing too. The source scheduler fixture's copied state falls
from 1,609,308 bytes to 397,196 with the same lifecycle/topology assertions.

Item and effect interpreter owners now track initialized extents. All live
lookup, boundary, pin, retirement and allocation scans honor those frontiers;
newly reached slots are initialized before becoming visible. Their diagnostic
headers retain the extents and clear both old/saved ranges before rebuilding
occupied records. Construction admission checks live program/peer/self-pointer
identity once, then captures each initialized owner prefix plus its complete
object index, scene epoch and frontier. Copy loads restore variables, callback
state, creator/attachment/member relations and cached execution/storage views
without reconstructing interpreter bindings. The source arrays remain fixed
process reservations and included in the broad global oracle during migration.

Evidence under A:/FreakMelee-test-archives:
- item-effect-span-oct12-gobj/receipt.json: 19 cases / 148,595 checks, including
  repeated copy restore, scheduler topology and allocation continuation.
- item-effect-span-oct12-item-final/receipt.json: 39 cases / 1,249 checks. Its
  added copy-history test perturbs inline variables, state pointers/identities
  and object indices, restores exact component bytes, discards a newly attached
  future actor, and recreates it twice at the same address/generation with
  initialized variables. Existing callback/fault/exhaustion coverage still runs.
- item-effect-span-oct12-effect-final/receipt.json: connected actual EF creation,
  source Tornado comparison and effect lifecycle coverage, 45 cases / 5,303
  checks / 1,551 model frees. Copy history restores perturbed variables, storage
  identity/pointers, creator views, member generations and lookup indices twice
  to exact component bytes. The fixture controls unavailable renderer/classes;
  it does not prove arbitrary effects in the complete game.

The older owner/scheduler fixtures were updated to link the current scene pools
and shared unavailable presentation boundary. The Item scheduler now constructs
manual test Items in the actual typed pool and verifies failed-constructor pool
occupancy/validity instead of counting obsolete ObjFree calls. Unexpected
class-backed practice-grid resolution remains a trapped fixture boundary.
Focused CTests for heap, arena, GObj, item actor and item scheduler all pass.

item-effect-span-oct12-cold/receipt.json and
item-effect-span-oct12-correction/receipt.json pass both stock and interpreted
eight-fighter exact traces, all 39 component endpoints and zero raw differing
bytes, including input zero and corrected-input replay. Independent receipt,
source/output and executable hash verification is recorded in
item-effect-span-oct12-cold-verification.json. Executable SHA256:
605718bd3e633a509b1bea06dedacf24866b9fe81cea6afc7643a51a3d8a5778.

The cold cohort's scene prefix is now 262,936 bytes, with 603,488 copied bytes for
stock and 671,520 for interpreted fighters (41 roots). Its 32 warm same-buffer
saves take 19.644/26.019 us and loads 19.941/21.428 us. The HSD/animation span
remains 1,566,004 copied bytes (58 roots), saving in 59.916/62.013 us and loading
in 49.022/49.550 us in that cohort. These isolated construction probes are NOT
a complete match or cold ring-buffer timing result; their combined operation
has not been measured as production history. The full replay loop still uses
the old component serializers and rendering, with roughly 32-42 ms 15-tick
intervals. No overall rollback deadline acceptance is claimed.

Next: admit the remaining match-control roots and ownership metadata, integrate
complete prebound copy-only history, and prove full forward/corrected replay
against the retained broad/component oracle. Scope must include reserved HSD
construction pools, ordinary allocation state and every remaining mutable
control/registry owner; current animation/HSD/scene spans alone are incomplete.
Renderer-state separation and complete production performance verification
remain after snapshot integration. The full PROJECT_PLAN goal stays active.


## Match controls, collision and reserved pools (2026-10-12)

Construction now preflights the full 39-component inventory once and admits
actual mutable spans directly from 23 control owners, plus collision and the
reserved HSD registry. Save/load do not invoke those serializers or validators.
The control spans include input/RNG/player state; GM loop/input-clock state;
camera/subjects; stage state, hazards/generator/spawn/start; dynamics;
effect/particle/light/material controls; versus/HUD/spawner state; controller
queues/rumble; RObj arguments; allocator/ID selection and texture identity.

The game-loop scheduling mask is borrowed by GObj. GObj captures that word;
the loop owner admits the prefix/suffix around it, preserving strict overlap
rejection. Ground colors include all nine GXColors, not just the first one.
The source RObj evaluator's fixed 100-float argument buffer is materialized at
construction, before sealing, so its address cannot first appear during replay.
This does not close the remaining selected OS heap metadata ownership.

Collision admission includes active runtime vertices/lines/joints AND mutable
source map arrays, the source MapCollData record, joint-list roots, bounds,
islands, dynamic attributes and query/counters. The map/epoch lease remains host
construction state. Reserved pools retain their real capacity: descriptors,
slot generations and exact free order are fixed roots; payloads copy their
initialized high-water prefixes. ObjAlloc's owner captures shared descriptors.
Registry bind/release refuse while history is sealed. At a scene transition,
private frame lifetime expires before ordered owner destruction/release; the
pool lifetimes remain live during destructors.

The construction diagnostic now overwrites every admitted arena prefix/root,
restores directly from its private frame without callbacks/validation, and
recaptures all 39 component records. Both HSD and scene spans match the complete
component inventory byte for byte for stock and interpreted eight-fighter
construction. This proves the registered spans restore; it does not prove that
every mutable game owner has been admitted or that production replay uses them.

Owner fixtures:

- collision-reservation-span-oct12-collision-b/receipt.json: actual collision
  algorithms, five cases / 171 checks / 14 refusals. The new copy-only case
  mutates moving-platform collision, source geometry, attributes and queries,
  then requires exact diagnostic recapture after two direct restores.
- collision-reservation-span-oct12-owners-c/receipt.json: 88,671 checks. The new
  reserved-pool copy test recreates future allocations at the same addresses,
  restores payloads/free order/failed claims/shared descriptor state, and rejects
  registry rebind/release until history expires. Existing synthetic stage
  cases now use actual nativeMatchScene backing transitions, rather than stale
  assumptions about static Ground/Item payload arrays.
- control-copy-proof-oct12-ctest.log: allocator, arena and GObj-consumer tests pass.

Final eight-fighter cold cohort: HSD/animation copies 1,566,004 bytes (58 roots);
scene copies 812,478 stock / 1,181,526 interpreted bytes (176 / 214 roots).
The interpreted construction probe measures 46.769 us save / 43.800 us load
for HSD and 38.391 us save / 34.600 us load for scene. Each is 32 warm saves
and loads into one reused buffer, NOT combined production or cold-ring timing.

Production remains on broad tracking and the rendered replay diagnostic remains
on the existing component history. Next coverage includes article-resource,
article-model/animation registries, Green Greens optional grid state and remaining
OS allocation state. Complete copy-only ring history and forward/corrected replay
through that path must precede production replacement. Rendering separation,
final performance acceptance and the full project goal remain unfinished.

Final integration evidence: control-copy-proof-oct12-final-cold/receipt.json
and control-copy-proof-oct12-final-correction/receipt.json pass stock and
interpreted eight-fighter cases, 480 controller samples, two 15-tick same-input
intervals beginning at input zero, and fresh/repeated corrected input at 240.
Every compared 39-component endpoint, continuing trace and broad oracle matches.
Both construction spans pass the overwrite/load/recapture probe. Independent
source/input/output/receipt/executable verification is recorded in
control-copy-proof-oct12-final-cold-verification.json. Executable SHA256:
669b5796db4490862937b9f39487026cc812ce3cc3f2f2b2864af5d2168408b9.
These replay intervals still use component serializers and original drawing;
median total intervals remain approximately 31-37 ms, not the final deadline.


## Resource/model registries, optional grids and fixed classes (2026-10-12)

Article-resource history now captures every collection prefix, including vacant
collections, and a counted node prefix for each collection. created is an actual
32-bit count; nodes are published only after a full record write. Restoring an
older count/owner hides discarded future nodes, and preserves first-free reuse.
The mutable generation counter is captured. Secondary-model history captures the
actual model registry and balanced matrix-claim context; its reserved allocation
bins/payloads remain owned by nativeHsdReservation. Pool geometry, package leases,
model/animation recipe ledgers and source descriptor identity are immutable in a
match. Their Prepare/Retire boundaries reject changes while scene history is sealed;
actual AObj/FObj controllers and graph objects have their existing copy owners.

Green Greens retains all 4,096 optional grid slots but copies the initialized
prefix and occupancy bitset. Bind clears a complete grid before publishing its
occupancy; Load restores the prefix count so discarded future grids are hidden.
The private component record stores that count too, and validates occupied slots
against it. No format version or compatibility path was introduced.

Two admission issues were found and resolved with evidence:

- The resource fixture's old page tracker failed BeginFrame with Windows error
  487. Diagnostic region zero crossed two AllocationBase values. Discovery had
  merged touching independent VirtualAlloc reservations. It now merges only
  overlapping regions; adjacent reservations stay separate for VirtualProtect.
  The fixture verifies each region's first/last allocation base and again passes
  both static and animated page-history restoration. This fixes the retained
  migration oracle; production still uses that older tracker for now.
- Green Greens failed at frame 102 when grMaterial lazily initialized its material
  class after admission. Scene construction now initializes the complete 20-class
  compiled inventory before sealing, using ClassInfoInit's existing initialized
  guard. No instances are allocated and existing counters/methods are preserved.
  Private grMaterial/refraction class data has explicit names and declarations;
  call sites reference it directly. Any future late class refusal now reports
  its name. This is general engine construction, not a Green Greens exception.

Evidence under A:/FreakMelee-test-archives/:

- registry-grid-span-oct12-owners/receipt.json: 88,689 checks, including 16,652
  grid checks / 21 refusals. Direct copy restores cell contents and removes a
  future grid from occupancy without changing full capacity.
- registry-grid-span-oct12-models/receipt.json: 657 checks / 14 cases / 137 graphs.
  Actual secondary-model destroy/rebuild, shared HSD and reserved graph copies,
  registry, Item model pointers and IDs restore directly; rebuilding after rewind
  returns the same graph address. The fixture's manually staged Item owner is an
  explicit boundary; this is not a complete match or model UI test.
- registry-grid-span-oct12-resources-final/receipt.json: static and animated actual
  resource cohorts pass (3,105 / 4,835 checks), including 39 source/VM animation
  pairs and resource-registry direct loads with discarded future collections.
  Registry checks are 177 / 180 with 14 refusals each. Peer components restore
  their own storage; resource registry restores through the new private frame.
- registry-grid-span-oct12-final-ctest.log: heap, arena, GObj and page tests pass.
- registry-grid-span-oct12-final-green/receipt.json: stock/interpreted eight-fighter
  Green Greens matches pass 480 samples and two 15-tick same-input intervals from
  inputs 0 and 120, including block/material spawning after class admission.
- registry-grid-span-oct12-final-correction/receipt.json: Final Destination passes
  stock/interpreted fresh and repeated corrected-input replay at input 240.
  Both live cohorts pass every component endpoint, full continuing traces, zero
  broad differing bytes, and overwrite/load/recapture proofs for admitted spans.
  Independent final pins/output/receipt verification is in
  registry-grid-span-oct12-final-green-verification.json. Executable SHA256:
  52fa74602fd79c600c4cbb9729ee4f3bc682c31591b389a40ba568d2348e8645.

Final Destination copied bytes: HSD 1,566,292 (62 roots), scene 842,586 stock /
1,211,634 interpreted (216 / 254 roots). Green Greens copied bytes: HSD 1,688,556
(62 roots), scene 1,000,306 stock / 1,369,354 interpreted (217 / 255 roots).
Isolated probes are warm construction copies, not complete production history;
the two live cohorts ran concurrently, so their timing is not a deadline proof.
Replay still uses component serializers and original HSD drawing. OS allocator
state, remaining global/mutable ownership audit, complete prebound ring history,
forward/corrected replay through it, and production replacement remain required.
Simulation/render separation and the full project acceptance follow that cutover.


## Selected OS heap control history (2026-10-12)

The selected HSD heap's descriptor, existing 32-byte Cell headers and Aurora
current-heap selection are now actual scene roots. Construction uses the existing
allocator snapshot to validate/enumerate them once. Normal arena Save/Load only
copy those fixed spans; they do not walk, sort, validate or serialize the heap.
Immutable asset payloads and unused heap capacity are not copied.

A retained-history lease freezes this heap's topology. Actor, script, animation
and HSD object allocation continues in existing fixed pools/arenas. A direct
OS allocation/free or global heap geometry change during the lease is an explicit
contract fault, with an operation/heap diagnostic; it cannot silently add an
uncaptured header. Current-heap selection remains mutable/captured. GM expires
private frame lifetime and directly releases this lease before ordered teardown.
The normal client is unchanged until this history admission is requested.
Any remaining runtime OS mutation site discovered by broader tests must be
migrated to construction or an appropriate fixed owner before production cutover.

Aurora adds OSLockHeapForHistory / OSUnlockHeapForHistory directly to its native
allocator implementation. Local Aurora commit aa99fc757c9a13d4596758f566cbba21be20b8a0
contains that change; the game's SDK declaration is updated too. No wrapper,
imported format version or runtime C compilation was added.

Evidence under A:/FreakMelee-test-archives/:

- os-heap-span-oct12-owners/receipt.json: 88,702 checks, including 65,710 OS checks.
  Real descriptor/Cell/current-selection bytes are overwritten, then restored
  directly and compared with the allocator's complete diagnostic snapshot twice.
  Separate hidden child processes confirm allocation, free and geometry mutation
  are rejected during the lease; releasing/reacquiring it succeeds.
- os-heap-span-oct12-ctest.log: heap, arena, GObj and page tests pass.
- os-heap-span-oct12-green/receipt.json: stock/interpreted eight-fighter Green Greens,
  480 samples and two 15-tick same-input intervals from inputs 0/120 pass.
- os-heap-span-oct12-correction/receipt.json: stock/interpreted Final Destination
  fresh/repeated corrected input at 240 passes. Both live cohorts have exact
  continuing traces, all 39 component endpoints and zero broad differing bytes;
  admitted-span overwrite/load/recapture probes also pass with the lease active.
  Independent source/input/output/receipt/executable checks are in
  os-heap-span-oct12-green-verification.json. Executable SHA256:
  3cbaa526ebbb8276a6ea0483c4d421a683e44a2b1a01487ce35a373c49565d75.

Final Destination scene copy is 844,950 stock / 1,213,614 interpreted bytes
(291 / 317 roots). Its HSD span remains 1,566,292 bytes. Green Greens scene is
1,002,670 stock / 1,371,334 interpreted bytes (292 / 318 roots), with HSD
1,688,556 bytes. The selected heap adds 2,364 stock / 1,980 interpreted bytes,
including descriptor and current selection, not its asset bodies. Separate warm
construction probes and concurrent live runs are not final performance proof.

This covers the selected allocator at the tested scene lifetimes, not every
other OS heap or every possible character/stage lifetime. Remaining global/owner
closure and complete copy-only ring replay must still be proven against retained
component/broad oracles. Production remains on page tracking and diagnostic replay
still uses component serializers plus original drawing. Next integration should
use the new frames for history and report every mismatched owner before recovery;
rendering separation and complete project/performance acceptance follow it.


## Copy-only history ring integration (2026-10-12)

NativeMatchHistory now stores two prebound NativeMatchArenaFrame spans per slot,
not component directories. Its save/load paths call only direct arena copies,
with private lifetime checks, sequential publication and future-frame invalidation.
Binding preflights output/metadata aliasing against every owner at construction.
All span lifetimes are checked before restore writes either span. Arena frames
record total copied bytes directly; no second sizing/object walk runs per save.

The census allocates/touches ring backing after admission. It retains packed
component state only once at each requested rewind target, for independent
recapture comparisons. Periodic diagnostic component validation remains outside
the ring's hot path. Reports identify two-span history copies rather than inventing
per-component serializer Load timings or graph validation that did not occur.
The old capture/load hooks and fake-world history test were removed. Tests now use
actual arenas, counted roots and allocation cursor rewind, including slot wrap,
replacement, stale leases, future invalidation and capture failure poisoning.

The new path exposes raw state omitted by the component inventory. PDB attribution
identified fighter damage logs: their complete small arrays, counters and flags
now live in FighterCollisionState, with all ftcoll call sites changed directly.
They are captured together because Fighter.dmg can borrow log records after append
counters reset. Item fighter-position/ECB tracking, selection coordinates, damage
logs/counters, contact flags and birth/category counters are also actual roots.
No collision or attack rules were changed.

Evidence under A:/FreakMelee-test-archives/:

- copy-ring-oct12-item-ctest.log: actual-arena history, arena, heap and GObj tests pass.
- copy-ring-oct12-owners/receipt.json: 88,702 allocator/owner checks pass after the
  explicit control fixture's declared buffers were refreshed for new item roots.
- copy-ring-oct12-item-correction/receipt.json: stock/interpreted eight-fighter
  Final Destination, 480 samples, fresh and repeated corrected input at 240.
  Each case publishes 512 captures / 3 direct restores without failure.
- copy-ring-oct12-item-green/receipt.json: stock/interpreted eight-fighter Green
  Greens, 480 samples and two 15-tick intervals from inputs 0/120. Each case has
  511 captures / 4 direct restores. Every 39-component endpoint, full continuing
  trace and resimulation broad endpoint match exactly, with zero raw differences
  after catch-up. Direct copy history, not component history, performed rewind.
- Independent final verification: copy-ring-oct12-item-correction-verification.json.
  Executable SHA256: 36ed1a33f3a4b5c0a87af91e5bac34f2eb93bd13792abf300fd3b40ed99ca5b9.
- Earlier mapping evidence: copy-ring-oct12-residual-modules.json and
  copy-ring-oct12-final-residual-modules.json attribute raw contribution ranges;
  they do not prove semantic exclusions. Particle/PObj rendering caches and
  animation traversal stacks remain under review. Discarded-future storage also
  differs at the old boundary; zero pool-state differences are preserved.

Actual copied peak: Final Destination 2,833,675 stock / 3,210,907 interpreted
bytes; Green Greens 3,250,555 stock / 3,527,563 interpreted bytes. The reservation
covers full admitted capacities: sixteen slots reserve approximately 1.306 GB stock
or 1.349 GB interpreted. This conservatively large backing is NOT copied every
frame, but its memory cost needs improvement before shipping. No existing pool
capacity was reduced merely to shrink the snapshot reservation.

The uncontended earlier Final Destination cold run measured interpreted direct
restores at 281.5/301.9 us and fifteen saves at 4.765/5.173 ms (318/345 us each).
This is real ring traffic, superseding isolated warm-buffer timing as the relevant
estimate. Full fifteen-tick rendered replay was 28.7/29.1 ms; drawing cost about
14.3-14.6 ms. Final Green/correction cohorts ran concurrently and are fidelity
checks, not deadline proof. Sub-100-us save/load and 15 simulation-only ticks
within 16.67 ms remain unproven. Ring storage/traffic needs further optimization.

Production netplay remains on the broad page tracker. Before replacing it, finish
residual global/owner closure and character/stage/lifetime coverage; classify
scratch from source rather than masking differences; exercise match teardown and
scripted content under retained history. Rendering/state separation is the next
large tick-cost reduction, with overall project acceptance still open.


## Private fighter traversal scratch (2026-10-12)

The three original ftAnim ancestor arrays are now caller-local stacks. Every
walk starts at depth zero and writes each occupied entry before reading it.
All real call sites pass their own array directly, including metal/part/Kirby
walks; no compatibility entry point or new gameplay rule was introduced.
This removes 720 bytes of traversal scratch from persistent globals and makes
interleaved/nested traversals independent instead of adding scratch to history.

Evidence in A:/FreakMelee-test-archives/: animation-tree-oct12-ctest.log passes
history, arena, heap, GObj and the actual three-type traversal fixture (129
checks, dirty scratch, interleaved walks, supported maximum depth).
animation-stack-oct12-correction/receipt.json and animation-stack-oct12-green/
receipt.json pass stock/interpreted eight-fighter 480-sample comparisons,
15-tick same/corrected-input history rewinds, all 39 component endpoints and
continuing traces with zero broad endpoint differences after catch-up.
Independent pins are in animation-stack-oct12-correction-verification.json;
executable SHA256 4f5c2bf52cf9527800621722051059bb1872246625971d967e12656c7149dec2.
These concurrent cohorts are fidelity evidence, not deadline measurements.

The old native_engine_fighter_pose/native_model_graph fixtures were also tried;
they fail on earlier NativeSourcePackage resolver-layout drift, not traversal
signatures. Their obsolete ancestor globals were removed, but this checkpoint
does not claim those broader fixtures pass. Actual game compilation and live
replay comparisons pass; fixture refresh remains separate follow-up work.

Pool audit: pool-capacities-oct12.txt records actual native sizeof values.
Largest fixed payloads are item 4096 x 6416 = 26,279,936 bytes and optional
Green grid 4096 x 2088 = 8,552,448 bytes. Core actor capacities were native
choices, not retail object-count limits. Original HSD ObjAlloc grows from the
heap with count-limit flags initially disabled; stock item spawn categories
have separate ItCo data thresholds, recorded from the USA 1.02 ISO in
retail-item-limits-oct12.json. No global item cap is implied by those thresholds.

User approved reducing fixed capacities (fighter 16; item 512 or 1024; broadly
4-8 times smaller), with measured stock demand/headroom checked first. Next
measure actual pool peaks and hot/cold snapshot copy cost before choosing the
remaining limits. The conservative 1.35 GB sixteen-frame backing is not a
retail requirement. Production rollback still uses page tracking; no claim of
complete owner closure or production cutover follows from this checkpoint.


## Match pool budgets and copy cost (2026-10-12)

User authorized substantially smaller fixed pools. Native capacities now are:
fighter 16 (was 64); Ground 512 (4096); Item 1024 (4096); ItemLink 1024 (4096);
GObj 1024 (4096); GObjProc 4096 (16384); AObj 2048 (4096); FObj 4096 (16384).
Script actor capacities are fighter 16, item 128, effect 128 (64/512/512).
Per-actor authorable script state capacity remains 8192 bytes. Optional Green
grid follows Ground's 512 slots, not the shared GObj ceiling. Scene arena is
2 MiB (8); ordinary HSD arena is 4 MiB (16). Generic pool validation ceiling
and per-asset measured model reservation demands are not preallocated arrays
and were not blindly divided. No runtime backing allocation was introduced.

Added construction/periodic actual pool reporting to native_snapshot_census.
Baseline eight-fighter Green Greens peaks: fighter 8, Ground 26, Item 23,
GObj 130, GObjProc 470, AObj 786, FObj 2482. Consequently controllers were
cut twofold, not fourfold; an eightfold channel reduction would already fail
the measured stock workload. These observations are not all-stage/all-fighter
maxima and cannot prove arbitrary custom content fits the new budgets.

The quiet late rewind exposed missing ifMagnify_804A1DE0 state. Its flags are
written by drawing, then consumed by Fighter's offscreen damage/bottom camera
logic and player statistics. The failed first cohort had two differing player
bytes (PDB attribution in pool-budget-oct12-residuals.json), not arena/pool
exhaustion. The real complete magnifier record is now a direct arena root.
The 39-component diagnostic format remains unchanged; the broad independent
oracle verifies this additional root. No gameplay rule or visual flag was masked.
This is a concrete counterexample to treating all HUD state as removable.

Final evidence in A:/FreakMelee-test-archives/:
- pool-budget-oct12-final-correction/receipt.json: stock/interpreted eight fighters,
  480 samples, fresh/repeated input correction and exact continuing traces.
- pool-budget-oct12-final-green/receipt.json: 600 samples, 15-tick intervals from
  inputs 240/360, exact continuing traces, all 39 component endpoints and zero
  broad endpoint differences after catch-up. Corrections and this cohort ran
  concurrently and are fidelity tests, not performance deadlines.
- pool-budget-oct12-quiet-magnifier/receipt.json: same Green Greens intervals,
  per-frame trace/object observers disabled; endpoints and raw state still exact.
  This timing cohort ran separately, before the other tests/build work.
- Independent input/output/executable pin verification is in
  pool-budget-oct12-final-correction-verification.json. Current executable SHA256
  ebb1ea24c84d82e7e7fc5edc72472092d08e5ac8b45987c94542a91ee55db335.
- pool-budget-oct12-final-cross-capacity.json: baseline vs reduced capacity stock
  and interpreted reference gameplay traces match exactly for the first 480
  inputs (fighter/partner/action/damage/item/RNG categories actually present).
- pool-budget-oct12-ctest.log: seven history/arena/heap/pool/GObj/animation tests.
- pool-budget-oct12-owners/receipt.json: 88,702 actual allocator/owner checks.
  Initial failed quiet run is retained in pool-budget-oct12-quiet, not accepted.

Green Greens peak copied bytes are now 2,690,907 stock / 2,967,819 interpreted,
versus 3,250,555 / 3,527,563. Interpreted reduction is 559,744 bytes (15.87%).
History backing is 341,598,720 stock / 385,170,560 interpreted bytes, versus
1,305,784,832 / 1,349,356,672. It still reserves full admitted capacities for
all sixteen slots and requires further reduction before release. No claim that
the remaining 2.97 MB is exclusively gameplay state has been established.

Measured plain CRT memcpy on this i7-10700K (memcpy-ring-oct12.txt), copying
3,527,563 bytes after allocation/first touch, five batches of 512 copies:
one reused buffer save/load medians 128.6/128.2 us; compact sixteen-slot ring
125.2/311.0 us; original 84,334,792-byte slot spacing 135.4/331.0 us. Reading
cold history alone can therefore cost about 300 us; warm single-buffer numbers
are not the realistic restore baseline. Benchmark sources/commands are retained
in .local/memcpy-ring-oct12.*. It is not an in-game gameplay-cost measurement.

Actual reduced interpreted quiet ring restores were 273.2/301.7 us; fifteen
saves totaled 4.660/4.665 ms (about 311 us each). Source dispersion/cache effects
vs the plain hot-source benchmark remain to be attributed. Endpoint Save
explicitly refuses initialized page tracking, so these direct-history timings
are not page-fault tracking costs. Full fifteen rendered ticks were 30.94/32.14
ms, including 17.56/18.37 ms drawing and 7.64/7.93 ms simulation. Rendering
separation remains a major required step; neither sub-100-us save/load nor the
15 simulation-only ticks within 16.67 ms target is established.

Production netplay still uses page tracking. Remaining snapshot cutover work:
owner/scratch closure, broader character/stage/longer-lifetime and teardown
coverage, tighter history storage, and gameplay/render separation. Only the
proven 0.56 MB reduction is counted; further exclusions need source ownership
evidence plus unobserved and observed replay comparisons.


## Match-history save-path profiling - 2026-10-12

Completed user-requested save attribution, documented in
melee/docs/native_match_save_profile.md. Optional CMake
MELEE_PROFILE_MATCH_SAVE (default OFF) instruments history guards/slot selection,
both arena guards, prefix memcpy, root length/copy loops, publication, dispatch
and callsite. Detailed mode records every root separately; coarse mode avoids
hundreds of clock pairs. Host profiler state is outside captured sections.
Post-replay copy controls use preallocated scratch, outside replay timing.

Eight interpreted Ice Climbers actors, Green Greens, two 15-tick replay
intervals: history Save 290.253/297.300 us. HSD prefix 135.247/140.447 us;
HSD 62 roots 41.873/40.940; scene prefix 9.327/8.193;
scene 329 roots 102.400/106.200. Guards/publication/dispatch about 1 us.
393 memcpy calls include two prefixes, 368 nonempty roots and 23 empty roots.
No runtime allocation/hash/compression/graph validation/page tracker in Save.

Warm controlled identical-byte/destination experiments: packed 393 chunks
242.394/231.584 us versus single memcpy 123.927/115.662 us. Live gather
236.686/225.784 us. Walking lengths/offsets without data copies 0.213/0.212 us
(does not include frame-length stores). Standalone 393 zero-byte calls cost
0.756 us; two equal chunks already cost 258.214 us versus one 123.436 us.
This implicates copy-pattern/size-dependent memory behavior, not mere call or
span-walk overhead. Exact CRT/cache mechanism remains unproven. Do not add
per-tick gather staging on this evidence; it would add another copy.

Coarse/detailed/inactive-before/inactive-after cohorts all PASS stock and
interpreted eight actors, 600 inputs, two intervals, all 39 component endpoints,
broad raw endpoints (0 differing bytes) and audio reconciliation. Independently
verified source/executable/receipt/log pins before rebuilding. Detailed clocks
are intrusive (312-387 us totals), not the normal budget. Scheduling/cache
variance remains; no universal timing guarantee is claimed.

Artifacts A:/FreakMelee-test-archives/save-profile-oct12-{final-control-a,
coarse,detail,final-control-b}; independent verification in
save-profile-oct12-final-control-a-verification.json. Instrumented executable
786e6c0e19da3f19f1a9e2d475b6e89d3b912c5e7bd08156f1f49fb44593f5d8
retained in save-profile-oct12-binary. Standalone memcpy-chunks-oct12.txt.

Restored profiling-OFF release; six focused CTests PASS and repeated normal
two-interval stock/interpreted cohort PASS. Independent verification:
save-profile-oct12-normal-verification.json. Normal executable
12461b1ddd9eab559053573190066372597a80e948f1fc398ba810bcb1e6c396.

Goal remains active: production cutover, snapshot closure, broader lifetime
coverage, contiguous mutable ownership/tighter storage, and simulation without
rendering remain required. This checkpoint profiles Save; it does not claim
sub-100-us save/load or fifteen simulation-only ticks within 16.67 ms.

Production confirmed-checkpoint cutover is locally verified in
`checkpoint-hud-oct12-final-c` (2026-10-12). Missing private timer, stock HUD and
score-predicate state was admitted from actual storage; early and late rendered
restore intervals now match all 39 owners and the broad memory oracle exactly.
Retained-object sudden death renews history leases without resetting payload.
Class amnesia clears retired light-cache pointers before the new admission.
See `native_confirmed_checkpoint_plan.md` for exact peer/phase results and scope.
The next performance target is simulation/render separation. No new copy-time
or fifteen-tick deadline claim is made by this correctness checkpoint.
