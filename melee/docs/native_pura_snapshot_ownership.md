# Poké Floats collision-joint and Battlefield mode ownership

The existing stage-hazard component now owns actual Poké Floats collision-map
storage and Battlefield's demo-mode control/parameter pointer. The broad page
oracle remains production coverage; no complete match restoration is claimed.

`stage_hazard_state.c` holds all forty-four original collision-map records with
their exact initializer, including the sentinel and two unused tail entries.
`grpura.c` uses that actual storage directly. Binding occurs during the stage's
initialization; each tick uses the cached joints to update collision vertices.
The loop still visits only the original forty-two records. No collision-update,
spawn, animation, material or camera decision/arithmetic changed.

Snapshot scene setup freezes each record's vertex/map/joint identity and reserved
field. Capture includes the complete array and parameter pointer; live joint
references are admitted for the forty-two used entries. The extra tail records
retain raw pointer bits without becoming graph roots. Candidate configuration
must match the scene lease, so altered indices cannot be smuggled into a restored
table. Reinitialization can establish different identities and retires old history.

Joint references identify retained ordinary class spans and admitted initialized
types. Admission now uses the generic safe class identity lookup before reading
type size, avoiding an unchecked info-pointer dereference. Capture caches class
backing by entry; Load checks all restored joint types before changing any stage
record. The class companion must restore graph payload before the stage owner.

The component supports forty-four root/reference slots and is now 5,720 bytes.
Brinstar and Mute City retain their original twenty/thirty record limits; all
unused metadata slots have a zero representation. Battlefield's original mode
initialization and source reads use its actual state; parameter-pointee payloads
and any overlays remain separate owners, not implicitly covered by this pointer.
No capture/restore allocation, gameplay callback, draw or physics runs.

Retained class backing does not by itself establish object occupancy or the
correct full graph. Candidate-wide class/model/collision agreement and all
external stage borrowers still require aggregate prevalidation before writes.
This does not complete stage-private/header coverage or whole-match restore,
history, resimulation and performance acceptance.

Initial receipts under `A:/FreakMelee-test-archives/`:

- `pura-oct11-owner-a`: 72 added checks/eleven malformed candidates, exact table
  initializer/sentinel assertions, all forty-one populated mappings through
  three actual class joints, repeated binding changes, unused invalid tail alias,
  changed identities/reserved fields/spans/types, late type rejection before
  writes, real class companion restoration, overlap, activity/epoch retirement,
  scene reconfiguration and Battlefield mode restoration. Stage/GObj setup,
  parameters, idle predicate and graph initialization are controlled. The actual
  stage collision updater is not simulated in this fixture.
- `pura-oct11-gm-a`: existing production control/player/camera-target restoration
  fixture passes with expanded stage component.
- `pura-oct11-body-verification.json`: all 31 Poké Floats and 25 Battlefield
  source bodies preserve decisions and arithmetic, allowing direct state names
  only. The complete mapping initializer is unchanged.

- `pura-oct11-stages-a`: Poké Floats, Battlefield, Brinstar and Kongo Jungle
  old/current hidden/muted rendered CPU scenarios, 4,800 samples each. Exact
  populated gameplay traces agree; forty private capture/validation boundaries
  pass per scene. Poké Floats mean Save 5.383 us, Validate 0.915 us, 5,720 bytes.
  No live Load or full rollback deadline claim.
- Four build targets and the focused GObj-consumer CTest pass.

- `pura-oct11-eight-a`: stock and independent interpreted eight-actor runs pass
  all thirty components at ten boundaries each. Previous/current and diagnostic
  on/off populated traces agree. No live Load.
- `pura-oct11-independent-verification.json`: source/frozen/live input hashes,
  original body/initializer preservation and compiled sections verified. Actual
  stage storage is `.mstate$m`, the lease/configuration cache is host `.data`.
