# Interpreter input, queued item effects and CPU destinations

Ordinary interpreted fighters can now use the native catch/taunt input checks,
CPU destination preparation and assignment, and owned CPU scalar settings.
Articles and package items can queue effects on a prepared primary-model joint.
Link and Young Link now use the existing `controls.try_aerial_jump()` spelling
directly; the unchanged native aerial-jump kernel is the same operation.

## Behavior and boundaries

- [Ground controls](scripting/ground_controls.md) preserve native held-item
  priority and catch eligibility, and taunt/bonus ordering. The input methods do
  not implement authored catch-policy hooks or tether behavior. Common-entry
  leaves are controlled in the fixtures; outer lifetime checks do not prove
  safe continuation inside every native setup routine after owner retirement.
- [Movement](scripting/movement.md) retains owned Fighter/program/storage
  identity across aerial-jump, ground/ledge and cliff predicates. Their native
  entry routines remain unchanged.
- [CPU controls](scripting/cpu_policies.md) expose destination preparation,
  destination assignment, approach spacing, mode, follow distance, level and
  two candidate gates. Native nonfinite destination and invalid follow-distance
  inputs remain no-ops; other scalar fields preserve native values. Existing
  generic CPU-level reads remain available; writes require an owned target.
  History, broader planners and simultaneous companion activation remain open.
- [Article models](scripting/article_models.md) prepare literal joint paths and
  modes 0/3 once, then call the actual native async effect queue. Exact Int
  effect arguments are enforced during compilation and execution. Six effect
  IDs using the same native joint-plus-facing-pointer helper are recognized.
  The native queue borrows joint pointers until flush and can grow its allocator;
  this is not allocation-free tick execution or a graph-replacement guarantee.

## Verification

The client, content tool, source-assets and selected integration targets build.
All 16 selected CTests pass after the final exact-type correction. Console
fixtures run production compilation, actors and bindings with unchanged native
reference kernels. No game, GPU or audio device was opened.

| Grouped fixture | Checks | Native comparisons | Interpreter coverage |
| --- | ---: | ---: | --- |
| Ground controls including catch/taunt | 89,151 | 4,537 | 4,581 source invocations |
| Ledge/jump ownership | 1,752 | 24 additional generic pairs | 54 additional identity cases across direct/VM paths |
| CPU destination/scalars | 5,534 | 1,438 | 331 calls, including three receiver calls |
| Existing CPU command/selector cohort | 7,249 | 612 queue / 80 selector pairs | Connected actor routes |
| Item async effects | 70,928 | 180 | Six Item/Article actor calls |
| Native effect argument-family inventory | 262,347 | 65,536 IDs | Native helper ABI comparison |

Particle resources/allocation and common motion setup are controlled leaves.
These comparisons establish the connected operation contracts, not complete
fighter gameplay, pairing, rendering or whole-match rollback acceptance.

Root independently rehashed all connected inputs of these final receipts:

| Receipt under `A:/FreakMelee-test-archives/` | Inputs | SHA256 |
| --- | ---: | --- |
| `script-ground-catch-taunt-oct10-final2/receipt.json` | 1,727 | `20e869a495429daf792e865735720b3352c1fc01409a5e224a51e495a4678d3c` |
| `script-owned-ledge-jump-retention-oct10-final2/receipt.json` | 1,701 | `85efd6188c13bd3c066038a76c037773f683c861cbe2880ebf6600c1aec721a6` |
| `script-cpu-destination-scalars-oct10-final4/receipt.json` | 1,704 | `3eccdc21aae9e4fa0192b39d37fd85bfb7248cbc457aa8fd697100515cebc45c` |
| `script-item-async-effect-oct10-final/receipt.json` | 1,699 | `552cc08509bc62f772643a6ccb58ac4fc1ad207711bd224b96c91725ad324938` |
| `script-position-effect-abi-facing-oct10-final/receipt.json` | 1,582 | `531ed36b1b5276741e6852bb4d6031f95d111ff1da88b43c97eb6af565fe3956` |

## Remaining work

The rebuilt tool still prepares the retained full Mario, Dr. Mario, Peach,
Game & Watch, Zelda and Sheik sources. The next first diagnostics are Link 754 /
Young Link 758 at `callbacks.accessory_before`, Popo 1414 at its older creator
helper-call syntax, and Nana 1377 at `targets.read_position_delta`. All retained
source bodies remain present.

Link's accessory-before/hitlag phases are distinct native slots, not aliases for
the existing accessory/cue phase. Add their actual scheduler and snapshot
integration before changing callsites. Catch-policy hooks need a separate
ownership-aware integration. CPU history/planners, paired roles and activation,
public resource-animation application and complete fighter behavior remain
unfinished. Compact whole-match save/load still uses broad Regions/Pages as its
oracle. The full goal remains active.

Preparation census:
`A:/FreakMelee-test-archives/source-input-async-cpu-census-oct10-final3/receipt.json`,
SHA256 `6ad21d6801d45c715f82d15657751306d2390e2cf4ef4be28197c4c2289fde0b`.
Root independently rehashed the rebuilt tool and all ten sources.
