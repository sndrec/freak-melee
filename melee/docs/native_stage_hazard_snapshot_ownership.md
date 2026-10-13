# Stage-private hazard controls

The next explicit component owns the actual private records for Corneria,
Venom, Green Greens, Brinstar Depths, Brinstar, Kongo Jungle and Icicle Mountain,
plus the shared debug conversation
selection. `stage_hazard_state.c` defines the simulation storage. The original
stage functions address those records directly, without forwarding wrappers or
an additional shadow representation.

| Stage | Actual state captured |
| --- | --- |
| Corneria | Current Arwing slot, spawn timer/mode, update gate, three actor handles/types/groups, five cannon world positions and parameter identity. |
| Venom | Current Arwing slot, spawn timer/mode, update gate, three actor handles/types/groups and parameter identity. |
| Green Greens | Initialization/update flags, all 32 block-state bytes and parameter identity. |
| Brinstar Depths | Debug animation selection and parameter identity. The normal Kraid behavior is already stored in its Ground records. |
| Brinstar | Bubble cooldown/debris count, base positions and all twenty bubble entries, including live/dormant joint borrowers. See [bubble ownership](native_brinstar_snapshot_ownership.md). |
| Kongo Jungle / Icicle Mountain | Moving joint state, row data and model/actor borrowers; see [moving-stage ownership](native_moving_stage_snapshot_ownership.md). |

All original zero initializers and the conversation selection's -1 are
preserved. An independent audit compares all 225 donor function bodies in the
four source modules, allowing only direct storage-name substitutions.
No gameplay decision, timing, RNG call, or stage callback is replaced.

## Capture and restore

The initial four-stage checkpoint used a 296-byte `NativeStageHazardSnapshot`
with up to three actor references. The Brinstar expansion is 2,800 bytes with
twenty paired actor references and optional class joint claims; see the linked
bubble ownership contract for its live/dormant reference rules. It stores the
active stage's typed record and the shared selection. Other stages have an empty canonical payload; their dormant private
records are not read or restored. The conversation event callback is shared by
Corneria/Venom and routes by the actual stage kind; its runtime conversation
state remains in the relevant Ground records.

The lease starts after the real deferred stage startup queue drains. Ground
initialization and GM exit retire it. Capture requires the idle GM boundary,
stable kind, pool geometry/lifetimes and active parameter pointer. Parameter
payloads remain borrowed; this component does not establish their full resident
or mutable ownership. Snapshot helpers select the actual records and perform
serialization; there is no allocation, game callback or draw during Save/Load.

Validate checks leases, canonical unused payload/handles, Arwing slot bounds,
the Corneria bool representation, and numeric generation-reference geometry.
It does not demand present occupation, so a saved actor can be resurrected.
Load requires GObj/Ground companion restoration, the stage classifier/userdata
kind and destructor, and the reciprocal Ground owner link before writing any
private value. Aggregate candidate agreement and prevalidation are still needed.

## Batch audit

`A:/FreakMelee-test-archives/stage-private-oct11-audit/inventory.json` records
286 top-level declarations across the 29 selectable-stage source modules at
donor `bd9585426`, with compiler types and function-reference/ancestor locations.
This is a declaration inventory, including redeclarations, not a proof that
every variable is mutable or immutable. Assignment scanning also cannot settle
writes through aliases, output parameters, escaped pointers or callees.

One confirmed indirect write is `lb_8000B1CC`: its third argument receives the
world position. Corneria passes the cannon-position cache there, so that cache
must be captured despite having no direct assignment in the stage function.
Remaining modules need the same lifecycle/escape review, rather than blanket
classification of initialized tables as immutable.

## Evidence and limits

The original four-stage checkpoint receipts under `A:/FreakMelee-test-archives/`
remain historical evidence for that smaller record. Brinstar evidence is
documented separately. Original receipts:

- `stage-hazards-oct11-owner-a`: 178 hazard checks and 25 malformed candidates
  across all four active shapes and an unrelated-stage empty shape; exact
  restoration, sixteen repeated controlled updates, no partial writes on
  candidate rejection, generation reuse/companion ordering, role/parameter/
  activity/retirement refusal, overlap and inactive-record preservation. Actual
  Ground and pool serializers are linked; GObj backing, parameters, scheduler
  and actor construction are controlled. No real stage simulation here.
- `stage-hazards-oct11-stages`: old/current rendered CPU runs on all four changed
  stages plus Final Destination, 2,400 samples each. Populated gameplay trace
  families match exactly, and shared/private stage Save/Validate pass all 20
  sampled boundaries in every case. The hazard component's observed per-case
  mean Save is 0.790-0.965 us; these are not whole-match timing measurements.
- `stage-hazards-oct11-eight`: stock and independent interpreted eight-actor
  runs pass all 28 components at ten boundaries each. Diagnostic-on/off and
  previous/current populated gameplay traces match exactly. No live Load.
- `stage-hazards-oct11-gm-a`: the existing GM/controller/input/RNG/player/camera
  target restore fixture passes sixteen repeated controlled ticks. It has no
  real stage startup/graph.
- `stage-hazards-oct11-independent-verification.json`: verifies owner/live/GM
  pins, donor function bodies, declaration inventory, actual compiled simulation
  versus host sections, and previous/current eight-actor trace agreement.
  Client/content/source-assets/GObj-consumer builds and the focused
  GObj-consumer CTest pass.

Still open: other stage-private variables, mutable shared parameter payloads,
variable storage such as Green Greens' allocated block arrays, other world/GM/
HUD/effect/dynamics owners, multi-heap and resident closure, aggregate restore/
history, whole-match differential resimulation and the full performance target.
The production broad page oracle remains active. Neither the declaration audit
nor this four-stage batch establishes complete stage ownership.
