# Owned stock-template admission census

This is a preparation census, not a gameplay or complete-roster acceptance test.
The real `native_content_tool` cloned ten independent stock drafts from the retail
ISO into `A:/FreakMelee-test-archives/fighter-callback-source-census/`, then checked
each complete authored script against its corresponding copied assets. No
installed package, user draft or original ISO was modified.

| Template | Current first result |
| --- | --- |
| Mario | Pass: 77 functions, 35 imports, 1744 instructions. |
| Dr. Mario | Unsupported binding at line 152: `callbacks.reset_root_x_on_damage()`. |
| Link | Arrow metadata at line 1332: `render standard`. |
| Young Link | Arrow metadata at line 1345: `render standard`. |
| Peach | Legacy `base ftDataPeach 341 30` declaration at line 8. |
| Game & Watch | Legacy `base ftDataGamewatch 341 40` declaration at line 6. |
| Zelda | Pass: 152 functions, 56 imports, 3154 instructions. |
| Sheik | Chain resource metadata at line 1543. |
| Popo | Legacy `base ftDataPopo 341 26` declaration at line 7. |
| Nana | Legacy `base ftDataNana 341 26` declaration at line 7. |

These are the first diagnostic per source, not an exhaustive missing-operation
list. The rejected packages remain rejected: no declaration was erased, metadata
silently admitted or stock behavior substituted to obtain a green result.

The preceding C-oracle translation work establishes useful source comparisons;
it does not establish complete direct-interpreter admission. Normal entry hooks
and their engine primitives are separate requirements. For example, the new
Game & Watch entry scheduler does not implement its `attack.enter_jab`,
`attack.enter_rapid_jab` or `attack.update_rapid_jab` imports by itself.

Next work is to connect the remaining generic bindings and owned asset/resource
declarations, then exercise the accepted programs as actual actors and matches.
Legacy base declarations must be replaced by the normal owned-layout contract;
they must not become donor-definition aliases in the interpreter.

## After owned policy and construction migration

The real copied-draft checks now progress to these first failures:

| Template | Current first result |
| --- | --- |
| Dr. Mario | `model.base_scale` in results scaling at line 352; held controls and owned signals are passed during preparation. |
| Peach | `items.held_id` at line 141. |
| Game & Watch | `costume.index` at line 147. |
| Popo | Article resource capacity at line 1621. |
| Nana | Article resource capacity at line 1847. |

Peach/GW common-motion policies now prepare real shared phases on their owned
rows. Climber entry groups were split into IceShot and Blizzard named moves,
with every original body byte preserved; the language's move scoping was not
weakened. These first failures still do not establish exhaustive body support or
runtime/game acceptance. The earlier table remains the initial census receipt.

The receipt records executable/source hashes, individual diagnostics and private
draft paths. It reports a completed census, not a passing ten-fighter suite.

## After color and held-query integration

`source-color-held-census-final-oct08` reruns the complete scripts against those same
private copied drafts using the newly built content tool. Mario and Zelda still
pass with the counts above. The other first results are:

| Template | Current first result |
| --- | --- |
| Dr. Mario | `state.taunt(341, 342)` at line 401 is not yet bound. |
| Peach | `items.notify_pickup(true)` at line 160 is not yet bound. |
| Game & Watch | Twelve-argument `articles.create_with_velocity` at line 172; the current direct factory has eleven arguments and does not implement the final source flag. |
| Link / Young Link | Secondary-model root metadata at lines 1343 / 1356. |
| Sheik | Chain resource metadata at line 1543. |
| Popo / Nana | Resource capacity metadata at lines 1621 / 1847. |

The script text and executable hashes, exact commands and diagnostics are in
that receipt. These results establish preparation progress only. Actual
secondary-model allocation, animation and rollback ownership remain required;
recognizing a metadata name is insufficient to accept a package.

## After common item policies, taunt entry and explicit factory collision

The full copied-source census at
`A:/FreakMelee-test-archives/source-common-policies-census-oct08/receipt.json`
uses the built native tool, exact source hashes and the same private drafts.
Mario/Zelda preparation counts remain unchanged. Later first failures are:

| Template | Current first result |
| --- | --- |
| Dr. Mario | Unbound `controls.taunt_actions()` at line 423. |
| Peach | Bool assignment from the Void `items.clear_held()` at line 218. |
| Game & Watch | Legacy `state.change(...)` at line 177. |
| Link / Young Link | Secondary-model metadata at lines 1343 / 1356. |
| Sheik | Chain resource metadata at line 1543. |
| Popo / Nana | Resource metadata at lines 1621 / 1847. |

The source-call mismatches need direct migration to the current generic
contracts; they must not become aliases or donor-handler fallbacks. This remains
a first-diagnostic census, not exhaustive support or whole-roster gameplay proof.

## After explicit motion-call migration and common taunt/clip queries

The next full census is
`A:/FreakMelee-test-archives/source-motion-calls-census-oct08/receipt.json`.
All tracked example and composition-script motion calls now use the interpreter's
`state.enter_at_speed` contract directly. Unused Bool assignments from Void item
slot clearing were removed; the test-only C reference exporter uses the same
motion-call spelling. Unsupported old spelling remains only in an intentional
compiler rejection test and unrelated untracked reference material.

Mario/Zelda still pass preparation. Dr. Mario advances to
`collision.ground_with_nudge()` at line 427, Peach to
`physics.ground_to_air()` at line 327, and Game & Watch to
`items.spawn_food(x,y,z)` at line 259. These require their actual common native
contracts; standing collision, force-air-and-consume-jumps and ordinary engine
creation are not behavior-preserving replacements.

Peach also has an independently identified runtime gap: its rare engine-created
items cannot yet use creator-restricted attachment/scale/slot controls, which
currently require a source Article specification. Source preparation alone will
not establish that these rare-item paths function. Link/YoungLink secondary
models, Sheik's chain and the Climber resource collections remain unadmitted.

## After native food, ground conversion and nudge collision

`A:/FreakMelee-test-archives/source-common-motion-census-oct08/receipt.json`
records the rebuilt client tool and all ten exact source hashes. The new common
operations prepare successfully; the next first diagnostics are:

| Template | Current first result |
| --- | --- |
| Mario / Zelda | Preparation passes; gameplay acceptance is separate. |
| Dr. Mario | Unbound `item.set_appearance` at line 516. |
| Peach | Unbound `controls.aerial_jump_reverse_threshold` at line 330. |
| Game & Watch | Historical three-argument `articles.signal_owned` at line 268; migrate directly to the explicit declared-Article form. |
| Link / Young Link | Secondary-model metadata at lines 1343 / 1356. |
| Sheik | Chain resource metadata at line 1543. |
| Popo / Nana | Resource metadata at lines 1621 / 1847. |

The historical `physics.air_to_ground` spelling has been removed from tracked
scripts and the test-only reference exporter. Callers use the existing exact
`physics.switch_to_ground` operation; no alias was added.

The actual four Arrow trail imports were also inspected. All have explicit
custom texture combiners, so they remain rejected by the current construction
budget. See the [real descriptor census](native_source_arrow_descriptor_census.md)
for their fields, allocation limits and the next generic combiner support batch.

## After owned appearance and common jump observations

`A:/FreakMelee-test-archives/source-appearance-jump-census-oct08/receipt.json`
records the rebuilt tool, commands and exact ten-template hashes. Four generic
read-only values now expose the shared jump thresholds and fighter horizontal
jump multiplier. Game & Watch's 26 historical signal calls now name the exact
declared Article; its old airborne probe and facing-ledged probe use existing
`collision.landed` and `collision.check_ground_ledge(facing.sign)` directly.
The native airborne helper returns only Ground/Air, making the old non-Ground
test equivalent to the existing Air test. No collision alias was introduced.

| Template | Current first result |
| --- | --- |
| Mario / Zelda | Preparation passes; gameplay acceptance is separate. |
| Dr. Mario | Unbound `item.track_accessory()` at line 526. |
| Peach | `controls.interrupt_allowed` assignment rejected at line 347; audit its native write contract. |
| Game & Watch | Unbound `state.catch_ledge()` at line 403; requires the actual shared ledge entry. |
| Link / Young Link | Secondary-model metadata at lines 1343 / 1356. |
| Sheik | Chain resource metadata at line 1543. |
| Popo / Nana | Resource metadata at lines 1621 / 1847. |

This remains a first-diagnostic preparation census. Appearance execution and
native shader construction have their separate fixtures; no complete fighter
activation or match validation follows from this table.

## After accessory registration, IASA writes and ledge entry

`A:/FreakMelee-test-archives/source-accessory-ledge-census-oct08/receipt.json`
records the rebuilt tool and exact ten-template inputs. The interpreter now
accepts declared Item accessory tracking/clearing, the native Fighter IASA write,
and common ledge entry. Preparation advances to:

| Template | Current first result |
| --- | --- |
| Mario / Zelda | Preparation passes; gameplay acceptance remains separate. |
| Dr. Mario | Unbound `item.reset_spin()` at line 528. |
| Peach | Eight-argument Article creation at line 417 needs direct migration to the existing explicit velocity/creation-value form. |
| Game & Watch | Unbound `visuals.set_current_group()` at line 433. |
| Link / Young Link | Secondary-model metadata at lines 1343 / 1356. |
| Sheik | Chain resource metadata at line 1543. |
| Popo / Nana | Resource metadata at lines 1621 / 1847. |

The [grouped Peach and Dr. Mario inventory](peach_common_binding_inventory.md)
checks the whole scripts beyond this first diagnostic, including runtime-only
restrictions and distinct owner relationships. The [dynamics/contact proposal](native_source_dynamics_rebound_hook_proposal.md)
identifies prepared source hooks and guarded continuation work still required.
No complete fighter activation or gameplay result is inferred from preparation.

## After Item lifecycle, scaling and primary model controls

`A:/FreakMelee-test-archives/source-model-lifecycle-census-oct09/receipt.json`
records the rebuilt tool and exact ten-template sources. Dr. Mario now passes
complete source preparation: 111 functions, 35 imports, 2894 instructions.
Mario and Zelda still prepare. This is compiler/package preparation, not a
whole-fighter gameplay or rendering result.

Peach advances to `item.hide_model()` at line430. Game & Watch still requires
`visuals.set_current_group()` at line433. Link/Young Link, Sheik, and Popo/Nana
remain at the same real secondary-resource metadata boundaries. The
[Item model/lifecycle checkpoint](native_source_item_model_lifecycle_checkpoint.md)
records this batch's distinct native operations and runtime evidence.

## After current visibility, Item expiry and positional feedback

`A:/FreakMelee-test-archives/source-visibility-expiry-census-oct09/receipt.json`
records the rebuilt tool and ten unchanged source templates. Mario, Dr. Mario,
and Zelda still pass complete preparation. Peach advances through whole-model
visibility, positional effects and expiry to the holder's `held_motion` query at
line488. Game & Watch advances through current visibility selection to
`absorber.create` at line452. Link/Young Link, Sheik and Popo/Nana remain at their
existing secondary-resource metadata boundaries. These are next diagnostics,
not statements that later parts of either script are already runtime-ready.

The [visibility/expiry checkpoint](native_source_visibility_expiry_checkpoint.md)
records the native helper comparisons, integration gates and remaining limits.

## After held observations and absorption

`A:/FreakMelee-test-archives/source-held-absorber-census-oct09-final2/receipt.json`
records the final rebuilt tool and exact ten-template inputs. Mario, Dr. Mario
and Zelda still prepare. Peach advances through canonical holder Item queries to
recipient export dispatch at line490. Game & Watch advances through absorption,
common deadzone reads and typed creator-owned palette helpers to Fighter hitbox
state/damage at line548. Secondary-resource boundaries for Link/Young Link,
Sheik and Popo/Nana remain unchanged.

The [held/absorber checkpoint](native_source_held_absorber_checkpoint.md) records
native comparisons and actual VM/lifetime checks. The next related capabilities
are described in the [recipient export plan](native_source_recipient_export_plan.md)
and [secondary resource plan](native_source_secondary_resource_plan.md). These
remain preparation diagnostics, not complete fighter gameplay acceptance.

## After Fighter ground/combat and Item observation controls

`A:/FreakMelee-test-archives/source-ground-item-census-oct09-final/receipt.json`
pins the current tool and ten-template source preparation results. Mario,
Dr. Mario and Zelda still prepare; Peach still needs recipient exported calls.
Game & Watch advances through Fighter hitbox state/damage, ordered ground-input
checks, landing lag and canonical callbacks/cues to its Article parameter access
at line1534. That Article's local `attrs` scope does not contain the root Fighter
parameter. Resolve the actual immutable author/recipient ownership across all
affected Articles before changing those references. Joint-position output and
smash-charge observations also remain open beyond this first diagnostic.

Link/Young Link, Sheik and Popo/Nana secondary-resource boundaries are unchanged.
The [ground/combat/Item checkpoint](native_source_ground_item_combat_checkpoint.md)
records native comparisons, continuation checks and fixture limits. Common
smash entry is not specialized Ness yo-yo entry; its source hook remains rejected.
No whole-fighter or gameplay acceptance is inferred from this preparation pass.

## After explicit package-owned root parameters

`A:/FreakMelee-test-archives/source-program-parameters-census-oct09-final/receipt.json`
records a rebuilt-tool pass over the same ten full templates. Game & Watch's
32 shared root parameter reads now use `program.attrs`, preserving immutable
module ownership rather than requiring a live creator. Preparation advances to
line1596, where its Article writes `item.airborne`, currently read-only in the
direct binding catalog. Mario/Dr. Mario/Zelda still prepare; Peach exports and
the other templates' secondary-resource boundaries remain unchanged.

The [program-parameter checkpoint](native_source_program_parameters_checkpoint.md)
records ownership, compiler/runtime checks and reference-tool scope. This
preparation progress does not constitute full fighter or gameplay acceptance.

## After the remaining Article API batch

`A:/FreakMelee-test-archives/source-article-api-census-oct09-final/receipt.json`
records Game & Watch preparing successfully: 364 functions, 80 imports and
6,492 interpreter instructions. Ordinary Article airborne writes, primary joint
world-position outputs, Article wind and foreign Fighter smash-charge queries
now have actual direct-interpreter backends. Mario/Dr. Mario/Zelda continue to
prepare; Peach recipient exports and the other templates' secondary-resource
boundaries are unchanged.

The [Article query checkpoint](native_source_article_queries_checkpoint.md)
records native comparisons, lifetime checks and controlled fixture boundaries.
Successful preparation does not establish complete fighter gameplay parity.

## After recipient-owned exports

`A:/FreakMelee-test-archives/source-recipient-exports-census-oct09-integrated/receipt.json`
records Peach passing its cross-fighter exported calls and advancing to the
unbound Item victim response at line651. Mario, Dr. Mario, Game & Watch and
Zelda still prepare; Link/Young Link, Sheik and Popo/Nana retain their real
secondary-resource metadata boundary.

The [recipient-export checkpoint](native_source_recipient_exports_checkpoint.md)
records connected cross-package VM execution, exact hash-collision matching and
retirement/fault semantics. The [remaining Peach audit](native_source_peach_remaining_api_audit.md)
groups later missing bodies, preparation hooks and runtime gaps; reaching line651
does not imply the rest of the template is admitted or gameplay-equivalent.

## After Item responses and attachment identity

`A:/FreakMelee-test-archives/source-item-responses-census-oct09-final2/receipt.json`
pins the rebuilt content tool and ten current full templates. Peach now passes
victim bounce, masked stage response and physical attachment-owner comparison,
reaching guard construction at line726. Mario, Dr. Mario, Game & Watch and Zelda
continue to prepare. Link/Young Link, Sheik and Popo/Nana retain their actual
secondary-resource metadata boundaries.

The [Item-response checkpoint](native_source_item_responses_checkpoint.md)
records the native composition/VM evidence and controlled fixture boundaries.
Canonical scripts and the test-only reference emitter use the truthful
`item.attachment_owner_is` name; the historical creator token compared attachment
identity. The [guard plan](native_source_guard_api_plan.md) is the next coherent
operation group, with an owned helper in the existing ShieldHit callback slot.
Preparation and fixture acceptance remain separate from complete gameplay.

## After guard volumes and owned reactions

`A:/FreakMelee-test-archives/source-guard-census-oct09-final2/receipt.json` pins the
rebuilt tool and ten current templates. Peach passes guard construction, scalar
fields and explicit owned-helper preparation, reaching the async attached effect
at line1000. Mario, Dr. Mario, Game & Watch and Zelda still prepare. Link/Young
Link, Sheik and Popo/Nana retain their secondary-resource metadata boundaries.

The [guard checkpoint](native_source_guard_checkpoint.md) records native
constructor/delivery comparisons, actual VM and lifetime evidence, and controlled
fixture limits. The shipping interpreter owns these reactions; the updated C
exporter is only a comparison oracle. Complete fighter gameplay remains separate.
