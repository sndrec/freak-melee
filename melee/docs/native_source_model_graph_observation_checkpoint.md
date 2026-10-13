# Interpreter model installation and held-item/shield observations

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch continues retained Link and Young Link source integration after
`8cec730d6`. It connects existing generic engine operations directly to the
interpreter without adding fighter-specific load or shield handlers.

## Connected operations

- `model_graphs.install(Int graph_slot, Int omission_index) -> Bool` calls the
  existing native installer. It selects copied placement metadata and one
  descriptor, inserts matching parts into both poses, registers drawing objects
  and preserves native failure/cleanup behavior. The interpreter retains the
  exact physical Fighter, published program/storage, owned assets and rig
  identities. Publication copies a prepared program into the runtime table, so
  admission compares its identity and bank while retaining the published pointer;
  it does not incorrectly require the prepared and published addresses to match.
- `items.heavy` reads the actual held Item's heavy flag. An absent or no-longer-
  physical slot returns false; a live slot requires valid Item attributes and
  retained actor/program identity. The held-slot relation is checked before
  publication. It performs no callback or allocation.
- The five read-only `shield.pushback_*` values read the native common table's
  damage multiplier, base, velocity multiplier and light-shield range. Scripts
  can combine them with their own editable parameters. Native Float values are
  preserved without clamps or finite-value substitution.

Model creation uses the normal scene heap and belongs at an explicit setup
transition, ordinarily `on load`. The runtime has no invented load-only phase
gate. Duplicate installations fail before allocation. No installer or native
HSD insertion algorithm changed.

See [reserved model parts](scripting/model_graphs.md),
[held items](scripting/held_item_controls.md) and
[fighter observations](scripting/fighter_observations.md).

## Evidence and limits

The client, content tool, source-assets target and selected integration targets
build. Six production input hashes were unchanged across the final grouped build;
all sixteen selected CTests pass. Root independently rehashed these receipts and
their pinned inputs. No game, renderer, GPU or audio device was opened.

| Receipt | Scope | SHA256 |
|---|---|---|
| `A:/FreakMelee-test-archives/script-model-graph-oct10-final/receipt.json` | 1,148 checks, sixteen native/interpreter/VM comparisons, 35 VM calls, fifteen native failures, eighteen rejections and four compiler negatives | `019189755c135ce9fdb4993b94a954167a48db4eb7eb242b4ba70600892afa7c` |
| `A:/FreakMelee-test-archives/script-fighter-observations-shield-weight-oct10-final/receipt.json` | 14,970 checks, 1,201 cases; fourteen heavy comparisons, 356 shield comparisons, 306 shield VM cases and forty observation/compiler refusals | `b6306ff53697613c37430b0bbc8e7d539fcaa9eb5f9ee19b679b923571efdcb0` |
| `A:/FreakMelee-test-archives/source-model-graph-census-oct10-final/receipt.json` | Actual source-package preparation of ten retained full sources | `ed5a751872d0097ee3a77072405f2e51f15b2701b3662e31b8710fa589ac4395` |

Model comparisons execute original insertion and the existing installer across
four modes and four descriptor selections, then use actual compiler/Fighter
actor/VM execution with owned source metadata. Graph allocation and registration
are controlled leaves. A completion observer tests publication guards after
actual cached writes; it is instrumentation, not an authored native callback.
This does not establish costume rendering or whole constructor acceptance.

Observation comparisons use actual ItemView capture/current logic and native
`itIsHeavy` and shield pushback bodies, with controlled payloads and common data.
They cover raw Float patterns and actual source VM reads. They do not establish
complete pickup traversal, shield collision traversal or in-game shield behavior.

Eight full sources still prepare: Mario, Dr. Mario, Peach, Game & Watch, Zelda,
Sheik, Popo and Nana. Link now stops at line 3480 and Young Link at line 3600 on
`collision.air()`. Full authored bodies, including trails, remain intact. The
next cohort is native tether collision and its retained continuations. Source
preparation is separate from paired activation and whole-fighter gameplay.
Companion constructor/member/teardown contracts remain open. Compact whole-match
save/load still uses Regions/Pages as its broad oracle.
