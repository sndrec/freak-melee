# Ordinary interpreted part-animation controls

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The interpreter now supports generic `visuals.set_part_animation`,
`visuals.show_part_animation` and `visuals.hide_part_animation`, alongside the
existing explicit apply operation. Selection only stores the requested variant;
show applies it with zero blend duration; hide removes the active override and
restores the current main animation or costume pose. Hiding retains the saved
choice. These are direct calls to shared `ftAnim` operations, with no stock
fighter handler or generated C involved.

The shared owned-pose validator now admits partial restoration before the first
layer mutation. It checks imported costume descriptor records, runtime rig
topology, current clip ownership/spans, native prefix skipping, subtree depth,
omissions and animation-family remapping. Admission accounts for the override
flags that hiding will clear. The binding retains source/program/storage, part
table, selected animation, costume, clip and import views across the native call;
retirement prevents result publication. Invalid input rejects before mutation.
Showing an unselected group and hiding an inactive group remain native no-ops.

Jigglypuff's death template also uses `visuals.set_group(0, 0)`, matching the
native reset to variant zero. The old `hide_group` spelling obscured that it did
not select the engine's actual hidden variant, `-1`.

See [presentation API](scripting/presentation.md#partial-skeletal-animations)
for authoring semantics.

## Verification

- Connected capture/presentation regression: **99,711 checks / 9,382 cases**.
  New controls run through the actual frontend, prepared program, actor,
  storage and VM. Actual native hide traversal is compared for costume-only,
  main-animation and blended restoration. The production validator also checks
  a registered remapped animation family. Selection/no-op semantics, malformed
  clips/rigs and seven post-native retirement cases are included.
  `A:/FreakMelee-test-archives/script-part-animation-controls-oct10-clean/receipt.json`
  SHA-256 `05b733c105b64d599605e722c8c5f0d848667ff6d15a6884e41c002c13fc7785`,
  1,816 independently rehashed inputs.
- Existing visuals/color/material regression: **18,184 checks / 2,265 native
  comparisons**. Its unavailable part/pose leaves remain explicit aborts;
  part-control coverage belongs to the connected fixture above.
  `A:/FreakMelee-test-archives/script-part-animation-visuals-regression-oct10-final/receipt.json`
  SHA-256 `7be224ed59357a9700439ec976690c0ad66d9a164d5b8165c2e48dcce8d0c728`,
  1,605 independently rehashed inputs.
- Fresh read-only 26-form audit still prepares **19/26 forms**, excluding
  Kirby and counting Sheik/Nana separately. Jigglypuff's 192 bodies are visited;
  reported failing bodies fall **10 -> 8**. The remaining groups are collision
  box/multijump policies, costume attachments and held-item hold-category
  queries. Its complete preparation and gameplay acceptance remain open.
  `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-part-animation-complete/receipt.json`
  SHA-256 `df67391272dde59131d1ab1da59047c8d556f0bb9c3af54eab3446f35da3aa83`,
  194 independently rehashed inputs.
- Seven Windows targets build; reference `native_fighter_script` CTest and
  diff check pass. Independent receipt/input verification is recorded in
  `A:/FreakMelee-test-archives/part-animation-independent-pins-oct10.json`.

The inner costume/main-animation restoration leaves record arguments instead
of evaluating rendered poses. Package assets and the selected pose provider are
controlled in this fixture; actual DAT import and whole-fighter animation
acceptance are not claimed. No game window, audio, installed packages or user
drafts changed. Earlier fixture failures were duplicate/unavailable boundary
definitions after its reused presentation fixture gained new common policies;
the final receipt pins the corrected fixture.

Continue the grouped capability/resource inventory, whole-fighter/paired
behavior acceptance and compact whole-match rollback. Goal active.
