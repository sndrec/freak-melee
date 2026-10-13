# Native match pool and packed snapshot first slice

This fixture tests the new private native storage/lifecycle contract. It does
not replace any game allocator or activate an explicit full-match snapshot.
HSD consumers remain unchanged pending the root's consumer review.

`nativeMatchPool_Bind` binds payload, slot metadata and ordered free-slot storage
reserved by the caller at the match transition. Each slot has a stable address.
Allocation pops a free slot, advances its generation, zero-initializes the actual
object payload and returns a private pool/lifetime/slot/generation reference.
Free invalidates occupancy and pushes that slot without running a callback or
writing the freed payload. Generation overflow refuses allocation instead of
aliasing an old handle. A new nonzero lifetime is required on every new binding.

The class-piece ceiling is now 65,536 slots; owner backing still reserves only
its requested capacity. A 47,553-slot console case exercises the measured Ness
resource planner requirement beyond the former 16,384-slot limit: complete
allocation/exhaustion, high-slot reuse with zeroed payload, stale-generation
refusal and complete release/validation. This is pool lifecycle coverage, not
a full-match snapshot or rollback performance result.

Snapshot save contains all slot generations, logical ordered free slots and only
occupied payload records, in ascending slot order. Load validates the entire
candidate's pool/lifetime/shape/counts, complete duplicate-free partition, record
ordering/generations, buffer independence and exact length before mutation.
Invalid load/save leaves live storage/output unchanged. No disk format, schema
version, script pointer API, runtime allocation, decoder or compatibility layer
exists here. Payload and reference lifetime belong to the game thread.

Occupied-only saving relies on allocation initializing every payload byte before
the consumer reads it. Consumer wiring must explicitly accept this deterministic
initialization policy. Uninitialized stock reads cannot be accepted as parity.
Inactive payload and unused free-order tail are not logical state. References
must be owned by restored game state or used transiently within a callback;
future-timeline references may not survive in host caches after a restore.

The source fixture uses actual current native headers/types:

* GObj/GObjProc linked topology, callback slots, self references, A/B/free-A/reuse-A,
  stale-generation guards, resurrection and exact replay allocation order.
* ItemLink chain payload, positions and cross links, including all90 slots of
  the declared grapple resource capacity (constructor acceptance is separate).
* Full Fighter/Item instance records including spawn/creator identities and the
  last authored state bytes (256/512 capacities).
* All 24 free orders of four GObj slots, saved across all five occupancy levels;
  empty/full/capacity failure and between-save writes.
* Seventeen malformed snapshot classes: pool/lifetime/payload/shape/count/peak,
  truncated/extra bytes, duplicate/out-of-range free and active records,
  partition collision and zero active generation. Every failure asserts the
  entire live fixture storage stayed unchanged. Invalid/aligned/overlapping
  binding, invalid/double/stale free, short/aliasing save, lifetime expiry and
  generation overflow also leave state unchanged.

The cross-pool fixture validates both snapshots before loading either and holds
all callback execution during loading. A real central aggregate must enforce
that same all-owner validation/publication contract. This slice provides atomic
validation for one pool; it does not yet provide a complete multi-owner load.

```powershell
python tests/native/native_match_pool/run.py --compiler .local/baseline72/compiler --output A:/FreakMelee-test-archives/native-match-pool-final
```

The runner compiles only the focused console fixture, uses CREATE_NO_WINDOW,
hashes live headers/sources and writes fresh A: logs/commands/receipt. The sole
warning exemption is the pre-existing duplicate MTXPerspective bound declaration
in the game header; utility/fixture warnings remain errors. No global build/game.

Current receipt: `A:/FreakMelee-test-archives/native-match-pool-checkpoint-msvc/receipt.json`:
124 lifecycle cases,17 malformed snapshots,57,894 assertions,zero callbacks;
native sizes GObj96,GObjProc48,ItemLink536,Fighter13,440,Item5,576 bytes. This is
storage/lifecycle evidence, not complete engine ownership/parity or performance.
