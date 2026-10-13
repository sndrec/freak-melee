# Shared HSD ObjAlloc storage snapshots

2026-10-11 checkpoint. `objalloc.c` explicitly saves the actual registry's shared
descriptors and object-arena cursor. It now also owns every actual ordinary
allocation block's payload and free-link bytes, from the fixed arena or OS heap.
Reserved payload still belongs to the separate reserved graph serializer.
Production broad rollback has not yet been replaced.

## Heap lifecycle correction

`HSD_CreateMainHeap` previously called `HSD_ObjInit` and then
`_HSD_ObjAllocForgetMemory`. The latter erased the just-built registry even though
the initialized descriptors remained live. Heap recreation now forgets the old
registry before `HSD_ObjInit` resets and registers the new descriptors. The
read-only census requires the named Vec/Mtx/RObj/Rvalue/ID descriptors in that
registry; its supplemental observation path is removed. Missing ownership is a
diagnostic failure, rather than a silently incomplete count.

The ordinary allocators and their slot order are otherwise unchanged. Arena
selection, heap destruction/creation, callback cleanup, HSD initialization and
animation-owner boundary checks remain at their existing call sites. Current
gameplay comparisons against the previous client are separate from the
descriptor-format fixtures below.

## Component contract

The snapshot copies every registered `HSD_ObjAllocData`: used/free/peak counters,
limits/flags, free-list head and native registry links. It copies `obj_heap`'s
top/cursor/size/remaining fields, physical block prefix and all its bytes.
Raw native addresses are private scene-local
references, not a script API or portable package format.

There is no heap allocation or callback during save/load. Up to 256 descriptors
are checked using fixed stack scratch; overflow or a cyclic registry refuses the
snapshot. The complete candidate validates before any descriptor or arena cursor
changes. Identity, registry order/shape, sizes/alignments and the ownership epoch
must match; descriptor accounting and fixed-arena cursor geometry must be valid.
The owner checks candidate references without following borrowed free-list
payload pointers. Block addresses, allocator identities and exact sizes must
match the actual retained directory, with complete lengths checked before any
descriptor or payload changes. A shorter directory prefix restores a boundary
before subsequent growth; OS heap/Cell metadata must restore alongside it.

`HSD_ObjAllocAddFree` directly records the actual backing at acquisition.
Ordinary allocate/free from existing slots keeps its original logic and gets
no directory traversal. Construction claims return before ordinary allocation,
so their backing is not duplicated here. The fixed 65,536-block directory uses
1.5 MiB on the 64-bit build and checks capacity before changing the allocator.
It neither allocates extra runtime memory nor grows on exhaustion. Registry
forget retires its count without clearing unused metadata capacity.

Only actual allocated blocks are copied, including free slots and their links.
Alignment gaps and the fixed arena's unused capacity are excluded. This first
conservative ownership pass retains entire blocks, not only occupied records.
It is not yet a claim about total game-state size or uninitialized future-slot
behavior. Normal OS heap growth remains the original allocator's behavior.

`HSD_ObjSetHeap`, allocator initialization and registry forget advance a native
epoch. Even same-address/same-shape reinitialization retires old history. These
are ownership transitions, not operations that an aggregate load may invoke.
The game/script/draw boundary must be idle, with no construction claim. Snapshot
buffers must live outside ordinary HSD backing; descriptor/global/fixed-arena
overlap is rejected here. Actual OS-backed block spans are also checked for
buffer/written-result overlap.

This component does not own the OS heap/cell metadata or current-heap selection,
HSD class storage/counts, ID roots,
source registries or live graph closure. Reserved pools preserve their own free
order independently. All companion payloads and roots must restore before the
engine or scripts resume. Saving accounting alone across allocation/destruction
is insufficient. Ordinary class storage/counts now have their own
[explicit class component](native_class_snapshot_ownership.md).
OS heap headers/current OS selection and default ID roots now have explicit
[control components](native_hsd_control_snapshot_ownership.md). The current
growth fixture uses them, rather than the older copied OS companion described
in the historical evidence below.

## Earlier accounting-only evidence

Evidence under `A:/FreakMelee-test-archives/`:

- `hsd-descriptors-oct11-owner-c`: 236 checks, including 115 shared-descriptor
  checks and ten rejected descriptor candidates. Real ordinary fixed-arena and
  reserved allocations change accounting; restoring the actual descriptor and
  reserved components recovers it. Ordinary payload/free-link bytes are a
  controlled copied companion in this fixture. Two descriptors occupy 176
  snapshot bytes; the 256-descriptor ceiling and empty registry are also tested.
- `hsd-descriptors-oct11-census-b`: 14 cases/48 checks, including missing named
  registry ownership, with all observations checked read-only.
- `hsd-descriptors-oct11-animation-a`: 90 cases/1,082 checks. Actual ObjInit and
  CreateMainHeap bodies run with controlled outer callbacks; the registry-order
  boundary, occupied animation bulk retirement, stale epochs and rejected active
  callback resets pass.
- `hsd-descriptors-oct11-resource-a`: ordinary 2,725 checks/14 cases and animated
  4,452/15 cases. Actual authored Arrow/Boomerang graph and shared-descriptor
  components restore and re-save exactly, alongside the existing VM/native
  controller comparisons and broad-page history replay.
- `hsd-descriptors-oct11-build-a.log`: client, content tool, source-assets,
  ObjAlloc and census targets build. ObjAlloc and census CTests pass.
- `hsd-descriptors-oct11-stages-b`: Battlefield, Great Bay and Brinstar all
  match the previous client across 2,000 rendered CPU samples each, with
  Link/Samus/Ness/Ice Climbers and medium items. All populated trace families
  match, and the complete candidate registry census reports no errors.
- `hsd-descriptors-oct11-source-a`: fresh interpreted Samus, Link, Young Link
  and Ness pass their stock/source comparisons across 1,800 samples each.
- `hsd-descriptors-oct11-independent-verification.json`: all six final receipt
  pin sets match (341; 1,593; 1,604; 3,335; 208; 55). Baseline bundle hashes and
  the connected fixture's production owner sources also match their evidence.

The initial owner `-a` fixture requested a four-slot construction quota with only
three slots free; that test setup was corrected. The initial stage `-a` baseline
bundle omitted the required UI folder and did not reach gameplay. Those failed
attempts remain archived and are not passing evidence.

## Ordinary allocation-block update

The current `hsd_reservation_snapshot` fixed-arena test restores payload/free
links through the real component, without its former copied-arena companion.
The additional actual OS-heap cohort mixes reserved/ordinary allocation, mutates
records, grows new blocks, prevalidates/restores all components, checks exact
re-save and reproduces subsequent allocation addresses. OS heap/Cell metadata
is still a controlled copied fixture companion. Malformed late block records
reject without changing any payload, descriptor or heap bytes.

The connected resource cohort constructs ordinary HSD Vec and Mtx records
beside the ordinary JObj and real authored Arrow/Boomerang graphs. Their values
are changed and restored by the actual ObjAlloc component, then all components
re-save exactly. No ordinary payload-copy companion supplies that restoration.

Current evidence is under `A:/FreakMelee-test-archives/hsd-blocks-oct11-*`.

- `owner-b`: 410 total checks; 115 fixed-arena/descriptor checks with ten
  refusals, 67 OS-backed object checks with six late-block refusals, and the
  retained class/reservation checks. Both small object components are 480
  bytes, including actual payload/free links. These are synthetic sizes,
  not a whole-game estimate. Initial `owner-a` is preserved: its purported
  wrong-address perturbation was actually the correct block address.
- `resource-a`: ordinary 2,736 checks/14 cases; animated 4,463 checks/15
  cases, including 39 native/VM animation pairs and exact ordinary Vec/Mtx
  restoration/re-save with the class and reserved graph components.
- `census-a`: 14 cases/48 checks, with read-only registry/topology observation.
- `stages-a`: Battlefield, Great Bay and Brinstar each match the previous
  client across 2,000 samples with Link/Samus/Ness/Ice Climbers and medium
  items; all populated trace families match with no census failures.
- `source-a`: interpreted Samus/Link/Young Link/Ness each pass their stock
  comparisons across 1,800 controller samples.
- `build-a.log` and `focused-ctests.log`: client/content/source-assets/ObjAlloc/
  census/heap builds succeed; ObjAlloc, heap and census CTests pass.
- `independent-verification.json`: all 345/1594/3336/210/55 receipt pins,
  baseline bundle hashes and five compiled production-source matches pass.

Production still uses the broad page oracle. Direct HSD allocations, ID/global
roots, OS metadata and aggregate restore remain open. No whole-match snapshot size,
sub-100us save/load time or eight-actor resimulation budget is established here.


## 2026-10-11 ordinary slot borrower references

nativeObjAlloc_Reference/nativeObjAlloc_Resolve expose ordinary physical block/
slot spans under the allocator epoch. Descriptor identity is compared against
the retained directory before following the caller's allocator pointer. Alignment,
range, requested size, directory count and construction/epoch guards apply.
Queries return references by value, removing an output pointer and its repeated
allocation-directory scan. A zero-epoch/empty span means failure. Explicit
snapshot Save/Load methods still reject buffers overlapping controls/backing.
These references do not encode liveness or a per-request generation: a freed slot
remains physically resolvable. Reserved construction slots keep their separate
reservation owner. Consuming particle/queue owners must still validate their
candidate lists/IDs/callbacks and restore allocator companions first.

objalloc-ref-oct11-owner-a adds 23 checks over the previous owner fixture,
including foreign/interior/oversized pointers, malformed epoch/block/slot/size,
active reservation exclusion, physical references after free, directory-prefix
rewind/regrowth and heap retirement. Existing allocator bodies are unchanged;
objalloc-ref-oct11-independent-verification.json checks all 1,732 source pins and
body preservation. Four native build targets plus ObjAlloc and GObj-consumer
CTests pass. No gameplay callers changed, so no new live-match/performance claim
is made. The 33-owner production census count is unchanged; particle/global and
aggregate restoration work is unfinished.
