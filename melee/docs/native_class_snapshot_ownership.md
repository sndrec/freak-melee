# Ordinary HSD class storage snapshots

2026-10-11 checkpoint. Ordinary HSD class-piece storage now has a concrete
physical ownership directory and an explicit component serializer in `class.c`.
The actual allocator, split/recycle behavior and construction/destruction calls
are retained. This covers ordinary model class records and class-piece material,
envelope and texture metadata. It complements the separate reserved graph owner.
Production rollback still uses the broad page oracle.

Texture-animation palette pointer tables and additive shape weights now also
use this allocator. See [graph buffers](native_graph_buffer_snapshot_ownership.md)
for their direct allocation/free paths, reserved demand contract and actual
restore/destruction/reuse evidence. Temporary shape interpolation scratch has
fixed host storage and is separately proven regenerated before consumption.

## Storage and performance

The four physical HSD allocation sites directly record each acquired block's
address and requested byte count: initial/resized size-class pointer tables,
`HSD_MemoryEntry` records, and ordinary piece chunks. Old pointer tables are
recycled into the piece allocator, so their physical blocks remain owned too.
The directory describes actual storage; it is not another copy of model data.
Reserved construction allocations return before ordinary allocation and are
not duplicated in this directory.

Recording a newly allocated block is constant work. Piece allocation/free from
existing storage gets no directory traversal or additional allocation. The
fixed directory admits 65,536 physical blocks (1 MiB of address/size metadata on
this 64-bit build). Capacity is checked before each OS allocation, with no growth
or fallback on exhaustion. This is an implementation ceiling, not a measured
whole-game requirement or a promise that ordinary HSD no longer grows its heap.
Root-class amnesia retires the directory by resetting its count, without
clearing the unused capacity. Every class amnesia advances the lifetime epoch.

Save/load use caller-provided buffers, perform no allocation or callback, and
walk only recorded blocks and the initialized class tree. There are at most
256 class definitions; duplicate/cycle checking of that small tree is bounded.
No OS-page protection or whole-MEM scan is involved. Class snapshot bytes are
the actual requested chunks plus their descriptors, class counts and roots.
Free pieces and internal unused bytes are deliberately included initially:
splitting/reuse writes free links there, and dropping them prematurely would
lose allocator history. Unrecorded heap space, immutable archive assets and
unused directory capacity are excluded.

## Restore contract

The component stores `memory_list`, its entry count, retained physical blocks,
all their bytes, and every initialized class's live/peak counts. Code/method
tables, class definitions and allocation addresses remain resident. Save/load
must run at an idle complete-tick boundary with no construction claim.

The candidate is fully validated before writes: epoch, initialized class
identity/tree/definition, count bounds, pointer-table ownership, physical block
prefix identities and exact sizes, lengths, and buffer overlap. A shorter block
prefix can restore a pre-growth boundary. The aggregate must also restore
OS heap descriptors and Cell headers before execution resumes; logical block
retirement does not itself release/recreate OS allocations. The component
does not follow arbitrary candidate model or free-list pointers to validate
their whole graph closure. This is private process-memory state, not a network
or persistent asset format.

The initialized class tree must remain fixed for a history window. A newly
initialized class changes that tree and rejects an older candidate rather than
silently leaving mismatched counts. Aggregate match preparation must initialize
the required class definitions before taking its first snapshot. Root/default
class references and other module globals still belong to companion owners.

Ordinary ObjAlloc records now have a separate
[payload/accounting component](native_objalloc_snapshot_ownership.md).
OS heap headers and default ID roots now have separate
[control components](native_hsd_control_snapshot_ownership.md). Current class
growth tests pair real components instead of copying heap metadata wholesale.
Direct HSD allocations outside the class/ObjAlloc allocators, OS heap/current selection,
and source registries remain separate ownership work. Script Fighter/Item
variables, persistent state, timers, callback bindings and actor relationships
must restore with these model records through the existing script serializers.
No callbacks or script execution may occur until the aggregate restore ends.

## Bounded evidence

`tests/native/hsd_reservation_snapshot` now exercises actual `hsdNew`/delete,
ordinary class chunks, table growth/recycling, split/free order, live/peak counts
and pre-growth restore. Occupied bytes and subsequent allocation addresses are
exact after restoration. Twelve malformed/frozen-tree candidates reject with
all owner/heap bytes unchanged. OS heap metadata is a controlled copied fixture
companion; the class serializer itself restores its actual payloads.

The connected `script_item_resources` cohort additionally constructs an
ordinary HSD JObj beside the real imported Arrow/Boomerang resource graphs.
It mutates its transforms and matrix, restores class/reservation/shared-ObjAlloc
components, and checks exact records and re-save. The existing broad-page
destruction/replay check remains separate; this is not complete explicit
rollback or rendered/gameplay proof.

Receipts and logs are under `A:/FreakMelee-test-archives/hsd-class-oct11-*`.
Final acceptance:

- `owner-b/receipt.json`: 343 total checks, including 107 class checks and
  12 class refusals. The synthetic class component is 4,336 bytes; this is not
  a game-state size estimate. The initial `owner-a` failed at fixture linkage
  because real class construction retained HashSearch; the final runner links
  the actual `hash.c` too. That failed attempt is preserved.
- `census-a/receipt.json`: 14 cases, 48 checks, read-only ownership observation.
- `resource-a/receipt.json`: ordinary 2,733 checks/14 cases; animated
  4,460 checks/15 cases, including 39 native/VM animation pairs and exact
  ordinary JObj/class component restoration.
- `stages-a/receipt.json`: Battlefield, Great Bay and Brinstar, each 2,000
  samples with Link/Samus/Ness/Ice Climbers and medium items. All populated
  gameplay trace families match the previous client; no new census errors.
- `source-a/receipt.json`: fresh interpreted Samus/Link/Young Link/Ness,
  each 1,800 controller samples, match their stock traces.
- `build-a.log`: client, content/source-assets, ObjAlloc, census and heap
  targets build. Focused `native_objalloc`, `native_heap` and
  `native_match_object_census` CTests pass.
- `independent-verification.json`: all 344/1594/3336/210/55 receipt pins,
  baseline bundle hashes and four compiled production-source matches pass.

No whole-match size, save/load timing or eight-actor/fifteen-resimulation-tick
deadline is established by this checkpoint.

## Borrowed backing references

`nativeClass_Reference` creates a fixed-span claim within an actual retained
ordinary class chunk, recording the class epoch, block index, offset and length.
It searches only registered blocks and returns the reference by value. An empty
span means failure; epoch zero is valid before the first heap retirement.
Snapshot Save/Load buffer alias checks remain in the component methods.
`nativeClass_Resolve` checks the epoch, index and numeric span in constant time;
neither operation allocates or dereferences a candidate object's contents.

This is backing identity, not proof of a live class object or generation. Free
pieces remain in retained chunks. Consumers must establish object type/role after
the companion restore, and an aggregate must validate membership against the
candidate class block set before writing. Scene/heap retirement invalidates these
claims. The main-camera component uses this generic facility for its fixed CObj
roots; connected class/camera tests cover interior/full spans, malformed spans,
overlap refusal and retirement. See [camera ownership](native_camera_snapshot_ownership.md).


2026-10-12 class candidate update: caller-reserved indexes now expose saved class
payloads and candidate-prefix membership without per-tick allocation. Saved type
headers for main camera, stage/hazards/grid, effect queue and particle attachments
are checked before diagnostic Loads. See [class candidate views](native_class_snapshot_view.md)
for lease/index rules, later-block exclusion and 140 passing live preflights.
This does not prove class-piece occupancy or complete allocation/graph closure;
full history rewind/resimulation and rollback performance remain open.
