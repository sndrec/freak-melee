# Interpreted special follow-through checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. All 42 selected follow-through cases have passing stock/source
comparisons across the initial batch and focused reruns. This supplements the
[special-entry checkpoint](native_source_roster_special_checkpoint.md) and
[CPU roster checkpoint](native_source_roster_cpu_checkpoint.md). It is
consolidated evidence, not one final sealed 42-case run or exhaustive fighter
completion. No production gameplay or scripting changes were needed here.

## Shared batch runner

`tests/native/fighter_roster_special_parity.py --matrix` accepts recorded cases
with explicit expected motion, damage, item and ownership requirements. Every
selected case is attempted after failures; failed cases can be rerun together
with `--retry-failed-from`. The default synthesized ground/air entry mode
remains available.

Each fighter is freshly cloned, its stock defaults imported into owned
parameters, its ordinary source prepared, then packaged, verified and accepted
in a private content store. Both stock and independent source consume the same
recording. Games run hidden, muted, unpaced and with rendering enabled. This
does not reuse the older generated-C package backends or inject callbacks.

The matrix requires actual move coverage, preventing matching idle runs from
passing. The shared comparator checks emitted action, fighter/companion pose,
physics, hit/hurtbox, item visual/event, damage, RNG and resource records. The
roster runner additionally checks full replay consumption and source/fighter/
item diagnostics, and requires live interpreted-fighter fault reports to be
present and zero. Source, inputs, native tools, executable, package files and
helper generators are hashed before and after each batch.

`tests/native/fighter_roster_followthrough_matrix.py` produces the private
42-case matrix. It reuses checked-in recordings and existing pad generators,
with explicit current coverage contracts. The matrix can be extended with
recorded interactions without adding another package-preparation pipeline.

## Covered behavior

| Fighter | Cases | Paths |
| --- | ---: | --- |
| Luigi | 4 | Held release, automatic maximum-charge launch, natural misfire, aerial entry followed by grounded hold/release. |
| Jigglypuff | 8 | Short/full/turning ground Rollout in both directions; short aerial Rollout in both directions; required contact/damage on right-facing short/full ground routes. |
| Marth | 9 | Full and partial grounded neutral charge, short aerial neutral; all high/neutral/low Dancing Blade routes on ground and in air. |
| Roy | 9 | The same routes with Roy's own full-charge duration and move contracts. |
| Mewtwo | 12 | Shadow Ball release with A/B, cancel/restart, roll/restart, full release, full store/restart; Teleport right/up/diagonal from ground and air. |

Shadow Ball cases require a real kind-112 article, in addition to the expected
motions. Aerial Teleport uses the existing explicit offline placement fixture
at sample 175; this is controlled positioning, not proof of an ordinary jump
setup. Other cases use their controller schedules without that fixture.

## Coverage corrections

The initial batch reported 39 PASS / 3 FAIL. All three failures were missing
coverage requirements. Independently comparing those completed traces without
their unmet motion requirements showed matching stock/source records; they
were not counted as passing the intended branches.

- Roy's seven-second charge increments at 30 ticks/sec after startup. The
  shared 210-frame button hold completed Marth's four-second charge but released
  Roy early. Roy now holds for 260 frames and reaches full-release motion 344.
- Upward-diagonal grounded Teleport immediately follows the airborne zoom/end
  route, motions 353/357/358. Stock `ftMt_SpecialHi_Enter` selects the airborne
  path when the stick direction points away from the floor. The requirement
  now checks that path rather than the horizontal ground route 354/355.
- The historical Luigi seed-3 press at sample 270 now gives a normal launch.
  Restoring its historical two-stock setup still gave a normal launch, and a
  seed-2 probe also did not exercise misfire. Neither is accepted misfire
  evidence. Current stock RNG tracing identified the preceding idle window:
  keep seed 3 and advance P1's press/release nine frames. Both runtimes then
  naturally reach misfire 348 and flight 351, with all traced state matching.
  The RNG function, chance parameter and native/script branch are unchanged.

The corrected matrix changes only those three cases' semantic inputs or
requirements. Other cases retain their original controller bytes and contracts;
renaming per-fighter charge recording files does not change those bytes.

## Evidence

Receipts under `A:/FreakMelee-test-archives/`:

| Receipt | Result |
| --- | --- |
| `source-roster-followthrough-oct11-a/receipt.json` | 39 PASS / 3 FAIL; all five fresh source packages prepare; input pins unchanged. |
| `source-roster-followthrough-oct11-b/receipt.json` | Corrected Roy full charge and Mewtwo diagonal pass; historical Luigi misfire still misses coverage; pins unchanged. |
| `source-roster-followthrough-oct11-c/receipt.json` | Seed-2 Luigi probe fails coverage; retained, not passing evidence; pins unchanged. |
| `source-roster-followthrough-oct11-d/receipt.json` | Original-seed, earlier-input Luigi misfire passes; pins unchanged. |
| `source-roster-entry-mode-oct11-matrix-regression/receipt.json` | Default synthesized Luigi ground-side entry still passes after the shared runner refactor; pins unchanged. |

Input matrices and recordings are retained in the corresponding
`source-roster-followthrough-oct11-inputs-{a,b,c,d}` directories. The client and
content-tool executable hashes remain identical throughout; all gameplay
packages use the current interpreter pipeline.

`source-roster-followthrough-oct11-d/consolidated.json` identifies the passing
receipt for every final case. Consolidation verifies that each accepted replay's
bytes and coverage arguments match the final matrix; all three gameplay batches
use the same client hash. It does not claim that the whole matrix ran anew.

## Remaining boundaries

The subsequent [interaction checkpoint](native_source_roster_interactions_checkpoint.md)
adds 32 selected counter/capture/reflector/throw/worn-item cases.

This is selected charge/release/chaining/steering coverage. It does not exhaust
all stage/opponent/seed combinations, interruption, death/retirement,
reflection/absorption, captures or every special's branches. Pixel/audio
acceptance, complete mutable-state definition, restored whole-match rollback
and the 15-ticks frame budget remain separate work. Kirby remains excluded.
The overall goal remains active and commits stay local.
