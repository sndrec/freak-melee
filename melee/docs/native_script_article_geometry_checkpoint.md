# Interpreted article geometry — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Subsequent [scheduler checkpoint](native_script_auxiliary_scheduler_checkpoint.md):
Pikachu/Pichu now pass full preparation, bringing the current census to 24/26.
The rejected package receipts below describe the earlier geometry boundary.

Pikachu/Pichu now compile all **210 bodies each with zero body diagnostics**,
including the new script-owned Thunder feedback helper. Both packages still
fail preparation at an unmigrated `auxiliary_item_attach` scheduler hook.
Full-roster preparation remains **22/26 forms**; this checkpoint does not claim
either fighter is ready for gameplay acceptance.

## Implemented

- Exact declared Article references expose read-only physics `position.x/y/z`
  and ordinary article `surface_normal.x/y/z`. The captured actor and declared
  schema remain authoritative; absent references return zero, stale captures
  fault, and native Float bits are retained. No model/collision evaluation.
- `articles.read_bone_position(handle, DeclaredArticle, index, x, y, z)` uses
  the existing indexed native Item bone table and `lb_8000B1CC` world-position
  helper. It works for Fighter/Item callers and the calling article itself.
  Bounds/missing data return false without modifying outputs. Actor, model,
  table, joint classes and parent-chain identities are retained through admission
  and evaluation. Fixed invocation scratch; no heap allocation.
- `item.copy_link_collision(slot)` copies the complete native collision record
  through validated same-bank links, without callbacks or copying other fields.
- Pikachu/Pichu follow the exact Ground/Air article declarations directly, use
  the existing `item.dynamic_joint(index).rotation` and `owner.action.category`,
  and run quake/rumble decisions in a script helper on the original fighter.
  The helper uses general camera/controller primitives and matches the stock
  helper's fighter position, quake kind and rumble arguments. No new specific
  fighter API, legacy alias or call to a stock special handler.

## Verification

- Real compiler/VM/published actor/retained-reference fixture: **176 checks**.
  Six physical fields include signed-zero, NaN/subnormal bits, absent references
  and read-only rejection. Bone queries cover Fighter/Item callers, real VM
  output publication, bounds, cycles and class/table/parent mutations.
  The native joint-position and scalar matrix bodies run; asset/class data and
  matrix-setup leaves are controlled. Full constraints or gameplay are not proven.
- Compiler/receiver-routing fixture: **109 checks**, including exact bone-query
  declaration identity, output layout, integral index typing and rejected writes.
  Its engine leaves deliberately reject; it establishes compiler contracts.
- Link/signal/collision fixture: **465 checks**. All four collision-copy slots
  compare complete Item bytes with the existing native copy implementation;
  absent links preserve the source. Prior ownership/reflection/reuse/signal
  lifetime coverage remains.
- Seven Windows targets build; script and parameter CTests pass **2/2**.
  Shipping templates match. Four final receipts independently rehash
  **1,706/102/1,702/195 inputs**, with additional production and build-log pins.
  Earlier failed fixtures remain archived; their build lists were stale, including
  a `.c.c` filename and missing rejecting leaves. No admission guard was relaxed.

No game window/audio, normal installed package or user draft was changed.

| Evidence | Receipt SHA-256 |
| --- | --- |
| `A:/FreakMelee-test-archives/script-article-bone-oct10-final/receipt.json` | `5e3fc583e2352e8fe9a435270f65d057290ee0093bfb596231d8b0c4d8a4f5c3` |
| `A:/FreakMelee-test-archives/script-article-bone-catalog-oct10-final/receipt.json` | `345fa1ba48786c37bb6afe0b47685fe728f52213fc9b65b78c00fd8b1c8ef580` |
| `A:/FreakMelee-test-archives/script-article-geometry-links-oct10-final/receipt.json` | `d7506e3a11fa58e6e653a80191b17097d9ae389ab31d2ccdb07ad2e862dbf0ff` |
| `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-geometry-final/receipt.json` | `8abd741e3852a0f54174b68d016e5abcc01470832c7d8c51b3a2bd65fd5c780a` |
| Independent rehash | `A:/FreakMelee-test-archives/article-geometry-independent-oct10/receipt.json` |

## Next boundary

Migrate auxiliary-item scheduler callsites and check every declared Fighter hook
before package publication, then run connected Pikachu/Pichu gameplay comparisons.
Package admission currently reports the first unsupported scheduler hook; zero
body diagnostics do not imply that all scheduler contracts are wired. Ness retains
its resource-metadata prerequisite; Samus still needs a valid independent draft.
Full fighter and compact rollback acceptance remain open.
