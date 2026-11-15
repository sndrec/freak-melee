# Interpreted common item policies, taunts and target colors

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The interpreter now exposes common pickup notification, held-item neutral
notification, the item reopen flag, the native common throw table and ordinary
engine item creation. These are reusable engine operations, not fighter-specific
behavior handlers. Source Article factories also take an explicit initial
collision flag; callers were migrated directly to the twelve-argument contract.

Taunt entry chooses between two owned motion rows using the original common
direction/animation rule. Controller-color queries accept either a full-width
creation ID or a retained Fighter Object. The Object route validates the captured
identity rather than reacquiring an Item's current owner.

Synchronous item callbacks and motion installation retain their participants.
In particular, held neutral notification validates the caller before reading its
held slot after the hook. A test protects that retired payload from reads, so
withholding the output alone cannot disguise an unsafe cached continuation.

Authoring contracts are documented in [item controls](scripting/held_item_controls.md),
[controller colors](scripting/colors.md), [taunt entry](scripting/language.md)
and [Article creation](scripting/article.md).

## Evidence and limits

All runs are hidden console fixtures. No installed collection, user draft,
original ISO or game session was changed.

| Connected fixture | Result | Receipt |
| --- | --- | --- |
| Common Fighter Item policies | 758 checks, 79 cases, 40 native comparisons, 80 source VM calls | `A:/FreakMelee-test-archives/script-fighter-item-policies-neutral-order-final/receipt.json` |
| Taunt entry and target colors | 6464 checks, 330 native comparisons | `A:/FreakMelee-test-archives/script-taunt-target-colors-settled/receipt.json` |
| Explicit initial Article collision | 180 checks, 25 cases | `A:/FreakMelee-test-archives/source-initial-collision-integrated-oct08/receipt.json` |

The fixtures run the production compiler, catalog, VM and actor paths, with the
actual common native policy/query bodies. Owned asset metadata, engine item
construction assets and motion/animation installation leaves are controlled.
All 26 common throw rows and all 256 controller-color selectors are covered.
These results do not establish retail DAT construction, GPU output, complete
fighter activation or whole-match parity.

The native client, content tool and separate test-only C reference exporter
build. All 39 selected interpreter/content/pool/lifecycle/allocator CTests pass
after the final continuation correction. The ordinary runtime remains the
interpreter; the reference exporter is not a package runtime dependency.

The [full copied-source census](native_source_roster_admission_census.md) reaches
later preparation failures: Dr. Mario's common taunt input chain, Peach's stale
Bool assignment from a Void operation and Game & Watch's old motion-entry name.
Link/Young Link, Sheik and the Climbers still require actual model/resource
ownership. No compatibility aliases, donor behavior or metadata-only acceptance
were added. The complete roster and project goal remain unfinished.

## Follow-up: common taunt inputs and resident clip queries

`controls.taunt_actions()` now invokes the actual shared taunt interrupt chain.
It honors `allow_interrupt` and the original input priority, with retained source
checks after rejected input decisions. A callback returning false can still
retire or fault its caller; the next input check must not run in that case.

`animation.clip_length(index)` reads a resident clip from the exact owned
animation table. Dynamic integral indices are supported; negative, oversized or
out-of-range indices fail before reading data. A valid row without a clip returns
zero. Native F32 frame bits are preserved. This query performs no allocation or
stock-kind lookup.

The combined fixture at
`A:/FreakMelee-test-archives/script-taunt-interrupts-final/receipt.json` passes
2287 checks across 106 cases, including 44 native comparisons and 72 nested
authored callbacks. It executes the actual shared taunt routine and the exact
clip-query entry through compiler/catalog/VM/actor paths. Input decisions,
physical construction and owned asset tables are controlled leaves; native
getter bodies provide the clip-frame reference. Neither actual controller
acceptance nor retail FigaTree import is established by this fixture.

The older `state.change` spelling was replaced directly with
`state.enter_at_speed` in tracked examples, composition fixtures and API docs.
The test-only C exporter uses the same name. Unused result assignments from Void
slot-clearing operations were removed. Three stale reference-exporter test
expectations now correctly reject a missing independent load handler or a
registered reflection callback without its required authored handler; production
validation was not weakened.

Client/tool/reference builds and the 39 selected integration CTests pass. The
three separately rebuilt reference-exporter tests also pass. The latest
[full-source census](native_source_roster_admission_census.md) identifies real
remaining common-engine operations and Peach's raw engine-item control gap.
Source preparation and controlled native comparisons still do not establish
complete fighter activation or gameplay.

## Follow-up: common motion, ordinary created items and food

The interpreter now invokes `physics.ground_to_air` through the actual
`ftCommon_8007D5D4` conversion, preserving its one-used-jump and ECB-lock policy.
`collision.ground_with_nudge` executes `ft_80084280`, including the opposite-facing
nudge branch, edge-balance response and Fall fallback. Lifetime checks prevent
a false native response from continuing with a retired or faulted fighter.

`A:/FreakMelee-test-archives/script-common-motion-collision-final/receipt.json`
passes 511 checks across 43 cases, with 28 native comparisons and 12 nested
callbacks. Source compilation, VM, actor storage, top-level routing and native
policy bodies are real; geometry, edge-balance and Fall response leaves are
controlled. The refreshed older movement fixture also passes 1916 checks across
229 cases and 122 native comparisons at
`A:/FreakMelee-test-archives/script-common-movement-calls-verified/receipt.json`.
Its extractor now includes the actual new conversion and current catalog helper
headers. These checks do not establish real stage or controller acceptance.

Creator-restricted item attachment, scale inheritance and held/auxiliary binding
now admit ordinary engine-created items. Their actual retained creator pointer,
generation and kind must match the caller; the existing exact source-bank path
remains separate. No Article specification is invented for an engine item.
Article, descriptor, attributes, logic table and model peers remain stable across
pickup callbacks before a result can be published.

`items.spawn_food(x,y,z)` runs the actual Foods helper, preserving its item-policy
gate, supplied previous position, fighter ECB-center current position, unowned
parents and successful-food counter. It is distinct from ordinary engine creation
and from an Article's food-dispersion operation. Completed creation/counter effects
are preserved if the caller subsequently retires; result publication stops.

`A:/FreakMelee-test-archives/script-ordinary-item-controls-food-review2/receipt.json`
passes 1023 checks across 114 cases, with 42 native comparisons and 109 source VM
calls. Ordinary item construction/assets, stage-policy/frequency getters and
render/transform leaves are controlled; common pickup/food bodies, VM actors and
physical source lifetimes are real. Food variant RNG and gameplay remain unverified.

All tracked `physics.air_to_ground` calls now use `physics.switch_to_ground`
directly. Its existing native conversion is identical; the obsolete spelling was
removed from the test-only exporter without a compatibility alias. Client, tool
and reference-exporter builds pass, as do 39 selected integration tests and three
separately rebuilt exporter tests. No game was launched for this checkpoint.

The rebuilt source census advances to appearance baking, a shared jump threshold
and explicit Article signal declarations. The [actual Arrow descriptor census](native_source_arrow_descriptor_census.md)
also establishes that all four trail roots require custom combiners. Metadata
remains rejected until real construction and ownership are implemented. The
project goal and complete roster activation remain unfinished.

## Follow-up: jump observations, appearance and declared signals

Four Fighter-only F32 observations expose the actual native shared data:
`controls.aerial_jump_reverse_threshold`, `controls.tap_jump_threshold`,
`controls.aerial_action_down_threshold` and `air.jump_horizontal_multiplier`.
They are read-only and preserve the stored bits. Missing common data rejects
the three shared thresholds; the per-fighter multiplier does not require it.
`A:/FreakMelee-test-archives/script-common-jump-observations-final/receipt.json`
passes the grouped suite's 33463 checks, 9435 property observations and 40
semantic expressions, including the exact native signed jump-direction test.
Physical objects/asset/actor lookup boundaries remain controlled.

Article-only `item.set_appearance(descriptor,frame)` now executes the ordered
native bake operations over its current program's owned descriptor table and
primary model. It retains the selected descriptor, preserves native motion and
recorded animation-frame fields, and clears the DATA command pointer after a
successful bake. Invalid indices are healthy no-ops. Source/model/storage and
state revision checks between animation operations prevent a nested callback
from having its replacement state cleaned up by the outer operation.

`A:/FreakMelee-test-archives/script-item-appearance-final/receipt.json` passes
1693 checks, 52 native comparisons including 20 bake pairs, and 57 appearance
negative cases. Actual frontend/VM/catalog, the native bake reference, transform
reset and GObj pools execute; actor spec/view/revision, descriptor assets and
animation evaluation are controlled. Retail animation rendering is unverified.

Game & Watch's 26 signal calls now specify their declared Article directly.
Its historical air-collision calls use the existing `collision.landed` property,
and its facing-ledged probe passes `facing.sign` to `collision.check_ground_ledge`.
Both native contracts were compared before migration; no new alias was added.

The native client/tool/reference exporter build and 39 selected integration plus
three reference tests pass again. The fresh preparation census is
`A:/FreakMelee-test-archives/source-appearance-jump-census-oct08/receipt.json`.
It exposes later accessory tracking, writable interrupt policy and ledge-entry
gaps, while real secondary-resource admission remains incomplete. No game or
whole-fighter acceptance is claimed.

The generic custom-combiner construction budget was committed locally as
`d710d9739`. Its connected native builder/compiler/free suite passes 11265
checks over 341 material cases, including all four actual imported arrow roots,
at `A:/FreakMelee-test-archives/hsd-custom-tev-final3/receipt.json`. The graph
environment remains uncovered; allocation-path coverage is not universal packed
register legality or permission to admit model metadata.

## Follow-up: IASA, declared Item callbacks and ledge entry

`controls.interrupt_allowed` is now a writable Fighter Bool backed directly by
`allow_interrupt`. Source bytecode can clear IASA on entry and read it later.
The grouped actual frontend/property/VM suite passes 34125 checks, including
9564 property observations and 40 semantic expressions, at
`A:/FreakMelee-test-archives/script-iasa-policy-oct08-final/receipt.json`.
The inherited asset/actor/native leaves remain controlled; this is not a
whole-fighter acceptance result.

Article-only `item.track_accessory()` selects the current program's declared
zero-argument Void accessory hook into its saved actor slot; a missing hook
rejects without silently clearing it. `item.clear_accessory()` explicitly clears
the slot and native pointer. No authored machine-code pointer or tick allocation
is introduced. The real source frontend, immutable Item preparation, VM, actors,
GObj pool, native accessory scheduler and snapshot roundtrip pass 137 checks in
33 cases with 12 source signals at
`A:/FreakMelee-test-archives/script-item-accessory-final/receipt.json`.
Descriptor/program publication and effect/graphics/physics/audio leaves are
controlled; actual DAT import and complete constructor/gameplay remain open.

Fighter-only `state.catch_ledge()` enters the actual common catch helper.
The helper and animation continuation retain their caller across nested work,
including a logically faulted source whose native generation did not change.
The [ledge checkpoint](native_source_ledge_entry.md) records 806 checks,
eight healthy native comparisons and 42 lifetime cases. Full motion/animation
stage/map/audio/effect leaves are controlled; registered actors, VM and pools
are real. The pre-change native reference is pinned to `24b898f1d`.

All tracked FMS callers now use `physics.ground_to_air`, `callbacks.reflect`,
`controls.try_aerial_jump` and `item.reflect` instead of the corresponding obsolete
spellings. Native contracts were traced before direct migration, and the
separate reference exporter follows the same spellings without aliases.
Native client/tool/reference builds, 39 selected integration tests and three
rebuilt reference tests pass. No game was launched.

The current census and whole-script inventories are linked from the
[roster ledger](native_source_roster_admission_census.md). Dr. Mario's next gaps
are a grouped Article lifecycle/model cluster; Peach additionally needs float,
guard, dynamics/contact and held-transition closures. Complete roster and
project acceptance remain unfinished.

The existing primitive-only Item runner was aligned with its configured rejecting
engine boundaries. Its unchanged native self-operation regression passes 7904
checks (1664 physics pairs, 96 sound pairs, 128 VM ticks) at
`A:/FreakMelee-test-archives/script-item-primitives-accessory-final2/receipt.json`.
This verifies the Item binding integration without fabricating authored actors;
the separate accessory fixture provides actual actor/scheduler coverage.

## Follow-up: Item model and lifecycle batch

The [model/lifecycle checkpoint](native_source_item_model_lifecycle_checkpoint.md)
records direct spin reset, attribute/instance/current-owner scaling, prepared
translation/local visibility and guarded native physics/removal. Generic live
Fighter root scale is readable through retained receivers. Canonical Dr. Mario
uses the appropriate receiver roles and authored motion predicate; its complete
source now passes preparation. No generated-C runtime or aliases were added.

Native client/tool/reference builds, 40 selected integration tests and three
reference tests pass. Full fighter gameplay and secondary resource acceptance
remain open; no game was launched. See the checkpoint for exact focused
receipts and controlled boundaries.
