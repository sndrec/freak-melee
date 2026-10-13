# Reserved HSD model graph snapshots

Later companion checkpoint: [OS heap metadata and default ID roots](native_hsd_control_snapshot_ownership.md)
now implements those control components and pairs them with actual class,
ObjAlloc and reserved payloads. The evidence below describes the earlier
reserved-graph checkpoint; production aggregate integration remains work.

2026-10-11 checkpoint. `native_hsd_reservation.c` now saves and restores all
registered reservations explicitly, using their actual fixed `NativeMatchPool`
storage. This covers admitted class pieces and ObjAlloc records used by authored
secondary models and article resource graphs. It does not move ordinary stock
graphs into reservations or complete the aggregate match snapshot.

## Physical ownership and boundary

The existing model census sizes backing at the scene transition. Constructors
claim their declared budgets, and the actual HSD allocators allocate from these
bins. The new serializer copies occupied payloads, every slot's generation,
ordered free slots and used/peak counts. It also preserves the reservation
registry's ended construction-failure flag. Free payloads and alignment padding
are excluded; free order and generations remain necessary for later allocations.

Save/load use caller-provided buffers. There is no heap allocation, page
protection, graph reconstruction, destructor, script invocation or reference
count adjustment in these functions. Current validation and copying scan the
registered bins and their slot metadata; this is not an O(live objects) promise.
Snapshot-ring storage must be reserved at the match transition.

The boundary must be idle: no active construction claim or remaining quotas.
The aggregate caller must also finish game/script/draw callbacks before invoking
these functions. Reservation addresses, backing, bin geometry and epochs stay
bound for the entire history lifetime. This is a private memory representation,
not a package, disk or network format.

## Validation and companion owners

The complete candidate is checked before any owner changes. Owner registry
slots, backing identity, bin count, allocator identity/alignment, pool identity,
epoch, payload geometry, lengths, generations and free-slot coverage must match.
Buffers cannot alias registry state, an owner, backing or a shared ObjAlloc
descriptor. Truncated/trailing data, changed owners, stale epochs or malformed
pool metadata reject without partial restoration. Validation does not follow
candidate graph pointers or require the current live roots to be resurrected
first.

The reserved records still contain native pointers. Their complete graph closure
requires companion owners: AObj/FObj controllers, source resource registries,
GObj/ItemLink records, script actor state, and any referenced graphs. Shared HSD
ObjAlloc descriptors (including used/peak and ordinary free lists), class counts,
ID registry and ordinary heap records remain separate aggregate ownership.
Restoring this component alone across graph destruction would leave those
companions inconsistent. All owners must restore before callbacks resume.

The shared ObjAlloc descriptor/cursor component is now implemented separately:
see [shared accounting ownership](native_objalloc_snapshot_ownership.md).
It preserves shared usage/free/peak/limits and free-list heads, plus actual
ordinary payload/free-link blocks. OS heap state and class counts remain its
companions. The connected graph
fixture now pairs both component saves/restores and checks exact re-save.

The broad page oracle continues to own production rollback. This checkpoint
adds a component serializer; it makes no whole-match size or timing claim and
does not establish the eight-actor, fifteen-resimulation-ticks frame budget.

Ordinary class-piece backing and class live/peak counts now have a separate
[class owner component](native_class_snapshot_ownership.md). The connected
mutation fixture restores it with reservation and ObjAlloc components, including
an actual ordinary JObj, Vec and Mtx. OS heap metadata, ID/global roots and other
direct HSD allocations still require aggregate ownership.

## Verification

The focused `tests/native/hsd_reservation_snapshot` fixture uses actual HSD
class-piece/ObjAlloc, reservation, pool/serializer and Aurora OS allocation
bodies. Two registered reservations share a descriptor with an ordinary free
slot. Four occupied records span four bins. It exercises generation reuse and
resurrection, occupied bytes, exact re-save, free order, failure-flag restoration,
late malformed candidate rejection, overlap, active claim and retired epochs.
Shared allocator descriptors and the ordinary heap are checked unchanged.

The connected `script_item_resources` fixture additionally saves real imported
Arrow and animated Boomerang resource graphs created by the source runtime.
It mutates root/child transforms, world matrices, scale vectors when present,
and actual material alpha/color, then restores exact records and resolves the
same source graph references. Its existing VM/controller comparisons and broad
page restore/replay remain separate checks. These are bounded native fixtures,
not renderer or whole-game acceptance.

Pinned evidence under `A:/FreakMelee-test-archives/`:

- `hsd-snapshot-oct11-b/receipt.json`: 121 checks, 10 malformed candidates,
  two owners/four bins. The four-record snapshot is 760 bytes against 69,632
  reserved backing bytes; this small fixture is not a game size estimate.
- `hsd-snapshot-oct11-resource-c/receipt.json`: ordinary 2,720 checks/14 cases;
  animated 4,447 checks/15 cases, including 39 native/VM animation pairs.
  Reserved graph components are 245,400 and 303,704 bytes respectively; they
  include all reservation free-slot/generation metadata and occupied payloads,
  and exclude the other match owners.
- `hsd-snapshot-oct11-build-a.log`: client, content tool, source-assets and
  ObjAlloc targets build successfully. `native_objalloc` CTest passes.

Initial attempts remain in the archive: `hsd-snapshot-oct11-a` reached an older
unrelated model-census expectation after its owner checks, while resource `-a`
assumed a child on every root and `-b` assumed a drawable directly on every root.
The focused owner suite and hierarchy-aware connected suite above are the final
acceptance. Earlier archives are not relabeled as complete passing runs.
