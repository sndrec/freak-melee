# Fox and Falco scripted translation

The ordinary-interpreter templates pass full source preparation, real package
staging/publication and twenty connected gameplay comparisons against stock
native fighters. See the [current gameplay checkpoint](native_script_fox_falco_gameplay_checkpoint.md)
for cases, fixes and remaining coverage limits, and the
[input/status checkpoint](native_script_input_status_checkpoint.md) for source
preparation/publication. The generated-C comparisons below are historical;
they do not establish acceptance of untested interpreter behavior.

[Fox template](../examples/fox_wip.fms) |
[Falco template](../examples/falco_wip.fms) |
[Reflector API](scripting/reflector.md) |
[Owned parameters](scripting/language.md#fighter-attributes)

Fox and Falco can now be copied into independent Workshop asset projects.
All four specials, gun/laser/trail articles, throw-shot decisions and radio
taunts are now scripted together. Recorded comparisons cover their integrated
behavior. Each template declares independent versus behavior;
the closure census and final acceptance below document its scope and limits.

## Source and authored behavior

| Area | Decisions in the script | Original source |
| --- | --- | --- |
| Startup | Momentum, release/gravity timers and effect selection | `ftFox/ftfoxspeciallw.c` |
| Loop | Release gating, turn priority, jump cancel and platform drop | `ftFox/ftfoxspeciallw.c` |
| Hit | Facing, wind, reflect effect and resuming loop/end | `ftFox/ftfoxspeciallw.c` |
| Turn | Facing flip, rotation ticks, timer and loop/end decision | `ftFox/ftfoxspeciallw.c` |
| All phases | Physics, collision transitions and motion flags | `ftFox/ftfoxspeciallw.c` |
| Basic fighter events | Attribute copy/reload, walljump, held-item and knockback layers | `ftFox/ftfox.c`, `ftFalco/ftfalco.c` |

Paths are relative to `src/melee/ft/kinds`. Both stock fighters share the Fox
Reflector functions with different attributes and animations. Hitbox timing
stays in their copied subactions.

Forty-five fighter fields become owned parameters; sixteen govern up-B. Twelve describe side-B startup,
recovery, gravity delays, deceleration and landing/freefall settings. Three describe blaster angle,
speed and landing lag. Article factories and signals name the package-owned
declarations directly; sounds, radio speaker, CPU capture and held-item policies
are additional source-owned defaults. The fourteen Reflector fields are: release lag, turn frames,
gravity delay, momentum divisor, fall acceleration and nine reflector fields.
The ownership flag imports from one byte into an editable bool. Move timers
and effect choice are ordinary rollback-tracked `state` variables.

The turn script preserves the stock degree-to-radian conversion and reads
rotation Y when setting rotation Y. The original getter was misleadingly named
`ftPartGetRotZ` despite reading Y; it and the script now use the correct name.
The reflection wind uses the
mapped hip position without a forward offset. These are source behaviors,
not arbitrary adjustments to make a recording pass.

## Independent assets

`fox_clone` and `falco_clone` retain their own fighter DAT, complete 327-entry
action bank, costume, skeleton mapping and 35 special-motion entries.
Neutral-costume joint counts were measured from the supplied disc graphs:
73 for Fox and 67 for Falco. Pending inherited article handlers use each
custom owner's DAT instead of a stock global asset registration. This does
not count as authored article behavior.

The Workshop offers the shared template for these bases. The client build
copies it beside other templates. Source comparison drivers import parameters
and compile real asset projects; the obsolete raw-template DLL target was
removed because it skipped that required import step.

## Recorded stock comparisons

The 2026-09-28 batch compared seven paths for each fighter with rendering
and identical seeds/controller recordings. Each recording has 1,200 samples.
Required-motion checks prevent a missed input from passing.

| Path | Required branch |
| --- | --- |
| Ground hold/release | Start, loop and end |
| Ground turn | Turn and return to loop/end |
| Ground jump cancel | Loop into common jump |
| Aerial hold and landing | Air start/loop into ground loop/end |
| Aerial turn | Air turn |
| Reflect Dr. Mario pill | Reflect-hit motion and item kind 49 ownership P2 to P1 |
| Battlefield platform drop | Ground loop into air loop, landing and end |

All fourteen final comparisons match action, fighter pose, hitbox/hurtbox,
item, damage and RNG traces. The initial common platform recording never
reached a platform for Fox; required-motion validation rejected it. His final
recording jumps left onto a side platform. Falco reaches the center platform.
The rejected input is not counted as coverage.

The [manual suite](../tests/native/fox_falco_reflector_parity.py) builds fresh
owned-parameter packages and reproduces these paths. Timing is documented in
the [recordings README](../tests/native/parity_inputs/README.md).
Local evidence: `.local/fox-falco-reflector-20260928/` and
`.local/fox-platform-drop-parity.log`.

Four Mario/Dr. Mario Cape regressions also match after replacing the former
Cape-specific descriptor shortcut with the generic reflector command: Mario
aerial Cape, damage interruption, pill reflection and Dr. Mario aerial Cape.
The compiled Cape fixture checks all nine descriptor values, including a true
ownership flag. The parameter import check compares all fourteen real Fox
defaults against source DAT bytes. Nineteen focused checks and the complete
configured Release build pass.

## Blaster audit and generic article groundwork

The neutral-special audit includes `ftfoxspecialn.c`, `ftFox/inlines.h`,
`itfoxlaser.c`, `itfoxblaster.c`, and shared ray helpers in `it/kinds/inlines.h`.
Repeating a shot requires a newly pressed B while the command-0 window is
open; holding B alone is not the same input condition. Ground and air entries
also differ in retained velocity. Command resets must precede animation
startup, and loop transitions run the stock attack bookkeeping.

The gun is an independent attached article with opening/closing and recoil
curves, visibility changes, short muzzle-effect records, and motion-dependent
removal. The laser has stretching, orientation, swept stage contact,
shield-bounce and reflector branches. Stock neutral and throw functions
also read the fighter's gun pointer, so replacing only the gun callback
would leave incompatible ownership bookkeeping. Those paths need to be
translated together; they remain pending.

A 2026-09-28 groundwork batch replaces anonymous float article scratch slots
with [named, typed local variables](scripting/article.md#local-variables),
zeroed per spawn in fixed item storage. [Article joint transforms](scripting/item.md#article-model-transforms)
provide model translation, Euler rotation and per-axis scaling. Both additions
are general authoring tools; no Fox/Falco move property or union alias was
introduced. Existing Mario, Dr. Mario and Luigi projectile scripts use a
named `bounced` flag directly. Separate holdable items retain their current
float scratch API.

The callback fixture verifies exact integer arithmetic above 2^24, bool and
float storage, attached-owner access and generated transform calls. Parser
checks reject undeclared/duplicate state, obsolete article scratch syntax,
and malformed joint paths. The real HSD model check exercises all nine
transform components, nested/sibling selection, matrix invalidation and
missing/quaternion-joint handling. This is API groundwork, not acceptance of
scripted neutral-special or gun/laser parity.

The configured native build and 21 focused compiler/module/content/model/semantic
checks pass. Five stock/script recordings also match after rebuilding fresh
Mario, Dr. Mario and Luigi packages: Mario Fireball contact and reflection,
Dr. Mario pill contact, and Luigi Fireball free flight and shield contact.
Each has 520 controller samples, 1,038 fighter states and 519 RNG states.
Luigi's Fireball is kind 105, and its shield recording removes the item on its
creation frame. The initial item-presence expectations were rejected; corrected
checks use kind 105 for free flight and the real shield-hit motion for contact.
No gameplay adjustment was made to satisfy those checks. Initial and final
validation records remain in `.local/article-state-regressions-20260928/`.

Rook was rebuilt for the new exact-client host/item layout, verified with 285
own-rig clips and replaced through `install-project`. Her source was unchanged
and no additional collection entry was created. The API docs and parent
project plan record this as a groundwork checkpoint; neutral/gun/laser decisions
are the next roster batch.

## Laser callback checkpoint (2026-09-28)

[Laser source](../examples/fox_falco_laser.fms) |
[Explicit launch](scripting/fighter.md#articleslaunch_projectile) |
[Launch initialization](scripting/article.md#launch-initialization) |
[Trajectory operations](scripting/item.md#trajectory-operations)

The separate script owns both laser kinds and both article states: velocity,
facing, root rotation, beam stretching, expiry, previous-position tracking,
stage-hit decisions, shield bounce and reflection. Shared engine calls remain
for stage intersection, contact-normal vector reflection and model setters;
no stock laser callback is called by the script.

Launch metadata lives in each item's fixed native article storage. A general
`on launched` callback runs after configured state, lifetime, appearance and
initial-update setup. Explicit launch data supplies a world angle, speed,
state and bone-local muzzle offset. The laser starts its stage sweep at the
owner's collision origin, matching stock, rather than at the muzzle. Its
initial velocity and common initial update are disabled by declaration.
Ordinary angle normalization preserves the original per-assignment float
rounding; very large/nonfinite inputs receive bounded, defined handling.

`native_fighter_laser_parity` loads the compiled example and calls the actual
original C animation, physics, shield-bounce, reflection and removal callbacks
alongside it. Across both kinds/states, eight angles, five speeds and three
length limits, 480 trajectories produce **35,520 bit-exact comparisons** of
velocity, facing, lifetime, model rotation/scale, and relevant local state.
The test also exercises 960 synthetic stage sweeps and the explicit-launch
compiler output. The sweeps check supplied origin, mutable contact position,
one-frame expiry and restoration; they are not real-stage gameplay recordings.

The native build and 22 focused compiler/module/content/model/semantic checks
pass. Compiler rejection cases cover repeated/invalid launch flags, a launch
callback without a projectile declaration, writes to read-only launch data,
invalid sweep arguments and malformed fighter launch arguments. Existing
projectile defaults remain on and existing launch call sites pass no override.
Evidence: `.local/laser-build.log`, `.local/laser-final-tests.log`,
`.local/laser-callback-parity.log`.

Rook was rebuilt with her source unchanged, verified with all 285 own-rig
clips and replaced through `install-project`; no duplicate collection entry.

This is callback and generic-launch groundwork. The laser script is deliberately
not combined with inherited blaster firing, whose C still writes the old item
union. Translate gun animation/ownership, neutral phases, throw shots and
cleanup together, then compare the complete pipeline in recorded gameplay.
No neutral-special, whole-launch, real-stage collision, or whole-fighter parity
acceptance is claimed by this unit checkpoint.

## Gun model checkpoint â€” 2026-09-28

[The partial gun script](../examples/fox_falco_blaster.fms) owns opening,
closing, recoil, and visibility for both Fox and Falco. Ordinary named state
stores its counters. Read-only tables store model values. Generic signals carry
requests from a fighter to its own attached article; no stock gun handler or
private fighter/item union is exposed to the script. Shared state declarations
store the original callback pointers directly, with distinct animation indices.

`native_fighter_blaster_model_parity` compares the compiled script with the
actual original gun physics, opening, closing, and recoil callbacks. It checks
**68,040 bit-exact samples** across both variants, all nine common states,
five opening indices, three direction values, fourteen recoil indices, and
eighteen ticks with opening/closing/recoil requests. Joint translation, scale,
rotation, matrix flags, and the opening/recoil counters match. Separate checks
cover missing model branches, recursive visibility, variant sound IDs and
trigger counts, table bounds, signal values, and chained callback sharing.
Sound requests are intercepted rather than played: this does not claim audio
playback parity. Signal argument/dispatch tests call generated callbacks and a
mock sender; live attached-item ownership still needs an integrated recording.

The signal host resolves creation numbers against live owned items and checks
both authored logic and attachment identity before dispatch. It rejects unknown
signals and nonfinite values. Module loading checks signal table size, null
callbacks, and duplicate identifiers. The compiler also rejects empty/duplicate
signal declarations, signal values outside their callback, malformed tables,
invalid sharing, and writes to joint existence.

Native build and 23 focused checks pass. Evidence:
`.local/blaster-model-build.log`, `.local/blaster-model-tests.log`, and
`.local/blaster-model-parity.log`. This is a model callback checkpoint, not the
completed blaster move. Ownership, accessory effects, spawning, neutral phases,
throw firing, and cleanup must switch together before the gun/laser overrides
are enabled in playable clones. The partial examples remain separate.

## Gun lifecycle checkpoint â€” 2026-09-28

The [gun script](../examples/fox_falco_blaster.fms) now also owns pickup-state
selection, owner-driven state changes, attachment cleanup, incapacitated-owner
scaling, interaction notifications, and the shot accessory callback. It uses
ordinary fighter/article variables, common motion constants, and generic
attachment configuration. The constructor can independently select its origin,
Z plane, initial collision, and special held-item registration. Cape's existing
defaults remain the same. Item kind can be an expression such as an owned
parameter rather than a required source literal.

The expanded test covers **70,576 owner/state cases** across both variants:
401 owner motions, nine gun states, attached/detached owner, present/missing
fighter handle, action-release flag, and all sixteen command combinations for
pickup. Expectations use the actual stock owner-motion/removal queries. State
transitions, animation advance, removal, effects and sounds are mocked. The
existing **68,040 bit-exact model samples** still pass. Additional checks cover
state 9 cleanup, state 10 scale following, creation settings, expression-selected
item kind, dynamic state bounds, and callback clearing on state transitions.

The stock muzzle-effect path passes the same point twice to its angle routine;
the result is zero. The script preserves this explicitly. Its unused six-entry
effect bookkeeping is not copied: source inspection finds no external readers.
The generic directional-effect operation resolves a semantic fighter bone and
passes a temporary world position/angle using the engine's existing effect
signature. The test intercepts the request to verify its arguments; live effect
rendering and constructor behavior still need the integrated recording.

One stock removal branch calls effect destruction after freeing the item. The
script omits that unsafe trailing call because the removal routine already
destroys the effects. Other cleanup paths clear owner references and effects
before removal. Immediate scripted removal ends the generated callback so it
cannot continue accessing the freed item.

Native build and all 23 focused checks pass. Evidence:
`.local/blaster-lifecycle-build.log`, `.local/blaster-lifecycle-tests.log`,
`.local/blaster-lifecycle-parity.log`. Rook rebuilt with unchanged source;
all 285 own-rig clips verified, package
`a6cf0b4c6b77701e297543389f961883c518c7bf7135e7acb7368011a02954f9`
replaced the prior installation without an extra collection entry.

The gun/laser examples are **still not enabled in playable WIP clones**.
Neutral phases, firing, throw shots and the fighter's cleanup callbacks must
switch together, then receive recorded whole-path comparisons. These unit
checks are not whole-launch, effect-rendering, stage-collision, neutral-special,
or fully-cloneable-fighter acceptance.

## Integrated blaster checkpoint - 2026-09-28

The shared template now includes both gun variants and both laser variants.
Neutral-B start/loop/end, repeated-shot input gating, accessory firing,
damage/death cleanup and back/up/down throw shots are authored in scripts.
There are no calls to stock Fox/Falco blaster or laser handlers in these paths.
The five blaster attributes are copied into owned FMP values by the normal
parameter importer. Gun handles and repeated-shot state are named variables.

Added generic operations: `on throw_animation`, `cue.article.consume()`,
`items.notify_pickup(flag)`, bone-local segment direction, and the fighter's
instance/base scale. The shared throw callbacks invoke authored decisions
before retaining their existing victim/release/end logic. See the
[fighter reference](scripting/fighter.md#throw-animation-events) and
[geometry reference](scripting/combat_visuals.md#bone-directions-and-instance-scale).

Fourteen recordings compare stock and freshly built owned-parameter packages:
for both Fox and Falco, grounded single/repeated/right/left shots, airborne
shots ending through landing, and back/up/down throws. Each comparison covers
520 controller samples, 1,038 fighter states and 519 RNG states, plus all
observed actions, item poses/fields and damage events. The first integration
batch caught a missing laser damage-dealt removal callback; it is now authored
and included in the focused laser test.

Some throw lasers hit and are removed within their creation tick, before the
end-of-frame item trace can list them. Those cases require the gun and throw
motion, and compare the full victim/damage sequence exactly. The expectation
was corrected after inspecting the stock recordings; gameplay was unchanged.
The reproducible runner is `tests/native/fox_falco_blaster_parity.py`; inputs
are checked in under `tests/native/parity_inputs/fighter-blaster-*`.

Native build and all 23 focused checks pass. Evidence:
`.local/blaster-integration-build.log`, `.local/blaster-integration-tests.log`,
`.local/blaster-integration-parity-fixed.log`, and
`.local/blaster-integration-regressions.log`. Validated recording results:
`.local/blaster-parity-20260928-151551/validated-summary.json`.
Rook source is unchanged; all 285 own-rig clips verified, installed package
`8a0fe0b371fc15a355d5fb8b49d0e4936e585683d947acd39676149b094d5545`
replaces the prior installation without duplication.

These recordings validate the integrated paths and real-stage contact;
they do not establish every effect's visual parity, audio parity, interruption
branch, costume, left-facing throw or fully cloneable fighter acceptance.

## Side-dash checkpoint â€” 2026-09-28

The shared template now owns all six side-B phases and both illusion/phantasm
article definitions. Startup momentum/gravity, animation-driven dash velocity,
B-button shortening, ground/air/ledge transitions, recovery friction/gravity,
helpless landing, article lifetime and damage-hitlag decisions are scripted.
The four-entry world-position/root-rotation history is ordinary fighter state;
no Fox/Falco private fighter or item union is accessed by the authored paths.
Stock source: `ftFox/ftfoxspecials.c` and `it/kinds/itfoxillusion.c`.

Twelve additional fighter fields import into owned FMP parameters (31 total).
The stock `x44` attribute is a recovery gravity delay despite its misleading
`FALL_ACCEL` source name; `x48` supplies fall acceleration, not the common
terminal velocity. The article's two lifetime values still use its copied,
package-owned FMP fields after the article parameter checkpoint below;
raw offsets are retained only in the source translation draft.

General engine additions support world-position article creation, a post-setup
`on created` notification, common rendering, three independent model instances,
root-X rotation reads, animation-driven ground/air movement, facing-aware
ledge checks, owner scaling and manual lifetime. All are reusable by other
fighters. Each item has fixed model-pointer storage; extra graphs use tracked
HSD pools and are automatically released. Script entry can use `entry explicit`
to own command resets and callback installation without injected setup.

The initial comparison caught an early accessory assignment in compiler entry
setup. Stock schedules the trail effect only when the dash begins; explicit
entry and the scripted dash transition now preserve that order and RNG.
Parity traces include the secondary trail model's transforms and visuals, with
stock's private extra graph normalized against authored model slots.

Fourteen comparisons match stock, covering both fighters, grounded/airborne
travel in each facing, an actual shortened dash and contact after run braking.
Each recording contains 520 controller samples, 1,038 fighter states and 519
RNG states, with exact action/item/pose/visual/damage comparisons. The shortened
grounded dash enters recovery at sample 143 instead of 144. Contact records
7% to Zelda at sample 173 (Fox) / 171 (Falco). Contact inputs were corrected to
wait until RunBrake ended; the earlier attempted inputs missed or never started
the special and were not counted as damage coverage.

Runner: `tests/native/fox_falco_side_parity.py`. Inputs are checked in under
`tests/native/parity_inputs/fighter-side-*`. Evidence:
`.local/side-parity-20260928-154452/validated-summary.json`,
`.local/side-parity-final.log`, `.local/side-contact-final3.log`,
`.local/side-release-build2.log`, and `.local/side-final-tests.log`.
The build and all 23 focused checks pass. All fourteen existing integrated
blaster/throw recordings still match after the creation/model API changes
(`.local/side-blaster-regressions.log`). Rook's unchanged script was rebuilt,
all 285 own-rig clips verified, and installed package
`edb825962f8ef16d094295d43a48c2de356b3157db0954a45ff32f3a7a8b0139`
replaced the prior installation without duplication. These are native stock-versus-script
comparisons; visual hashes do not establish every rendered effect, audio,
wall/ledge/interrupt branch, arbitrary scale or costume. Neither fighter is
fully script-owned yet.

## Owned article parameter checkpoint â€” 2026-09-28

The importer now copies laser lifetime/maximum length and trail/fade lifetime
into qualified FMP fields. Each owned Fox/Falco draft has 35 parameters: 31
fighter values plus four article values. The shared template yields only that
fighter's gun, laser and trail declarations. Compiled callbacks and projectile
initialization use the same match-immutable parameter table, without source DAT
tuning reads. Editing values does not require rebuilding behavior.

The article scope is generic, accepts typed float/int/bool fields, and has no
fighter-specific state aliases. The stock article-to-asset catalog is shared
by import and runtime asset lookup rather than maintained in duplicate tables.
Workshop conversion covers every move file and appends newly imported values
without discarding existing owned tuning. Common engine item attributes and
asset graphs still reside in the owned DAT; this checkpoint migrates the
explicitly declared behavioral tuning, not every engine asset property.

Validation: all 14 side-dash recordings and all 14 blaster/throw recordings
still match stock. The native build and all 23 focused checks pass, including
real default import for Mario, Dr. Mario, Luigi, Fox and Falco, rejection of
an article offset beyond its source block, and typed article callbacks/launch
setup with a null DAT. The loaded module test changes owned float/int/bool
values without rebuilding. Generated owned source contains no article DAT
attribute reads. These assertions concern declared behavioral tuning, not
common item engine attributes or independently authored art.

Evidence: `.local/article-parameters-final-build.log`,
`.local/article-parameters-final-tests.log` (22 passed; the expanded import
test's Dr. Mario root-name assertion was corrected and passes in
`.local/article-parameters-import-final.log`),
`.local/side-parity-20260928-160830/summary.json`,
`.local/blaster-parity-20260928-161255/summary.json`, and the corresponding
`article-parameters-{side,blaster}-parity.log` files. Workshop multi-file import
is built but has not received a new interactive UI test. Rook's unchanged
source rebuilt against the current module layout, verified all 285 own-rig
clips, and replaced its previous installed package with
`42de68addcadde6cdef19be28585c30f21ae3e689c20e350303be0e4dfdc9832`
without adding a collection entry (`.local/article-parameters-rook-refresh.log`).

## Up-special checkpoint - 2026-09-28

Firefox/Firebird now owns all seven motions (353-359) in the shared script:
ground/air charge, ground/air travel, ground/air recovery and rebound. The
script makes steering, platform pass-through, collision-angle, phase-timer,
effect, bone-rotation and freefall decisions directly; it does not call a stock
Fox special handler. Sixteen source constants import into editable FMP values,
bringing each package to 51 fields (47 fighter, four article). Mutable timers
and launch angles are named fighter state, not stock move-union aliases.

The translation retains source float operation order and rounded constants,
including the model's full-turn subtraction. Ground travel alone advances the
platform grace counter, matching stock. The generic additions are named-bone
rotation, XY vector angle, a soft-floor skip query with documented side effects,
surface normals/contact flags, vertical-only animation velocity and transient
ground-item pickup exclusion. The compiler accepts 128 KiB sources and 64
helpers; a boundary test checks all 64 and rejects a recursion cycle through
the final helper. There are no gameplay-time allocations for these operations.

All 22 rendered stock/script recordings match exactly under the existing
fighter-state comparator: ground/air steering up/left/right/diagonal, neutral
stick default, solid-floor rebound and Battlefield platform pass-through for
both fighters. Recordings also cover travel leaving a ledge, grounded recovery
and landing during airborne recovery. The suite checks required motion IDs;
neutral or wrong-special recordings cannot count as coverage. Each ordinary
run compares 520 controller samples, 1,038 fighter states and 519 RNG states;
platform cases compare 1,200 samples, 2,398 states and 1,199 RNG states.
Pose, hitbox and hurtbox hashes are included. This is not a rendered-pixel,
audio, every-angle, ceiling/wall, interruption, slope or all-costume proof.

Evidence: `.local/fire-parity-20260928-170918/summary.json` (16/16),
`.local/fire-parity-20260928-171320/summary.json` (6/6),
`.local/fire-parity-corrected.log`, `.local/fire-parity-extra-final.log`,
`.local/fire-dash-final-build.log` and `.local/fire-dash-final-tests.log`
(all 23 focused checks passed). The initial recording attempts were corrected
because their B input selected side-B and a grounded downward direction selects
grounded travel rather than rebound. Rebound coverage now begins airborne.
The [manual suite](../tests/native/fox_falco_fire_parity.py) and inputs are checked
in. Rook's unchanged source rebuilt for the new host layout, verified 285
own-rig clips and replaced the installed package with
`950b877e05382fde8f49f3c20748252a1057284bbd355727e01a10e3420a10a1`
without a duplicate (`.local/fire-dash-rook-refresh.log`).

## Radio and lifecycle checkpoint - 2026-09-28

All six radio motions now use the shared script. Its `wait_input` predicate
owns down-release recognition, the six-player usage check and entry direction.
Named state owns phase/facing; the subaction cue starts the stage sequence.
The `action_status` predicate preserves the original statistics query. The
registered `death1` handler cancels the sequence and clears its registration.
No authored callback calls a stock Fox/Falco special or taunt handler.

New operations are generic: [stage events](scripting/stage.md),
[input history and player slot](scripting/fighter.md#player-and-down-input-history),
action counters, blended Wait entry and death callback registration. Predicate
handlers return booleans; ordinary event handlers cannot return a value. The
compiler rejects tracked death callbacks without a declared handler in both
script formats. Channel names hash at compile time; simulation dispatch does
not allocate or look up strings. Corneria/Venom own their conversation
objects, RNG selection and text/audio playback. The old direct conversation
entry functions were replaced at their call sites by the stage provider.
Both stock and script use this provider plumbing, so comparing those runs is
not independent validation of that plumbing; its decisions were retained from
the original source.

This exposed three native data issues, repaired at their source:

- Conversation SIS tables now import with other stage assets and use a fixed
  root cache. Starting another conversation reuses the native table rather
  than reading an unimported archive address or allocating again.
- Venom's radio overlay is map 1 and needs ten indexed joint/material
  animations. Import now retains that layout rather than a single descriptor.
- Arwing laser spawn data now initializes both current/previous positions
  from the computed muzzle position. The decompiled self-copy left the current
  position indeterminate; native stack bytes changed item traces. The function
  also returns its spawned object instead of falling off a nonvoid function.

The stock action-counter query at ID `0x72` reaches beyond the decompiler's
partial `by_attack_hi` array into the containing statistics object. The read
now uses a bounded byte offset and `memcpy`, retaining the original field
without out-of-bounds array indexing. This does not claim to repair every
statistics layout declaration or writer.

Custom definitions also declare explicit ISO sound-bank dependencies, read
from staged next-scene definitions before fighter activation. These indices
refer to the SSM loader, independently of behavior inheritance. Defaults were
checked against the stock external-character table and SSM filenames. The
initial Falco dependency incorrectly selected Dr. Mario's bank; missing voice
playback ended its Venom dialogue early and changed RNG at sample 970.
Bank metadata is not yet editable script syntax or custom sound packaging.
Audio duration follows device demand, so this suite uses normal pacing and
does not treat accelerated simulation as an audio-duration oracle.

The lifecycle audit covers attribute copy/reload, walljump, normal and extended
item pickup/drop, held-item visibility, knockback layers, death resets,
damage/death cleanup of the owned gun, and radio interruption. Extended item
callbacks now explicitly use the same generic item operations as the normal
ones. Stock Fox/Falco user-data-removal and extra-motion hooks are null, so no
destruction decision was invented. Gun ownership uses a named article handle
instead of the stock union pointer/global article registrations.

Final comparisons cover Fox/Falco on Corneria/Venom, both facing directions:
all eight paths match over 1,200 controller samples, including fighter pose,
hitbox/hurtbox, actions, stage item/damage traces and RNG. Each requires all
three directional radio motions, so missed entry cannot count as a pass.
Evidence: Fox's four passes in `.local/radio-parity-20260928-181339/`, then
Falco's four corrected-bank passes in `.local/radio-parity-20260928-182344/`.
The earlier failed Falco Venom runs remain recorded, not counted as acceptance.
The [manual suite](../tests/native/fox_falco_radio_parity.py) builds fresh
owned-parameter packages; `--fighter` and `--stage` restrict a repeat batch.

The native build and all 24 focused checks pass (`.local/radio-final-build.log`,
`.local/radio-final-tests.log`). Stage tests check SIS reuse without arena growth
and indexed Venom overlay animations. Compiled callback tests exercise predicate
results and death registration/clearing; the replay does not exercise radio
death cancellation. These traces do not compare audio waveforms or rendered
text pixels. Rook rebuilt without source changes, retained 285 own-rig clips,
and replaced its prior installed package without duplication with
`c0927c2ac927507a25d10bebcc62595ebd4b354cf333cbad5a9b5ea31bafb15a`
(`.local/radio-rook-refresh.log`).

## Shared idle and statistics policies - 2026-09-28

Two further shared-engine decisions now use generic script predicates:
[`idle_variants` and `motion_is_angled`](scripting/language.md#predicate-callbacks).
The idle policy runs when a choice table exists and the current idle ends. Its
result allows the ordinary weighted selection or repeats the current idle;
the shared Fox/Falco template preserves their different held-item policies.
The motion classifier receives the queried integer ID as `event.motion`,
independently of the fighter's current action. The template preserves the
stock range and its three excluded tilt states. Neither hook asks the engine
to classify a fighter by donor identity.

Compiled-module tests exercise both idle results and compare the classifier
against the original enum-based predicate across motion IDs 0..400. Invalid
event contexts are rejected. These hooks are source/compiled-callback checks,
not a new held-item gameplay recording or statistical bonus end-screen test.
Adding them reached the old 128 action-handler limit, so both script forms now
share a 512-handler compiler limit. Checks accept the full boundary and reject
one extra callback, including the generated host-check collection beyond 128.
The limit affects compiler storage, not per-tick execution/allocation.

Build and all 24 focused checks pass (`.local/fighter-query-final-build.log`,
`.local/fighter-query-final-tests.log`). Rook rebuilt without source changes
and replaced its prior install with
`24dacb669802160c488de58b93a8ed61e4b4177f27d889d5c13ba99f6cce1fa6`
(`.local/fighter-query-rook-refresh.log`).

## Broader coverage

The closure below accepts the audited versus behavior. Ending/cutscene gun
helpers are called by demo code, outside the
current versus scope. Kirby's copied-special integration belongs to its later
roster translation. Further Reflector
coverage should include airborne projectile contact, rapid successive contacts,
end-phase ground/air transitions, interruption and all costumes. Further
blaster coverage should include left-facing throws, shield/reflect/absorb,
interruption, altered scales and all costumes. Recorded parity establishes
its tested paths, not every possible branch or presentation detail.

## Authored CPU closure candidate - 2026-10-02

The shared template now owns the stock Fox first-grab special choice, both
fighters' ranged candidate and recovery sequences, and their projectile-defense
response. Named parameters control probability, level gates, script indices,
range, stick inputs and delays. The existing gun resource distinguishes Fox's
capture choice from Falco's ordinary capture behavior; it does not query engine
fighter identity. The misleading stock `ftCo_CpuFireBlaster` helper actually
queues down-B, and the authored `cpu_reflect_response` preserves that command
sequence and conditional RNG draw.

The new generic [`cpu_projectile_defense`](scripting/cpu_policies.md#on-cpu_projectile_defense)
hook is placed after shared motion/category exits and ground-item priority.
It preserves stock fallback when absent. A declared false result continues
common defense without repeating a donor decision. Both defense dispatch sites
and the existing capture-policy continuation revalidate source generation
before cached fighter access.

Baseline17 full-template generated-source comparisons passed for Fox and
Falco: 146,145 stock comparisons per fighter (292,290 total), plus 12
source-generation continuation checks and six edited-parameter checks per
fighter (292,326 checks total). The unchanged original capture and defense
callers are extracted from frozen baseline16, retaining their full common
tails. Baseline17's actual callers invoke the generated callbacks. The fixture
compares RNG state/draw count, grab-attempt counter, command bytes, common
motion/item priority and common throw/pummel continuation. Ranged comparisons
exercise all queue counts 0..8, target/facing/terrain conditions; recovery
compares the original command sequence. Removed and recycled source generations
stop before common continuation. Separate parameter checks prove authored
gates, command values, delay, range and recovery edits take effect.

Evidence is `.local/fox-falco-cpu-source17/{fox-direct,falco-direct}` with source
hashes, build logs and results, and `acceptance.json` in their parent directory.
Reproduce after normal project import and script-build, using the matching
snapshot rather than raw `script-c` on an import template:

```powershell
python tests/native/fox_falco_cpu_stock_compare.py --snapshot .local/baseline17 --reference .local/baseline16/sdk --project .local/fox-falco-cpu-source17/fox --out-dir .local/fox-falco-cpu-source17/fox-direct --family fox
python tests/native/fox_falco_cpu_stock_compare.py --snapshot .local/baseline17 --reference .local/baseline16/sdk --project .local/fox-falco-cpu-source17/falco --out-dir .local/fox-falco-cpu-source17/falco-direct --family falco
```

The source fixture controls terrain and grabbing-query boundaries; it does not
claim those geometry algorithms or rendered gameplay were newly validated.
Queue overflow is excluded, with ample real command-buffer capacity. The
compiler CTest covers both handler-generation routes and rejects writable or
out-of-context category use. The template remains inherited until complete
closure review and coordinated drawn gameplay acceptance. Earlier gameplay
evidence above is preserved; it does not establish this CPU candidate's
runtime behavior.

## Owned rigs, Fire travel effect and drawn CPU acceptance - 2026-10-02

Both base definitions now contain all 52 valid semantic bone bindings from
retail `PlCo.dat`. The forward and inverse maps match exactly; all four stock
costume graphs per fighter agree with 73 Fox and 67 Falco joints, with no
omissions. The stock Reflector attachment at raw part 5 resolves to the left-leg
joint, rather than the hip; that original attachment is preserved. The Fire
travel effect attaches to Hip. Reproduce the asset census with
[`fox_falco_rig_stock_compare.py`](../tests/native/fox_falco_rig_stock_compare.py).

The named `FireTravel` effect owns the original travel recipe's rotation
decision through generic `creator.facing`, named creator state and effect-joint
channels. It never calls the stock updater or reads the original fighter union.
The actual full-template generated DLLs match the unchanged original callback
over 18,432 finite-angle/facing comparisons; 16 production registry/lifetime
rejection cases also pass. Evidence: `.local/fox-falco-model-source17/acceptance.json`.
The fixture uses synthetic HSD graphs and explicit allocation/lookup boundaries;
it does not exercise actual recipe constructors, EF scheduling, rendered pixels
or effect-specific rollback restoration.

Baseline17 passes twelve hidden, muted, accelerated drawn stock/script pairs:
ground and aerial Fire travel in both directions, plus CPU levels 4 and 9 for
each fighter. All required Fire motion states are observed. CPU matches produce
damage and gun/laser article states, rather than comparing two idle fighters.
The batch compares 11,360 controller samples and 22,720 fighter states, including
pose, hitbox/hurtbox, actions, item/damage traces and RNG. Evidence:
`.local/fox-falco-runtime17/results.json`. These packages still use inherited
behavior identity; this is candidate acceptance before removing fallback.

The full native build and six focused checks pass, including the new CPU parser
contract, effect compiler/generated modules, rig assets, owned-parameter import
and callback lifetime. The parameter roundtrip also checks all 17 new CPU
defaults bit-exactly, alongside the 47 imported fighter fields. Logs:
`.local/fox-falco-owned-build.log` and
`.local/fox-falco-focused-tests-corrected.log`. The preceding failed roundtrip
used the obsolete assumption that the template had only imported fields;
that failure remains in `.local/fox-falco-focused-tests.log`.

Rook rebuilt from unchanged authored source for the new Definition layout,
replacing its installed package once with
`0a83eaf60c1c33e6253e4f8274db71e378ad90ac04660ed00e8c57da31dfa286`.
Its 285 own-rig clips remain intact. This was an SDK refresh, not another Rook
move change (`.local/rook-cpu-policy-sdk-refresh.json`).

## Independent versus behavior accepted - 2026-10-02

The final census found authored replacements for all 140 non-null motion
callbacks across each fighter's 35 extended states (341..375), and every
non-null stock lifecycle callback. The two empty stock blaster physics slots
are deliberately null replacements. Radio, throw, idle, motion classification
and CPU decisions route through their authored hooks. Applicable stock effect
recipes have no further fighter-state updater beyond the replaced FireTravel.
Remaining auxiliary/removal callback tables are null; demo constructor helpers
and Kirby copies remain outside the agreed versus scope.

After setting `fighter: behavior independent`, real package builds and loader
admission pass. All twelve drawn pairs pass again with the shared comparator
requiring the custom behavior identity, normalizing only that identity against
stock. Fire states are reached and both CPU levels produce active input and
positive opponent damage. Final evidence:
`.local/fox-falco-independent-runtime17-corrected/acceptance.json` and
`results.json` (11,360 samples, 22,720 fighter states).
The first independent preparation rejected a misplaced declaration before
launch; its output is preserved separately and is not counted.

The final independent template also passes the unchanged-original effect
fixture (18,432 comparisons plus 16 rejection cases) and both CPU fixtures
(292,326 checks) in `.local/fox-falco-independent-model-source17` and
`.local/fox-falco-independent-cpu-source17`. Four focused independence/guard/
parameter tests and the final full client build pass:
`.local/fox-falco-independent-focused-tests.log` and
`.local/fox-falco-independent-build.log`.
No newly discovered branch or altered physics was needed for promotion.

[`fox_falco_closure_parity.py`](../tests/native/fox_falco_closure_parity.py)
reproduces the final drawings with fresh packages and identical recordings.
It requires independent behavior and rejects idle CPU matches without damage.
Its `--prepare-only` mode performs no game launch. Wider coverage listed above
and exact pixels/effect-specific rollback remain separate concerns.
