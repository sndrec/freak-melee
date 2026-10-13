# Stock fighter scripting: current coverage

Updated: 2026-10-13. The supported roster has been translated and admitted
to the ordinary interpreter. Remaining work is bounded behavioral/editor
acceptance, not another wholesale stock-code translation pass.

## Supported forms

The 22 single-fighter sources are Mario, Dr. Mario, Captain Falcon, Ganondorf,
Luigi, Jigglypuff, Fox, Falco, Marth, Roy, Donkey Kong, Bowser, Pikachu, Pichu,
Samus, Link, Young Link, Mewtwo, Yoshi, Ness, Peach and Mr. Game & Watch.
Zelda/Sheik and Popo/Nana supply four separately owned family members.
All 26 pass normal source preparation. Kirby remains deliberately excluded
from cloning; stock Kirby remains playable.

Scripts own fighter-specific specials, lifecycle/common/CPU decisions and their
authored entities/resources. Shared engine primitives remain native. Ordinary
package loading never uses generated-C special handlers as a fallback. Donor
identity is authoring provenance, not runtime ownership of editable behavior.

## Recorded behavioral coverage

| Cohort | Evidence | Limits |
| --- | --- | --- |
| 22 single-fighter CPU cases | [CPU checkpoint](native_source_roster_cpu_checkpoint.md) | Fixed-seed scenarios, not all CPU situations. |
| Paired-family CPU and transforms | [Family gameplay](native_source_family_gameplay_checkpoint.md) | Selected Zelda/Sheik and companion sequences. |
| 176 special entries | [Special-entry checkpoint](native_source_roster_special_checkpoint.md) | Ground/air entry coverage across 22 Singles, not every charge/aim/interrupt branch. |
| 42 special follow-through cases | [Follow-through](native_source_roster_followthrough_checkpoint.md) | Selected Luigi, Jigglypuff, Marth, Roy and Mewtwo scenarios. |
| 32 selected interactions | [Interaction checkpoint](native_source_roster_interactions_checkpoint.md) | Stock opponents in this cohort. |
| 18 two-source interactions | [Two-source cohort](native_source_roster_duo_checkpoint.md) | Six packages, selected counters/reflect/capture/throws and independent variables. |
| 100 selected results outcomes | [Results checkpoint](native_source_roster_results_checkpoint.md) | Model/lifecycle/scene admission, not every result pose, pixel, audio or re-entry. |
| Samus grapple/resource and interruption paths | [Samus gameplay](native_source_samus_gameplay_checkpoint.md) | Recorded cases; rare terrain/ledge/retirement paths remain. |
| Rook jump/snack gameplay | [Rook checkpoint](native_rook_interpreter_checkpoint.md) | Custom authoring/presentation and every recipe are separate acceptance. |

Several cohorts consolidate initial runs with corrected reruns. Preserve that
provenance instead of claiming one complete sealed final-binary matrix. Admission
alone is not gameplay; an untested branch is not necessarily unimplemented.

## What remains

- Actual clone/edit/save/reopen/install sessions in the current Workshop,
  including paired members and edited rig/resource metadata.
- Focused untested special, interruption, object retirement/reuse and interaction
  branches when they are relevant or exposed by play. Do not require every
  fighter/opponent/stage permutation before user testing.
- Visual/audio review of changed custom behavior and genuine discrepancies.
- Current source-only package sharing and isolated recipient editing.
- Broader restored-state/match-transition coverage and later multi-PC tests.

The explicit snapshot and GPU-free reconstruction cutovers are already in place;
their current correctness/performance boundaries are in
[rollback architecture](native_rollback_current.md). Optimization is deferred.
Whole-move transfer and Kirby are also deferred. Keep generic script APIs and
the [author-facing guide](scripting/index.md) current as behavior evolves.
