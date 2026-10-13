# Two interpreted fighters: interaction checkpoint

2026-10-11. All 18 selected controller interactions pass stock/source comparison
across the initial batch and one corrected fixture rerun. Both participants use
fresh independent source packages. This extends the
[interaction checkpoint](native_source_roster_interactions_checkpoint.md),
whose opponents used stock behavior. It is not exhaustive fighter completion
or a final sealed 18-case gameplay run.

## Coverage

Six packages are freshly prepared: Mario, Fox, Marth, Roy, Mewtwo and Peach.
The stock run uses the corresponding two stock fighters; the source run uses
the two ordinary independent packages from a private shared collection. Both
runs consume the same recording and offline positioning/item fixtures.

| Actors | Selected interactions |
| --- | --- |
| Marth / Fox and Roy / Fox | Ground and airborne laser counters. |
| Marth / Peach and Roy / Peach | Sword contact against Toad. |
| Marth / Mario and Roy / Mario | Counter against a melee attack. |
| Marth / Marth | Counter with both actors using the same package. |
| Mewtwo / Mario | Confusion reflection; grounded right/left, airborne and shield captures; forward throws in both directions; Disable against an exposed or shielded target. |

The shared runner now prepares all required packages before gameplay, reports
every dependent case if preparation fails, and supports `--p2-package-id`.
Both independent identities are checked by the comparator. Source coverage
requires exactly two live, fault-free actor reports, rather than accepting the
presence of one report. The existing single-source mode still requires one.

The same-package Marth case reports two actors of kind 34. Their source state
bytes have different digests, and the declared `counter_damage` values finish
at 9 and 0 respectively. Only one countered. This provides an actual instance
of distinct mutable state for actors sharing a program; it does not cover
every state field, callback lifetime or rollback restoration.

## Fixture correction

The original Roy/Mario case matched stock/source traces but failed coverage:
Mario's forward tilt hit before Roy's counter guard became active. The stock
guard enables at animation frame 8. Moving only Mario's attack press from
controller sample 180 to 186 exercises the counter response (motion 370).
Roy's counter input is unchanged. Both reference and source use the revised
recording. No production gameplay, script template or client binary changed.

## Evidence

Artifacts under `A:/FreakMelee-test-archives/`:

| Receipt / manifest | Result |
| --- | --- |
| `source-roster-duo-oct11-inputs-a/matrix.json` | Initial 18 recorded contracts. |
| `source-roster-duo-oct11-a/receipt.json` | Six packages prepare; 17 PASS / 1 coverage FAIL; input pins unchanged. |
| `source-roster-duo-oct11-inputs-b/matrix.json` | Same cohort with only the Roy/Mario attack timing corrected. |
| `source-roster-duo-oct11-b/receipt.json` | Fresh Mario and Roy packages; corrected case PASS; input pins unchanged. |
| `source-roster-duo-oct11-b/consolidated.json` | All 18 accepted cases match the final matrix's input bytes/arguments; both trace log hashes and two actor reports verified; common binary hashes confirmed. |
| `source-roster-duo-oct11-default-regression/receipt.json` | Existing single-source synthesized Luigi side-B entry PASS. |

`tests/native/fighter_roster_scripted_opponents_matrix.py` builds the cohort.
`tests/native/fighter_roster_special_parity.py --matrix` runs it; use
`--retry-failed-from` to group failed-case reruns. Tests stay hidden and muted,
run unpaced with rendering enabled, and record complete replay consumption.
Emitted action, fighter state, hit/hurtbox and model digests, damage, items,
resources and RNG must match; the fixtures also demand the relevant motions
and contacts. This checkpoint involved Python harness changes, not a native
rebuild or a compiler-warning check.

## Remaining boundaries

This cohort covers two source actors, not every roster pairing, move branch,
seed, stage, interruption or retirement. Pixels/audio and complete restored
match state remain separate acceptance. Ordinary Workshop template selection
and staged source files were inspected for all 26 scoped forms, but that is
static evidence rather than a new live UI clone transaction. Kirby remains
excluded. The full project goal stays active; commits remain local only.
