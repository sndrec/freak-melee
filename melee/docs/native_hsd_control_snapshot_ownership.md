# HSD control state and OS heap ownership

2026-10-11 checkpoint. Aurora now has an explicit OS heap-header component;
Melee has an explicit default ID-table root component. They pair with the
existing ordinary class, ordinary ObjAlloc and reserved graph components.
Production rollback still uses the broad page oracle. Aggregate integration,
other module roots and remaining direct mutable HSD allocations remain work.

## OS heap ownership

`aurora/lib/dolphin/os/OSAlloc.cpp` retains the actual allocation, splitting,
free/coalesce and list order algorithms. Snapshot lifetime epochs advance only
when allocator/heap geometry changes: InitAlloc, successful CreateHeap,
DestroyHeap, AddToHeap and AllocFixed. Normal allocation/free and current-heap
selection do not add tracking work or retire a snapshot.

The private snapshot stores the selected heap's descriptor identity, size,
free/allocated heads, arena identity and lifetime epoch; the OS current-heap
selector; and every actual 32-byte Cell header at its native address. Payload
bytes belong to their explicit model/game owners, not the OS component. On this
64-bit build, the representation is a 64-byte header plus 40 bytes per Cell.
It excludes all allocation payloads and unallocated arena bytes.

Snapshot operations use fixed host scratch for at most 65,536 cells. No vector,
runtime allocation, page protection, object construction or callbacks run.
They are single-threaded idle-boundary operations, matching the allocator's
existing thread contract. Current cell collection is sorted by address;
candidate topology lookup uses binary search. This is O(n log n) metadata work,
not a constant-time save/load or established sub-100us claim. Normal allocation
paths do not use this scratch. Profile the aggregate before optimizing this
representation or claiming the fifteen-resimulation-tick frame deadline.

Validation collects the healthy current free/allocated lists, verifies size,
owner, previous links, bounds and total byte accounting, sorts them and merges
adjacent extents into actual owned ranges. Candidate headers must fit exactly
inside those ranges, with no overlap and the same total owned bytes. Candidate
lists are followed through serialized records, never candidate memory pointers.
Owner/previous links, complete reachability and increasing free-list order must
hold. Cycles, foreign heap cells, truncation, wrong owner/lifetime/geometry and
buffers overlapping any OS arena/control/scratch storage reject before writes.
This also supports disjoint ownership after a fixed carve; it does not assume
the arena's entire address range belongs to the selected heap.

Prevalidate every component before restoring. OS headers must load before old
payload copies: a current allocation may have split or merged an older free
range, so its current payload boundaries need not match the snapshot's Cell
addresses. After OS restoration, class/ObjAlloc/reservation payloads must
restore before any allocation, free, script callback, ID lookup or draw resumes.
Loading headers alone does not restore gameplay. The HSD current-heap selector
is a separate game global; it is not the OS selector saved here.

`OSHeapSnapshot.h` is the same public C declaration in Aurora and Melee's
external Dolphin headers; equality is independently checked. There are no
format versions or pointer translation/compatibility layers.

## Default ID roots

`native_id_snapshot.h` defines a private 816-byte component: the 101 native
default-table bucket heads and a lifetime epoch. Actual ID entry bytes already
belong to ordinary ObjAlloc or reserved backing. Their full-width keys, data
references and links restore through those owners. Setup/forget retires root
history; normal insert/replace/remove does not.

Root save/load never traverses live or candidate entry graphs. Buffer checks
consult actual ordinary/reserved backing catalogs instead, so they remain safe
between companion restores when entry bytes and roots disagree temporarily.
Metadata overlap, malformed aligned roots, wrong size/epoch and active
construction reject. Candidate pointer graph closure is not asserted by this
small root component. Callbacks/lookup must wait for the complete aggregate.
Native call-site audit currently finds game uses of the default table; any
caller-owned alternative table belongs to its caller's explicit state.

Ordinary/reserved backing-independence queries are the owners' actual range
checks, shared with their serializers. There is no callback reconstruction,
alias allocator or compatibility wrapper. Other module roots, source registries,
the HSD selector, direct HSD buffers and script Fighter/Item state still need
aggregate orchestration. Interpreter variables, timers, persistent state,
callbacks and actor/resource relationships cannot be dropped.

## Evidence

All fixtures use actual OS/class/ObjAlloc/ID/reservation implementations.
`hsd_reservation_snapshot` now restores class and object heap-growth cases
through the actual OS header component, with no copied-heap companion.
Colliding full-width IDs exercise removals, reused slots, growth and root/entry
restore; absent and present-null lookup remain distinct. The standalone OS
cohort covers split/coalesce, payload preservation (headers only), next
allocation addresses, two heaps, foreign ownership, cycles, geometry retirement,
disjoint ownership after carving and scratch exhaustion.

The connected source resource fixture now pairs all five components around
actual imported Arrow/Boomerang graphs, an ordinary JObj, Vec/Mtx, and ID
removal/insertion/growth. The old ID resolves to the original JObj; new IDs
disappear. Every component re-saves exactly. Its broad-page destruction/replay
and 39 native/VM animation comparisons remain independent checks.

Final evidence under `A:/FreakMelee-test-archives/hsd-controls-oct11-*`:

- `owner-a`: 66,088 checks, including 108 class checks, 68 OS-backed ObjAlloc
  checks, 32 ID checks/three refusals, and 65,644 OS checks/nine refusals.
  Most OS checks are the 65,536 successful minimum allocations used to reach
  the real scratch ceiling; this is not 65,644 distinct gameplay cases.
  The four-cell metadata component is 224 bytes, not a game-state estimate.
- `resource-a`: ordinary 2,748 checks/14 cases; animated 4,475 checks/15
  cases, with 39 native/VM animation pairs and exact five-component restore.
- `census-a`: 14 cases/48 read-only ownership checks.
- `stages-a`: Battlefield, Great Bay and Brinstar each match the previous
  client across 2,000 samples with Link/Samus/Ness/Ice Climbers and medium
  items; every populated trace family matches, with no census failures.
- `source-a`: interpreted Samus/Link/Young Link/Ness each pass their stock
  comparisons across 1,800 controller samples.
- `build-a.log` and `focused-ctests.log`: client/content/source-assets/ObjAlloc/
  census/heap builds succeed; ObjAlloc, heap and census CTests pass.
- `independent-verification.json`: all 351/1596/3339/214/55 input pin sets,
  baseline bundle hashes, seven compiled owner-source matches and the OS
  snapshot header mirror match.

No whole-match size/timing, complete explicit aggregate, multi-PC acceptance
or eight-actor deadline is established here. Goal and broad production oracle
remain active.

## Heap selector and constraint buffer checkpoint

`native_memory_snapshot.h` now owns the actual private HSD current-heap selector
inside `memory.c`: a 16-byte epoch/handle component. This is distinct from the OS
selector. Invalid handles, retired heap geometry, construction claims, aliased
buffers and nonzero reserved fields reject before writes. An unset `-1` selector
is legitimate state. Restoring it does not change `__OSCurrHeap`.

`native_robj_snapshot.h` owns the actual `robj.c` expression argument root, its
allocation heap and all buffer values. Empty state is 24 bytes; allocated state
is 424 bytes (24-byte header plus 100 floats). Existing expression evaluation
still lazily allocates the same buffer and keeps its contents. Counted argument
flags can leave tail entries unwritten, so neither clearing the buffer nor
assuming every value is regenerated is justified. A native activity depth
refuses capture/load during the actual evaluator and update callback, including
nested evaluation. No snapshot operation runs expressions or constructors.

Validation checks candidate physical heap provenance without depending on the
current allocation's occupancy. This permits prevalidating a snapshot after
the original allocation has been freed/coalesced. Save additionally requires a
live allocated Cell header; Load requires that header restored before writing
the 400-byte payload. To restore across first allocation, rewind OS metadata,
then restore the empty root. The next actual expression allocates at the
original address. These checks do not prove that another owner's candidate has
not claimed the same allocation; that remains aggregate candidate closure.

Aurora provides a fixed physical-range catalog for all live heaps, built once
per heap geometry epoch. It derives ranges from actual Cell lists, includes
carved holes/disjoint extents, rejects overlapping heaps and has a bounded
65,536-range ceiling. Allocation/free/coalescing does not invalidate the catalog
or add tracking to those paths. Queries use address binary search plus the heap
identity. `OSPrepareHeapSnapshotOwnership` can prepare it before starting
history; the first query otherwise prepares it. This immutable host catalog is
not saved state or a substitute for actual allocation payload ownership. OS
Cell snapshot collection still has its existing O(n log n) cost.
Allocated-payload checks additionally walk the bounded actual allocated list:
an old header inside a coalesced extent can otherwise look allocated even after
metadata rewind. This lookup is not claimed constant-time; measure it with the
complete aggregate before choosing a faster validated allocation index.

Evidence under `A:/FreakMelee-test-archives/hsd-control-oct11-*`:

- `owner-final-b`: 137 checks, eight malformed candidate refusals and three actual
  HSD expression/update evaluations. Selector independence, retained tail,
  active evaluator/construction refusal, overlap, exact re-save, first lazy
  allocation rewind, coalesced candidate validation and Cell-before-payload
  restore order pass. `owner-a` failed fixture callback typing, `owner-b/c`
  passed a null update target contrary to HSD's contract; those are not accepted
  evidence. `owner-d/final` were earlier narrower passing cohorts. The final
  fixture leaves a stale interior argument Cell behind an older free extent,
  verifying that header fields alone cannot authorize a payload write.
- `alloc-final-b`: 66,106 checks, including 65,662 OS checks (mostly the existing
  65,536-allocation scratch-ceiling exercise), with nine OS snapshot refusals.
  Physical provenance accepts an allocation after free, while the live Cell
  check refuses it. Carved holes, extent overflow, pointer alignment, capacity,
  independent buffers and unchanged epochs across allocation/free pass.
- `build-c.log`: native client, content tool and source-assets fixture build
  successfully with the real Aurora implementation.
- `eight-final`: all 18 owners save/validate at all ten boundaries in both
  rendered hidden/muted 1,200-sample eight-actor scenarios. Stock captures the
  allocated 424-byte expression component; this interpreted family captures
  its empty 24-byte form. All diagnostic-on/off and previous/current populated
  traces match exactly. `final-verification.json` independently rechecks the
  frozen/original owner pins, allocator/live pins and mirrored OS headers.

The live census now attempts 18 components. Full aggregate restore, remaining
engine/world/particle state, other OS heap metadata/payload coverage and measured
complete snapshot/resimulation performance remain open. Broad production page
capture is unchanged.
