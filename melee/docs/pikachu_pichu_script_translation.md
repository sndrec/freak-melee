# Pikachu and Pichu script translation

Current interpreter migration: Pikachu and Pichu have separate complete source
files, with each source declaring only its own three articles. Article factories
and signals name those declarations, and article code uses explicit owner
parameter receivers. Both prepare all 210 functions without diagnostics and
publish independently owned interpreted packages with zero native imports.
See the [connected interpreter checkpoint](native_script_pikachu_gameplay_checkpoint.md)
for the startup visibility correction, replay coverage and remaining limits.
Its corrected focused matrix passes **46/46 exact pairs** against
native stock, including 39,400 controller/RNG samples and 78,800 fighter states;
the documented wider lifecycle/visual/network cases remain outside this evidence.
The generated-C acceptance below is historical and is not proof of
interpreter acceptance.

[Pikachu source](../examples/pikachu_wip.fms) |
[Pichu source](../examples/pichu_wip.fms)

## Historical generated-C independent behavior acceptance

Actual shipped-source fixtures pass 39,006 original-C comparisons, eight edited
parameter checks and 104 owned phase callbacks per fighter (96 originally
nonnull). Exact rig maps were checked against all eight retail costumes, with
53 Pikachu/46 Pichu joints and 29 semantic bindings each, no omissions.

A focused candidate matrix passes 48 hidden, muted, accelerated drawn pairs:
40,800 controller samples and 81,600 fighter states. It covers all four specials,
Jolt reflection/absorption, Thunder contact/shield/absorption/interruption,
repeated jab, forward-smash contact, held bat, active CPU levels4/9, near/far
recovery and the red costume. The shipped template then passes twelve selected
pairs (12,000 samples, 24,000 states). Its generated C differs from the accepted
candidate only in the exported source hash; owned FMP/CPU profile bytes match.

Evidence: `.local/pikachu-pichu-independent-acceptance19.json`,
`.local/pikachu-pichu-closure19-ready/acceptance.json`,
`.local/pikachu-pichu-independent-final19/results.json`, and
`.local/pikachu-pichu-independent-source19/{pikachu,pichu}/results.log`.
The source comparison defaults to actual shipped independence; adding it only
in a scratch fixture requires explicit `--candidate`.

Live Thunder-chain stock loss/owner removal, auxiliary attachments and runtime
instance scaling remain unproven in gameplay. The source suite covers scale
math and lifecycle decisions, not those missing trajectories. Wider wall/ledge/
startup interruptions, every CPU branch, pixels/audio, results construction,
Workshop and network rollback are not newly accepted by this batch. Earlier
partial checkpoints below are historical evidence, not current template status.

## Assets and authoring

The independent `pikachu_clone` and `pichu_clone` bases import their own
fighter, animation and costume archives. Both have 320 actions and 14 demo
entries. Common parts data and costume traversal independently agree on
53 Pikachu joints and 46 Pichu joints. They share 26 special motion states,
but motion IDs are not animation indices: Skull Bash Launch and Flight
have a different order in those two tables.

New drafts are available in the existing Workshop fighter list. Scripting
opens that fighter's uncompiled [Pikachu](../examples/pikachu_wip.fms) or
[Pichu](../examples/pichu_wip.fms) template.
Each project imports its own named tuning into `fighter_parameters.fmp`;
The templates do not share runtime state or mutable parameters.
The compiled moves and articles call no stock Pikachu special/item
handlers and read no fighter/motion union or live DAT tuning. Parameter import
retains the selected fighter's two Jolt and one Thunder declarations.
Each has separate owned article assets, parameters and state.

## Quick Attack and Agility

The translation follows `ftPikachu/ftpikachuspecialhi.c`. Both fighters share
the authored startup, travel and recovery decisions, with independently owned
tuning, animations and subactions:

| Phase | Ground | Air |
| --- | --- | --- |
| Start | 353 | 356 |
| Dash | 354 | 357 |
| Recovery | 355 | 358 |

Ordinary named variables replace the move union's gravity delay, dash duration,
first input direction, second-dash flag and collision age. The script preserves
the separate grounded/aerial stick threshold comparisons, normalized speed,
second-dash angular limit and speed multiplier. The second dash retains both
motion changes through animation frames 12 and 13; dropping the first transition
would change subaction processing. Recovery keeps the source's ground drift,
vertical velocity decay, common air deceleration, freefall and landing lag.

The scripts own floor/platform/wall/ceiling choices, explicit collision-box
dimensions and ground/air transitions. The early platform check marks a soft
floor as skipped, so its position in the condition matters. Independent ceiling
and wall checks remain independent even when the first changes motion. Bone
rotation/scale and clearing kinetic/animation samples preserve the original
travel pose. Pikachu's jittered trail retains the two RNG calls and Pichu omits
them, selected by package-owned configuration rather than a live fighter-kind
branch. Pichu's self-damage remains in her copied subactions.

Reusable operations are documented in [movement](scripting/movement.md),
[bone scaling](scripting/combat_visuals.md#bonesset_scalebone-x-y-z) and
[forced-turn callback binding](scripting/functions.md#callbacksfacing_changedfunction).
The turn helper aligns both velocity and the ordinary saved first-input X
value. No Pikachu special handler or private move-union access is emitted.

Twenty-eight complete 700-sample stock/script pairs match: ground up/horizontal
travel in both facings; aerial diagonals in both facings; downward and horizontal
air travel; a second dash; Yoshi's Story slopes; Battlefield downward travel;
neutral-stick defaults and a small left stick below the travel threshold;
and Mario Cape turning each fighter during recovery. Both Cape recordings
require an actual left-facing recovery frame before the subsequent rightward
dash, exercising velocity and saved-input realignment.
Both ports' traced pose, hitboxes, hurtboxes, body contacts, visibility, damage,
actions, item core/pose/visual/relations and RNG remain compared. The second-dash
recordings observe both ground travel and a subsequent air dash. The initial
Cape fixture pressed B during braking, and the next missed; neither
is counted as forced-turn acceptance. The final fixture observes the turn.
The slope recordings observe air travel from ground startup; the first harness
wrongly required ground travel there, and corrected offline gates retain full
comparison.

Evidence: `.local/pikachu-pichu-up-first/`,
`.local/pikachu-pichu-up-first/corrected-slope-coverage.json`,
`.local/pikachu-pichu-up-edges/`,
`.local/pikachu-pichu-up-cape-contact/` and
`.local/pikachu-pichu-up-acceptance.json`. Build and all 36 focused backend checks
pass, including invalid callback targets/arguments, unknown bone roles and
malformed collision arguments. The windows stay hidden, drawing stays enabled
and replay pacing is accelerated. This is selected trace acceptance, not exact
pixel/audio or whole-fighter acceptance.

Four regressions using the final combined template also match: Pikachu Jolt
reflection, Pichu Jolt absorption, Pikachu automatic full-charge Skull Bash and
Pichu left-facing aerial Skull Bash. Their original interaction/motion gates
remain required. Evidence: `.local/pikachu-pichu-up-regressions/`.

Rook's unchanged scripts were rebuilt for the SDK, with 285 own-rig clips and
zero borrowed clips; installation replaced her prior package, leaving one Rook.
No additional Rook gameplay acceptance is claimed.

```powershell
python tests/native/pikachu_pichu_up_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/pikachu-up-new
```

## Thunder Jolt

The translation follows `ftPikachu/ftpikachuspecialn.c`,
`it/kinds/itpikachutjoltground.c` and `itpikachutjoltair.c`. Grounded and airborne
entry are motion states 341 and 342. The first article is the moving controller;
the second is its linked surface-path model. Stock names call these Ground and
Air, but the fighter creates the first kind for both grounded and aerial shots.

The scripts own initial segment sweep, velocity, lifetime, floor-normal turn
limits, root orientation, link creation/synchronization, animation restarts,
hitbox history, expiry, contact sound, effects, reflection, shield and
absorption callbacks. They retain the original float intermediate calculations
and state/animation ordering. Pichu's self-damage remains in her owned
subaction data. Original engine move/item callbacks were not changed.

Generic [article links](scripting/article_links.md) supply fixed slots,
creation-ID resolution, reciprocal checks, named signals, collision snapshots
and indexed joint reads. Explicit creation binds authored assets and behavior
before model construction. Author identity is independent of damage owner,
so reflection cannot cause a subsequent linked part to use the reflector's
fighter package. Scripts choose state, lifetime and physics initialization;
there is no Thunder-Jolt-specific engine API.

Sixteen complete 700-sample stock/script pairs match: for each fighter,
ground/air entry in both facings, Yoshi's Story slopes, repeated shots, Fox
reflection and Ness absorption. Both item kinds must appear. Reflection
requires observed P1-to-P2 item ownership transfer; absorption requires the
actual `absorb` event. Both fighters' pose/combat state, actions, visibility,
damage, item core/pose/visual/relations and RNG remain compared. Repeated
shots hit Zelda three times in both paths. This is selected runtime acceptance,
not a claim of exact pixels/audio or whole-fighter parity.

The first interaction harness used nonexistent event labels `reflected` and
`absorbed`. Those were coverage-check errors, not missing contacts. Retained
traces match under the actual ownership-transfer and `absorb` gates; fresh
interaction recordings replace those attempts in acceptance. Evidence:
`.local/pikachu-pichu-jolt-acceptance.json`,
`.local/pikachu-pichu-jolt-batch/` and
`.local/pikachu-pichu-jolt-interactions/`.

The build and 36 focused backend checks pass, including creation-ID reuse,
removed targets, owner transfer, author isolation, reciprocal unlinking,
whole collision-copy and parser bounds. Null/missing-author constructor and
parent-removal guards were added after the matrix; two final-build ground/
reflection regression pairs recheck ordinary creation. Rook's unchanged
scripts were rebuilt for the SDK, retaining 285 own-rig clips and zero borrowed
clips; her installed package was replaced. No new Rook gameplay acceptance.

```powershell
python tests/native/pikachu_pichu_jolt_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/pikachu-jolt-new
```

## Skull Bash

The translation follows `ftPikachu/ftpikachuspecials.c` and its charge/effect
helpers in `ftpikachuspecialn.c`. It includes both starts, charging, release,
launch, flight, grounded/airborne recovery, hit response and ground/air
transitions. The motion IDs are:

| Phase | Ground | Air |
| --- | --- | --- |
| Start | 343 | 348 |
| Hold | 344 | 349 |
| Launch | 347 | 352 |
| Flight | 345 (stock unused callbacks) | 350 |
| End | 346 | 351 |

The one-shot state-change callback observes the horizontal smash-input
timer and initializes an ordinary integer charge variable. Charge increases
before the maximum comparison. Hitbox damage uses the common integer damage
update, preserving the stock float-to-integer conversion, staleness and
fighter scaling. Launch velocity keeps the stock intermediate float rounding.
A hit stops horizontal travel and only clears nonnegative vertical speed.

Charge and launch effects use ordinary authored accessory functions bound
after transitions. Effect flags, hitlag pause/resume and one-shot clearing
retain the stock ordering. No new fighter-specific scripting operation was
needed: the template uses existing
[function callbacks](scripting/functions.md#binding-a-motion-callback),
[movement operations](scripting/movement.md),
[state-change and hit callbacks](scripting/fighter.md),
[hitbox damage updates](scripting/fighter.md#hitboxactive-and-hitboxset_damage)
and [owned parameters](scripting/language.md#fighter-attributes).

## Selected runtime acceptance

Fourteen 700-sample stock/script pairs match: for each fighter, grounded and
airborne taps in both facings, automatic full-charge release, Zelda contact,
and Yoshi's Story floor geometry. Comparison retains both fighters' traced
poses, hitboxes, hurtboxes, body contacts, visibility, actions, damage, item
state/events and simulation RNG. Entry, launch and flight coverage is
required; sustained recordings also require Hold. Contact recordings require
real victim damage. An additional offline check confirms the initial facing.
Pichu's recorded self-damage matches stock; it remains driven by her owned
subaction data rather than a fighter-specific scripting primitive.

The first runner incorrectly required Hold to appear in the end-of-tick
trace for taps. Both stock and script enter and exit Hold within the same
callback phase. Corrected offline comparison of retained recordings still
checks every traced state field. The first Pichu contact attempt missed the
target; it is rejected coverage. Six additional approach samples produced
the accepted replacement contact recording.

Evidence: `.local/pikachu-pichu-skull-first/corrected-coverage.json`,
`.local/pikachu-skull-contact-fixed/results.json` and
`.local/pikachu-pichu-skull-acceptance.json`. Runtime windows are hidden,
drawing remains enabled and replay pacing is accelerated. The native build
and all 35 focused fighter/script checks pass.

```powershell
python tests/native/pikachu_pichu_skull_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/pikachu-skull-new
```

## Thunder

The translation follows `ftPikachu/ftpikachuspeciallw.c` and
`it/kinds/itpikachuthunder.c`. The script owns all eight fighter phases:
ground Start/Loop/Hit/End (359â€“362) and air Start/Loop/Hit/End (363â€“366).
Its named variables hold the first segment's creation ID, completion status
and sampled tip position. A damage callback clears that status. Ground/air
transitions preserve the source's callback, animation, effect and velocity
ordering; aerial self-contact applies the owned hit velocity and gravity.

Each package owns its Thunder article assets, tuning and three authored states:
delayed hidden startup, falling travel and quantized shrinking. Segments share
one collision group and form a creation-ID chain. The leading segment reports
its tip to the summoner and sends the following segment a stop height; that
propagates through the chain. Ground impact preserves active hitboxes while
disabling queued contact hitlag, matching `it_80272AC4`/`it_80274C60`. Clearing
hitboxes there was a translation error revealed by the Battlefield recording.

Reusable additions are [explicit grouped creation](scripting/article.md#explicit-creation),
[linking existing articles](scripting/article_links.md#linking-existing-owned-articles),
creation payloads, nested article branches, [damage callbacks](scripting/functions.md),
full model show/hide and item contact/damage-hitlag controls in the
[Item API](scripting/item.md). The owner action-category query retains the
engine's coarse category value, rather than encoding a Pikachu-specific test
in the host. No stock Thunder fighter or item handler is emitted.

Stock native Thunder initially created no articles. Its original Quick Attack
union alias placed a status integer in the upper half of Thunder's 64-bit item
pointer. Thunder now accesses its own pointer/status member, clears the pointer
with `NULL`, and stores the legitimate 0/1/3 status in an integer instead of a
native C boolean. These correct native layout/type mismatches without changing
the original decision rules. Comparisons below use that corrected stock path.

Twenty-eight complete 700-sample stock/script pairs match. For each fighter:
ground/air in both facings, airborne self-contact in both facings, repeated
casts, Yoshi's Story slopes, Battlefield interception, offstage casts, actual
victim damage, shield contact, opponent damage interruption and Ness absorption.
Both fighters' complete traced state, pose, hitboxes, hurtboxes, body contacts,
visibility, damage/actions, item core/pose/visual/relations and RNG stay compared.
Contact gates require real damage, GuardSetOff or an `absorb` event; interruption
also requires a damage motion, so Pichu's self-damage cannot satisfy it.

Initial platform expectations were corrected from self-contact to End after
observing stock interception. Those retained recordings then exposed the
contact-hitlag translation error and are excluded; fresh platform recordings
pass with active hitboxes preserved. Early contact fixtures missed their
opponent, and grounded Magnet sat below the lightning before the summoner
caught its tip. They are excluded. The final Magnet fixture jumps into the
bolt's path and requires actual absorption in both recordings.

Evidence: `.local/pikachu-pichu-thunder-acceptance.json`,
`.local/pikachu-pichu-thunder-matrix/`,
`.local/pikachu-pichu-thunder-final-contact/` and
`.local/pikachu-pichu-thunder-absorb-air/`. Build and all 36 focused backend
checks pass, including link lifetime/author isolation and malformed nested
branches. Runtime windows stay hidden, drawing stays enabled and playback
is accelerated. Six final-template regressions also match: Pikachu's second
dash, Pichu's left-facing aerial diagonal, Pikachu Jolt reflection, Pichu Jolt
absorption, Pikachu full-charge Skull Bash and Pichu left-facing aerial Skull
Bash. Their original motion/contact gates remain required. Rook's unchanged
scripts were rebuilt for the SDK, with
285 own-rig clips and zero borrowed clips; installation replaced her package.
This supplies no additional Rook gameplay acceptance.

```powershell
python tests/native/pikachu_pichu_thunder_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/pikachu-thunder-new
```

## Metadata correction

Reviewing the new bases found Bowser's clone using audio bank 12 instead of
stock Bowser's bank 15. The definition now selects `koopa.ssm`. A native check
compares sound-bank masks and action/demo counts for all 14 supported stock
clone bases against the original engine tables. This validates metadata;
it does not substitute for listening to effects or reviewing rendered output.

## Fighter-wide policies checkpoint - 2026-09-30

The combined template now owns load/wall-jump configuration, death/costume
selectors, held-item visibility, auxiliary-item presentation, knockback model
response, first-jab bookkeeping/restart, forward-smash effect hitlag callbacks
and CPU policies. Reusable engine primitives retain ordinary transition,
collision, item, animation and planner work. All decisions in these authored
handlers are script-owned; none calls a stock Pikachu/Pichu handler.

First-jab entry needs an integer transition-flag result and a one-shot callback
after the new attack category is assigned. The generic
[`on jab_start`](scripting/common_policies.md#on-jab_start) and
[`callbacks.state_changed`](scripting/functions.md#callbacksstate_changedfunction)
preserve that ordering. `on forward_smash_start` installs normal effect hitlag
callbacks after shared smash entry. The auxiliary visibility hooks replace the
two separate stock attachment/reset slots. Saved visibility group arguments now
accept expressions, including the costume index, with actual table bounds.

The scripts also own spacing, attack classification, ranged candidate selection
and recovery command construction. Recovery retains the original 60-unit
horizontal threshold, two directional segments, 15-frame waits and exactly one
RNG draw in the near branch. Destination coordinates and queued input are
generic [CPU operations](scripting/cpu_policies.md#on-cpu_recovery), not a
Pikachu-specific engine method. The six up-special collision-box dimensions
apply instance scale at use, matching stock's scaled live attributes while
keeping package-owned tuning values unmodified.

Fourteen full stock/script pairs match: for each fighter, 700-sample repeated
jabs, actual forward-smash victim contact and held-bat pickup/up-special/drop;
plus four 2200-sample CPU cases (Final Destination level 9, shielded opponent,
level 4 and Battlefield). Both ports' traced fighter state, pose/combat/visibility,
item state and RNG remain compared. The jab fixture requires animation restarts
and excludes a second-jab motion; both perform 33 restarts. CPU fixtures require
actual opponent damage. Four final-template regressions also match: Pikachu's
Thunder interruption and second dash, Pichu's Thunder absorption and left-facing
aerial diagonal up-special, with their original contact/motion gates retained.

Initial jab coverage mistakenly counted action-log motion changes; repeated
entries to the same motion ID are omitted there. Fresh checks count animation
frame resets. Forward smash is motion 60, not 58; early inputs also left Zelda
outside hit range. Those attempted contact cases are excluded. Fresh recordings
approach closer and require real victim damage; equal noncontact traces alone
do not establish contact behavior.

Evidence: `.local/pikachu-pichu-policy-acceptance.json` records the accepted
folders, comparisons and hashes. Build and all 36 focused backend tests pass.
Both script forms check integer jab flags, one-shot bindings, auxiliary slots,
dynamic visibility bounds, destination coordinates and the shared RNG call.
Malformed callback context/arity is rejected. Rook's unchanged scripts were
rebuilt for the SDK, preserving 285 own-rig clips, zero borrowed clips and one
replaced installed package. Tests remain hidden, accelerated and normally drawn.

```powershell
python tests/native/pikachu_pichu_policy_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/pikachu-policy-new
```

## Costume variants and CPU recovery checkpoint - 2026-09-30

All four costume variants now match for both fighters through Thunder and
both up-special dashes: sixteen 700-sample stock/script pairs. The custom
skins copy the retail model bytes and only rename their two public asset
symbols. Each skin declares a package-owned [`costume.variant`](scripting/model_attachments.md#properties)
separately from `costume.index`, which remains the engine's asset slot.
Pichu's authored death/reset selector uses the variant for its headwear.

These comparisons found two generic asset bugs. A replaced custom skin still
uses asset slot zero, so Pichu's red headwear initially selected the default
visibility. Blue and green then exposed different texture-animation bindings:
the independent fighter importer retained only one costume-table row, and the
engine selected that row by asset slot. The importer now keeps every variant
row in the fighter's own DAT; visibility and texture animation select the
skin's variant, with a bounds-checked row-zero fallback. There is no runtime
dependency on another fighter's model tables. The asset roundtrip checks also
caught an unreferenced gap after an edited texture table; its next relocation
target is an upper bound, while the shared variant count comes from the
visibility table. The action insertion/deletion package test passes again.

Eight 1000-sample CPU recovery pairs also match: each fighter begins airborne
on each side of Final Destination, at near and far distances. The reusable
offline [`--fixture-airborne`](native_match_launch.md) uses shared fall, movement
reset and collision-box initialization, consumes air jumps, and leaves the
normal CPU planner's choices intact. It does not force a destination, recovery
queue or random result. Recordings require actual recovery decisions with the
expected horizontal-distance branch, identical planner destinations, airborne
startup/travel/end motions and two initial dashes. Both ports' fighter state,
pose/combat/visibility, item state and RNG match throughout.

The first recovery coverage check mistakenly required grounded startup motion
353, despite starting airborne. The completed recordings reached 356/357/358;
they were compared again with the corrected gates rather than rerun. The
accepted report records the actual decisions and dash counts separately from
the original failed coverage report. This verifies both distance branches,
not every possible seeded outcome of the near branch's random choice.

Evidence: `.local/pikachu-pichu-variant-recovery-acceptance.json`,
`.local/pikachu-pichu-costume-accepted/results.json` and
`.local/pikachu-pichu-recovery-first/accepted.json`. The complete build and all
38 focused backend checks pass, including all stock fighter asset imports,
edited action package roundtrips, costume metadata and offline fixtures.
Rook's unchanged source was rebuilt for the SDK, retaining 285 own-rig clips
and zero borrowed clips while replacing the existing installed package.
No new Rook gameplay or direct pixel/audio acceptance is claimed.

```powershell
python tests/native/pikachu_pichu_costume_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/pikachu-costumes-new
python tests/native/pikachu_pichu_recovery_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/pikachu-recovery-new
```

## Remaining work

Broader clone acceptance still needs additional interaction comparisons.
The targeted CPU recovery and nondefault costume comparisons above close those
coverage gaps. Auxiliary attachments and scaled collision boxes still need
targeted runtime evidence. Thunder's owner removal/stock loss during a live chain,
clanks, held items, costumes/scales, direct audio/pixel review and Workshop/rollback
integration are not established by this batch. Skull
Bash's wall response, successful airborne contact, charge interrupted by
damage, held items, additional costumes and opponents, direct effects/audio
review and Workshop/rollback integration are not established by this batch.
Jolt controller reflection before surface contact, shield bounce, additional
costumes/scales and direct audio/pixel review remain outside this matrix.
Quick Attack/Agility's direct wall/ceiling impact, ledge catches, interrupted
startup, costumes/scales and Workshop/rollback integration remain open.
Keep the bases marked partial until the relevant translations and acceptance
are complete; a draft that imports successfully is not that acceptance.

## Independence source checkpoint â€” 2026-10-02

The source census found no remaining Pikachu/Pichu versus decision requiring a
new generic API. The promoted template remains inherited; only the isolated
source fixture adds `behavior independent` and tests the real module-entry
ownership guard. This is source acceptance, not a new gameplay acceptance.

Both bases now own the exact retail v1.02 PlCo semantic maps: 29 bindings each,
53 physical/runtime parts for Pikachu and 46 for Pichu, and no omissions. The
fixture reconstructs the full inverse maps, including unnamed semantic entry
53, and traverses all eight costume joint graphs. In both rigs raw part 5 is
`WaistN`; the semantic maps retain each fighter's distinct shoulder/hand/head
indices. Existing article bindings (Thunder slot 0, ground Jolt slot 1, air
Jolt slot 2), effect bank 7, sound banks 24/23, action/demo counts 320/14 and
owned demo archive root strings remain explicit immutable dependencies.

`ftData_AssignStockCallbacks` was audited slot by slot. Load/attribute reload,
death, all eight special entries, pickup/drop (ordinary and extension slots),
item visibility and knockback notifications are authored. Pikachu's two
auxiliary attachment visibility callbacks are authored; Pichu's source slots
are null and the shared script branches to no action. Landing, user-data
removal, absorb, late/color updates, extra motion tables, matrix callbacks,
paired callbacks, demo selection callbacks and model-event/getter hooks are
null for both stock families. Demo archive names are immutable owned metadata,
not retained callbacks. No fighter-specific aerial-jump, shield, dynamics,
heavy-item, throw or versus result/intro/viwait decision was found outside the
authored moves. Kirby-copy behavior remains deferred.

The named common branches are jab setup/restart and forward-smash effect
hitlag bindings, plus CPU ranged candidates and recovery. Those already use
the script's owned hooks. CPU spacing initialization/update and ordinary
attack/grab/capture/charge/threat predicates are also explicit script policy.
Pichu's voice, walljump, headwear and omitted up-special trail choices use
owned configuration; her self-damage remains in the owned copied subactions.

The actual generated callbacks were compiled with frozen baseline18 headers
and Clang, separately from the content tool's ordinary project preparation.
The driver extracts the unchanged stock CPU/lifecycle/common policy bodies
and original queue/RNG operations from that snapshot. Each fighter passes
19,503 policy checks and four edited-parameter checks: **39,006 policy checks
and eight tuning edits in total**. Recovery covers both sides of the exact
60-unit threshold, both seeded near choices, full RNG state/draw counts and
existing queue prefixes. Ranged selection covers target absence, both facings,
ray/floor outcomes and every candidate capacity from zero through eight.
Spacing compares original RNG and floating-point results. Lifecycle checks
cover all four headwear variants and out-of-range values, ordinary/extension
item flags, knockback, auxiliary visibility, jab bookkeeping and smash hitlag
bindings. The independent module replaces all 104 motion gameplay slots,
including all 96 originally non-null callbacks, while preserving motion
metadata and the common camera callback. This ownership check does not
execute every special phase body anew.

Quick Attack collision boxes match the stock arguments at six instance scales
in grounded/aerial recovery. The stock reload scales the six DAT box fields;
the script keeps owned parameters unscaled and multiplies them at the collision
call. The existing Pichu death script first hides all headwear and then reveals
the chosen group; stock assigns each group once. The fixture compares final
visibility, not those incidental same-callback write counts. No rendering or
intermediate-frame difference is claimed from that sequence.

Effect construction was audited separately against frozen dispatcher and
constructor bodies. Jolt root effect `0x4BD`, Skull Bash `0x4BE/0x4BF`, the
up-special trail `0x3F4`, Thunder cloud `0x4C3` and impact `0x40C` are particle
generators. Thunder hit model `0x4C0` uses ordinary joint attachment and installs
no fighter updater. None reads the stock Pikachu/Pichu move union. The shared
rotation updater that reads a common fighter union is installed by `0x48C`,
outside these recipes. No authored model-effect callback is required by this
census; direct pixel/audio review remains outside source acceptance.

Evidence is under `.local/pikachu-pichu-independence`: `rigs.json`,
`callback_slots.json`, both normal imported projects, and each `*-direct`
directory's generated independent source, original excerpts, input hashes,
build log and results. The live disjoint rig definitions are explicitly hashed
alongside frozen SDK/tool inputs. Production templates were not changed and
no game, global build or commit was launched by this fixture.

Reproduction uses normal parameter import before source compilation:

```powershell
$tool = '.local/baseline19/bin/native_content_tool.exe'
$iso = 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso'
& $tool clone-stock $iso Pikachu .local/pikachu-source-new 'Pikachu source comparison'
& $tool import-parameters .local/pikachu-source-new examples/pikachu_pichu_wip.fms .local/pikachu-source-new/behavior.fms
python tests/native/pikachu_pichu_policy_stock_compare.py --snapshot .local/baseline19 --project .local/pikachu-source-new --out-dir .local/pikachu-direct-new --family pikachu
python tests/native/pikachu_pichu_rig_stock_compare.py --iso $iso --output .local/pikachu-pichu-rigs-new.json --validate-clones
```

Repeat the project commands with `Pichu`, a fresh project/output directory and
`--family pichu`. The final standalone compile explicitly selects MSVC 14.44
and frozen snapshot include paths; it does not rely on the tool's embedded
live-header path. No project DLL build is needed before this source fixture:
it reads the imported FMS/FMP, generates actual independent module C with the
frozen tool, and compiles that into its standalone comparison harness.
Frozen tools' `script-build` embeds a live SDK include path and must not be used
to claim frozen-SDK compilation. Independent drawn regressions and the
interaction edges in the historical ledger above remain pending.

## Baseline19 preparation checkpoint (before promotion) â€” 2026-10-02

The source fixture was refreshed once per fighter against baseline19's matching
tool and SDK. Both pass 19,503 original-C policy comparisons, four edited-parameter
checks and the module-entry guard for all 104 authored phases (96 originally
non-null). Combined totals remain 39,006 policy comparisons plus eight
edited-parameter checks. Baseline18 evidence above is preserved. New evidence:
`.local/pikachu-pichu-closure19-source/{pikachu-direct,pichu-direct}` contains the
generated C, unchanged source excerpts, source hashes, build commands/logs and
results. These are source checks, not new gameplay or scale-rendering acceptance.

[`pikachu_pichu_closure_parity.py`](../tests/native/pikachu_pichu_closure_parity.py)
prepares reusable whole-independence acceptance. The default focused batch is
24 recordings per fighter (48 pairs, 40,800 controller samples), retaining exact
accepted FMR bytes and fighter-specific corrected trajectories. It spans all
four specials, Jolt reflection/absorption, full charge and Skull contact,
two up-special dashes/Cape turn/slope, Thunder self-contact/platform/victim/
shield/interruption/absorption, repeated first jab, smash contact, held bat,
CPU levels 4/9, near/far recovery and red-costume Thunder. The optional
`all-accepted` suite contains124 pairs / 101,200 samples; `costume` selects all 16
accepted costume recordings. Fighter and `group/name` filters allow a bounded
follow-up without regenerating trajectories.

Plan validation checks the requested motion/item/contact gates and supplemental
gates against **both** retained historical traces. Tap Hold is not required
because it enters/exits within a callback; slope and recovery gates require the
observed air motions; the Pichu contact case uses its accepted replacement FMR.
Cape requires left-facing recovery followed by rightward travel, interrupted
Thunder requires actual damage motion rather than Pichu self-damage, and CPU
recovery requires an actual decision, the requested distance branch and both
initial dashes. Runtime comparison still checks both ports' complete traced
fighter/combat/item/relations/visual/RNG state and verifies independent behavior
identity. CPU fights require real opponent damage and attack activity.

Prepared candidate packages and executable commands are preserved under
.local/pikachu-pichu-closure19-ready, with plan.json, packages.json and
commands.json receipts. Preparation succeeded for both packages and the
red retail costume copies; no new gameplay launched.

The runner uses normal project import, frozen `script-c`, and direct Clang with
MSVC 14.44 and frozen include directories. It validates snapshot hashes, verifies
the package with its copied base and parameters, and records source, FMP,
generated-C, DLL, package, skin, recordings and command receipts. Games use the
shared hidden/muted/fast runner with drawing enabled. `--prepare-only` creates
packages/commands; it launches no game. `--plan-only` reads/copies inputs and
checks historical coverage without invoking the native tool or a compiler.

```powershell
python tests/native/pikachu_pichu_closure_parity.py --iso '..\Super Smash Bros. Melee (USA) (v1.02).iso' --tool .local/baseline19/bin/native_content_tool.exe --exe .local/baseline19/bin/melee_native.exe --out-dir .local/pika-closure-candidate --candidate --prepare-only
```

At that preparation checkpoint, `--candidate` added independence only to
isolated imported FMS. Current shipped acceptance omits that flag and
requires the shipped template to declare `behavior independent`, so candidate
results cannot silently count as shipped acceptance. Gameplay launches require
root's coordinated runtime slot; package preparation is not gameplay acceptance.

Live Thunder-chain stock loss/owner removal, auxiliary attachments and runtime
instance scaling remain unproven. Source scaling checks do not fill those gaps.
The costume recordings exercise initialization/visibility, not another stock
loss while articles live. The broader interaction and pixel/audio/Workshop/
rollback limits in the historical ledger remain explicit; this preparation
does not close them.
