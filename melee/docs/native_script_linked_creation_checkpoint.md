# Interpreted linked creation and terrain observations — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Pikachu and Pichu now reach all **209 script bodies each with five failures**,
down from nine. Full-roster preparation remains **22/26 forms**, excluding
Kirby and counting Sheik/Nana separately. Neither Pikachu nor Pichu is accepted
yet; preparation is distinct from gameplay acceptance.

## Implemented contract

- `item.create_linked(DeclaredArticle, slot, backlink, x, y, z, facing)` uses the
  existing authored child constructor and returns a full-width creation ID, or
  −1 when allocation fails or a live child already occupies the parent slot.
  The declared article must belong to the calling item's published program.
  Both links exist before `on created`; native `on spawned` still precedes them.
  Reflection transfers damage credit while retaining original authorship and
  attachment ownership. No stock special handler or fighter-specific primitive.
- `item.sweep_stage_normal(previous_x, previous_y, previous_z)` executes the
  original stage-sweep helper and retains its normal output and hit/miss side
  effects. `item.surface_normal.x/y/z` are mutable Float fields in ordinary
  authored article storage, not holdable-recipe storage.
- `item.check_ground()` executes the existing item ground-check helper. These
  operations retain source identity across engine work and reject retired actors
  before publishing outputs. No tick allocation or native gameplay-body change.
- Pikachu/Pichu source callsites now use exact article declarations, generic
  linked creation and explicit absent-ID checks directly. No compatibility alias.

## Grouped verification

- Real compiler/binding/constructor/actor/VM fixture: **19,369 checks, 48 cases,
  1,078 native frees and 1,024 storage-reuse cycles**. It exercises all sixteen
  slot/backlink combinations, links visible during Created, occupied/stale slots,
  allocation failure, reflected damage ownership, callback deletion, and the
  existing positional and ordinary child factories.
- Terrain/observation fixture: **9,626 checks, 1,464 cases, 1,141 native-body
  comparisons and 154 VM calls**. It checks original ground/sweep bodies,
  hit/miss normal effects, Float bits and source-retirement publication guards.
- Existing link and nested Article signal regression: **441 checks**.
- Seven Windows targets build; script and parameter CTests pass **2/2**.
  Shipping Pikachu/Pichu sources match repository bytes. Four receipts were
  independently rehashed, including 1,700/1,691/1,702/193 input pins; constructor
  implementation/header and the build log are separately pinned.

The constructor fixture controls asset/model/physics leaves. The observation
fixture controls stage-geometry query leaves. These are connected engine and VM
checks, not complete Thunder Jolt/Thunder gameplay, rendering or rollback proof.
No game window, audio, normal installed package or user draft was changed.

| Evidence | Receipt SHA-256 |
| --- | --- |
| `A:/FreakMelee-test-archives/script-linked-child-oct10-final/receipt.json` | `8af3b54f07aee699ba64cf3aa3ba1587a2f7bfe5ca8b8f9b6b39b5f6c2177d55` |
| `A:/FreakMelee-test-archives/script-normal-sweep-oct10-final/receipt.json` | `ee9db6285eb6592e92463ab90e755135631c440bbab3e2852e95c1e3163675ec` |
| `A:/FreakMelee-test-archives/script-article-links-linked-create-oct10/receipt.json` | `662ba5b3e7204a297c145b1d586cc4a0bcea6cdc201317f8f4fa2291fa2d9b01` |
| `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-linked-child-a/receipt.json` | `da4de297e42eed2cb394e4c8728c344322d916349fc15be0e1aa277f068eb05f` |
| Independent verification | `A:/FreakMelee-test-archives/linked-creation-independent-oct10/receipt.json` |

## Remaining work

Pikachu/Pichu still require linked-item coordinates/joints/collision observations,
their own indexed article-joint observations, and typed Fighter observations.
The audit reports the first failure per visited body; resolving these sites can
expose additional operations in those bodies. Ness retains an animation-resource
metadata prerequisite; Samus lacks a valid independent draft in this census.
Full fighter gameplay and compact rollback acceptance remain open.

The later [article geometry checkpoint](native_script_article_geometry_checkpoint.md)
resolves these body sites. Both sources now compile all 210 bodies with zero body
diagnostics; package preparation still requires auxiliary-item scheduler migration.
