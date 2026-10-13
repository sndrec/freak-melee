# Interpreter visibility, expiry and positional feedback checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch adds generic engine operations required by complete authored fighter
scripts. It does not call stock special handlers or introduce per-fighter API
extensions. The ordinary client executes interpreter bytecode.

## Engine contracts

- `visuals.set_current_group` invokes actual `ftParts_80074B0C`, preserving saved
  selection and invalidating channel-zero draw visibility. `current_group` and
  `previous_group` read the native signed selections; invalid indices yield -1.
  Admission checks the owned imported table and the complete draw traversal.
- `item.hide_model` / `show_model` invoke actual Item whole-tree helpers. Model
  animation rate invokes `lb_8000BA0C`; the native tree walk skips the primary
  root, stops at instance boundaries, and changes descendant joint AObjs only.
  It does not change Item simulation speed or material/texture/shape animation.
- `item.begin_expiry` invokes `it_8027518C`, copying the common expiry timer and
  setting native timer/manual flags. It neither removes the item immediately nor
  invents a visibility/state policy.
- `item.spawn_effect_at_position` supplies the exact synchronous native Vec3 ABI.
  Source-derived capability metadata separates source-root requirements from
  fighter-parent requirements. Of 63 Position IDs, five read source-root scale,
  one additionally requires Fighter userdata, and thirteen create model effects.
  Rootless Items can spawn effects that do not read their root. Fighter-only
  effects reject before construction. Coordinates must be finite.

## Evidence

Hidden private console fixtures execute the actual compiler/VM where applicable
and extracted native helper bodies; no game windows were opened for this batch.

| Grouped fixture receipt under A:/FreakMelee-test-archives | Result |
| --- | --- |
| script-visibility-groups-oct09-final/receipt.json | 17,249 checks; 2,245 complete-Fighter comparisons, including 544 current setters; 774 query comparisons. Controlled program/view admission; actual native table/reset/draw helpers. |
| script-item-model-controls-final2/receipt.json | 2,488 checks; 151 native comparisons total, including 56 new visibility/rate pairs and 20 continuation cases. Controlled actor/graph views. |
| script-item-expiry-final/receipt.json | 387 checks; 47 cases; 57 VM signals; 10 complete Item timer comparisons. Actual owned Item actor/storage and scheduler; controlled asset/geometry leaves. |
| script-position-effect-context-final/receipt.json | 262,346 checks across all 65,536 IDs; source-derived ABI/context inventory and 30 native helper copy cases. |
| script-article-position-final/receipt.json | 1,465 checks; 140 VM-to-native spawns, all 63 Position IDs with/without roots, retirement and invalid argument cases. Spawn leaf captures copied position; no real particle rendering. |

The consolidated native client/content-tool/reference-exporter and selected test
builds passed. Forty integration tests and three C-reference tests passed. The
C exporter remains test-only; ordinary packages do not require generated C.

## Remaining acceptance

No full fighter match, renderer effect appearance, Item DAT construction, or
whole-scene rollback parity is claimed here. Engine JObj class extent is checked
when needed, but HSD exposes no allocation-generation query for arbitrary JObjs.
Effect construction currently has no script-calling construction hook; a future
hook would require its own continuation audit.

The source census still prepares Mario, Dr. Mario and Zelda. Peach's next real
gap is holder held-motion/frame querying; Game & Watch's is absorber creation.
Other secondary-resource boundaries are unchanged. The overall project goal
remains active and incomplete.
