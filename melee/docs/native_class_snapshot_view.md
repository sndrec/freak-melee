# Ordinary-class candidate payloads and borrowed types

`nativeClass_ViewSnapshot` prepares an immutable view of the class component,
using a caller-reserved array of saved payload pointers. Existing complete
component validation, index capacity and buffer independence/alias checks all
precede index writes. The snapshot and frozen class definitions remain leased
through inspection; no per-tick allocation or hidden allocator-owner cache is
introduced. Two views can coexist using separate caller indexes.

`nativeClass_ReadSnapshot` reads the saved bytes for an ordinary class reference.
The reference must belong to the candidate's block prefix, match its epoch and
fit its span; retained directory address/size must still agree. A later current
allocation does not become owned merely because live Resolve accepts it.
`nativeClass_ResolveSnapshot` returns that candidate span's physical address.
`nativeClass_SnapshotTypeKnown` compares a supplied type identity against saved
frozen definitions without following unknown pointers. `SnapshotObjectType`
compares the saved HSD_Class header with that type and its recorded size. Reads
use memcpy and permit unaligned payloads. Block lookup is constant-time after
one preparation pass; type lookup scans the saved small definition table.

These are ordinary physical spans and saved type headers, not class-piece
occupancy/generation or complete graph proofs. Current retained-directory prefix
and immutable definition leases are still prerequisites. The existing class
validator/loader and source class construction/allocation functions are unchanged.
There is no persistent format or compatibility layer.

The offline self-restore preflight uses one fixed 512-KB host pointer index and
checks the main camera's two private leased CObj types, stage joints/cameras,
stage-hazard joints/reflection camera, active Green Greens cell joints, active
effect queue joints, and particle camera/joint attachments against the saved
class candidate before any Load. It preserves dormant-pointer treatment and
checks only explicit active class claims. The view does not escape this preflight.
No source callback, simulation, rendering or allocation runs during inspection.

Evidence in A:/FreakMelee-test-archives/:

- class-view-oct11-owner-a: real class/ObjAlloc/OS and owner fixture, with 24 new
  checks (152 class checks total). Epoch-zero views, saved-versus-overwritten
  object headers/bytes, later-block exclusion despite successful live Resolve,
  malformed reference spans/epochs, unknown/undersized types, insufficient/aliased
  index refusal and independent simultaneous views pass. Type/scene residency
  and constructor/control setup retain the fixture's documented limitations.
- class-view-oct12-eight: stock/interpreted eight-actor cases each pass ten class
  and actor preflights, all 34 captures/validations and 340 same-boundary Loads/
  identical recaptures, with prior/current and diagnostic-on/off trace equality.
- class-view-oct12-stages: Fountain, Green Greens and Corneria each pass 4,800
  rendered samples, forty class/actor preflights and 1,360 Load/recapture pairs.
- class-view-oct12-independent-verification.json: final source/executable pins,
  prior/current eight-actor traces, unchanged pre-existing class function bodies
  and the host `.data` index verified. Four native build targets and focused
  pool/ObjAlloc/GObj-consumer CTests pass.

Total live class preflights: 140; same-boundary Load/recapture pairs: 4,760.
Other class/ObjAlloc borrowers, reserved graphs, allocation/free-list topology,
particle lists/IDs/callbacks, articles/VM/world/HUD/resident closure and complete
aggregate candidate publication/history rewind/resimulation remain open. Production
retains the broad page oracle; no completed rollback replacement or timing bound
is claimed. Preflight work is outside the diagnostic component-Load timing.
