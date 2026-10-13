# Interpreted roster: match-end and results checkpoint

2026-10-11. All 100 selected results outcomes pass across the initial full run
and a fresh 24-case post-fix rerun. The initial run had 84 passes and 16 failures;
all 25 bases (26 scoped forms, excluding Kirby) passed ordinary source-package
preparation. A shared animation-query contract fix resolves the failures in
Donkey Kong, Link, Young Link and Peach. The final rerun passes all four outcomes
for those fighters plus Mario and Ice Climbers, with six fresh preparations.
This is combined bounded evidence, not one sealed 100-case final-binary run.

## Coverage

`tests/native/fighter_roster_results_parity.py` prepares each package through
the ordinary clone, parameter import, source build, package, verify and accept
paths in a private collection. It includes 22 single-fighter bases, Zelda-primary
and Sheik-primary alternate packages, and the Popo/Nana companion package.

For each base, three recordings choose the three ordinary victory variants;
a fourth loses. Each recording contains 900 controller samples. The offline
position fixture moves the losing fighter below the blast zone. Ordinary
Fall, death, game-end and results construction then run; the fixture does not
invoke result callbacks directly. The host runs rendered, unpaced, hidden and
muted stock/source comparisons.

Acceptance checks require the natural versus-to-results scene transition,
the expected actual result model count and selected variant, complete input
consumption, and fault-free gameplay/results script owners. The companion
requires two result models; an alternate package requires its selected member.
Scene and result diagnostics must agree with the corresponding stock run.
The scene exit also reports authored state before destroying gameplay actors,
so a fault cannot disappear merely because its actor retired.

## Shared fixes

- Results admission compared the absent companion value `-1` with the positive
  `Ft_Kind_None` enum. It consequently required a nonexistent partner for single
  packages. Admission now requires a partner only when the partner mapping is
  nonnegative. Diagnostics distinguish primary and partner clip/registration
  failures.
- `animation.clip_length` required the gameplay table even during results
  construction. Stock load callbacks for the four affected fighters query
  gameplay IDs while their results actor has selected the smaller demo table.
  `ftData_80085E50` returns NULL for an ID absent from the active table, and
  `lbAnim_8001E8F8` returns zero for NULL. The interpreter now admits the owned
  gameplay or demo table with its matching count and returns that same zero.
  It does not substitute the gameplay clips or add a fighter-specific exception.
- Forged table/count combinations, null tables and indices outside the admitted
  range still fail before publishing a result. The kinetic fixture covers both
  active banks, empty/absent clips and those rejection paths.

The fighter scripts and stock behavior handlers were not changed for these
fixes. Client and content-tool builds succeeded. The kinetic fixture passes
26,148 checks, including 7,104 native-body comparisons; its asset publication
and actor identity are controlled console boundaries.

## Receipts

| Evidence | Result at this checkpoint |
| --- | --- |
| `A:/FreakMelee-test-archives/source-roster-results-oct11-a/receipt.json` | Full initial cohort: 25 preparations pass; 84 cases pass, 16 fail; pinned inputs unchanged. |
| `A:/FreakMelee-test-archives/source-roster-results-oct11-b/receipt.json` | All 24 cases pass: the four affected bases plus Mario and Ice Climbers, four outcomes each, with six fresh preparations; all 93 pins independently rehash unchanged. |
| `A:/FreakMelee-test-archives/script-kinetics-demo-oct11-c/receipt.json` | Console regression passes. Earlier `-a`/`-b` attempts failed compilation and are not runtime acceptance. |
| `A:/FreakMelee-test-archives/source-results-oct11-demo-build.log` | Native client and content tool rebuild succeeded. |

Both gameplay receipts report unchanged inputs during their own runs. The
initial run used the pre-query-fix binary; its successful cases are retained as
that evidence, not relabelled as runs of the final executable. Taking the newer
result for each repeated outcome gives 100 unique passing cases.

## Limits

This cohort checks actual model admission, lifecycle ownership and scene flow.
The gameplay comparer stops its fighter/item/RNG trace at match end; the results
checks do not compare model pose every frame, pixels, audio or result-scene RNG.
Recordings finish in results, so they do not establish return-to-CSS/re-entry or
every alternate-family ending form. This is also separate from a live Workshop
clone transaction and from compact whole-match save/load acceptance.

See the [CPU roster](native_source_roster_cpu_checkpoint.md),
[family gameplay](native_source_family_gameplay_checkpoint.md) and
[two-source interactions](native_source_roster_duo_checkpoint.md) for the other
bounded gameplay cohorts.
