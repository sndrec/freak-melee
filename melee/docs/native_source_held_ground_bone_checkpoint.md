# Interpreter held creation, item grounding and recipient bone checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This slice connects three generic operations without stock special-handler
delegation: prepared held-Article creation, native Item ground/air markers and
recipient raw-bone sampling. Link and Young Link now name their exact declared
Articles in factory and owned-signal calls; unused physical-kind attributes were
removed. Their tuning, callbacks and move decisions remain authored source.

## Shared operations

[`articles.create_held_at_center`](scripting/held_center_factory.md) selects an
immutable Article declaration during preparation. It uses the native ECB item
center, preserves the supplied previous XYZ position and runs construction,
Created and pickup in native order before registering the held item. Retained
caller and child identities are checked across synchronous callbacks. Raw parts
retain native integral width; creation handles preserve all 32 bits, including
the existing `0xffffffff`/`-1` ambiguity. Mutable damage ownership and attachment
changes are respected. Constructor NULL is a healthy absent result if the
caller survives; it can include an unpublished child cleaned after hook failure.

[`item.grounded`](scripting/item_ground_state.md) writes through the unchanged
native ground/air helpers. Air entry also clears `xDC8_word.flags.x1F`.
Workshop holdables retain their motion-based read projection and reject these
writes explicitly. Existing `item.airborne` keeps its separate direct-field
contract. No original native physics or collision algorithm changed.

[`targets.read_bone_position`](scripting/target_bone_queries.md) reads the
target's raw imported part slot, with a single native vector sample and staged
XYZ publication. It accepts foreign stock targets and another source package;
it does not require matching caller/target banks. An initially faulted target
can still be read while its owned data is live. Native parentless translation
and cached matrices retain their original behavior. Evaluated native constraints
have bounded rig/provenance admission; a direct HSD classifier identifies the
loader's private NULL-expression dummy without replacing matrix math.

## Verification

The actual client, content tool and source-assets target build. Thirteen selected
CMake integration checks pass, covering pools, DAT import, heap, script resources,
combat, presentation, Item actors/scheduler/programs and move entry. No game,
GPU or audio process was launched for this slice.

Held creation passes 2,882 checks / 70 cases / 65 native argument comparisons /
56 VM calls. The actual compiler, prepared owner resolver, actors, fixed GObj
pools, Item constructor/scheduler and pickup wrapper execute. Selected package,
model, attachment, animation and audio leaves are controlled. Child retention
starts after construction; this does not prove detection of arbitrary payload
replacement before capture, retail DAT rendering or complete Link behavior.

Receipt: `A:/FreakMelee-test-archives/script-held-center-oct09-final2/receipt.json`;
SHA256 `c0de4eefdb7a65fe928737838aabdcb8c2cb8f4b4dc4ffb303227c8a5c3e8efc`.
Root independently verified all 1,685 pinned inputs. The migrated prior factory
regression passes 3,150 checks / 24 cases, including 1,024 constructor/free cycles;
its receipt is `A:/FreakMelee-test-archives/script-item-spawn-held-ownership-oct09-final/receipt.json`,
SHA256 `5c5071030ef4cddedb776426d04adb77506f91fde745f61fccab2b4025806eb7`.
All 1,679 pinned inputs were independently verified.

Item grounding passes 378 checks / 62 cases / 29 complete native Item comparisons /
seven actor VM calls. Actual program preparation and Item actors execute over
controlled assets/userdata. Strict Bool typing, ownership/storage failures,
the live Destroyed window and holdable rejection are covered. Retail collision,
rendering and full Sheik behavior remain outside this proof.

Receipt: `A:/FreakMelee-test-archives/script-item-ground-state-oct10-final/receipt.json`;
SHA256 `afe79a9581b04e398566080e1e62741bfc67a804e2ebf4f067e0ee1e2a18b5cd`.
Root independently verified all 1,707 pinned inputs.

Recipient bones pass 260 checks / 26 cases / 17 native comparisons / two query
VM calls. Actual compiler, actors, storage and fixed pools execute across two
source banks, foreign stock targets and Fighter/Item callers. Native root/SRT
sampling and atomic output publication are compared; 25 negatives and 11
controlled observations cover type, lifetime, schema and unsafe expression
destination channels. PATH, vector and callback/effect destinations reject
before evaluation. The native dummy loader/classifier executes, but full retail
IK/constraint sampling is not proved. Graph construction and destruction memory
are controlled; publication checks are not arbitrary destructive native callback
containment. Existing HSD lazy allocations remain outside a no-allocation claim.

Receipt: `A:/FreakMelee-test-archives/script-target-bone-queries-oct10-final2/receipt.json`;
SHA256 `6f451de69829fce4e1c7221a5da3d47458c21709af4bb4c4e3459321fa4b1fed`.
Root independently verified all 1,703 inputs. The separate actual typed costume
import audit finds 49 joints in each Popo/Nana costume and no RObj/IK descriptors
on their queried raw 29/47 ancestor chains. It does not construct live graphs.
Its receipt is `A:/FreakMelee-test-archives/script-target-bone-costume-oct10-02/receipt.json`,
SHA256 `a6a04ed692e38c838ed1eab9c1752aab6ca10830a8d029a1d687d5fe5acc9d79`;
all 18 inputs were independently verified.

Older target queries pass 475 checks / 64 native comparisons; recipient exports
pass 757 checks / 86 cases / 47 VM calls over three source banks. Their isolated
cohorts explicitly abort if the unselected bone engine path is called. Existing
omitted dependency closures in the old query runner were repaired; no successful
bone or unrelated engine stub substitutes for the connected fixture.
Current receipts are `A:/FreakMelee-test-archives/script-target-queries-oct10-pause-final/receipt.json`
(SHA256 `3ad783f9cc7f99886b6c9d775ea27664378403879bfd4a82f4e63cb12ea7a4c6`)
and `A:/FreakMelee-test-archives/script-target-exports-oct10-pause-final/receipt.json`
(SHA256 `a949cb60547dbf0869a35e1e506f51602e21d4f349d3e19b7b2ff2e964221ae9`).
Root independently verified their 1,688 / 1,690 inputs after the final change.

## Resume boundary

The ten retained source bodies still prepare Mario, Dr. Mario, Peach, Game &
Watch and Zelda. Exact next diagnostics are Link 521 / Young Link 525 at
`visuals.held_item_visible`, Sheik 1320 at `item.sweep_segment`, and Popo 641 /
Nana 671 at `targets.motion`. These are preparation diagnostics, not runtime
fighter acceptance. No bodies were removed to obtain a passing check.

Receipt: `A:/FreakMelee-test-archives/source-held-ground-bone-census-oct10-pause-final/receipt.json`;
SHA256 `49ef6f0f396ededa7ab8ad3365cf4b676ff5a2ab7494af84818e158856534de4`.
Root independently verified the rebuilt tool and all ten source hashes.

Next work is the remaining shared bindings, paired-role/member mapping and full
fighter behavioral acceptance. Public resource-animation application and compact
whole-match save/load remain unfinished; Regions/Pages still owns broad
snapshots. The user requested a pause at this coherent integration boundary.
