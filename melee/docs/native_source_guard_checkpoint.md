# Direct interpreter: guard volumes and owned reactions

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch adds a generic Fighter guard-volume family. Scripts own reaction
decisions and bind an explicit root helper; no stock counter handler is called.
The [guard reference](scripting/guard.md) documents the author-facing interface.

`guard.create(raw_joint, x, y, z, radius, Helper|None)` calls the actual native
descriptor constructor and binds the existing ShieldHit instance slot.
`guard.update_position()` preserves the native skip-update reset. Five Bool
flags, two Float hitlag overrides, integer recorded damage and Float recorded
facing are exposed independently, with their native types and semantics.

## Ownership and native delivery

The joint index addresses the imported costume joint table, rather than a
semantic FtPart. The backend validates the selected owned package, rig table,
joint class and extent, together with the source's native identity and prepared
program. Full-width integral admission prevents narrowing a large invalid index
into a valid joint. Invalid indices and negative/nonfinite radii are healthy
no-ops preserving the existing volume and callback. Offsets and hitlag fields
retain native Float bits; they do not inherit the radius restriction.

The compiler accepts only a direct owned helper name or None. The backend then
checks root ownership, zero arguments and Void result before mutation. These
are distinct gates: the fixture does not claim compile-time signature rejection.
Native reconstruction clears any previous interpreted ShieldHit slot alongside
its native pointer assignment; source construction installs the new helper
afterward. None explicitly clears. No legacy guard-hit event alias remains.

The actual Fighter ProcessHit ShieldHit branch runs the owned helper, then
checks retained Fighter lifetime before cached native reads and continuation.
Ordinary shield reactions and native hitlag ordering remain intact. Physical
retirement, faults and substituted userdata/program/schema cannot continue
through a stale cached Fighter after callback delivery.

Canonical Peach, Link/Young Link and Marth/Roy sources, and their tracked
reference fixtures, use explicit helper arguments. The test-only C exporter
uses that same contract; generated C remains an oracle, not the shipping runtime.

## Grouped evidence

Client/content-tool/reference targets build. All 40 selected integration CTests
and nine reference/compiler CTests pass. The connected guard fixture uses the
actual frontend, catalog, compiler, Prepare/Publish, Fighter actor, Execution,
VM, storage, receiver and physical pools.

`A:/FreakMelee-test-archives/script-guard-final3/receipt.json` passes 432 checks
in 83 cases, with 44 native comparison pairs and 57 actual VM calls. All five
hidden runner stages exit successfully. Root independently verified all 1,651
recorded input hashes. Independent native bodies are pinned to pre-batch commit
`786a1430fd0461185c701d38c9165ccbe91874c7`.

The cohort compares complete native constructor/update bodies and the exact
ShieldHit delivery branch. It covers raw joints outside the semantic part range,
invalid-volume callback preservation, replacement/None, all properties and Float
bits, helper ABI failures, and callback faults/retirement/reuse/identity changes.
Some substitutions allow a helper's local VM writes to finish; the retained
native continuation rejects the changed identity afterward. This is not a claim
that every mutation retroactively suppresses the already-completed VM tail.

Imported rig/package/class data, position writes and ordinary shield contact/audio
leaves are controlled. Unrelated ProcessHit hitlag/tails are excluded from the
fixture. Collision detection, motion/animation transitions, rendering and complete
in-game counter behavior remain separate acceptance. No game was launched.

The additional test-only C-reference cohort at
`A:/FreakMelee-test-archives/guard-reference-cohort-oct09-final/receipt.json`
passes 4,594 Peach counter comparisons and 38,050 checks across two fixtures.
Both paths construct the guard before delivering contact through its installed
callback. A focused Link/Young Link guard-only native/DLL cohort passes 48 cases
and 538 checks, including installed callback ownership. Root verified all 2,024
recorded inputs. These actors have no interpreted slots; slot clearing is an
explicit unavailable boundary, while the connected fixture above tests the VM.

Historical full-Link composition/activation regeneration remains blocked by
unrelated entry metadata and an obsolete ownership predicate with different
lifetime semantics. It was not silently replaced. The bounded guard results do
not establish full-Link export or gameplay; Peach's pluck suite was not rerun.
Current article constructor/reference units and archived Peach reference bodies
are separately pinned in the receipt.

## Preparation and remaining scope

`A:/FreakMelee-test-archives/source-guard-census-oct09-final2/receipt.json` pins the
rebuilt tool and ten exact templates. Peach advances from guard creation at
line726 to `effects.spawn_async_attached` at line1000. Mario, Dr. Mario,
Game & Watch and Zelda still prepare; Link/Young Link, Sheik and Popo/Nana retain
their secondary-resource metadata boundaries.

The [remaining Peach audit](native_source_peach_remaining_api_audit.md) groups
the later effect, input, collision, animation, contact and observation work.
The [async-effect proposal](native_source_async_effect_api_plan.md) records the
existing native queue contract and the bounded next operation.
The [float/air plan](native_source_float_air_api_plan.md) groups seven reusable
operations and the callback-capable native continuations they need.
Preparation and bounded fixtures do not establish complete fighter gameplay or
whole-match rollback acceptance. The full project goal remains active; commits
stay local.
