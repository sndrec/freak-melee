# Batch source diagnostics

2026-10-11 refresh: all 26 currently selected supported Workshop forms pass
normal source preparation, including the independent `samus_full.fms` template.
Receipt: `A:/FreakMelee-test-archives/clone-roster-current26-oct11-a/receipt.json`.
It pins current templates/tools and the copied owned source drafts. This is
body/metadata/package acceptance; runtime and rollback acceptance stay separate.

Use the read-only audit before implementing a fighter integration batch:

```powershell
.\build\native\native_content_tool.exe source-audit PROJECT_DIRECTORY SCRIPT_FILE
```

It uses the production package importer, declarations, typed body compiler,
move lowering and bytecode emitter. It does not substitute permissive bindings,
publish actors, modify a draft or install content. `source-check` and
`script-build` retain normal first-error rejection.

With an optional caller-reserved `NativeScriptCompileDiagnostics` buffer, the
compiler collects failures from separate metadata nodes and function bodies.
Each body contributes its first error. A failed body rewinds expressions,
arguments, emitted instructions and interned recipient-export names before the
next body. Any error leaves the candidate invalid and the accepted pointer
unchanged; incomplete code never proceeds to execution/storage publication.

The summary reports stored/total diagnostics, functions checked, metadata/body
coverage, an outer package diagnostic and final package acceptance. Buffer
overflow continues counting/checking without writing past capacity. Structural
syntax, index, pose, asset import and prerequisite metadata failures stop later
dependent stages. `bodies_complete` means every indexed body was visited, not
that all errors within each body or all later callback/resource stages were
checked. Metadata rows can share a single missing entity/schema cause; the raw
diagnostic count is not a task count. A package error after successful bodies,
such as an unsupported hook scheduler, remains a failed audit.

## Initial batch roster inventory

One full audit still admits 16/26 forms, excluding Kirby and counting Sheik/Nana
separately. For the three forms reaching body compilation, the audit now exposes
**37 body errors across 538 visited functions** in one pass:

| Form | Bodies visited | Body errors | Main batches exposed |
| --- | ---: | ---: | --- |
| Jigglypuff | 192 | 23 | Hitbox query/group control; oriented effects; collision-box policy; multijump configuration/drift; explicit motion transitions; callback cleanup; attachment/costume visibility; KO policy. |
| Mewtwo | 182 | 11 | Release signal payload; reflector argument type; capture arming; hip/root animation policy; attack colors; typed owner/Item observations and effects. |
| Bowser | 164 | 3 | Vector normalization and Item ECB upper extent. |

Fox/Falco and Pikachu/Pichu expose prerequisite entity metadata/owned-asset
failures; their bodies are explicitly not checked. DK reaches a missing
heavy-item damage scheduler after all 257 bodies pass. Ness stops before body
compilation at animation-resource metadata. Samus has no valid independent
draft in this census. None of this establishes complete clone gameplay parity.

Group these sites by underlying generic engine capability and source correction,
implement the cohort, then rerun the audit and relevant behavioral comparisons.
Avoid interpreting repeated missing-schema rows as separate implementation work.

The [vector/Item-ECB batch](native_script_vector_ecb_checkpoint.md) subsequently
cleared all 164 Bowser bodies. His package now reaches its missing
`throw_victim_motion` scheduler. Remaining first-per-body diagnostics are 34
(23 Jigglypuff and 11 Mewtwo); preparation remains 16/26 until the scheduler and
later integration contracts also pass. This illustrates both the usefulness of
the batch inventory and its explicit stage boundaries.

The subsequent [shared scheduling batch](native_script_throw_heavy_scheduler_checkpoint.md)
connects six throw, heavy-item and dynamics hooks together. Donkey Kong and
Bowser now pass complete preparation, bringing this audit to **18/26 forms**.
The same 34 Jigglypuff/Mewtwo body diagnostics remain available as a grouped
inventory; the other failures are prerequisite entity/import/resource work.
Each preparation pass still needs later activation and gameplay acceptance.

The [presentation and engine-policy batch](native_script_presentation_policy_checkpoint.md)
then adds four common operations and migrates template callsites to existing
generic APIs. Remaining first-per-body diagnostics fall **34 -> 11**: ten in
Jigglypuff and one in Mewtwo, across 374 visited bodies. Preparation stays
**18/26**. The next capability groups are collision-box/multijump policies,
attachments/part animation and owned Item joint-path effects; prerequisite
resource failures elsewhere remain visible without pretending their dependent
bodies were checked.

The common idle/statistics/jump-sound scheduler cohort subsequently clears
Mewtwo's package preparation: **19/26 forms** now pass. Its last body needed
existing joint-world-position sampling followed by a Position-ABI effect,
matching `it_802C5E5C`; no joint-attached effect extension was necessary.
Three shared hook contracts were connected together, with worn-item sound
dispatch at both shared launch and multiple-jump sites. Jigglypuff retains ten
reported failing bodies; the same six other forms retain prerequisites or an
absent independent draft. See `native_script_common_policy_checkpoint.md`.

## Evidence

The [part-animation controls cohort](native_script_part_animation_checkpoint.md)
adds ordinary selection, show and hide together, preserving native restoration
and saved-selection behavior. Jigglypuff's failing-body inventory falls **10 -> 8**;
pickup callbacks expose their missing `items.hold_kind` query. Attachments and
collision/multijump policies remain grouped targets. The read-only roster still
prepares **19/26** forms; this is not full clone gameplay acceptance.

The [held-category cohort](native_script_hold_category_checkpoint.md) then
connects the generic `items.hold_kind` property using the native Item getter.
Both pickup bodies now pass, reducing Jigglypuff's inventory **8 -> 6** without
changing its template. The roster still prepares **19/26** forms. Remaining
targets are collision-box/multijump policies and the attachment lifecycle.

The [multi-jump/collision-box cohort](native_script_multi_jump_checkpoint.md)
then connects those shared movement operations together. Jigglypuff now reports
**3** failing bodies, down from 6: the Rollout collision body reaches its next
missing oriented-effect operation, plus the accessory load/remove bodies.
Preparation remains **19/26** forms. The batch's native-body comparisons verify
the new interpreter plumbing; complete costume and gameplay acceptance remain
separate work.

- Connected compiler/ground-controls regression: **115,211 checks / 1,852
  cases**. Six new audit cases cover multiple independent body errors,
  successful later bodies after failure, recipient-name rollback, bounded output,
  ordinary first-error mode, multiple metadata failures, structural early stop
  and successful preparation. This adds 20 checks to the existing native/VM
  regression; it does not add runtime gameplay acceptance.
  `A:/FreakMelee-test-archives/script-compile-audit-oct10-a/receipt.json`
  SHA-256 `9f1030c0fcd08db77a82974fce5253fb966e555f999362c65d061c0ff1ba82ca`,
  1,742 independently rehashed pins.
- Real 26-form audit:
  `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-final/receipt.json`
  SHA-256 `e80a42834dce75d22e634a08322c39b301655cb791aad5733fd96aa54705f693`,
  183 pins, including the tool, compiler/package/tool changes, templates and all
  files in the private copied drafts. Raw compiler diagnostics total 407,
  including dependent metadata rows; the completed body passes contribute 37.
- Independent verification:
  `A:/FreakMelee-test-archives/compile-audit-independent-pins-oct10.json`.
  Seven Windows targets build, `native_fighter_script` CTest passes, and diff
  check passes. No visible/audible game launch or installed-content change.

## Current roster refresh

The [Pikachu/Pichu source split checkpoint](native_script_pikachu_split_checkpoint.md)
refreshes this census on the current build and templates: **24/26 forms** now
pass preparation. Pikachu/Pichu compile all 210 bodies without body diagnostics after
the [article geometry batch](native_script_article_geometry_checkpoint.md).
The [auxiliary-item/jab scheduler checkpoint](native_script_auxiliary_scheduler_checkpoint.md)
connects the remaining declared hooks; Pikachu/Pichu now pass full preparation.
Ness now passes its [animation-resource prerequisite](native_script_animation_components_checkpoint.md);
its current audit visits 418 bodies and reports 14 first-errors-per-body after the
[common-state batch](native_script_ness_common_state_checkpoint.md), still
rejecting publication. Samus has no valid independent draft in this census. Preparation remains distinct from connected gameplay acceptance.

The [connected Pikachu/Pichu interpreter checkpoint](native_script_pikachu_gameplay_checkpoint.md)
now passes 46/46 focused stock/clone replay pairs after correcting native empty
visibility selectors and omitted authored Skull Bash effect flags. That gameplay
evidence does not imply full acceptance of the other prepared forms or the
documented wider lifecycle/visual/network cases.

The following [vector/collision batch](native_script_ness_vector_checkpoint.md)
reduces Ness to 10 first-errors-per-body across the same 418 indexed functions.
The current 26-form read-only census still accepts 24 forms; Ness rejects and
Samus has no independently prepared draft. This is preparation coverage,
not stock/clone gameplay parity.

The subsequent [entity/environment batch](native_script_ness_entity_checkpoint.md)
leaves three Ness diagnostics across the same 418-body audit: dynamic child
selection and two resource-joint animation removal paths. The 26-form census
still accepts 24; Ness rejects and Samus has no valid independent draft. Later
preparation and gameplay can reveal further work after those bodies compile.

The [resource-animation batch](native_script_resource_animation_checkpoint.md)
compiles all 418 Ness bodies without a body-level diagnostic. The subsequent
[smash-entry batch](native_script_ness_smash_hook_checkpoint.md) integrates both
entry schedulers: the canonical audit now reports zero diagnostics, complete
metadata/body coverage and package acceptance. The current census accepts
25/26, with Samus remaining. Private Ness package publication also passes;
neither preparation nor publication establishes actor/gameplay parity.
