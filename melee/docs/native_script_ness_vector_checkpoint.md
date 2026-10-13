# Ness interpreted vector and collision batch

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The complete Ness audit now reports 10 first-errors-per-body across 418 indexed
functions, down from 14 in the preceding common-state batch. Metadata and body
coverage remain complete, but the package still rejects. This count is not an
exhaustive task count: later errors and final preparation can remain hidden.

## Shared primitives and native contracts

- `physics.rotate_velocity_unit_axis(x, y, z, angle)` invokes
  `lbVector_RotateAboutUnitAxis` on Fighter self velocity. Ness's rebound uses
  the same unit Z axis and native formula as `ftnessspecialhi.c`. The script
  retains the move decisions, radian angle calculation and state updates.
- `vectors.rotate(x, y, z, axis, angle, out_x, out_y, out_z)` invokes
  `lbVector_Rotate` on a stack copy in both Fighter and Article contexts. Three
  Float outputs publish only after the source guard. Axes 1/2/4 represent X/Y/Z;
  other axes reject without changing outputs because the native helper has no
  defined result for them. Ness's slope correction uses the same Z-axis call
  as `ftnessattackhi4.c`.
- `ceiling.normal.z`, `wall.left.normal.z`, and `wall.right.normal.z` complete
  the shared normal observations for Fighters and Items. They copy stored
  `CollData` components, without a collision query or normal reconstruction.
- The bat reads `creator.model.root_scale_y`, the existing generic property for
  actual rendered HSD root Y scale. Stock `itnessbat.c` copies `HSD_JObjGetScaleY`
  to all three bat axes; this is not Fighter instance/model scale.

No fighter-named primitive, stock callback delegation, allocation, compatibility
alias or new mutable state was added. See [math](scripting/math.md) and
[movement](scripting/movement.md) for authoring contracts and examples.

## Verification

The seven Windows targets build. The script and parameter CTests pass 2/2.
The real-library controlled fixture passes 13,436 checks across 124 cases in
four independent owner/primary processes. It covers 864 exact native axis-vector
rotations, 256 exact native unit-axis Fighter velocity rotations, and 648
bit-preserving surface-normal reads from raw Fighters, source-owned Fighters
and source-owned Items. Inputs include signed zero, nonfinite normal bits,
three invalid axes and all three valid axes. Invalid-axis outputs stay intact.

Four actual interpreted Fighter helper executions also exercise rotation outputs
and the new normal fields, comparing the entire Fighter against native helper
results. The preceding common-state and Item animation cohort remains covered:
244 native animation graph comparisons, 16 animation frames and four Pages
restores. The controlled scene has no renderer, game window or sound.

The first fixture attempt correctly rejected an Item context that omitted the
scene lifetime. The fixture now supplies the same pool lifetime as the VM; no
production ownership check was weakened. The failed attempt remains archived.

Receipts:

- `A:/FreakMelee-test-archives/source-item-vector-collision-oct10-b/receipt.json`,
  SHA-256 `c2bd11e4de5a8b3f2c2f79af902c4e231e6933b95f184bdfee81c8ef1b497050`.
- `A:/FreakMelee-test-archives/source-roster-vector-collision-oct10-a/receipt.json`,
  SHA-256 `d9831c90b686a8996f744cb887661bdb2770172f57440e375b9b964ceb0d19b4`.
  All 24 previously accepted non-Kirby forms still prepare. Ness rejects with
  10 diagnostics; Samus has no valid independently prepared draft.
- `A:/FreakMelee-test-archives/ness-vector-collision-oct10-final/receipt.json`
  independently rehashes the frozen fixture, current code/assets and audit.

## Remaining acceptance

The fixture initializes controlled userdata on real HSD objects. It does not
prove full Ness construction, scheduler, physics/collision, article gameplay,
renderer/audio or stock/clone replay parity. The bat spelling has source and
compilation coverage, not new live bat behavior proof.

Remaining audit families concern root access to immutable Article parameters,
Item terrain/audio operations, stage identity, dynamic authored child selection
and removal of animations through retained resource joints. Keep these generic;
do not bypass preparation rejection or substitute stock callbacks.
