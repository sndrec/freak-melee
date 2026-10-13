# Particle, generator and AppSRT control ownership

Particle/generator/AppSRT globals reside in `particle_state.c`. Their source
consumers access the original public symbols or directly renamed private fields.
The old private mutable header and unused matching-only storage are removed.
There is no live mirror, replacement particle interpreter or allocation wrapper.

The component captures scratch float bits, six counters, generator ID
cursor, attachment joints/camera, sixteen particle heads, pending/active generator
heads, deferred SList head and prepared bank/configuration/callback bindings.
Ordinary ObjAlloc physical-slot references claim particle/generator/list backing;
class spans and known type identities claim joints/camera. The control component
does not duplicate payload, freelist, allocator descriptor or model state.

Prepared bank/configuration/hook bindings are leased for the scene; capture rejects
changes and Load requires current bindings to agree. Definition payload residency,
same-epoch re-preparation and full candidate list/ID/callback consistency remain
aggregate work. Numeric spans do not assert that a free slot is a live particle.
This component performs no list walking, source callbacks, physics, drawing or
allocation. The shared allocator/class companions restore first.

The original effect initializer requested `0xA4` bytes for AppSRT records. The
native struct is 184 bytes, with a 179-byte used prefix; the old aligned slot was
168 bytes. The call now requests `sizeof(HSD_psAppSRT)`, preserving the intended
record and full initialization. Source AppSRT transforms/decisions remain intact.
This fixes a native layout mismatch, not an intentional gameplay change.

Evidence under `A:/FreakMelee-test-archives/`:

- `particle-control-oct11-owner-b`: 117 added checks/thirteen malformed
  candidates; actual storage/serializer, real ObjAlloc/class/list/AppSRT backing,
  source AppSRT constructor and generator-ID wrap helper, repeated control
  updates, physical spans and borrowed class types, immutable binding refusal,
  independent payload restoration, overlaps/activity/retirement. Camera/joint
  setup and particle/generator contents/control mutations are controlled; full
  source particle interpretation and candidate graph recovery are not exercised.
- `particle-control-oct11-gm-b`: connected production control/player/camera
  fixture passes with particle lifetime hooks linked. Allocator/list endpoints
  abort if particle Save/Load is attempted here; those run in the actual owner
  fixture. Four build targets and allocator/GObj-consumer CTests pass.

Production retains the broad page oracle. Remaining asset/resident/world/HSD/HUD
ownership, candidate-wide topology validation and complete aggregate restore,
resimulation, graphics regeneration and performance acceptance are unfinished.


Final live evidence: particle-control-oct11-stages-a passes Fountain, Brinstar
and Corneria at 4,800 samples each with exact prior/current traces and forty
passing particle-control captures/validations. Sampled Save means include
29.825 us (Fountain) and 31.840 us (Corneria); these require aggregate profiling,
not a complete latency claim. Final particle-control-oct11-eight-b passes all
34 owners at ten boundaries in stock/interpreted eight-actor scenarios, with
exact prior/current and diagnostic-on/off traces. No live Load is performed.
The independent verification receipt checks final source/frozen/live pins,
source bodies with private renames/the AppSRT size correction and compiled
simulation/host sections. Earlier eight-a preceded the final Load binding guard.


## 2026-10-12 particle display cache ownership

The component now occupies **4,824 bytes**, an increase of 696 bytes. Actual
particle display storage lives in `psdisp_state.c`, accessed directly from the
original display/sort code and the existing particle control owner:

- `particle_display_frame` retains the original initial value 0x7B and the
  original 255-to-1 wrap in `psDispParticles(..., 0)`.
- `particle_sort_frames[16]` remembers which link lists were sorted at that serial.
- `particle_non_edge_heads[17]` retains the non-TexEdge split for subsequent
  display passes. The seventeenth original slot is retained, although the source
  sorter/display only indexes 0 through 15.

`particleSort` relinks the actual `HSD_Particle.next` chains and replaces the link
head. When a link has already been sorted at this serial, it returns the cached
split without relinking. AppSRT drawing also compares its saved `frameNum` against
the display serial to decide whether to reuse its matrix data. Thus the display
serial and sort splits are not simply discarded draw scratch: restore them with
particle/AppSRT allocation payloads and the main sixteen particle list heads.
The new head references claim ordinary particle allocator physical spans. They
do not prove candidate occupancy, list termination, or membership in the saved
main chain; full aggregate topology validation remains open. No pointer chain
is followed by this control serializer. Buffer-overlap checks include all three
actual display globals.

### Values rebuilt during a draw call

The remaining `psdisp.c` matrix, size, blend and color caches are deliberately
excluded from this component. `psDispParticles` sets local `needs_setup = 1` on
every invocation. Before the first non-negligible particle draws, its setup block
resets point/line widths, channel control, channel colors, TEV colors and blend
mode; reads the current view/projection; computes the inverse view, projection
rows and billboard axes; and resets the cached position-matrix index. Subsequent
particles in that invocation reuse those values. The two used projection rows
are assigned in both projection branches; the unused third row is not state to
capture. `psSetupTevInvalidState` resets the `psdisptev.c` TEV-key cache in this
same setup path. `psSetupTev` can modify particle kind bits; those bits remain in
the owning particle payload rather than being excluded as draw-only state.

The fog pointer is zero-initialized and never assigned in this source unit, and
the second entry of the cached matrix-index array is unused. These observations
apply to the current direct callers and idle snapshot boundary. They do not
justify skipping drawing or its state/RNG changes during resimulation, nor do
they make GPU/backend state part of this component.

### Evidence

Final receipts use the `particle-display-oct12-` prefix under
`A:/FreakMelee-test-archives/`. The owner fixture invokes the actual extracted
`particleSort` body, restores the real ObjAlloc payload alongside controls, checks
edge/non-edge ordering and cached repeated calls, then repeats updates across a
serial wrap and requires identical captures. It adds 33 assertions (150 particle
checks total), including four new malformed cache-head claims and three new
output-overlap cases. Particle construction and serial advancement are controlled;
this fixture does not invoke the full GX particle renderer. Live rendered tests
provide separate draw-path evidence. Pre-existing display function bodies are
preserved after normalizing the three direct storage-name changes.

Production still retains the broad page oracle. Full candidate validation,
complete restore/resimulation, GPU-state regeneration and final timing remain
unfinished.

Final `particle-display-oct12-owner-b` passes 86,834 fixture assertions;
`particle-display-oct12-gm-a` passes the connected loop/control/player/camera
fixture. The owner fixture has 150 particle checks and 17 malformed candidates.
`particle-display-oct12-eight-a` passes stock and interpreted eight-actor cases:
each has 350 same-boundary and 35 eight-tick older Load/recapture pairs, with
object creation/destruction/reuse and unchanged diagnostic-on/off gameplay.
`particle-display-oct12-stages` passes Fountain of Dreams, Green Greens and
Corneria at 4,800 rendered samples each, with 1,400 same-boundary and 35 older
pairs per case and exact old/new traces. Total: 4,900 same-boundary and 175 older
pairs across these five cases. The independent verifier confirms final source
and executable pins, all 29 original psdisp function bodies after direct storage
renames, and the three actual state symbols in `.mstate$m`. Four build targets
and focused allocator/pool/GObj-consumer CTests pass.

Eight-actor capture peaks are 2,622,928 bytes (stock) and 2,794,776 bytes
(interpreted). These are sampled component storage totals, not final worst-case
history or total rollback cost. Older-state tests recapture without ticking and
recover the broad current endpoint before continuing; full resimulation remains
unproven. Raw oracle differences remain, including intentionally excluded draw
scratch and not-yet-classified state; no coverage mask was introduced.
