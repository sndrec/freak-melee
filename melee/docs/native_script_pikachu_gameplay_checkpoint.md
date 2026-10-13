# Interpreted Pikachu/Pichu gameplay checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Both fighters now prepare all **210 functions**, own three articles each, and
publish independent interpreted source packages with **zero native imports**.
The corrected focused matrix passes **46/46 exact replay pairs**, including
39,400 controller/RNG samples, 78,800 fighter states, 1,776 action transitions,
8,901 item states, four item events and 195 damage events. All four previously
failing comparisons pass after the effect bookkeeping correction.
The current gameplay runner imports tuning afresh from each stock-derived draft,
saves the current shipping script, packages/verifies/installs it into a private
test collection, and compares drawn, hidden, muted replays against native stock.
No generated-C module is built or loaded.

## Startup visibility correction

Pikachu's stock death/reset policy selects group 1, alternative 0 even when that
group has no alternatives. The interpreter incorrectly required a selected value
to occur in an active channel, faulting the fighter before gameplay. The actual
`ftParts_80074A4C`/`ftParts_80074B0C` setters store a signed-byte selector. Rendering
compares that selector against alternatives; it never indexes an array with it.
Empty rows retain the value, and unmatched values hide alternatives.

The generic binding now follows those semantics while retaining owned table,
mesh, rig and lifetime validation and the **-1..127** argument bound. A grouped
fixture compares 72 empty/unmatched-selector cases directly with the native
setters, including complete Fighter bytes and DObj flags. Its final totals are
**18,673 checks / 2,337 native comparisons**. Seven Windows targets build and
script/parameter CTests pass **2/2**.

## Skull Bash effect bookkeeping correction

The original charge/launch callbacks set `x2219_b0` after spawning effects 1214
and 1215. The translated helpers omitted the corresponding `effects.active =
true`, although that flag controls common transition cleanup. Both scripts now
set it explicitly after the spawn, exactly where the native callbacks do. This
is authored bookkeeping, not a fighter-specific engine operation or implicit
side effect added to the generic effect API.

Before this correction, the connected grouped suite passed **42/46 pairs**:
32,100 controller samples, 64,200 fighter states, 1,352 action transitions,
8,508 item states, four item events, 130 damage events and 32,100 RNG samples.
Four exact comparisons failed: Pikachu CPU level 4; Pichu Skull Bash contact,
CPU level 4, and Battlefield CPU level 9. All first diverged in RNG at Skull
Bash recovery, before traced poses/physics/damage differed. A bounded debugger
probe caught the first differing RNG caller in Pichu's contact case in particle
generator processing, followed by HUD randomness. This is not evidence that all
four failures share that exact caller. The consolidated corrected run subsequently
matches all 46 cases, including all four former failures; no RNG normalization or
artificial RNG consumption was added.

## Reproduce the current grouped run

```powershell
python tests/native/pikachu_pichu_interpreted_parity.py `
  --output A:/FreakMelee-test-archives/pikachu-pichu-new-run `
  --prepared-root A:/FreakMelee-test-archives/source-full-roster-audit-oct10-scheduler-final
```

Use a fresh output directory. Prepared projects supply independently copied stock
assets; the runner replaces their parameter files from the current templates.
The existing acceptance manifests and input recordings under `.local` are
explicit dependencies. The default covers 23 focused scenarios per fighter;
repeat `--case` to select a grouped subset. A variant costume requires a separate
installed skin and is rejected rather than silently testing the default costume.
The runner retains all case failures and emits a failing final receipt if any
comparison, required scenario coverage, VM check or input-hash check fails.

Scenario coverage includes all four specials, reflected/absorbed Jolts,
Thunder contact/shield/absorption/interruption, second dash/forced turn/slopes,
repeated jab, forward-smash contact, held bat, CPU levels 4/9 and near/far recovery.
Exact trace equality is stronger than the approved player-visible behavioral
requirement, but a failed exact trace is not silently counted as acceptance.

Live Thunder-chain stock loss/owner removal, auxiliary attachments, runtime
instance scale, wider wall/ceiling/ledge/startup interruptions, variant costumes,
pixels/audio, full results flow, Workshop usability and network rollback remain
outside these replay results. No normal collection or draft is changed.

## Evidence

- Visibility fixture: `A:/FreakMelee-test-archives/script-visual-empty-selectors-oct10-final-b/receipt.json`
  (`ad65bd4447796a87b1261cb7f663da23295051fd552cd71fe2eca66d02068b63`).
- Initial 18-pair acceptance: `A:/FreakMelee-test-archives/pikachu-pichu-interpreter-oct10-c/receipt.json`
  (`24ba91e74758ccdcc6b4d69d393c4b9150f0dcc5be1b88c26f273479411921f6`).
- Wider 28-pair batch, **24 PASS / four FAIL**:
  `A:/FreakMelee-test-archives/pikachu-pichu-interpreter-extra-oct10-a/receipt.json`
  (`eba3f4c78ba7351b5968b6924afbde3bc9566dedf94c532a2504151d7445f778`).
- Bounded debugger probes: `A:/FreakMelee-test-archives/pichu-skull-rng-debug-oct10-deep/`.
  They deliberately quit after 181 captured calls and are diagnosis, not complete
  replay acceptance. No debugger timing result is used for performance claims.
- Corrected full run, **46 PASS / zero FAIL**:
  `A:/FreakMelee-test-archives/pikachu-pichu-interpreter-oct10-effects-final/receipt.json`
  (`988ec626ea6e5d3f66caa8a812d6076d53f62fe6e592f63755c090f4b87ed023`).
- Independent closure:
  `A:/FreakMelee-test-archives/pikachu-pichu-gameplay-independent-oct10/final.json`
  (`abc5309ef45fcb4265e56a99431c47cbc5042928c682ea224aa8a8a0be89baf7`).
  Rehashes 261 current game inputs, 1,607 visuals-fixture inputs, both real
  publication receipts (34 pins each), current shipping templates and build
  inputs; scans all 92 game logs for VM faults. Publication cohorts pass 43
  checks each and intentionally create no actors; gameplay actors are exercised
  by the separate actual game runs.

Independent rehash of the first three receipts verified **1,607 / 232 / 243**
inputs before the documented effect-template correction. Those historical
package receipts retain the old templates and are not hashes of the corrected
shipping source. The final closure verifies the corrected shipping source.
The goal remains active; Ness metadata, Samus independent draft
and the remaining gameplay/rollback acceptance are still open.
