# Interpreted single-fighter roster CPU checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. This checkpoint compares normal independent Workshop packages with
stock gameplay, beyond source preparation. The final sealed batch passes all
22 cases: 39,600 inputs per runtime, with complete replay consumption, no reported
source/fighter/item VM faults and unchanged input hashes.

## Workload and ownership

`tests/native/fighter_roster_source_parity.py` attempts every selected fighter,
even when preparation, publication or gameplay comparison fails. Each case
starts from `clone-stock`, imports the exact shipped template's parameters,
checks/builds interpreted source, packages the project, verifies its canonical
packet and accepts it into a fresh private collection on A:. Existing installed
content and user drafts are untouched. No generated behavior C or DLL is loaded.

The native client runs two separate rendered, hidden, muted processes per case:
stock and independent source. Both use level-nine CPU P1, neutral human Zelda P2,
Final Destination, four stocks, 1,800 controller samples and initial RNG seed
`0x1BADB002`. The replay supplies neutral external input; ordinary CPU code,
scheduler, animation, stage collision and article callbacks execute. There are
no direct script callback injections or manufactured parent Fighter objects.

The strict comparator checks both players' fighter/action/damage/contact traces,
item state/event/model/material traces, linked resources and RNG. Only the
expected independent kind/behavior identity is normalized. Numeric tolerances
and ignored digest fields are not used. The receipt records actual motion
coverage and complete replay consumption. Any source/fighter/item interpreter
diagnostic or missing/faulted live fighter report rejects a case.

The harness pins itself, both executables, ISO, template sources, parameterized
owned source, copied project files, canonical packages, replay, placeholder
portrait and comparison tools before and after the batch. Hashing streams the
ISO rather than loading it into memory. Portraits are private placeholders.

## Corrections found together

The initial 22-fighter batch had 13 passing cases and nine failing cases. The
failures included script omissions, native portability and observation errors;
source preparation alone had accepted all nine.

- Mario, Dr. Mario and Luigi explicitly reset command zero and throw cues at
  the stock move-entry sites. Those resets belong to their scripts; there is no
  implicit interpreter prologue. Luigi's fireball creation effect passes its
  original facing scalar through the existing general effect primitive.
- Game & Watch copies its special-attribute scratch at Load. Its attached props
  query `targets.motion(item.current_owner)` rather than the stricter held-item
  relation. They are attached to the fighter without occupying its held slot.
- Yoshi's squash table values and turn intermediates now round at the same
  binary32 assignments as stock.
- Hookshot attributes, collision-node extents and model matrices use the
  attachment owner's instance scale, not its animated root-joint scale. The
  native reference's retail PPC stack-spill offset wrote outside a PC local;
  the portable local spill keeps the intended MSL calculation without that
  stack-layout dependency. Spool interpolation preserves float-rounded
  constants, double arithmetic and the final binary32 store.
- Young Link's fire arrow creates its original stage-embedding effect and uses
  its own sound bank. The ordinary article API exposes a primary-root effect
  with a copied local offset; no Young-Link-specific primitive was added.
- Generic effect admission recognizes the two early synchronous world-position
  generator families, including Mewtwo's existing full-charge flash. A separate
  Joint/Vec3 argument family covers local-offset attached effects. `double(value)`
  exposes the existing VM binary64 conversion for any script.
- Live animation admission accepts the native unanimated translation/rotation/
  scale references installed by hand attachments, only when their targets are
  current attachment-owner bones. Foreign or animated references, expressions
  and duplicate channels remain rejected. All direct callers retain the actual
  attachment owner; no compatibility path is introduced.

Read-only diagnostics were corrected separately: Ness's yoyo linked nodes,
Link/Young Link's boomerang extra models and both arrows' extra models now appear
in the same ordered graph view as script-owned resources. Those changes do not
modify the stock simulations or advance animation while tracing.

## Supporting evidence

Artifacts are retained under `A:/FreakMelee-test-archives/`:

- `source-roster-cpu-oct11-final-a/receipt.json`: all 22 current single-fighter
  packages pass their 1,800-sample stock/source comparisons in one sealed run.
  Every case consumes the full replay; source, fighter and item diagnostics are
  empty and all live fighter fault reports are zero. All input pins are unchanged.
- `source-roster-cpu-oct11-a/receipt.json`: initial complete batch, 13/22 pass;
  failures are retained. Subsequent b through i receipts record grouped fixes.
- `source-roster-cpu-oct11-i/receipt.json`: both Link and Young Link pass the
  entire 1,800-sample comparison with unchanged pins and no VM diagnostics.
- `script-position-effect-roster-oct11-final/receipt.json`: independent native
  argument-family inventory, 65,536 IDs, 524,674 checks and 55 actual helper-copy
  cases. Includes numeric generator branches and the typed local-offset helper.
  Allocation/particle leaves are controlled; this is not rendered VFX acceptance.
- `script-item-descriptor-attachments-oct11-a/receipt.json`: five descriptor
  stages for both boomerangs, both bombs and a synthetic sibling graph, comparing
  native HSD kernels and fixed-pool controller behavior. This controlled fixture
  does not directly test the new live attachment-owner admission. Real roster
  cases exercise its positive path; negative attachment cases remain source
  checks rather than a new fixture claim.

The client/content tool and refreshed `native_script_compile_test` build.
`native_script_compile` and `native_samus_template_source` CTests pass. Existing
stock compiler warnings remain; this is not a warning-free build claim.

## Boundaries

This is one fixed-seed CPU cohort for 22 single-fighter templates, not exhaustive
whole-fighter completion. Zelda/Sheik alternate members and Popo/Nana companion
packages have a subsequent [family gameplay checkpoint](native_source_family_gameplay_checkpoint.md)
covering CPU play and explicit transform/special sequences. Broader paired
acceptance remains separate. Kirby remains excluded
by the agreed scope. Different seeds, stages, opponent actions, every special,
rare interruption/death/retirement paths and full results flow are not covered
by this cohort. Read-only hashes do not constitute pixel/audio acceptance or a
complete mutable-state definition. Compact whole-match savestates and the
15-rollback-ticks frame-budget target remain open. The overall goal stays active.

A subsequent [special-entry checkpoint](native_source_roster_special_checkpoint.md)
covers all four specials from ground/air across these 22 bases, including
deliberately unavailable aerial entries. Its initial 176-case batch and focused
reruns are separate evidence, not retroactive expansion of this CPU receipt.
