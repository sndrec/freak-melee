# Ness script translation

## Current direct interpreter status

The October 10 component declaration batch prepares Ness's real material-only
Yo-yo animation through the shared DAT importer. The complete current template
audit visits 418 functions with metadata and body coverage complete. The
[resource-animation batch](native_script_resource_animation_checkpoint.md)
removes the remaining body errors. The
[smash-entry batch](native_script_ness_smash_hook_checkpoint.md) integrates
`up_smash_enter` and `down_smash_enter`. Canonical preparation now accepts all
418 functions, and a private source package stages, publishes and retires
without a native fighter definition. The preparation census is 25/26; Samus
remains unprepared. The [actual scene/grounded comparison](native_source_ness_scene_checkpoint.md)
now verifies construction and eight stock/source scenarios: idle, all four
grounded specials and three smashes. All emitted Fighter/Item fields and action
timelines match with no script faults. Airborne/interrupted/damage/charged,
match-end and rollback acceptance remain open. See
[the component checkpoint](native_script_animation_components_checkpoint.md).

The generated-C results below are historical source/ownership evidence, not
acceptance of the current interpreter backend.

The authored Ness template supplies all four specials, all eleven articles,
three smashes, ordered resource lifecycle, common jump/item/presentation and
Ness CPU policies. It declares `behavior independent`: the generated definition
uses `source_kind=None`, retaining copied rig/archive metadata rather than stock
gameplay callbacks. The raw catalog base still supports untouched stock clones.

The full composition passes 1,135,039 bounded source checks on SDK47. Independent
SDK49 DLL/package validation passes 856 ownership checks: 31 Definition callbacks,
142 original non-NULL fighter phases and 118 article callbacks belong to the
module. Whole-fighter drawn acceptance remains pending; these console gates do
not prove every stage, pixels/audio or rollback/network execution. Earlier
Fire/Magnet gameplay evidence and all component source receipts below remain
historical evidence; their partial/pending labels describe those earlier batches.

## Source and copied assets

The live clone registry previously had no Ness definition or template.
The USA v1.02 ISO was inspected independently: `PlNs.dat` has 326 action
entries, 14 demos and four costumes; `PlNsAJ.dat` imports 276 clips.
The default costume imports 63 joints. Roots are `ftDataNess`,
`PlyNess5K_Share_joint` and `PlyNess5K_Share_matanim_joint`.
Action and demo counts were derived from their raw DAT table spans before
passing them to the actual importer. Sound bank 21 is `ness.ssm` in
`lbaudio_ax.static.h` and the stock sound mapping.

Stock `ftNs_Init_OnLoad` registers eleven article slots: Fire 0, Flame 1,
Flash 2, Thunder ball 3, Thunder trails 4ÃƒÂ¢Ã¢â€šÂ¬Ã¢â‚¬Å“7, Flash explosion 8, bat 9 and
yo-yo 10. The first batch imports seven fighter floats and five article floats
into named project parameters. The editable direct spawn-joint index is 42,
preserving stock's raw `parts[FtPart_R2ndNa]` access.

Metadata and importer evidence are preserved in `.local/ness_metadata`.

## Accepted PK Fire batch

The reference sources are `ftnessspecials.c`, `itnesspkfire.c` and
`itnesspkfirepillar.c`. Fighter entry clears the projectile cue and command
zero, changes to motion 356/357, starts animation and installs the accessory
callback. That callback consumes the cue before resolving the raw spawn
joint, adding the facing-relative offset and selecting the ground/air angle
and speed. Projectile velocity is supplied before construction; the requested
position is `SpawnItem.prev_pos`, while `SpawnItem.pos` records the owner's
item center. The shared constructor initializes the item's current position
from `prev_pos`. Ground loss enters Fall; air landing uses special landing
with the owned lag parameter.

The projectile initializes state/lifetime, then sets root rotation. It expires
after decrementing life and spawns an unlinked flame on damage or clank.
Reflection and shield bounce use shared item routines. The flame inherits
current damage ownership and the immutable package author; its secondary
parent is the projectile, as in stock.

The flame sets model scale before its pre-decrement lifetime test. Its physics
and collision callbacks preserve stock's unconditional `GA_Air` assignment,
including preservation of the separate landing flag. Its common landing
callback zeroes velocity. Incoming accumulated damage reduces life and clamps
expired life to one. No authored callback delegates to a stock Ness handler.
The launch cue is `cue.article.consume()` (bit 0); `cue.throw.consume()`
consumes a different bit.

## Comparison fixtures

[`ness_pkfire_stock_compare.py`](../tests/native/ness_pkfire_stock_compare.py)
compiles the full, unchanged stock fighter/projectile/flame source files with
[`ness_pkfire_stock_test.c`](../tests/native/ness_pkfire_stock_test.c) against
a frozen SDK. Its reference tuning is read independently from raw `PlNs.dat`.
The comparison covers both facings, ground/air launches, allocation failure,
cue consumption, ordered engine calls, spawn current/previous positions and
pre-construction velocity, lifetime/rotation, flame damage ownership and
secondary parent, full fractional expiry range, integer accumulated damage
with a deliberately different incoming-damage float, landing flag preservation,
stage responses, shield bounce, reflection, absorption and reference cleanup.
Shared engine boundaries are recorded by fixture stubs; gameplay comparisons
exercise the actual common routines. The frozen baseline3 candidate passes
6,446 original-C checks with zero failures, including an explicit comparison
of the authored spawn-joint parameter with `FtPart_R2ndNa`. That comparison
rejected an initial off-by-one ordinal before gameplay. Candidate evidence is
in `.local/ness_fire_preflight8` and `.local/ness_fire_direct8`; the runtime and
SDK hashes are in `.local/ness_fire_snapshot.json`.

[`ness_pkfire_parity.py`](../tests/native/ness_pkfire_parity.py) prepares ten
hidden, muted, accelerated, drawn cases. It checks all twelve imported floats
bit-for-bit before packaging. Contact cases require the actual flame and target
damage, guard setoff, reflected owner transfer or absorption event. Landing
requires the special landing motion. All ten cases pass on the frozen
baseline3 host/SDK (core checkpoint `c34a4774f`), with the same candidate
package and independently checked twelve imported floats:

| Cases | Required observed behavior |
| --- | --- |
| Ground right/left, air right/left | Fighter state 356/357 and projectile 66 |
| Ground/air damage | Projectile 66, flame 67 and target damage |
| Ground shield | Target GuardSetOff 181 |
| Ground reflect | Continuously traced projectile transfers owner P1 to P2 |
| Ground absorb | Stock Ness target absorbs projectile 66 |
| Air landing | Projectile 66 and LandingFallSpecial 43 |

These ten accepted cases contain 7,000 controller samples, 13,980 fighter
states, 445 item states, 17 damage events and 6,990 RNG states. Both clients
exit zero. Fighter geometry, item core/relationships/pose/visual digests and
RNG traces match exactly; no sound-waveform comparison is claimed.
The first low-air damage/landing fixtures landed before the projectile cue,
so they were rejected as coverage. Raising their fixture height from 10 to
45 entered the required branches; neither failed fixture is counted.

The accepted package is
`8a83586effaa18f9b61f22e6169b4d91fd9f1725edfeda51f7bab6fa9aea906a`.
`.local/ness_fire_acceptance.json` contains package/source hashes, per-case
trace counts and rejected-fixture evidence. Accepted logs are under
`.local/ness_fire_initial`, `.local/ness_fire_remaining` and
`.local/ness_fire_retry1`. The saved package template matches the owned
source byte-for-byte.

The partial default template was wired into the shared examples copy list
and workshop's Ness template selection at checkpoint `14f03cb46`. Its
presence does not broaden accepted coverage.

## Accepted PSI Magnet batch

The unchanged reference is `ftnessspeciallw.c`. Twelve additional raw tuning
words become named parameters, including the actual integer gravity delay and
absorption joint. The absorption volume uses stock's direct joint index 1;
the attached effect uses the verified raw `FtPart_L1stNb` ordinal 23.
Five ordinary fighter variables own release lag/latch, gravity delay,
turn frames and the sound timer.

| Phase | Ground motion | Air motion |
| --- | --- | --- |
| Start | 367 | 372 |
| Hold | 368 | 373 |
| Hit | 369 | 374 |
| End | 370 | 375 |
| Turn | 371 | 376 |

Entry preserves the exact counter initialization and aerial momentum division.
Start latches release, installs the effect/hitlag callbacks and enters Hold.
Hold and Hit preserve release timing and periodic sound ordering, including
sound processing after entering End. Hit refreshes the effect only when
returning to Hold. The Turn callbacks preserve the stock Z-rotation read and
Y-rotation write, rather than correcting that unusual axis combination.
All ten IASA callbacks remain empty, as in stock.

Air physics delays gravity, then applies owned acceleration with the common
terminal speed and quick air deceleration. Hold/Hit/Turn maintain absorption
position updates. Ground/air collision transitions preserve the exact flags,
current animation frame and air-drift clamping order; only Hold/Hit recreate
the absorber. End destroys effects before the common Wait/Fall transition.

The authored absorption callback truncates the damage-times-multiplier formula
before calling the [generic healer](scripting/absorber.md). It sets facing
from the absorption direction and preserves the Hit-phase early-frame test
before changing motion and recreating absorption. No script calls a stock
Ness handler.

[`ness_magnet_stock_compare.py`](../tests/native/ness_magnet_stock_compare.py)
links the complete unchanged stock source and the actual common
`nativeFighterDamage_Heal` implementation with the compiled module. Its
[`fixture`](../tests/native/ness_magnet_stock_test.c) compares ordered common
operations, all ten animation/IASA/physics/collision callbacks, counters,
velocity, facing, damage, exact hitlag callback pointers and turn rotation.
Fractional damage, zero healing, repeated Hit frames and over-healing exercise
the original formula and integer player-report conversions. The frozen
baseline4 candidate passes 21,489 checks with zero failures; PK Fire also
passes its 6,446 source checks against this same module. Evidence is in
`.local/ness_magnet_direct6` and `.local/ness_fire_baseline4_direct3`.

[`ness_magnet_parity.py`](../tests/native/ness_magnet_parity.py) requires the
intended motions/events in eleven hidden, muted, accelerated, drawn cases:

| Cases | Required observed behavior |
| --- | --- |
| Ground short/hold, both facings | Start 367, Hold 368, End 370 |
| Air short/hold | Start 372, Hold 373, End 375 |
| Air Hold landing | 373 to 368, then End 370 |
| Air End landing | 375 to 370 |
| Ground/air absorption | Projectile 66 absorption and Hit 369/374 |
| Air healing | Absorption, Hit 374 and observed damage reduction |

All eleven cases match exact fighter geometry, article core/relationships/
pose/visual digests and RNG traces, with both clients exiting zero. They
contain 7,700 controller samples, 15,378 fighter states, 22 item states,
three absorption events, three damage changes and 7,689 RNG states. The
healing fixture observes common damage from 0% to 1% at sample 237, then
projectile absorption and healing to zero at sample 274.
The prior ten PK Fire cases also pass against this exact candidate.
No sound-waveform comparison is claimed.

Three first-pass fixtures matched actual traces but missed their requested
branches: long air hold landed before air End, low air Start landed in Hold,
and the airborne attacker landed before firing. Corrected heights enter air
End and actual absorption. A second Start-landing attempt still entered Hold;
it remains rejected, and the accepted landing case exercises End instead.
These four coverage rejections are retained and excluded from accepted totals.
The Start collision transition is covered directly by original C, not claimed
as runtime coverage. Stock has no caller entering the two Turn states and
their IASA callbacks do nothing; their authored callbacks are source-compared,
not advertised as input-reachable gameplay.

The accepted baseline4 package is
`a8195866ffe4e1623b496c631a117f4852c81983fb8b72c1fcd51a62c4221e11`.
All 24 imported raw words match independently checked DAT bytes. The source
matches the saved template in `.local/ness_magnet_preflight3` byte-for-byte.
`.local/ness_magnet_acceptance.json` records hashes, counts and rejected
fixtures. Logs are under `.local/ness_magnet_initial`,
`.local/ness_magnet_remaining`, `.local/ness_magnet_retry1`,
`.local/ness_magnet_retry2`, `.local/ness_magnet_heal1` and
`.local/ness_fire_magnet_regression`. Frozen baseline4 source/runtime hashes
are in `.local/baseline4_snapshot.json`; the shared core checkpoint is
`0bd48e5cc`.

## PK Flash source acceptance, baseline22

The inherited candidate now authors all eight ground/air PK Flash phases
348ÃƒÂ¯Ã‚Â¿Ã‚Â½355, the Flash article's three phases and the explosion's one phase.
The copied base owns all 51 valid semantic bone bindings from the v1.02
`PlCo.dat` map and 63 raw runtime parts for Nr/Ye/Bu/Gr, with no costume
omissions. Flash/explosion assets bind slots 2/8. These facts are recorded
in `.local/ness-closure-next-batch/rigs.json`; the spawn parameter deliberately
uses raw ordinal 24, matching stock `parts[FtPart_L2ndNa]`, rather than the
semantic L2ndNa binding at part 13.

The [source driver](../tests/native/ness_flash_stock_compare.py) links unchanged
baseline21 `ftnessspecialn.c`, `itnesspkflash.c` and `itnesspkflashexplode.c`
against the actual imported candidate compiled with baseline22 headers.
Its [fixture](../tests/native/ness_flash_stock_test.c) passes **11,006 checks,
zero failures**. It compares ordered engine calls and resulting motion,
counters, commands, facing, velocity, constructor parent/position/velocity
fields, model scale/rotation, lifetime and
hitbox state. Coverage includes all eight animation/IASA/physics/collision
phases, entries in both facings, constructor success/failure, charge/release,
foreign damage ownership, reflection, four directional stage contacts,
release child creation, explosion charge/damage/radius and every presentation
threshold. Explosion physics clears Y/Z and preserves X, as in the original.

Named imported parameters own six fighter words and sixteen article words;
all 22 imported words match independently decoded DAT bits.
Additional authored defaults expose the input deadzone, angle conversion,
spawn part/height, collision flags, stage sound, quake/rumble thresholds,
IDs/durations and explosion lifetime. Creator input reads the validated
original creator; rumble intentionally uses the current damage owner.
Same-author signaling detaches a reflected Flash even after its damage owner
changes. The generic signal backend's identity/author/logic/depth validation
is tested separately; the move fixture mocks this engine boundary and does
not claim to prove that resolver.

Flash-only damage/death2 cleanup clears the article's ownership/controller,
the fighter handle and dynamic callbacks, then resets root X. Normal article
destruction clears fighter state only when the current owner is still its
creator. The fixture's `ftNs_Init_OnDamage` boundary restricts other retained
resources to NULL: it compares only Flash teardown plus the unconditional
root reset. Complete yo-yo/Thunder/bat cleanup and their cross-move resource
interactions remain pending; this batch does not replace ordinary load or
whole-fighter lifecycle behavior.

Evidence and input hashes are in `.local/ness-flash-prep/source22`; no PK Flash
gameplay or sound-waveform acceptance is claimed. Earlier PK Fire/Magnet
runtime results remain historical evidence, not a regression claim for this
new module. The candidate remains partial and inherits untranslated behavior.

Reproduce with a fresh project copied from the owned Ness asset project,
then perform normal parameter import and explicit SDK compilation. Frozen
`script-build` is unsuitable because it embeds live SDK include paths.
Use the frozen baseline22 compiler and headers throughout, with MSVC
`vcvars64.bat -vcvars_ver=14.44` loaded for the link libraries:

```powershell
$tool = '.local/baseline22/bin/native_content_tool.exe'
$sdk = '.local/baseline22/sdk'
$clang = '.local/baseline22/compiler/clang.exe'
$project = '.local/ness-flash-prep/project22-final'
& $tool import-parameters $project examples/ness_wip.fms "$project/behavior.fms"
& $tool script-c "$project/behavior.fms" ness_clone "$project/behavior.generated.c"
& $clang -std=gnu11 -O2 -ffp-contract=off -shared -fuse-ld=lld-link `
    -DMELEE_NATIVE=1 -D_CRT_SECURE_NO_WARNINGS -D_DLL -D_MT `
    "-I$sdk/src" "-I$sdk/extern/dolphin/include" `
    "$project/behavior.generated.c" -o "$project/module.dll"
python tests/native/ness_flash_stock_compare.py --sdk-root $sdk `
    --reference-root .local/baseline21/sdk --project $project `
    --stock-dat .local/ness_metadata/PlNs.dat `
    --out-dir .local/ness-flash-prep/source22 --compiler $clang
```

## Unregistered PK Thunder article source batch (baseline24)

The composed [Thunder candidate](../tests/native/ness_thunder_candidate.fms)
adds the ball and four trail kinds to the existing partial script. It is a test
input, not the Workshop default, and retains stock fighter inheritance. The
shipped `ness_wip.fms` stays Flash-only for this addition. The stock SpecialHi
constructor writes its legacy item union after item creation can dispatch an
authored `on created`; enabling these article logics with that constructor would
mix two incompatible state layouts. Nine authored fighter phases and their
ordered damage/death/cleanup must close that boundary before activation.

Ball state owns six full-bit integer trail handles, sixteen XYZ history samples
and sixteen angles, speed, age, next trail index, reflected and controller flags.
Trails own their sample index, blink counter and parent enable flag; backlink0
holds the ball generation. Reads use same-author typed references, and detach
signals do not depend on current damage ownership. Six creation attempts occur
at ages 0,2,4,6,8,10 in kind order 1,1,1,2,3,4; failures consume their attempt.
Five ball attribute imports round-trip the original DAT bits. All thresholds,
conversion constants, motion IDs and bounded structural counts are named owned
parameters. History/trail counts must stay within their declared 16/6 capacities.
The source candidate's private three-component probe exists only for offline
compiler coverage.

The copied base now has nine explicit article asset bindings, including Thunder
slots3..7. These immutable bindings do not install article callbacks. The default
base still inherits the untranslated fighter behavior; no independence is claimed.
The new generic surfaces are `item.collision_group` (read-only integer),
`stage.internal_kind`, `item.release_camera()` and all three
`vectors.cross_normalized_x/y/z(ax,ay,az,bx,by,bz)` queries. The vector Host binds
`lbVector_CrossprodNormalized` itself; generated C creates stack input/result
vectors. No result-extraction wrapper, persistent vector scratch or global
last-result state is involved.

The [comparison driver](../tests/native/ness_thunder_stock_compare.py) links the
unchanged baseline23 ball/trail files with the actual freshly imported candidate
compiled against frozen24. It mechanically extracts three original fighter
hold/cleanup predicates plus the original normalized-cross/angle/mirror cores
and Dolphin cross product, hashing their complete source files. Engine boundaries
are explicit fixture mocks. The [fixture](../tests/native/ness_thunder_stock_test.c)
passes **170,957 checks, zero failures**: all trail indices and both facings,
allocation failures, six-attempt order, inactive/reflected/foreign-owner paths,
steering deadzone/acute/obtuse/parallel/zero vectors, every directional collision
mask and zero sign, all history samples, radius/rotation/blink, lifetime edges,
reflection chain reset, damage detachment, four cleanup predicates, destruction,
shield mirroring and interaction. All three generated cross components match
exactly for the probe vectors. IDs, flags, counters and call order are exact;
other float comparisons permit `2e-6 * max(1,abs(a),abs(b))`, so this is behavioral
source agreement rather than an all-bits claim. The script's binary32 owned
pi/tau and degree constants can round differently from original mixed-precision
expressions; neither side receives compensating gameplay adjustments.

Evidence is `.local/ness-thunder-prep/source24`, including independent five-word
DAT import verification, commands, original extraction and SHA-256 manifest.
Fresh project inputs are `.local/ness-thunder-prep/project24`. The earlier private
candidate23SDK receipt is scoped preflight evidence only, superseded for this
comparison by frozen24. Generic resolver/lifetime behavior has separate engine
contracts; this move fixture does not prove those mocked boundaries. No Thunder
gameplay, rendered geometry, sound or whole-fighter acceptance is claimed.

The frozen24 receipts preserve the historical article-only candidate and generated C.
The current combined fighter/article reproduction is given below; frozen24 cannot
compile the new transition Host fields.

## Remaining scope

Seven fighter smash states still need translation and integration: bat forward
smash and six yo-yo smash phases. The nine PK Thunder phases have the separate
source acceptance below; they remain unactivated in the default template.
Seven article slots remain unactivated: five Thunder slots plus bat and yo-yo.
Bat/yo-yo construction, initialization, solvers, entries and animation decisions
now have separate bounded source comparisons, linked below. Full load/death/damage
composition and default activation are still open. Ordinary item/knockback
policies, common aerial-jump release physics, CPU special steering and
kind-specific shared policies also remain pending.
Kirby copy behavior is deferred.

The copied base retains stock fallback callbacks for untranslated behavior;
that fallback is not counted as a script port. Only source-compared and
runtime-tested paths will be recorded as accepted. Games must be hidden,
muted, accelerated and drawn, using identical recorded inputs and a matching
frozen host/compiler/SDK snapshot.

### Thunder fighter source acceptance (unregistered candidate)

The unregistered `tests/native/ness_thunder_candidate.fms` authors entry,
animation, interrupt, physics and collision for all nine motions 358..366. The
default `ness_wip.fms` remains Flash-only and inherited. No Thunder gameplay or
whole-fighter independence is claimed.

Fresh normal import and generation using frozen27 passed **241,607 combined
source checks, zero failures**, retaining the accepted article comparison and
adding 1,088 fighter scenarios. The matrix includes all 36 phase callbacks,
both facings, animation-end/ground-loss/landing/cliff branches, strict self-contact
and platform bypass, floor/ceiling/left/right/both-wall geometry, wall slide and
rebound, constructor success/failure, both entries, and damage detachment across
the motion boundaries with absent, owned, foreign-owner and reflected chains.
Original fighter and article bodies link unchanged from baseline23. Original
engine math bodies are mechanically extracted; other engine boundaries remain
explicit mocks, whose resolver/lifetime correctness has separate engine tests.

IDs, flags, callbacks, branch choices, counters and ordered engine calls retain
exact checks. Scalar floats retain `2e-6 * max(1,abs(a),abs(b))`. Only shallow-wall
rotation velocity uses `2e-6 * max(1,length(original_velocity))`: editable pi/tau
are binary32, whereas the original compound arithmetic uses double macros.
Maximum observed component difference is **0.00000238418579**. Four cases exceed
the original scalar bound: facing +/-1, right-wall or both-wall mask, normal
sample 3 of 16 (67.5 degrees), without floor contact. This is documented
behavioral agreement, not an all-bits claim. Neither side receives compensating
physics, RNG or angle adjustments.

The candidate owns loop timers, gravity delay, contact state, launch angle,
previous velocity, launch-effect timer and accumulated fall acceleration as
ordinary named fighter fields. Ten additional original fighter attribute words
round-trip into named parameters; all 15 Thunder ball/fighter import words are
checked against the stock DAT. The complete candidate uses 125 owned parameters.
Unused scalar writes and the second collision-position vector have no readers
and are omitted. The previous-velocity vector remains because launch collision
restores it. Original repeated `collPos1.x` initialization becomes one X write;
Y is filled before its first read. No invented Y/Z initialization is added.

The actual-source common-entry fixture separately passes **53 ordering/lifetime
checks**, against unchanged baseline24 FallSpecial/DownBound originals. It tests
normal continuations plus removal and same-address generation reuse. It does not
cover the independent sandbag orientation/presentation body.

Evidence is `.local/ness-thunder-fighter-prep/source27`, `transitions27`,
`project27` and `acceptance27.json`, including source hashes, import words,
commands, generated C, candidate/default hashes and frozen27's receipt. Earlier
private25 and frozen26 runs are preflight evidence, superseded by frozen27.

Reproduce from a fresh asset-only Ness project, copying fighter/costume/animation
DAT, CPU profile and project metadata; do not copy a module or generated C:

```powershell
$tool = '.local/baseline27/bin/native_content_tool.exe'
$sdk = '.local/baseline27/sdk'
$clang = '.local/baseline27/compiler/clang.exe'
$project = '.local/ness-thunder-fighter-prep/project27'
& $tool import-parameters $project tests/native/ness_thunder_candidate.fms "$project/behavior.fms"
& $tool script-c "$project/behavior.fms" ness_clone "$project/behavior.generated.c"
python tests/native/ness_thunder_stock_compare.py --sdk-root $sdk `
    --reference-root .local/baseline23/sdk --project $project `
    --stock-dat "$project/fighter.dat" `
    --out-dir .local/ness-thunder-fighter-prep/source27 --compiler $clang --fighter
python tests/native/ness_thunder_transition_compare.py --sdk-root $sdk `
    --reference-root .local/baseline24/sdk `
    --out-dir .local/ness-thunder-fighter-prep/transitions27 --compiler $clang
```

Generated C compiles directly into the fixture with explicit frozen SDK paths;
no `script-build`, global build or game is invoked.

Activation remains gated. Cleanup is ordered Flash detach, Thunder detach,
effect cancellation, dynamic callback clear and root rotation reset. This is
the original relevant order only while unported yo-yo and bat resources are
absent. Stock OnDeath still clears legacy union handles, so initialization/reset
of authored handles must be owned before activation. Full life reset and all
resource cleanup must close before this candidate replaces the default template.
Common jump and CPU remain additional whole-fighter closure work.

## Bat/YoYo resource lifecycle source checkpoint30

The unregistered [resource closure fixture](../tests/native/ness_resource_closure/README.md)
now composes owned load/death sentinel initialization and ordered four-resource
damage cleanup. Its console runner accepts `--snapshot` and `--output`, verifies
immutable SDK/tool/compiler hashes, extracts unchanged original bodies, generates
the actual callbacks and compiles directly against the snapshot. The tracked
namespace contains source and original-input receipts only; generated outputs
remain in scratch. Shipped Ness and the accepted Thunder candidate are unchanged.

Frozen30 source acceptance passes **67,270 lifecycle/publication checks plus122
Bat construction/init checks**, all with zero failures. The cleanup matrix covers
all16 resource-presence masks, motions357..367, reflected Thunder, pause/x7 flags,
high-bit handles and20-node reverse release. It preserves YoYo -> Flash -> Thunder
-> Bat order, nested resume before clearing the still-published YoYo handle,
outer resume after that clear, owner/bound flags, callbacks, effects and root
reset. Normal Bat overwrites damage/death2 only on successful construction;
YoYo fills only NULL callbacks, publishes/clears its auxiliary reference and
installs hitlag callbacks even on creation failure. Actual Bat constructors and
their descriptor initializer agree with generated mode fields, lifetime then
attachment, and pickup state selection followed by animation advance.

Engine allocation, attachment/state changes, signal/export resolution and model
operations are explicit fixture boundaries. This evidence compares authored
decisions and call ordering; it does not relabel mocked boundaries as newly
accepted actual backend or gameplay coverage. The full composed candidate had a
fresh normal157-parameter import and direct frozen30 C compilation, but remains
inherited/unregistered. No default promotion or game was run for this checkpoint.

YoYo initialization is explicitly pending: original `it_802A24D0` seeds collision
current position from the link before collision initialization propagates the
history fields. Existing resource creation initializes those fields at zero
before later script position writes. A generic optional initial XYZ constructor
input is required; no raw-history workaround was added. Four YoYo solvers,
actual string/tip model graphs, seven smash phases and common presentation hooks
remain open. Presentation must include DeadUpStarIce's0.8 attribute-scale factor
and publish common `item_gobj`, without installing smash damage callbacks. This
checkpoint closes bounded cleanup/publication source evidence, not whole Ness
independence or activation.

```powershell
python tests/native/ness_resource_closure/run.py `
    --snapshot .local/baseline30 `
    --output .local/ness-resource-closure-source30
```

## YoYo initialization source checkpoint32

The separate [initialization fixture](../tests/native/ness_yoyo_initialization/README.md)
closes checkpoint30's collision seed gap using the actual generic resource node
descriptor and six-argument creation form. It remains unregistered; neither the
shipped Ness draft nor the previous accepted Thunder/resource candidates changed.

Frozen32 source acceptance passes **566,018 checks, zero failures**, across504
unchanged-original/generated pairs plus60 defined allocation/model failure
cases. Counts2/3/20/90, motions341..347, both facings, three XYZ positions and three
instance scales cover saved creator/action,345-only reversal, exact linked order,
raw42 attachment, tip radius2*instanceY, all four collision position/history
vectors, defined initial ECB/flags, surface0, source model selection, initial
only-tip draw and material-only remove/add/rate/request/advance ordering. Pickup
and successful retirement are included. The generated path executes the actual
frozen resource backend and joint code; collision initialization and fixed-ECB
functions are extracted unchanged and executed in both paths.

HSD heap/model graphs/animation/render and active Definition/item entry remain
explicit fixture boundaries. This is source decision and initialization evidence,
not real DAT model or pixel acceptance. Named fixture FMP values represent owned
tuning; edited counts are mirrored into the unchanged original attribute input.
Original undefined allocation-failure paths are not run. Authored failures are
checked for deterministic atomic release instead, with that judgment documented.
Four YoYo physics solvers, fighter smash phases, common presentation and gameplay
remain open; this checkpoint does not establish whole Ness independence.

```powershell
python tests/native/ness_yoyo_initialization/run.py `
    --snapshot .local/baseline32 `
    --output .local/ness-yoyo-initialization-source32
```

## YoYo solvers, entries and live owner identity checkpoint39

The [four-physics fixture](../tests/native/ness_yoyo_solvers/README.md)
independently passes 5,075,363 checks across 2,556 original/generated pairs and
six defined lifetime/default-owner cases. Initialization uses the production
resource and joint paths; stage responses and constructed HSD graphs remain
explicit controlled boundaries. Maximum measured numeric difference is zero.

The [entry and Bat animation fixture](../tests/native/ness_yoyo_entries/README.md)
independently passes 1,370,954 checks across 2,604 healthy source pairs and
16 separate owner safety cases on frozen39. This includes the real target-ID
registry for owner motion queries. Missing or removed owners return no motion;
a retained old ID cannot resolve a recycled object. A current raw item-owner
pointer refers to its current live object, without recovering historical identity.
The fixture does not claim complete smash behavior, rendering or gameplay.

These candidates remain separate and unregistered. Remaining smash integration
needs common smash entry dispatch and complete move composition. SDK41 now
exposes the shared charge record, victim-history clearing, exact
hitbox states and a fixed collision-box sweep. Those contracts expose engine
semantics, not Ness-only helpers. Move-local timers, article
references, move decisions and rehit timing remain ordinary authored state;
shared charge fields must still participate in the common engine charge update.
Common jump, CPU, item/presentation and full lifecycle integration are additional
whole-fighter requirements. Existing source acceptance does not count inherited
stock handlers as translated code.

Fresh receipts are `.local/ness-yoyo-solvers34-root/receipt.json` and
`.local/ness-yoyo-entries39-root/receipt.json`. Reproduction commands and precise
controlled boundaries are in the linked fixture documents.

## Shared smash contracts (SDK41)

The [common charge API](scripting/smash_charge.md) reads and writes the same
charge record that the original common update consumes. Fresh source comparisons
pass 480 common-update/input pairs and 120 bounded-write cases across generated
action and callback forms (11,378 checks). The unchanged common functions run
against both independently configured expected storage and generated storage;
rate, color, rumble and sound remain explicit ordered presentation boundaries.
Manual mode retains the original lack of limit clamping and automatic A-release.
This proves storage/update interoperability, not complete Ness smash behavior.

The fixed-box [stage query](scripting/stage.md#terrain-queries) and live
[hitbox victim clearing](scripting/combat_visuals.md#hitboxes) executes through the shared
engine functions. Both generated forms check full-width contact flags, previous
and current positions, box dimensions, and out-of-range indices. The
native build and 45 relevant console tests pass. Fresh receipt:
`.local/smash-contracts41-root/receipt.json`.

SDK42 removes the misleading duplicate `hitbox.reset_detection` operation:
the underlying stock function only enables a capsule. Scripts now use
`hitbox.set_enabled(index, true)`; it accepts guarded integer-expression indices
and a Bool. Generic additions also cover stack-vector rotation, the Bool IASA
flag, held Attack input, Song immunity and all four direction/contact masks.
The native build and 49 related console checks pass. The charge fixture also
passes freshly against SDK42:
`.local/smash-contracts42-root/receipt.json`.

## Delayed yo-yo hitbox cleanup correction

The authored smash candidate intentionally fixes the stock
[yo-yo glitch](https://www.ssbwiki.com/Yo-yo_glitch). In
`ftNs_AttackHi4_YoyoCheckTimedRehit`, the original timer refresh clears capsule
victims and enables capsule 0 without setting the fighter's `x2219_b3` cleanup
marker. `Fighter_ChangeMotionState` only clears attacks on ordinary transitions
when that marker is set. The refreshed capsule can therefore outlive its smash
and persist through later interrupted attacks.

The generic `hitbox.set_enabled(index, true)` operation now maintains that marker
for every valid attack slot. The authored refresh still clears victims and enables
the capsule in the original order. Ordinary exits clean it up; transitions that
explicitly preserve hitboxes retain both the attack and its cleanup intent.
Disabling one slot does not clear the fighter-wide marker. No Ness-specific
scripting operation or stock-handler modification is required. Comments at
`YoyoCheckTimedRehit` explain the cause and intentional correction.

The [composed smash comparison](../tests/native/ness_smash_composition/README.md)
contains the original unlatched branch as a negative control, plus completion,
interruption, landing/ledge cleanup and preserve-hitbox controls. This is scoped
source evidence; it does not claim a rendered reproduction or full motion-engine
coverage. The default independent template includes this authored correction. Existing
saved/compiled Workshop drafts retain their selected source; changing a template
does not overwrite a user's draft or rebuild a previously installed module.

Frozen SDK44 passes a fresh root reproduction with 6,528 original/generated
pairs, 10 lifetime cases and 30 cleanup regressions (214,186 checks). The 180
intentional cleanup-marker differences are explicitly asserted; other move
decisions and compared fields remain checked against the original. Receipt:
`.local/ness-smash-composition44-root/receipt.json`.

## Whole-fighter independent composition

The [composition fixture](../tests/native/ness_fighter_composition/README.md)
assembles accepted component source without editing those historical inputs.
One ResourceDamage helper preserves YoYo -> Flash -> Thunder -> Bat order.
Bat and YoYo creation preserve their supplied positions instead of substituting
the owner's ECB center. YoYo pause/resume callbacks install at construction,
even on allocation failure. The two original NULL Fire IASA slots are authored
no-ops; no nonempty fighter decision is inherited. CPU binary32 endpoint
judgments and the intentional yo-yo cleanup-marker delta remain documented.

The [independent activation gate](../tests/native/ness_fighter_activation/README.md)
uses a fresh normal clone/import, frozen SDK/Clang compilation and native package
validation in an isolated store. No game or package install is part of this gate.
The candidate binds 285 own-rig clips and four borrowed clip dependencies, with
own 51-pair/63-part rig, eleven article assets, sound bank21, effect bank10 and
copied demo root strings. Workshop's existing ness_wip selection and CMake copy
path need no new registration. Source acceptance receipts are
`.local/ness-fighter-composition47-accepted/receipt.json` and
`.local/ness-fighter-activation49-final/receipt.json`.

Root independently reproduced the DLL/package gate on SDK49 and SDK50. The
SDK50 built candidate matches every non-comment statement in the shipped template;
its receipt is `.local/ness-fighter-activation50-root/receipt.json`.
