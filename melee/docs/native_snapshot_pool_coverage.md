# Restored-pool byte coverage diagnostic

The bounded older-capture experiment now classifies each differing oracle byte
against the restored live geometry of eight known fixed pools: GObj, GObjProc,
fighter, Ground, item, ItemLink, AObj and FObj. This replaces approximate symbol
attribution for these spans with actual pool ownership; it is still a diagnostic,
not a new snapshot representation or a coverage exclusion mask.

`nativeSnapshotEndpoint_ReportDifferences` accepts an explicit caller-owned list
of names and pools. It validates every binding before reporting, checks span
arithmetic and rejects overlapping pool descriptors, metadata, free-order arrays
or payload backing (including duplicated/shared registrations). There are at
most sixteen registrations, four spans each, held on the stack. No allocation,
callback, simulation or rendering occurs. The registry is supplied after all
older component Loads and byte-identical recaptures, before broad endpoint
recovery; no allocation/free may occur during the report.

Every differing byte remains in the original raw report and total. Additional
per-pool counts distinguish:

- **State:** occupied object bytes, descriptor bytes, all slot metadata and the
  active free-order prefix. Metadata padding is conservatively included here;
  no implicit padding exclusion occurs.
- **Free payload:** bytes within `object_size` of currently unoccupied slots.
  `nativeMatchPool_Allocate` clears every object byte before any consumer receives
  the slot. This classification relies on clients honoring logical pool ownership;
  a stale borrowed pointer reading a free slot is still a bug to detect.
- **Stride padding:** backing bytes between `object_size` and the next slot stride.
  These are outside the pool's public object extent, including for occupied slots.
- **Free tail:** free-order entries beyond `free_count`. Allocate pops only the
  active prefix; Free writes the new entry before extending it. Snapshot restore
  intentionally copies only the prefix needed to reproduce future allocation.

Bytes outside these precise spans remain **unclassified**. Per-pool categories
and unclassified bytes must sum exactly to the unchanged raw differing-byte total.
The live runners require all eight pool registrations and zero differences in
restored state spans. Inactive storage is not copied just to make broad snapshots
byte-identical. None of these classifications proves HSD/OS allocation ownership,
borrowed graph liveness, complete state closure or resimulation correctness.

The endpoint fixture uses real pool binding/allocation/validation with controlled
geometry. It injects six differences: three state bytes, one free payload byte,
one occupied stride-padding byte and one inactive free-tail byte. The report must
count all six accurately without modifying descriptor, metadata, payload or order.
An empty registry must instead report all six as unclassified. Invalid, overlapping
and overflow registrations must return failure before emitting any report.

The production broad page oracle and endpoint recovery behavior remain unchanged.
Original endpoint Save/Restore/Matches function bodies are preserved. Normal game
code does not perform this classification.

## Verified 2026-10-12 cases

| Case | Raw differences | Required pool state | Inactive pool storage | Unclassified |
| --- | ---: | ---: | ---: | ---: |
| stock-eight | 2420 | 0 | 2268 | 152 |
| interpreted-eight | 2419 | 0 | 2265 | 154 |
| fountain-of-dreams | 891 | 0 | 713 | 178 |
| green-greens | 251 | 0 | 61 | 190 |
| corneria | 3126 | 0 | 2912 | 214 |

These are eight-tick intervals at the diagnostic's known scene-resident boundary,
not an upper bound or proof that every other game system is covered. The residual
bytes still require ownership or a source-backed transient classification.

Evidence under `A:/FreakMelee-test-archives/`:

- `pool-coverage-oct12-endpoint-b`: 46 total assertions, including 27 new pool
  classification assertions. Actual endpoint and match-pool code; controlled
  region discovery and geometry. Six injected differences are fully accounted,
  including three state bytes which must not be misclassified as inactive.
- `pool-coverage-oct12-eight-a`: stock and interpreted eight-actor clients pass all
  35 component owners, 350 normal and 35 older Load/recapture pairs each. Both
  diagnostic-on/off gameplay and prior/current reference traces agree.
- `pool-coverage-oct12-stages`: Fountain of Dreams, Green Greens and Corneria each
  pass 4,800 rendered samples, 1,400 normal and 35 older pairs, with exact old/new
  traces. All five cases preserve the complete current endpoint before continuing.
- `pool-coverage-oct12-independent-verification.json`: final source/executable
  pins, full eight-owner accounting, unchanged endpoint Save/Restore/Matches
  bodies and continuing traces verified. Four build targets and focused
  allocator/pool/GObj-consumer CTests pass.

The next ownership pass should trace the remaining 152-214 bytes in these cases,
including texture/TLUT identity counters and source collision/animation/display
scratch, before complete compact restore-and-resimulate experiments. Other scenes,
resource lifecycles and candidate graph closure still need independent coverage.
