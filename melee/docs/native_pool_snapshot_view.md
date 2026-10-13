# Read-only pool snapshot candidates

`nativeMatchSnapshotPool_View` validates a complete pool component and returns
its saved header, generation table and ordered occupied records. A null records
pointer means invalid. It replaces the boolean-only validation entry point; all
consumers use the view directly and no deprecated compatibility function remains.
The component byte layout is unchanged.

`nativeMatchSnapshotPool_Find` binary-searches the ordered records for a saved
slot and returns its read-only payload, without restoring or reading current
object bytes. Payloads can be unaligned; inspect fields through memcpy. The view
and its snapshot bytes must stay immutable/resident at an idle boundary.
`nativeMatchSnapshotPool_Resolve` checks pool identity, saved generation and
lifetime, then returns the stable physical address for a slot occupied in the
candidate. A currently free or reused slot does not invalidate its older saved
life. Retiring the pool binding invalidates both queries. Foreign references are
compared against the known pool before any pointer is followed.

Preparation uses the existing full metadata/order/span validation. Lookup uses
binary search and constant-time generation reads; no per-slot index allocation,
live-data mirror, callback, constructor, heap traversal or pool mutation occurs.
The generic facilities apply to GObj/procedure, fighter, Ground, item/ItemLink,
animation and reserved HSD pools. Each typed owner retains its own additional
validation; a generic view does not establish cross-owner graph agreement.

Evidence in A:/FreakMelee-test-archives/:

- pool-view-oct11-owner-b: actual pool utility fixture passes 124 lifecycle cases,
  17 malformed component cases and 153,030 checks with zero callbacks. Fifteen
  candidate-view checks cover saved payload versus reused live bytes, old/new
  generations, absent slots, malformed views, foreign/lifetime references and
  retirement without changing backing. Existing full-capacity, ordered free
  reuse, typed payload and malformed Load refusal checks remain passing.
- pool-view-oct11-companions: real allocator/class/reservation/camera/stage/effect/
  particle owner restoration fixture passes after the API migration.
- pool-view-oct11-eight: stock/interpreted eight-actor clients each pass all 34
  captures/validations and 340 actual same-boundary Loads/byte-identical recaptures;
  current/reference and prior accepted gameplay traces agree.
- pool-view-oct11-independent-verification.json: final fixture/companion/live
  source pins and prior/current traces verified. Five native build targets and
  focused match-pool, ObjAlloc and GObj-consumer CTests pass.

This is a primitive for aggregate candidate preflight, not a completed aggregate
loader. Cross-component relationships, class/ObjAlloc candidate membership,
resource/remaining world/HSD/HUD state closure and rewind through creation/reuse
remain open. Production still uses the broad page oracle; no history rewind or
full rollback deadline is claimed.
