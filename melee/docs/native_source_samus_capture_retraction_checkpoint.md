# Samus capture retraction checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. Samus remains an unregistered authoring candidate. This extends the
[held-update checkpoint](native_source_samus_held_update_checkpoint.md) through
mode5 retraction, common CatchPull hooks and completed source Item cleanup.
No production gameplay code changed in this checkpoint.

## Connected comparison

The interpreted fighter creates each Item through the unchanged shared factory,
using imported common Item data and retail Samus model/controller graphs.
The real CatchPull anchor hook enters mode5 and binds the endpoint; ready/follow
hooks are checked around completion. The actual installed AccessoryBefore
callback runs the source solver. The unchanged original `fn_802B8814` operates
on a separately constructed chain and primary graph.

Seventy-two cases cover four source motions, three scales, both facing
directions, and fully deployed, half-deployed or endpoint-only starting chains.
Each tick compares deployment, position/velocity, collision history, RNG
progression, joint poses/flags, exact controller/track state, material/palette
state and complete world-matrix overrides. Float values retain the existing
`2e-6 + 2e-5 * max(abs(actual), abs(expected))` bound. Controller graph pointers
are excluded from raw equality because the graphs are independently allocated.

After completion, the next bound source update removes the actual Item and
releases its capture anchor, reserved part139 and retained callbacks. Its fixed
pool reference is retired and GObj usage returns to the case's initial count.
The native comparison graphs are explicitly disposed; native completed-item
removal is not claimed as a comparison.

## Evidence

`A:/FreakMelee-test-archives/samus-capture-oct11-final/receipt.json` passes with
the frozen inputs unchanged and live inputs matching at completion:

- 3,900,510 assertions;72 constructor pairs.
- 144 shared factory cases, including288 bound held and216 pre-launch hitlag
  updates,36 bound Death1 removals, four ignored motions and one pool refusal.
- 72 capture-retraction pairs,296 compared updates and72 completed removals.
- The earlier576 normal held and432 hitlag comparisons remain intact.
- 62,156 material/palette comparisons,17 census cases and174 palette refusals.

The shared Link regression receipt is
`A:/FreakMelee-test-archives/script-item-resources-capture-regression-oct11/receipt.json`.
It uses the existing sealed production snapshot with the current fixture files;
its result must be read separately from the fresh Samus production-input run.
Both core and animated cohorts pass:7,033 assertions,25 cases and39
interpreted/native animated graph pairs, including retirement and restore/replay.

## Fixture corrections and limits

The native comparison now executes the same shared Item state transition as the
source path. Initially it was only assigned mode5, leaving its primary model at
the descriptor origin with the wrong facing and retained animation. That was
a comparison setup mismatch, not a script behavior bug.

The shared factory reset also began at ID0 while earlier manually staged fixture
Items remained alive at IDs100 and101. A later factory Item then collided with
one of those test actors. The counter now advances above all existing Item
identities before this cohort; normal gameplay obtains all IDs from that factory.

Parent Fighter payload, publication, capture state and starting deployment are
staged. There is no actual grabbed victim or reciprocal victim setup, scheduler,
flight/contact/other constraint modes, renderer, audio or whole-match rollback
acceptance. Geometry/effect leaves remain controlled and are not reached by
this mode5 cohort. Independent registration waits for the remaining contracts.
No visible game was launched, installed package changed or upstream push made.
