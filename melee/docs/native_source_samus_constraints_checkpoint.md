# Samus slack and wall-constraint checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. Samus remains unregistered. This extends the
[controlled-flight checkpoint](native_source_samus_flight_checkpoint.md) with
slack, wall extension and hanging on actual retail HSD graphs, plus continued
flight updates through their next states. No production API or script change
was necessary for the covered behavior.

## Connected comparisons

The existing 144 flight pairs now run all eight normal ticks, dispatching the
original mode-2/3 routines after transitions rather than stopping immediately.
They compare 1,152 updates and 96 transitions, retaining the controlled-contact
inputs and aerial-wall exclusion documented at the prior checkpoint.

Another 216 real Item factory pairs cover modes 3/6/8 against original
`fn_802B8524`, `fn_802B895C` and `fn_802B8D38`. They use the same four motions,
three scales, two facings and three deployment patterns. The cases cover:

- Stochastic slack-chain motion and transition to retraction when the parent
  leaves an allowed motion.
- Wall extension and transitions into hanging, retraction or pull mode.
- Hanging constraints, parent position/velocity/displacement and duration,
  including modified-mode physical L hold and ground/attack exit order.
- Initialization of hang duration when extension completes.

Each new pair ends after eight ticks or the first state transition. Destination
mode 7 is not executed. Parent state, initial deployment and solver entry are
staged; this is not full Fighter construction.

The unchanged original `it_802A4454` body uses the existing original map
remapping/validity functions on controlled tables shared with the resource
kernel. Surface -1 is stationary; surface 5 supplies moving geometry. The copied
stage binding uses the same table getters and validity function. Both segment
probes return unblocked; their coordinates, order and count are checked against
the original inline terrain check. No solver decision, timer logic, graph
operation or move handler is substituted.

## Evidence and remaining boundaries

`A:/FreakMelee-test-archives/samus-constraints-oct11-final/receipt.json` passes
19,213,448 assertions. It includes 576 real factory cases, 72 slack pairs,
144 wall pairs, 878 new constraint updates and 140 transitions. Earlier
constructor, held, retraction and hitlag cohorts remain passing, including
167,230 material/palette comparisons. All frozen inputs remain unchanged and
match the live source/library inputs.

The shared Link regression passes 7,033 assertions and 39 animated graph pairs
in `A:/FreakMelee-test-archives/script-item-constraints-regression-oct11-final/receipt.json`.
It uses the prior fresh production snapshot with the current fixture files;
production code has not changed since that snapshot. The native client build
and focused CTests from the preceding checkpoint remain valid for that same
production code. No game/audio/UI launch or installed-content change occurred.

Mode-7 pull, hang expiry and blocked-terrain exits still require connected
acceptance. The mode-7 decompiled C reads `dir_ptr` before assigning it in a
branch; the retail executable must establish the intended reference behavior
before comparing that branch. The subsequent
[pull checkpoint](native_source_samus_pull_checkpoint.md) resolves that artifact
against retail and adds nonterminal pull comparisons. Actual stage geometry, aerial wall effects,
captured-victim lifecycle, full Fighter construction and rendered gameplay also
remain open. The goal stays active, with no independent Samus registration or
upstream push.
