# Samus ordinary retraction and post-launch hitlag checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. Samus remains an unregistered authoring candidate. This expands the
[capture-retraction comparison](native_source_samus_capture_retraction_checkpoint.md)
with ordinary mode4 retraction and post-launch hitlag on real retail graphs.
No production code or scripting extension was needed for these paths.

## Scope

`tests/native/script_item_resources/samus_solver_cases.h` now owns the shared
retraction setup and substantive graph/chain comparison. The old capture-only
filename was replaced, with direct include and frozen-input callsites updated.

The additional72 factory cases cover four starting motions, three instance
scales, both facings and full/half/endpoint-only deployment. Ordinary retraction
runs through the real bound AccessoryBefore callback and is compared with
unchanged original `fn_802B8684`. This covers ground/air pull speeds, stochastic
forward constraints, collision history and X/Z velocity damping. Completion
must switch both paths to mode0; two subsequent held updates compare with
`fn_802B7E34`, including endpoint transforms and retained animation clocks.

Both mode4 and mode5 cohorts first run four actual bound AccessoryHitlag
callbacks. Launch counters are explicitly beyond their thresholds. The original
`it_802BACC4` constrains the independent native chain, while the interpreted
callback operates on the source factory Item. The576 comparisons cover partial
deployment, link spacing, history, RNG progression and model orientation without
animation advancement. Retraction then continues from those constrained chains.

Joint/controller/track, material/palette and override-matrix checks remain the
same as the prior checkpoint. Controller state and RNG progression compare
exactly; floats retain the documented bounded tolerance. Ordinary completion
uses the authored remove helper after its held updates; capture completion
removes on the next bound update. Both release all retained callbacks/resources.

## Evidence and limits

`A:/FreakMelee-test-archives/samus-retraction-hitlag-oct11-final-b/receipt.json`
records the current frozen-input batch:

- 8,124,936 assertions;72 native/interpreted constructor pairs.
- 216 real factory cases,432 bound held callbacks,324 pre-launch hitlag
  callbacks,36 Death1 removals, four ignored motions and one pool refusal.
- 72 mode4 pairs,267 normal retraction updates and144 returned-held updates.
- 72 mode5 pairs,292 normal retraction updates and72 completed removals.
- 576 post-launch hitlag constraint updates across the two cohorts.
- Earlier576 normal held and432 pre-launch hitlag graph comparisons remain.
- 95,976 material/palette comparisons,17 census cases and174 palette refusals.

Normal retraction counts differ from the earlier capture-only receipt because
this batch starts with four hitlag constraint updates. Both frozen cohorts are
valid evidence for their respective setup; no older receipt was overwritten.

Parent Fighter construction/publication, initial deployment/positions and capture
state remain staged. There is no actual victim, flight/contact/wall solver,
game scheduler, renderer/audio or whole-match rollback acceptance. Geometry and
effect leaves remain controlled and are not reached here. Independent Samus
registration still waits for those contracts. Tests are hidden console processes;
no visible game was launched, installed package changed or upstream push made.
