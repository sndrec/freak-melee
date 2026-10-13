# Mute City cars and Peach's Castle wind controls

Adds actual stage-private state to the existing hazard snapshot component.
This is partial owner coverage; production still uses the broad page oracle.

Mute City now keeps its thirty full car records, car-order permutation, collision
control and parameter pointer in `stage_hazard_state.c`. All source callsites
address that storage directly. The private included header's mutable array and
obsolete private type file are removed. Car fields, original zero initialization,
spawn/retirement conditions, callbacks, position/speed math and effects remain.

The car sorter had an invalid `j >= 0` comparison of `order[j - 1]` when j was
zero. The loop now requires `j > 0`. Depending on adjacent storage at index -1
cannot be preserved during native ownership changes. This is an explicit native
bounds correction, not an intended change to car ordering rules. The fixture
runs the actual sorter with ascending inputs forcing every insertion to the
front, and checks the resulting complete descending permutation. Its per-car
physics call is a controlled endpoint; live comparisons exercise the full solver.

Castle now keeps its parameter pointer, wind borrower, angle step and strong/
weak mode in the same actual state unit. Original initial values, including
0.017453292f and strong-wind 1, are preserved. Its wind field's payload belongs
to the shared dynamics component and is not copied again by the stage component.

`NativeWindReference` is a generic dynamics-owner reference to a retained physical
slot with a reset epoch and bounds/reserved checks. It is not a wind-request
generation or occupancy assertion: original priority replacement can overwrite
a slot without invalidating existing caller aliases. No stock replacement or
expiry behavior changes. Unknown/interior addresses and retired epochs reject.

The hazard snapshot now supports thirty GObj/userdata pairs and is 4,488 bytes.
Brinstar remains limited to its original twenty bubbles; extra root slots are
zero. Mute City borrowers use the actual Item pool and classifier/userdata/
destructor/reciprocal entity checks, rather than being misclassified as Ground.
Candidate car order must be a permutation of 0..29. Castle wind addresses must
agree with the recorded physical slot and live owner epoch. All candidate checks
and restored borrower checks complete before any hazard state is written.
Capture/restore uses fixed storage with no allocation or gameplay callbacks.

The real Item/GObj companions restore material records before the stage owner;
the dynamics companion owns wind payload. Candidate-wide graph/collision/
material-owner agreement before companion writes remains aggregate work. Item
role checks alone do not prove the item's complete graph or all its borrowers.
Other stage-private/header variables, attributes, variable payloads and complete
match restore/resimulation/performance acceptance remain unfinished.

Initial evidence under `A:/FreakMelee-test-archives/`:

- `car-wind-oct11-owner-c`: 128 new checks/thirteen malformed candidates, full
  sort permutation, repeated controlled updates, material generation reuse and
  Item/GObj companion resurrection, late rejection without partial writes,
  destructor mismatch, exact Castle controls and wind-payload separation,
  bad slots/epochs/parameters/flags and activity/retirement refusal. Actual
  owners/serializers; GObj construction, matrix setup, parameters, stage idle
  and material destructor endpoint are controlled.
- `car-wind-oct11-dynamics-a`: original dynamics/wind fixture plus generic
  physical-slot reference tests pass (473 checks). No complete match proof.
- `car-wind-oct11-gm-a`: existing production control/player/camera-target
  restoration fixture passes with the expanded component dependencies.

- `car-wind-oct11-stages-a`: five old/current hidden, muted, normally rendered
  CPU scenarios, 4,800 samples each: Mute City, Peach's Castle, Brinstar,
  Corneria and Final Destination. Populated gameplay traces agree exactly,
  including Mute City after the bounds correction. All forty stage-private
  capture/validation boundaries pass per scene. Mean hazard Save 1.915 us in
  Mute City and 2.208 us in Castle, 4,488 bytes. No live Load.
- `car-wind-oct11-body-verification.json`: all 71 Castle and 63 Mute City
  bodies preserved except direct state references and the documented sort
  boundary correction. The old mutable car header is absent.

- `car-wind-oct11-eight-a`: stock and independent interpreted eight-actor runs
  pass all thirty components at ten boundaries each. Diagnostic-on/off and
  previous/current populated traces match exactly. No live Load.
- `car-wind-oct11-independent-verification.json`: source/frozen/live hashes,
  source bodies and compiled sections verified. Actual Castle/Mute state is
  `.mstate$m`; the stage lease is host `.data`. Four build targets and the
  focused GObj-consumer CTest pass.
