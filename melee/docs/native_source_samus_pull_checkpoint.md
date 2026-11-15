# Samus pull constraints and retail direction-vector correction

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. Samus remains unregistered. This extends the
[slack/wall checkpoint](native_source_samus_constraints_checkpoint.md) into
nonterminal mode-7 pull with real retail graphs and resolves the suspicious
decompiled direction pointer against the supplied executable.

## Retail source of truth

The US 1.02 ISO's DOL has SHA-1
`08e0bf20134dfcb260699671004527b2d6bb1a45`. At `802BA4F4`, retail passes stack
`0x28` as the direction output to `it_802A3C98`. The clamp then loads X, Y and Z
directly from stack `0x28`, `0x2C` and `0x30` at `802BA508`, `802BA518` and
`802BA52C`, before writing the next link's position. There is no uninitialized
direction pointer in these instructions.

The decompiled `it_802BA3BC` read `dir_ptr->x` before assigning `dir_ptr = &dir`.
Native C now directly reads `dir.x/y/z` in both clamp blocks, matching the
retail loads and the existing interpreted script. This repairs a decompilation
artifact, rather than choosing new behavior for a console bug. No script or
fighter-specific scripting API was added.

`tests/native/samus_grapple_article_composition/retail_pull_audit.py` reads the
DOL directly from the supplied ISO, verifies its hash and the relevant raw
PowerPC instruction words, and writes a receipt using only Python's standard
library. Static evidence is recorded in
`A:/FreakMelee-test-archives/samus-pull-retail-oct11/receipt.json`. The complete
function `802BA3BC..802BA5DC` has SHA-256
`3c69097146cc00f8822e5caf96ca6b645acf25cf11d3759614f4ccd2b8b3d3e4`.
This is instruction evidence, not execution in Dolphin or on console.

## Connected nonterminal pull

Another 72 real Item factory pairs enter mode 7 and compare the bound
interpreted callback with `fn_802B8B54` using the corrected native expression.
They retain the four-motion, three-scale, two-facing and three-pattern matrix.
At least half the chain begins deployed; the third pattern uses one-third
undeployed instead of endpoint-only so the cohort stays away from full-Fighter
completion. Each pair runs two normal ticks or its first ground transition.

The endpoint begins with an explicit over-length gap. All 72 pairs verify that
the first pull update clamps this segment to its configured length, exercising
the branch corrected from retail. Positions, velocities, deployed flags,
collision history, graph/controller/material/palette state and world matrices
compare as before. Parent position, self velocity and displacement also match.
Moving-line -1/5 inputs use the original shared remapping math on controlled
tables, as in the preceding wall cohort.

## Evidence and remaining boundaries

`A:/FreakMelee-test-archives/samus-pull-oct11-final/receipt.json` passes
19,793,080 assertions, including 648 total Item factory cases, 72 pull pairs,
108 pull updates, 36 ground transitions and 72 verified endpoint clamps. All
earlier constructor, flight, slack/wall, retraction and hitlag cases remain
passing. Frozen inputs remain unchanged and match their live source/library
inputs after the production correction.

The fresh Link regression passes 7,033 assertions and 39 animated graph pairs
in `A:/FreakMelee-test-archives/script-item-pull-regression-oct11-final/receipt.json`.
The rebuilt native client/source-asset target and four focused asset, parameter,
resource-metadata and script compiler CTests pass. No visible or audible game
was launched and no installed content changed.

Terminal pull completion into ledge/jump states, blocked-terrain and expired-hang
exits, aerial wall effects, actual grabbed victims, full Fighter construction
and rendered gameplay still require connected acceptance. The first solver
entries and parent/chain state remain staged. This does not establish whole
Samus clone acceptance or full-game rollback. The goal stays active; no
independent registration or upstream push was made.
