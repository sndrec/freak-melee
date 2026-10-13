# Samus grapple homing checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

`targets.nearest_available_fighter()` is a generic Fighter target-ID query.
It calls the unchanged common `ftCo_800A4A40` policy: self exclusion, caller
CPU blast margins, common availability/death filters, 2D distance and first
registry-order ties. Allies remain eligible. Full creation-ID bits are returned
as Int, matching ordinary owned state and the existing target-handle contract.
Physical registry admission precedes membership-based lifetime checks; selected
generation, caller schema and scene are checked before publication. No target
pointer, new runtime allocation or native Samus handler is exposed.

Samus's composed source adds `Tether_ReadHomingTarget`. During extending flight,
modified mode 1 and physical L trigger the search. The Article normalizes the
3D vector from its endpoint to the selected fighter and writes all three
velocity components using its owned scaled launch speed. If no target is found,
velocity is retained. The script owns the gating and steering; the common
engine primitive owns only target selection. Falling flight does not home.

## Evidence and remaining scope

Connected target receipt:
`A:/FreakMelee-test-archives/nearest-available-fighter-oct10-final/receipt.json`.
The cohort compares direct calls and Fighter VM calls with unchanged native
search/filter/distance bodies. It covers 139 nearest-query cases, alongside the
existing target-bone, ahead, shield and capsule comparisons. Compiler/runtime
rejection checks include Article context, extra arguments, a cycle and an
unreadable foreign registry head. Blast bounds and device identity are controlled
leaves; pool objects, actors, compiler, VM and source lifetime checks are real.

Composed Samus receipt:
`A:/FreakMelee-test-archives/samus-grapple-homing-oct10-final/receipt.json`.
All 279 candidate bodies prepare; the missing independent-Fighter declaration
remains deliberate. The existing geometry, lifecycle, sequence and hang batches
remain green. Those batches do not execute homing with live grapple resources.

Modified capsule creation and its mode-1-to-2 transition still remain open;
until that is authored, the candidate cannot claim complete modified behavior.
Throw-accessory authoring, constructor/resource/solver comparisons, lifecycle
destruction and full gameplay acceptance are also pending. No game was launched
and no installed package was changed. Samus remains unregistered.
