# Donkey Kong script translation

The [independent template](../examples/donkey_wip.fms) authors all four
specials, cargo acquisition, carrying locomotion, heavy-item carrying and eight
cargo throw motions. It now declares `behavior independent`; its compiled
Definition has no stock behavior identity. Holding damage is source-compared,
but entry still lacks a confirmed stock runtime path. Shared CPU classification,
spacing, charging/cancellation and cargo tactics are now authored, alongside
load/reload, heavy-item throw movement and heavy-item damage decisions.
Kirby cloning and custom copy
abilities are deferred by user decision, outside current acceptance.

The 2026-10-02 closure checkpoint below records independent package acceptance.
Earlier dated checkpoints describe the inherited drafts at those dates.

## Authored behavior

| Move | Source reference | Script ownership |
| --- | --- | --- |
| Headbutt | [ftdonkeyspecials.c](../src/melee/ft/kinds/ftDonkey/ftdonkeyspecials.c), [effect callbacks](../src/melee/ft/kinds/ftDonkey/ftdonkeyspeciallw.c) | Ground/air entry, command reset, animation finish, delayed air gravity, horizontal deceleration, collision transitions and one-shot TransN effects. |
| Spinning Kong | [ftdonkeyspecialhi.c](../src/melee/ft/kinds/ftDonkey/ftdonkeyspecialhi.c) | Entry velocity/clamps, consumed jumps, animation finish/freefall, ground steering, air gravity/drift, ground/air transition flags, ledge catch and effect hitlag/cleanup. |
| Giant Punch | [ftdonkeyspecialn.c](../src/melee/ft/kinds/ftDonkey/ftdonkeyspecialn.c) | Ordinary persistent charge and move variables, all ten motions, full/partial punch damage and momentum, escape/cancel input, color/effects, collision transitions and primary/secondary damage cleanup. |
| Hand Slap | [ftdonkeyspeciallw.c](../src/melee/ft/kinds/ftDonkey/ftdonkeyspeciallw.c), [terrain placement](../src/melee/ft/kinds/ftDonkey/ftdonkey.c) | Ground-only entry, repeat/end phases, one-shot cue, four terrain-following world hitboxes, attack statistics and asynchronous effect/hitlag setup. The unused air-end motion is also authored. |
| Cargo carrying and throws | [ftCo_CargoWait.c](../src/melee/ft/kinds/ftCommon/ftCo_CargoWait.c), [ftCo_CargoWalk.c](../src/melee/ft/kinds/ftCommon/ftCo_CargoWalk.c), [ftCo_CargoJump.c](../src/melee/ft/kinds/ftCommon/ftCo_CargoJump.c), [ftCo_CargoThrow.c](../src/melee/ft/kinds/ftCommon/ftCo_CargoThrow.c), [common throw bookkeeping](../src/melee/ft/kinds/ftCommon/ftCo_Throw.c) | Forward-throw finish acquires the victim, initializes owned clip lengths, selects all carry/throw callbacks and victim poses, prioritizes throw/jump/drop/turn/walk input, owns startup/turn/landing timers and paired throw transitions. Holding-damage callbacks are authored; no stock runtime entry for motion 360 has been confirmed. |
| Heavy-item carrying | [ftdonkeyheavywait0.c](../src/melee/ft/kinds/ftDonkey/ftdonkeyheavywait0.c), [ftdonkeyheavywalk.c](../src/melee/ft/kinds/ftDonkey/ftdonkeyheavywalk.c), [ftdonkeyheavyturn.c](../src/melee/ft/kinds/ftDonkey/ftdonkeyheavyturn.c), [ftdonkeyheavyjump.c](../src/melee/ft/kinds/ftDonkey/ftdonkeyheavyjump.c), [pickup](../src/melee/ft/kinds/ftCommon/ftpickupitem.c) | Pickup finish, idle, three walking variants, turnaround, jump startup, jump/fall, landing and holding-damage callbacks are scripted. The script owns timers, interrupt priorities and destination states; common heavy-item throwing stays a reusable engine primitive. Damage keep/drop entry, throw movement and load/reload setup are also owned. |
| CPU policies | [shared CPU setup/planner](../src/melee/ft/kinds/ftCommon/ftCo_0A01.c), [attack/capture decisions](../src/melee/ft/ftcpuattack.c) | Own spacing defaults/update, special-attack and cargo classification, charge entry/cancellation and walking a captured victim to the closer edge before throwing. |

The move callbacks call shared engine primitives, never stock Donkey Kong
special handlers. Import-only `attribute` declarations seed named values in
[`fighter_parameters.fmp`](scripting/language.md#fighter-attributes).
Generated behavior reads those owned values, without stock DAT offset reads.

Death, charge-dependent color reset, item presentation and knockback callbacks
are authored as well. Initial setup and special-attribute reload use generic
asset-copy primitives; no DK load handler, heavy-item flag or attribute pointer
is inherited. Cargo and heavy-item movement use owned animation-derived lengths
and ordinary variables. Named movement tuning is scaled explicitly when needed;
generated behavior does not read DK's live special-attribute structure.

## Five-costume comparison checkpoint - 2026-09-28

Ten fresh comparisons cover all five retail costumes: normal, black, red,
blue and green. Each costume runs a charged Giant Punch with real Zelda
contact, then a separate aerial cargo back throw beginning while facing left.
The punch recordings require charge-loop and release entry plus victim damage;
the throw recordings require motion 366 with the intended facing, victim
damage, an aerial finish/landing and return to idle. Both fighters' pose,
hitbox, hurtbox, visibility, movement and damage traces and gameplay RNG match.
These selected recordings do not establish exact pixel/audio equivalence or
every interaction possible with those costumes.

The new [costume runner](../tests/native/donkey_costume_parity.py) extracts DATs
through the existing native tool, builds one independent authored fighter and
imports four additional costumes as installed dependencies. Only the two main
public root names are normalized for the custom slot; every other byte and all
file offsets remain retail data. The acceptance report verifies that equality
against the extracted files, including an unchanged default costume.

Evidence: `.local/donkey-costume-acceptance.json` records ten complete trace/
input comparisons, package IDs, all five asset hashes and the owned source /
generated C hashes. Tests stayed hidden with ordinary drawing; window visibility
was observed during the batch. The initial harness used the wrong basename
for its coverage-log lookup after one matching run; that incomplete batch is
excluded and all ten cases ran fresh after correcting the harness. No engine,
script API or Rook package change was needed. The preceding build and 34
backend checks remain the current code checkpoint; they were not repeated for
a runtime harness/documentation addition.

Broader opposing-fighter, interruption and lifecycle acceptance remains open.
No confirmed stock entry into cargo holding-damage motion 360 has been found;
this does not justify inventing a gameplay entry or postponing the rest of
the roster. DK's scripted callbacks for it remain available. Continue the
remaining vanilla fighter translations with reusable API extensions; Kirby
cloning and custom copy abilities stay deferred.

## Owned CPU policies checkpoint - 2026-09-28

All eight CPU policy hooks are authored. CPU spacing defaults are editable
owned parameters; the shared random draw remains at its original point.
Charging reads the ordinary `state.punch_charge` variable, never DK's union.
Cargo decisions query a connected floor island and append ordinary controller
commands. A declared callback replaces the fighter-kind branch; returning
false continues common logic rather than calling the original DK branch.

Six fresh 3,000-controller-sample recordings match stock, comparing both
fighters' full traces, action/damage events and gameplay RNG. Level-nine CPUs
fight stationary and shielding Zelda on Final Destination and Battlefield;
levels one and four also fight stationary Zelda on Final Destination. Every
case requires actual victim damage. Level-nine stationary cases require
charge, loop, cancellation and released-punch entry. Shielding cases require
cargo acquisition, slow/middle walking and forward cargo throw. Together they
require actual leftward and rightward cargo input. One shielding recording
finishes its match before the controller stream ends; the complete stock and
clone streams still match through that end. These are selected planner paths,
not proof of every CPU branch or every level.

The first Final Destination shielding run diverged at sample 866 because a
negative float local converted directly to an unsigned command byte. The
compiler now truncates through an integer before encoding the byte, preserving
signed stick values with defined integer conversion. Both callback forms have
contract coverage for negative locals in one- and two-argument commands. All
six runtime cases reran with that fix; preliminary traces are excluded. An
initial all-CPU setup was refused before launch by the direct-match human-slot
requirement and contributes no runtime evidence.

Evidence is `.local/donkey-cpu-policy-acceptance.json`, with input/trace/script/
parameter/profile hashes, all eight generated callback bindings and cargo
coverage checks. The native build and all 34 focused backend checks pass.
Windows visibility was observed hidden during the batch; drawing remained
enabled. The preceding 80 manual-input comparisons were not rerun for these
CPU-only hooks. No new pixel/audio or complete costume/lifecycle acceptance
is claimed.

The reusable API is documented under [CPU policies](scripting/cpu_policies.md)
and [collision-island bounds](scripting/stage.md#stageisland_boundsx-y-z-probe_y-found-left_x-left_y-right_x-right_y).
There is no DK-specific script API, live DAT tuning read or stock special
handler delegation. Rook rebuilt from unchanged behavior source for the SDK,
retaining 285 own-rig clips and zero borrowed clips. Her package
`818b821c85cf765665db9759e762b0caee7eeea55b37d680eff9d3a11b64cea5`
replaced the old installation; Collection lists one Rook. This validates the
package, not another Rook gameplay run. DK remains partial pending the wider
acceptance below, and Kirby cloning/custom copy abilities remain deferred.

## Owned setup and heavy-item policies checkpoint - 2026-09-28

DK's script now owns load/reload callbacks, animation-derived carry lengths,
heavy-throw physics/collision overrides and heavy-item damage policy. It does
not set the stock heavy-item flag or special-attribute pointer. Carry speeds
use named owned tuning and the current instance scale. All 184 motion callbacks
are authored: 40 heavy-item, 72 cargo and 72 special callbacks.

Eighty complete stock/script recordings match after removing that setup
dependency: 37 specials, 26 cargo and 17 heavy-item cases. The latter include
actual weak hits retaining the barrel in motion 350, strong knockback dropping
it on the damage frame, and giant/tiny carrying followed by buff expiry.
Scale acceptance requires 1.8 or 0.5 throughout each walk variant and a return
to 1.0; merely spawning a mushroom or comparing equal traces is insufficient.
The fresh heavy-drop case follows stock's normal damage path rather than
requiring an immediate idle return.

These tests uncovered a shared native mushroom-state defect. Its original
32-bit cross-member union aliases overlap 64-bit callbacks with scale values.
Growth/shrink now store callbacks, start/target scale and saved movement in
one named state structure, and semantic replay hashing includes those values.
An unrelated Like Like capture alias referencing the removed mushroom fields
now uses its own capture state directly. Fighter traces record instance scale.
This repairs a native layout mismatch while retaining the original scaling
and movement rules. Stock and scripted clients shared the defective path,
which is why equal preliminary traces did not establish scaling fidelity.

Evidence: `.local/donkey-owned-setup-acceptance.json` records all case/input/
trace hashes and ownership checks. Special/cargo runs precede the isolated
mushroom correction; the fresh heavy batch includes it. Growth/shrink/expiry
reran after explicitly initializing the new velocity-clear flag. All 34
fighter/replay/item-fixture backend checks pass, as does the native build;
compiler, common-policy and replay checks also pass after the final change.
Windows visibility was checked during hidden tests with drawing enabled.
Exact pixels/audio, all costumes and every possible interruption are not
newly established.

New generic operations are documented as [motion callback binding](scripting/functions.md#binding-a-motion-callback),
[heavy throw entry](scripting/fighter.md#on-heavy_throw_enter),
[heavy-item damage](scripting/fighter.md#on-heavy_item_damage) and
[instance scale](scripting/movement.md#current-model-scale).
They expose reusable engine operations; no DK-specific script namespace was
added. Cargo holding-damage motion 360 is authored but its entry still lacks
runtime evidence. Shared CPU attack classification, charge cancellation and
cargo tactics remain open. DK remains a partial template.

Rook rebuilt from unchanged behavior source for the current SDK, preserving
285 own-rig clips and zero borrowed clips. Package validation passes; the new
installation replaces the previous one. No new Rook gameplay acceptance is
claimed. Kirby cloning/custom copy abilities remain deferred.

## Cargo acquisition and locomotion checkpoint - 2026-09-28

All 72 phase bindings for motions 351–368 are authored, along with common
forward-throw finish policy. Carrying turn/jump/landing timers and three walk
animation lengths are ordinary fighter variables. The script selects victim
poses and preserves stock's unusual slow-walk victim-motion mapping. Shared
walking, jumping, landing and capture primitives do not call DK handlers.

Twenty-six complete stock/script recordings match after these changes: the
eighteen throw cases below, plus eight carry paths covering all walk variants,
left turnaround, full/short button and tap jumps, ledge fall and an elevated
Battlefield platform drop. Every case requires its intended motions and actual
victim damage. Jump acceptance also requires full hops to peak above short hops.
Both fighters' complete traces and RNG match; comparisons do not prove pixels
or every interruption time. The holding-damage motion 360 is bound but has no
new runtime coverage, and its shared damage-entry decision remains inherited.

Evidence: `.local/donkey-cargo-locomotion-acceptance.json`, with trace/input/source
hashes and generated binding checks. All accepted packages have identical owned
script source. Preliminary walking inputs fell off the ledge before the expected
ground throw; preliminary Battlefield inputs failed to acquire Zelda. Those
recordings are excluded, and corrected sequences cover both intended paths.
The build and 32 backend checks pass. Windows visibility was checked during
a hidden run; drawing stays enabled. The earlier four-special matrix was not
rerun. Rook rebuilt from unchanged source for the SDK layout, preserving 285
own-rig clips and zero borrowed clips and replacing her installation.

New reusable APIs are documented under [walking and jump startup](scripting/movement.md#authored-walking),
[owned animation lengths](scripting/movement.md#animation-lengths),
[throw finish](scripting/fighter.md#on-throw_finish) and
[held-fighter transitions](scripting/fighter.md#held-fighter-transitions).
Heavy items, load/reload, damage-entry and CPU/shared-kind branches remain open.

## Cargo throw comparison checkpoint - 2026-09-28

Eighteen complete stock/script recordings match: all four cargo throws on
ground and in air, facing both ways, plus two later-in-jump up throws requiring
the paired grounded throw motion. Every recording requires the intended entry,
starting facing, actual victim damage and return to idle. Aerial recordings
must also reach their paired ground motion or finish into normal fall after
throw entry. The comparison includes both fighters' full traces and RNG.

The initial up throws finish in air before landing, in both stock and script.
A preliminary check incorrectly required landing during every aerial throw.
Those complete traces are rechecked with the correct finish/landing policy;
separate late up throws cover the landing branch. A preliminary late input was
already grounded when it attempted the throw, so it is excluded.

Evidence is `.local/donkey-cargo-throw-acceptance.json`, combining the original
sixteen recordings and the corrected two landing recordings. It records
source/trace hashes and verifies all 32 cargo motion phase bindings in generated
C. The native build and 32 fighter backend checks pass. Windows visibility was
checked during the hidden, normally drawn batch. The earlier four-special
matrix is a previous checkpoint and was not rerun for this addition.

Reusable additions are [shared throw update/movement and animation facing](scripting/fighter.md#throwsupdate).
No new DK-only API or live fighter-union property was introduced. Rook was
rebuilt from unchanged behavior source for the host layout, retaining 285
own-rig clips and zero borrowed clips, and replaced her previous installation.

These recordings cover airborne-to-ground transitions, including back-throw
facing reversal. Leaving a ledge during an already grounded cargo throw is not
newly covered. Cargo entry, held-fighter locomotion, heavy-item behavior,
animation-derived setup and CPU kind branches remain open. DK is still partial.

## Four-special comparison checkpoint - 2026-09-28

Thirty-seven complete stock/script recordings match. These include fresh runs
of the fourteen Headbutt/Spinning Kong paths below, plus Giant Punch release,
full charge/reentry, shield cancel, escape and both starting facings on ground
and in air. A full-charge → Spinning Kong → punch sequence checks cross-move
charge retention. Damage recordings cover both interruption while charging
and damage after cancellation, followed by another charge/punch. The latter
preserves stock's distinction: damage outside the registered move cleanup does
not necessarily erase stored charge.

Hand Slap covers both facings, single and repeated sequences, real damage to
Zelda, and repeated slaps on Yoshi's Story's terrain. Repeat acceptance requires
at least two animation restarts. Ground-only scripting leaves airborne down-B
disabled rather than inventing an air move. Starting aerial charge recordings
also exercise landing transitions; they do not cover every possible ledge fall
or collision time in every punch phase.

Evidence is `.local/donkey-all-special-acceptance.json`, which rechecks both
fighters' complete traces, RNG, required motions, actual contact and restarts,
and records trace hashes. It combines the main batch with a corrected Hand Slap
contact recording and the active-charge damage recording. The original slap
contact recording was too far away and dealt no damage in either run; it is
excluded. All three packages use identical owned script source. The native
build and 32 fighter backend checks pass. Test windows remain hidden while
drawing runs normally; Windows visibility was checked during the batch.

Reusable additions are documented under [terrain queries](scripting/stage.md#terrain-queries),
[world hitbox placement](scripting/combat_visuals.md#hitboxset_world_positionindex-x-y-z),
[cue consumption](scripting/combat_visuals.md#cueconsumebit),
[secondary damage callbacks](scripting/combat_visuals.md#damage-callback-lifecycle),
[escape checks](scripting/movement.md#interrupt-checks-and-platform-dropping)
and [single-plane move declarations](scripting/language.md#moves-and-subactions).
They describe shared engine operations, not DK-specific properties. Punch
charge and base-damage snapshots remain ordinary editable fighter variables.

Rook was rebuilt from unchanged behavior source for the new host layout, with
285 own-rig clips and zero borrowed clips. Package registration/release passes,
and the new package replaces her previous installation. No Rook gameplay run
or pixel/audio comparison is claimed for this checkpoint.

## Heavy-item carrying checkpoint - 2026-09-28

The script owns heavy pickup finish and all 40 callbacks for motions 341–350.
It reads walk clip lengths into ordinary fighter variables and shares the
cargo walking/jump/landing primitives. Heavy turnaround permits throw/jump
interrupts only after facing flips, unlike cargo's earlier interrupt window.
The authored holding-damage motion is bound, but its entry/coverage remains
open. Shared item-throw states still rely on inherited setup for DK's movement
policy; this checkpoint does not declare complete fighter ownership.

Thirteen complete 700-sample stock/script comparisons match. They cover four
directional smash throws, three walking speeds, left turnaround, early blocked
and late permitted turn interrupts, full/short button and tap jumps, landing,
and an off-edge carry/air throw followed by death and rebirth. Forward throw
requires actual damage to Zelda. Both fighters' pose, hitbox, hurtbox,
visibility, action/damage traces, ordinary item state and RNG match. Full jumps
peak above short jumps. The ledge case initially had an invalid idle-return
expectation; its unchanged full traces were rechecked for stock's actual
fall/throw/death/respawn path. No game change was made to satisfy that check.

Evidence: `.local/donkey-heavy-acceptance.json`, including input/trace hashes,
owned-source hash and generated 40 heavy/72 cargo binding checks. The game
build, 33 fighter/replay backend checks and the item-fixture contract check
pass. Windows reported the test game hidden, with drawing enabled. Earlier
special/cargo matrices were not rerun, and exact pixels/audio are not claimed.

Two generic additions are documented as
[`on heavy_pickup_finish`](scripting/fighter.md#on-heavy_pickup_finish) and
[`controls.try_heavy_item_throw()`](scripting/movement.md#interrupt-checks-and-platform-dropping).
A fixed-capacity [offline item fixture](native_match_launch.md) lets stock and
clone receive identical ordinary items at a selected input sample; it is
rejected with netplay/rollback probes and does not run during normal play.

Rook rebuilt from unchanged behavior source for the host layout, retaining
285 own-rig clips and zero borrowed clips. Her previous installation was
replaced by `9347ba2fb7926670d433cbdb6ab3160abc13078abde7c0fb7c37f2613947f464`.
No new Rook gameplay comparison is claimed.

## Earlier two-special checkpoint - 2026-09-28

Fourteen 430-sample stock/script comparisons match exactly. Headbutt covers
ground/air entry facing both ways and aerial transition back to the grounded
motion. Spinning Kong covers ground/air entry facing both ways, stick steering
and release, a grounded run off the stage, and aerial finish into helpless
fall; stationary aerial cases also reach special landing and idle. Each move
has a grounded contact recording requiring actual damage to Zelda.
The comparison includes both fighters' pose/hitbox/hurtbox/visibility digests,
action and damage events, and the shared RNG sequence.

Evidence is `.local/donkey-special-final/results.json` and its per-case logs.
`.local/donkey-special-acceptance.json` rechecks the comparisons and entry/
transition coverage. Windows reported the running game window hidden; drawing
remained enabled. The initial batch caught the wrong drift helper and included
an invalid expectation that every aerial spin lands before finishing; it is
excluded from acceptance. Fourteen focused backend checks pass in
`.local/donkey-script-tests.log`. The native build succeeds. Rook's existing
installed package still passes registration/release validation; her behavior
was not rebuilt or retested in gameplay for this change.

Spinning Kong uses
[`physics.air_accel_friction()`](scripting/movement.md#physicsair_accel_friction).
Its stock helper retains momentum under ordinary aerial friction when the
stick is released. `physics.air_accel()` uses a different helper which can
clamp that momentum to zero; substituting it diverged in a run-off-stage
steering recording. The translation preserves the original helper choice.

The asset base copies 337 action entries, 14 demo entries, 75 joints and the
46-entry motion table. These counts come from the retail `PlDk.dat` tables
and a complete `PlDkNr.dat` joint traversal. The base is a starting definition,
not a claim that inherited callbacks are independent of stock-kind checks.
New Workshop drafts open this partial template uncompiled.

## Remaining ownership and acceptance

All DK-specific branches found in the shared CPU source are now owned by
scripts. Cargo holding-damage motion 360 has authored callbacks but no
confirmed stock runtime entry; it is not newly covered in gameplay. The wider
interruption/contact timing, opposing fighters, every CPU branch and match
lifecycle remain broader coverage work beyond the independent template's
bounded acceptance below. Stock Kirby's copied DK ability remains outside the
deferred Kirby clone scope.

Run the selected-move comparison using:

```powershell
python tests/native/donkey_special_parity.py --iso '../Super Smash Bros. Melee (USA) (v1.02).iso'
python tests/native/donkey_cargo_throw_parity.py --iso '../Super Smash Bros. Melee (USA) (v1.02).iso'
python tests/native/donkey_heavy_item_parity.py --iso '../Super Smash Bros. Melee (USA) (v1.02).iso'
python tests/native/donkey_cpu_parity.py --iso '../Super Smash Bros. Melee (USA) (v1.02).iso'
python tests/native/donkey_costume_parity.py --iso '../Super Smash Bros. Melee (USA) (v1.02).iso'
```

The runner builds an independent project/package, records controller input,
runs stock and clone with drawing enabled in hidden windows, compares both
fighter traces and the RNG sequence, and checks that the intended move and
required contact actually occur. It retains inputs, owned parameters,
generated C, packages and traces. Passing traces do not establish exact pixels.


## Owned rig and named model-policy candidate, 2026-10-02

The retail PlCo map contains 52 semantic roles and 75 runtime/physical joints,
with no omissions; all five costumes were checked. The existing 46 own states
cover all 177 nonnull stock phase callbacks. Both extended item notifications
now use the same authored pickup/drop presentation decisions as their regular
counterparts. HandSlapFacing reads current creator facing and owns the original
root-Y decision; its named position recipe remains queued at the original
bone-world-position sampling phase. Heavy/cargo gameplay coverage limits from
earlier checkpoints remain; this census does not add successful cargo-entry
coverage.

The rig fixture passes against the actual ISO's PlCo table, inverse map and
costume trees: `.local/donkey-bowser-closure/rig-validated.json`. The common
production-body group fixture passes catalog family/count, both members'
animation phases, secondary removal/recycling, atomic partial-allocation
failure, creator/module/rig changes, and deferred sampling. Its allocation and
typed constructor resource boundaries are explicit doubles.

The generated named model policies now pass 46,080 exact combined DK/Bowser
source comparisons and eight lifetime/registration rejection cases using
matching frozen snapshot19 binaries, SDK and compiler. Two repeated Hand Slap stock/clone pairs pass in both directions,
comparing 860 controller samples and 1,720 fighter states.
All runs are hidden, muted, accelerated and drawn. Gameplay trace acceptance
does not establish effect pixels, broader capture/interrupt edges or complete
fighter independence. The templates still retain their stock source kind.

Evidence: `.local/donkey-bowser-model-source19/acceptance.json` and
`.local/model-group-runtime19/acceptance.json`. The backend build and twelve
focused checks also pass; [the shared effect ledger](native_model_effects.md)
records lifetime, single-model regression and constructor-boundary details.

## Independent versus closure checkpoint (2026-10-02)

The shipped template now declares `behavior independent`. Its compiled
Definition has `source_kind = None`; it retains owned asset/rig/animation
metadata and generic engine primitives, with no stock fighter callback fallback.
The actual shipped DK/Bowser pair passes 5,461 original/generated source checks:
all 46/23 motion rows replace every original nonnull phase, exact 52/48 semantic
bone maps retain 75/76 runtime parts, and lifecycle, ordinary item, CPU and common
throw decisions remain owned. The fixture includes actual common victim-pose/
weight routing, DK cargo completion and punch collision decisions. World/terrain
probes, captured-victim initialization and presentation are explicit fixture
boundaries; source equality does not replace gameplay acceptance.

Ten combined hidden, muted, accelerated drawn stock/independent pairs pass,
comparing 14,550 controller samples and 29,100 fighter states. These cover DK and
Bowser CPU level 4 on Final Destination and level 9 on Battlefield, DK cargo and
heavy forward throws, DK punch-release contact, Bowser Flame contact, down-B
victim contact and side-B back-throw victim behavior. Four actual shipped-package
pairs also pass (6,600 samples/13,200 states): both Battlefield CPUs, DK heavy
forward throw and Bowser capture back throw. Candidate and shipped generated C
are identical after excluding only the exported source hash; owned parameters
and CPU profile bytes are identical.

The source review corrected a real DK translation defect: float local collision
flags rounded 0x0c4d508e to 0x0c4d5090 at motion 377/frame 12.5. Explicit `let int flags`
now preserves the original ordered calls and exact mask. Pre-fix evidence remains
in `.local/donkey-bowser-closure-source19-prefixed-flags/diagnostic.log`. Other
exactly representable flag locals were left alone.

Evidence: `.local/donkey-bowser-independent-acceptance19.json`,
`.local/donkey-bowser-independent-source19/acceptance.json`,
`.local/donkey-bowser-independent-runtime19/acceptance.json` and
`.local/donkey-bowser-independent-final19/acceptance.json`. Reproduce with
`tests/native/donkey_bowser_closure_source_compare.py` (normal shipped mode;
`--candidate` is explicitly pre-promotion only) and
`tests/native/donkey_bowser_closure_parity.py` using matching frozen19 inputs.

These selected paths do not establish every planner/CPU level, costume, holding-
damage entry, interruption or lifecycle trajectory, pixel/audio fidelity, results/
Workshop behavior, or network rollback. Earlier dated receipts remain bounded
evidence for the inherited drafts; the current independence claim comes from
this shipped-source and package checkpoint.
