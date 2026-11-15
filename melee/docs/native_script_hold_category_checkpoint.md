# Interpreted held-item category checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

`items.hold_kind` is now an ordinary read-only `int` property. It calls the
native `itGetHoldKind` getter on the selected held Item. This is the engine's
three-bit hold category, distinct from `items.held_kind` (Item identity) and
`items.held_use_category` (use behavior). An absent physical Item returns zero.
No fighter-specific helper, generated module, or per-tick allocation is added.

The query retains the selected Fighter actor/program/storage and Item identity,
attributes and source specification. Malformed physical classes, missing
attributes and Items already being removed reject the read without publishing
an output. All queries through this backend now confirm the live caller before
checking that its held or auxiliary slot still contains the captured selection.
This ordering avoids reading the slot of a retired caller.

The connected compiler/actor/VM fixture compares all eight representable hold
categories with the actual getter, plus an ordinary engine Item. It checks
absent and stale physical slots, missing attributes for hold/use/weight
classification, wrong generation/scene/class, wrong entity, removal and changed
Fighter kind. Existing pickup/drop/release, retained receiver, Item signal and
held-observation cases run in the same cohort.

## Evidence

- Connected fixture: **1,524 checks / 166 cases**, with **61 native comparisons**
  and the existing **28 held-observation comparisons / 8 receiver signals**.
  `A:/FreakMelee-test-archives/script-held-category-oct10-complete/receipt.json`
  SHA-256 `c720b84f7d62885348a1a28274486c4561d825f6423584c888fa230d1e0183b7`;
  **1,723 independently verified pins**.
- Read-only roster audit: **19/26 forms prepare**. Jigglypuff's 192 visited
  bodies now report **6** failing bodies, down from 8. The two pickup bodies
  pass with the existing script's hold-category branching.
  `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-hold-category/receipt.json`
  SHA-256 `9057bc8d7363f201b65ee3cf7f8259fb75e320b0adedee3c2dfe3d0ac41850d7`;
  **197 independently verified pins**.
- Independent verification:
  `A:/FreakMelee-test-archives/hold-category-independent-pins-oct10.json`.
- Seven Windows targets build; `native_fighter_script` CTest passes.

The fixture controls publication/import data and downstream render, audio and
transform leaves. Its source host dispatches property reads to the actual
Fighter Items backend; it does not execute the complete production property
dispatcher. The production dispatcher is built and roster preparation uses
the real binding catalog. No visible/audible game launch, installed-content
change, complete costume rendering proof or full clone acceptance is claimed.

Jigglypuff's remaining inventory is collision-box/multiple-jump policies and
costume attachment methods. Attachment load/dynamics/cleanup must be migrated
as a cohesive lifecycle group; the documentation labels their existing
generated-C reference implementation accordingly.
