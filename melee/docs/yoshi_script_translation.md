# Yoshi script translation

Yoshi's generated module is independently authored with `source_kind =
Ft_Kind_None`. [The scripts](../examples/yoshi_wip.fms) own all four specials,
three articles, the five custom shield states, roll/spotdodge presentation,
aerial jump, material initialization, load/reload/death, ordinary
CaptureWait/CatchPull policies, throw-victim setup, CPU decisions and both
ordinary and extended item callbacks. Immutable asset metadata is owned
explicitly. The baseline6 independent module passed all **49 gameplay cases /
34,520 controller samples**, hidden, muted, accelerated and drawn.

The earlier checkpoints below describe their accepted scope at the time.
They retained stock callback inheritance until the baseline6 closure described
at the end of this ledger. Independent ownership does not establish every
unrecorded interaction or imply that shared engine services are scripted.

## Copied assets and editable parameters

The stock 1.02 ISO contains 314 actions, 14 demonstration entries and a
70-joint green costume. The 28 custom motion states occupy 341–368. Fighter
root `ftDataYoshi` and costume roots `PlyYoshi5K_Share_joint` and
`PlyYoshi5K_Share_matanim_joint` are imported independently. Audio bank 28
matches `yoshi.ssm`. Article slots 0, 1 and 2 contain EggThrow, Star and EggLay.
Each article now has an authored policy in the template.

The source template's stock attribute declarations are import inputs.
`import-parameters` replaces them with named, owned fighter/article values.
Egg charge, release latch and creation handle are ordinary fighter variables;
the article's launch angle, speed and spin argument are its own variables.
Creation/release formulas do not read Yoshi's native motion or item unions.
The saved hand joint (31) and TransN origin joint (1) are editable parameters.

Both native Egg Roll entries initially select motion 360, despite the legacy
name of motion 356 suggesting a grounded start. The copied-base metadata
records the entry actually selected by the stock handlers.

## Egg Throw

The source of truth is
[`ftyoshispecialhi.c`](../src/melee/ft/kinds/ftYoshi/ftyoshispecialhi.c) and
[`ityoshieggthrow.c`](../src/melee/it/kinds/ityoshieggthrow.c).

| Path | Authored behavior |
| --- | --- |
| Ground/air entry | Clear release latch, charge, article handle, all throw cues and command 0; enter 364/365; install accessory and start animation. |
| Animation | Increment integer charge each tick while B is held. Finish normally on ground or enter Fall in air. |
| Creation cue | Sample joint 31, create the authored article with fighter-origin collision history, attach to that joint and track damage/death cleanup. |
| Release cue | Consume command 0 only while the held article exists. Clamp stick magnitude, apply the angular deadzone and sign, calculate world angle, signal release, forget the handle and clear damage/death callbacks. |
| Ground/air transition | Preserve the current frame with the stock flag mask, restore cleanup/accessory callbacks and clamp air drift on leaving ground. |
| Air collision before release | Use the ordinary shared landing query. |
| Air collision after release | Check ground/ledge in both directions, then attempt common wall jump and ledge catch in stock order. |
| Article release | Select flight animation, initialize lifetime, release attachment while preserving an ordinary held item, clear throw-speed damage, disable automatic lifetime, apply release offsets and calculate common spin speed with the stock sign rules. |
| Flight | Decrement lifetime after testing for expiry, use common gravity and the alternate shared surface response. |
| Explosion | Enter state 2 with flags 18, hide model, request owner rumble, disable physics, initialize expiry, stop motion, overwrite explosion lifetime, queue both effects and play sound 280088. |
| Contacts | Damage, clank and shield contact explode only outside state 2; shield bounce and reflection use common item behavior. |

## Ground Pound and stars

The source of truth is
[`ftyoshispeciallw.c`](../src/melee/ft/kinds/ftYoshi/ftyoshispeciallw.c) and
[`ityoshistar.c`](../src/melee/it/kinds/ityoshistar.c).

| Path | Authored behavior |
| --- | --- |
| Ground entry | Convert to air before entering 366. Start animation, clear commands 0/1 and only throw cue 0, zero self velocity and vertical animation delta, block ground pickup. |
| Air entry | Enter 368, start animation and perform the same reset. |
| Startup animation end | Enter 368 at frame 20 with the original preservation mask. |
| Dive animation end | Set command 1; keep the dive motion active until collision. |
| Startup/dive physics | Copy facing-scaled TransN Z into X velocity and TransN Y into Y velocity. Ground startup clamps negative Y to zero; dive clamps to the tuned dive speed or forces that speed when command 1 is set. |
| Ground contact | Enter landing 367 only when command 0 permits it and velocity is nonpositive; otherwise convert back to air. |
| Ledge contact | Retain the stock second ledge-entry call after the successful common ledge check. |
| Landing | Finish animation through normal grounded completion, use common friction and fall when leaving the floor. |
| Landing accessory | Spawn stars on each side of TransN using world offsets and opposite facing, then clear the accessory callback. |
| Stars | Initialize tuned horizontal speed and common vertical drop speed, disable automatic lifetime, end when the model animation finishes, apply gravity and facing-scaled acceleration, and sweep the floor without entering an item landing state. |
| Star contacts | Damage, clank, shield hit and absorption emit effect 1041/sound 115, clear contact hitlag and remove the star. Reflection and shield bounce use common item behavior. |

Both articles use `item.gravity(false)` to apply only the common gravity
step. The ordinary no-argument operation also updates common item spin,
which would overwrite Egg Throw's release spin on each physics tick.

At the initial Egg Throw/Pound checkpoint, ordinary item
pickup/drop/visibility and knockback presentation were authored with the
original flags and part-animation selections. Custom shield material setup
and damage callbacks were completed in the later common-action batch.

## Egg Roll

[`ftyoshispecials.c`](../src/melee/ft/kinds/ftYoshi/ftyoshispecials.c)
defines the full eight-state policy. Egg Roll creates no separate article;
its live fighter hitbox supplies damage. All tuning is imported into named
owned parameters, and the roll's lifetime, visibility phase, squash phase,
hit-group timer, rotation, travel speed and pending turn use ordinary variables.

| States/path | Authored behavior |
| --- | --- |
| Entry | Both entries face the input and select 360. Ground entry then converts to air. Clear commands 3–0, start animation, save model scale, apply the smash-age speed multiplier and reset that timer. |
| 356/360 startup | Consume the stock visibility sequence. Ground startup is stationary; air startup applies tuned gravity. Collision preserves the frame when switching variants. Both advance to 361 with the pose paused at frame zero. |
| 361 initial air travel | Apply tuned stick acceleration/cap and gravity, initial rotation and lifetime. The first floor bounce chooses grounded travel or airborne travel from vertical rebound speed. Preserve the stock subsequent overwrite of landing/aim speed with the initial bounce speed. |
| 357/362 travel | Apply squash, alternate live hit groups and recalculate integer damage from speed. Rotate the egg, decrement lifetime and enter the relevant end state on expiry. Ground movement preserves the source slope and material-friction formula; air movement uses bounded stick acceleration and gravity. |
| 358 turn | Clear hitboxes on accepted opposite input, retain pending facing and ground speed, apply reversal acceleration and source tilt/yaw formulas, emit floor dust, and resume grounded travel after crossing the reverse-speed threshold. Preserve attack-stat callback ordering. |
| 359/363 end | Clear hitboxes, apply the respective velocity multipliers, restore scale/rotation/held-item presentation and run the remaining visibility/squash sequence. Ground end uses unscaled common friction; air end selects Fall or special Fall with tuned landing lag. |
| Walls/edges/floor | Use the same custom collision box, turn edge-speed threshold, contact masks, effects, quake, rumble, rebound velocities and motion preservation flags. Air bounces preserve the differing initial/travel horizontal-speed policies. |
| Hit/reflection/interruption | Hits shorten lifetime and reduce stored speed with stock minimums. Reflection reverses the owned movement/turn variables. Damage emits the original egg-break effect/sound and restores presentation; death restores presentation. |
| Material/visibility | The exact fourteen-entry visibility sequence selects the live model group and resets both egg material groups to frame zero. Held and attached-item visibility follows the original ordering. Break effects use `co_attrs.xBC.size`, exposed as `model.effect_scale`. |

Egg Roll's source imports, compiled module build and eight gameplay comparisons
passed. [`yoshi_egg_roll_parity.py`](../tests/native/yoshi_egg_roll_parity.py)
requires the intended startup, travel, turn and end states and checks entry
facing. All eight cases matched 600 controller samples each in
`.local/yoshi-egg-roll-final`, including right/left ground and air travel,
ground turn, ground stop, air stop and turn into ground stop. Three cases also
recorded matching damage events. The ground-stop recording presses B before
the fighter leaves the floor, so its required grounded end state is exercised.

The final compiled module also passed all 5,620 original-C comparisons in
`.local/yoshi-roll-final-direct-c.log`: the accepted 3,400 Egg Throw/Pound/Star
checks plus 2,200 Egg Roll movement samples and twenty grouped
material/visibility comparisons. State 356's movement is covered directly;
these recordings start in stock entry state 360 and do not exercise state 356
through gameplay. Direct smash-age initialization and scaled-effect arguments
still need full engine setup beyond this fixture. Wall impact, reflection and
damage interruption also need dedicated recordings.

## Verification boundary

The copied costume and fighter metadata were inspected directly from the
local 1.02 ISO. Source comparison established the callback ordering and
parameter mappings above. This initial special-move checkpoint had scoped
runtime evidence; the later baseline6 checkpoint accepts independent ownership
with the retained coverage limits below.

[`yoshi_script_stock_physics_test.c`](../tests/native/yoshi_script_stock_physics_test.c)
compares the full compiled template's Ground Pound, Star and Egg gravity
callbacks against the original C functions over 3,360 exact velocity samples,
plus 40 Star reflection comparisons including model rotation.
It varies imported owned tuning, facing, command latches, starting velocities
and animation deltas. The compiled full template passed all 3,400 exact
comparisons. Compile-time assertions also protect the serialized DAT view's
angle offset, star offset and total size.

[`yoshi_special_parity.py`](../tests/native/yoshi_special_parity.py) builds an
owned-parameter package and prepares ten stock-versus-clone recordings:
tap/charged/aimed ground and air Egg Throw, left-facing throws, and ground/air
Ground Pound with stars. Each comparison requires the relevant move and
article kind to occur, preventing an idle-only recording from passing.
It uses hidden accelerated matches with drawing enabled and writes logs and
per-case results under the explicitly supplied output directory.

The first matrix exposed a native stock DAT-layout defect: `ftYs_DatAttrs`
declared its opaque four-byte slots as pointers, shifting the later offsets
by twelve bytes on the native 64-bit build. Those slots are now serialized
`u32` words. Comparisons use that corrected stock baseline. The recordings
delay facing input until after the ready countdown and select Egg Throw
with pure upward input before applying aim; both logs must also show the
intended facing when the move starts.

The seven Egg Throw cases each matched all 480 samples in
`.local/yoshi-parity-release-fix`. The forward-aimed case also recorded one
matching damage event against Zelda. All three Ground Pound cases matched
the final template revision in `.local/yoshi-parity-final-pound`. These hidden
accelerated drawn runs compare fighter
state, item state, pose, collision state, action transitions and RNG state.

This matrix does not establish damage interruption while holding an
egg, ledge catch, wall jump, reflection, shield bounce, absorption, all stage
surface responses or complete clone acceptance. Those need dedicated
recordings and direct callback comparisons.

## Shared capture enclosure preparation

The common captured-victim lifecycle now snapshots its launch, timer, mash,
shrink, escape and damage settings into fixed victim-owned state. Stock Yoshi
and Kirby release directly into that lifecycle; later ticks no longer call
stock Yoshi or Kirby tuning getters. Both common motion families use the same
callbacks, with Kirby's different model restoration and damage arithmetic
preserved explicitly. Semantic tracing includes the snapshot and holder
reference.

`tests/native/capture_enclosure_test.c` compiles the corrected original C
references alongside the common implementation. Its recording engine stubs
compare callback order and arguments, timer/velocity state, shrink scales,
full effect descriptor, capsule endpoints and arbitrary captured-item fields.
The optimized standalone fixture passed **19,324 exact comparisons** in
`.local/capture-enclosure-result.log`. It covers both facings and initial
movement modes, negative/zero/fractional shrink durations, mash restart,
damage escape, and invincibility durations 0/1/7. Yoshi's original boolean
getter still converts a raw 7 to 1; Kirby retains 7 invincibility frames.

Egg Lay's ten holder states and three-state article are now in the template.
The accepted candidate preserves the preceding special scripts and uses
generic capture operations. Hidden, muted, accelerated, drawn comparisons
cover both initial movement modes and facings, whiff landing, catch landing,
timer expiry, mash escape, damage while enclosed and third-party holder
interruption. The direct fixture uses controlled engine stubs alongside these
runtime comparisons.

The item branch is restricted by stock capture collision eligibility
(`xDD0_flag.b4`). Ordinary common items do not set this flag. The explicit
offline `--fixture-capturable-item` proxy sets it and initializes the stock
release bookkeeping sentinel `xDD4_itemVar.zako.idx = -1`, meaning the item
belongs to no adventure enemy generator. These settings only apply to the
requested replay fixture; normal item spawning retains its original fields.
Proxy comparisons establish captured-item attachment, release, cleanup and
Egg Lay article behavior. They do not establish adventure-enemy asset,
generator or enemy callback parity. The three proxy runtime cases matched
all required captured-item and Egg Lay article branches.

The holder candidate groups the source states as follows:

| Ground / air motion | Authored policy |
| --- | --- |
| 346 / 351 | Start tongue animation, arm fighter and item capture, finish on whiff, and re-arm capture after landing or leaving the floor. |
| 347 / 352 | Hold a fighter, retain the catch frame, suppress conflicting grabs and motion, and enter release on command 0. |
| 348 / 353 | Attach the captured item to the mapped TransN2 bone, preserve damage/death cleanup callbacks, and enter release on command 0. |
| 349 / 354 | Hide the victim and change body status on command 1; snapshot the enclosure settings, model and facing before throw damage and link release on command 0. |
| 350 / 355 | Release the captured item on command 1; create an unowned Egg Lay article with its velocity already set on command 0. |

All holder motions use the source ground friction or aerial gravity/friction.
The eight caught/release states preserve the source transition flags and
animation frame when ground/air collision switches them. The grabbed-victim
event uses the generic captured lifecycle with opposite facing, forced air,
holder tracking and no animation donor. The article owns its damage-to-time
factor and manual lifetime. Its aerial and grounded states count down before
physics; expiry plays the source cue, hides the model, changes collision mode
and waits another 40 frames before destruction.

The reusable acceptance harness is `tests/native/yoshi_egg_lay_parity.py`.
It snapshots the matching host, content tool, compiler, SDK, UI and assets;
generates C and compiles using that frozen SDK; checks that all ten holder
motions and the article are authored; and keeps installed packages in an
isolated content store. Its 18-case fighter/regression matrix is
`tests/native/parity_inputs/yoshi-egg-lay-matrix.json`; the item proxy matrix
is `tests/native/parity_inputs/yoshi-egg-lay-items.json`. Every future game
launch explicitly uses hidden windows, muted output, accelerated simulation
and drawing. The complete muted batch passed **21 cases and 12,870 controller
samples** against the frozen host in `.local/yoshi-egg-lay-runtime5`: 12
fighter capture/enclosure paths, three item proxy paths and six preceding
special regressions. Both article release cases reached states 1, 0 and 2,
then removed the article after its 40-frame expiry phase. Mario's hit removed
the captured item at sample 154 and prevented article creation. All compared
fighter/item state, pose, visual, collision, action, damage and RNG traces
matched. This batch does not establish custom shield, the remaining fighter
lifecycle, every article surface/interaction response or adventure-enemy
integration.

## Accepted common-action and lifecycle batch

`examples/yoshi_wip.fms` now includes the common player policies. Its verified
package remains isolated in `.local/yoshi-common-runtime3`, with complete
matching binaries, SDK headers, compiler, UI/assets and SHA-256 manifest.
Subsequent shared state-array SDK edits are outside this accepted snapshot.
The declarative template re-imports to exactly the same executable source
and **91 owned parameter values** as the accepted candidate, excluding comments.

`tests/native/yoshi_common_stock_compare.py` loads the module against that
frozen SDK and passes **2,730 original-C comparisons**. These cover six shield
entries and ordered engine calls, animation/interrupt branches, roll completion
and cue consumption, live versus saved model selectors, 1–13-frame aerial turns,
material animation initialization and health frames, production CaptureWait
setup/initial entry with reciprocal links and four victim model scales,
catch-frame remap edges, throw-victim body status, and the actual common
JumpAerial animation/FallAerial entry bodies including their shared-word write.
The fixture tests the reflector installation/window policy and preserves the
stock empty reflect-hit callback.

The gameplay acceptance passes **43 cases and 22,920 controller samples**, all
hidden, muted, accelerated and drawn. Every compared fighter/item state,
pose, visual, collision capsule, action, damage event and RNG state matches.
The **22 common-action cases / 10,050 samples** exercise regular and reflect
shield startup/hold/release, both rolls, spotdodge, held/released shield damage,
shield exhaustion through grounded stun, regular shield entry after damage,
death/rebirth, forward/reverse jumps through fall with continued drift, standing
catch/pummel/escape, dash catch/up throw, back and forward throws, Fox/Zelda
victims, airborne victim capture, powershield melee and laser-contact timing
edges. The airborne victim is constrained onto ground immediately after capture;
this case does not claim a surviving CapturePulledHi trace. Laser contact cases
do not establish visible projectile ownership reflection.

The other **21 cases / 12,870 samples** rerun the accepted Egg Lay ground/air
whiffs, catch/mash/damage/facing, landing, fighter interruption, ordinary captured
item proxy/release/interruption and expiry, plus Egg Throw, Ground Pound and
Egg Roll regressions. The ordinary item proxy remains capture-eligible through
an explicit offline fixture and does not establish adventure-enemy asset,
callback or generator parity.

Two actual native contract corrections were required before this acceptance:

* The importer now preserves Yoshi's serialized byte array at
  `0x12C..0x137`, after scalar-word decoding. The actual stock values are
  `46,44,42,40,38,36,34,33,32,31,29,29`; the old importer reversed each four-byte
  group. The template imports these as named byte parameters, and CatchPull
  now agrees with the corrected stock baseline. The actual-ISO regression
  compares all 12 bytes and their adjacent scalar.
* On JumpAerial completion, common FallAerial entry writes selector23 into the
  same union word as the original Yoshi turn counter. The original callback
  then decrements it to22 and rotates the root once, even for a forward jump.
  The script reads and writes the generic `motion.fall_animation` selector
  only in valid Fall/FallAerial context, preserving that common-policy side
  effect. The old frozen candidate was rejected at comparison2,713; all18
  added completion boundaries pass against the corrected candidate.

Evidence is recorded in `.local/yoshi-common-runtime3/acceptance.json`,
`common-fixture-results.log`, the common matrix/edge results, and both accepted
special regression result files. Source coverage inputs are versioned in
`tests/native/parity_inputs/yoshi-common-matrix.json` and
`yoshi-common-edges.json`. Coverage assertions were corrected against actual
stock branches and preserved trace pairs were re-compared before acceptance.

## Accepted CPU decisions

The template owns Egg Roll travel steering in both planner callers,
grounded/air ranged candidates, angle/distance special selection, Ground Pound
threat geometry, recovery commands and the two size-dependent recovery/jump
decisions. Four named CPU parameters bring the imported total to **95 owned
parameters**. This historical baseline4 checkpoint explicitly retained
`source_kind = Ft_Kind_Yoshi`; baseline6 subsequently removed gameplay
inheritance.

The compiled candidate passes **9,297 exact original-C comparisons**. The
fixture extracts unchanged original functions and compares actual frozen
common caller bodies with the scripted policy: ordered queries and commands,
candidate lists, target/behavior changes and the caller-owned travel counter.
It includes distinct floor islands with identical bounds, missing islands,
strict travel-edge and distance limits, adjacent float values at the original
double angle boundaries, bounded candidate-list saturation, threat geometry,
giant recovery gate ordering and tiny/full/forced jump command streams.

The matching frozen baseline passes **49 gameplay cases / 34,520 controller
samples**, hidden, muted, accelerated and drawn. Six CPU cases contribute
11,600 samples: level9 and level4 travel on Final Destination, level9 travel
on Battlefield, a shielded target, offstage recovery and Ground Pound threat
integration. Required Egg Roll travel/turn states and Pound/article states
are observed; offstage recovery records 126 CPU recovery calls. The other 43
cases rerun all accepted common and special comparisons, contributing 22,920
samples. Fighter/item states, actions, poses, visuals, collision capsules,
damage events and RNG states match exactly.

**Giant/tiny and `x90` outcomes are direct original-C/common-caller coverage
only.** The recorded gameplay batch does not establish those outcomes.
The laser-contact and ordinary capturable-item proxy limits described above
also remain.

Evidence is in `.local/yoshi-cpu-runtime4/acceptance.json`,
`cpu-fixture-results.log` and its five matrix result files. Versioned inputs
are `tests/native/parity_inputs/yoshi-cpu-matrix.json`; source comparisons are
`tests/native/yoshi_cpu_stock_compare.py` and
`yoshi_cpu_stock_policy_test.c`. The frozen host/SDK manifest remains unchanged.

The definition now owns its exact 39 semantic bone mappings, three article
bindings, enclosure graph, raw enclosure-frame bytes and effect-bank ID9.
Its 70-part rig has no omission records. The asset ownership checkpoint is
`9cc5223ce`; that historical checkpoint retained stock callback inheritance
while closure was verified.

## Extended item callback closure

Ordinary and extended pickup/drop hooks now use the same authored shared item
operations as stock Yoshi. The candidate passed **224 exact original-C/common
caller comparisons** against the frozen baseline5 SDK: negative/zero/positive
flags, heavy items and all six hold kinds, held/auxiliary/secondary slots,
hammer cleanup and ordered item accounting/release. The fixture executes the
real shared pickup/drop inline helpers rather than replacing their behavior
with success markers.

These are source-level callback comparisons; this batch launches no games and
does not add to the 49 accepted gameplay cases above. Evidence is
`.local/yoshi-owned-source5/acceptance.json` and `item-fixture-results.log`.
The versioned comparison is `tests/native/yoshi_item_callback_stock_compare.py`
with `yoshi_item_callback_stock_test.c`. All baseline5 digests remained unchanged.

## Accepted independent ownership: baseline6

The template declares `fighter: behavior independent`. The compiler clears
inherited behavior pointers, emits `source_kind = Ft_Kind_None` for the
generated module and checks that every non-NULL inherited motion callback is
replaced. The raw Yoshi catalog base retains stock fallback for plain,
unmodified copied clones; this does not supply behavior to the independent
module.

The source audit accounts for all **19 non-NULL stock function-table callbacks**
and all **97 non-NULL animation/input/physics/collision callbacks across 28
motion states (341�368)**. They are authored. Motion 367 retains its original
NULL input slot; the camera callback remains the common engine service
`ftCamera_UpdateCameraBox`. All 26 Yoshi-specific common/CPU predicates route
through the module's authored hooks before stock fallback. Generated C contains
no `ftYs_*` calls.

The definition owns 39 semantic bone mappings for the 70-part rig, no omission
records, Egg Throw/Star/Egg Lay asset bindings in slots 0/1/2, the enclosure
joint graph in slot 3, serialized attribute bytes `0x12C..0x137`, effect bank 9,
audio bank 28 and the four immutable demo roots:
`ftDemoResultMotionFileYoshi`, `ftDemoIntroMotionFileYoshi`,
`ftDemoEndingMotionFileYoshi` and `ftDemoViWaitMotionFileYoshi`. Stock Yoshi's
optional demo lookup/creation callbacks remain NULL. `animation_source_kind =
Ft_Kind_Yoshi` is retained solely to identify imported clip tags for rig
remapping; it does not select gameplay policy. Package verification reports
270 own-rig clips and two borrowed clips.

The matching frozen baseline6 (`e904cea53`) compiled and verified package
`5e2709f4f1f1127471413fcbfbb0b156952f5da2bf5d6d876924b6dacc091a04` with **95 owned
parameters**. Its module SHA-256 is
`c6010102cbafeaf5e2d21de3ad5ace2b5d213b6ef2c063376f6f63bb12fa05c5`.
All **49 stock-versus-independent comparisons / 34,520 samples** passed:
six CPU cases / 11,600 samples, 22 common-action cases / 10,050 samples and
21 special/capture/item-proxy regressions / 12,870 samples. These reuse the
accepted branch assertions described above. No game process remained when
the batch released its runtime slot.

The comparator explicitly validates stock P1 kind/behavior `14/14` and clone
P1 custom/custom identity before normalizing only those two action identity
tokens. All compared fighter state, positions, action timing, pose, hitbox,
hurtbox, visibility, item state/relations/pose/visuals, damage, body contact and
RNG fields remain exact. No compared digest was removed or masked. Clearing
unused stock Yoshi union state changes the full replay hash domain; a stock
fighter and an authored fighter also store variables differently, so those
whole-state hashes are not a stock/script variable projection.

Behavioral parity is accepted when exact RNG or undefined stock behavior
cannot be reproduced cleanly, provided any deliberate gameplay difference is
identified explicitly. No deliberate Yoshi gameplay or RNG change was needed
for these 49 cases. The identity/ownership changes above are intentional; the
console-layout corrections documented in earlier checkpoints preserve the
stock data contract. Acceptance does not extend exactness beyond the recorded
and original-C comparisons.

Retained coverage limits are:

* Giant/tiny recovery and caller-owned `x90` outcomes are covered by the
  original-C/common-caller fixture (**9,297 total CPU comparisons**) only,
  not observed gameplay outcomes.
* Grounded startup state 356, wall impacts, reflection and interruption branches
  not explicitly reached by these recordings retain their earlier source or
  limited runtime evidence. The laser-contact cases do not establish projectile
  ownership reflection.
* Captured-item fixtures use an eligible ordinary-item proxy. They do not
  establish adventure-enemy assets, callbacks or generator behavior.
* Gameplay traces do not emit a direct stock/script variable projection. The
  original-C fixtures retain their variable comparisons.

Evidence is preserved in
`.local/yoshi-independent-runtime6/independence6-acceptance.json`,
`independence49-results.json`, per-case input/stock/clone/comparison logs,
`preflight.json` and `runtime-evidence.json`. All 3,176 frozen input hashes
still match `freeze.json`. The reusable scratch runner and preparation notes
are in `.local/yoshi-independent6-plan/run_yoshi_suite.py` and `README.md`.

### Historical full-state self-replay failure and corrected check

An extra fixture-free clone recording/replay was attempted to verify full
semantic state, including script storage. The original attempt was **not
accepted** because of a separate replay-startup defect, not a Yoshi gameplay
acceptance gate. The first
CPU-only launch was rejected by the existing human-controller requirement.
The corrected human-P1/CPU-Yoshi-P2 recording entered independent gameplay,
finished its timed match at sample 3,837 and continued recording outside VS
until the bounded 90-second timeout. Its flushed FMR contains 26,750 samples.

Full verification replay exited before gameplay at sample 0:
`RNG divergence: recorded 2271b5e5, got 00000001`. The valid recording header's
initial seed is `b46cab88`; match creation legitimately advanced it to
`2271b5e5` before recorded sample 0, whose log already contains created fighters.
Playback installed its PAD callback before the boot PAD read, so the verifier
consumed recorded sample 0 before initializing the replay seed. The failed
evidence was retained without claiming a completed fix. No RNG or semantic
digest was bypassed to make this check pass. All attempted recordings/logs are
retained under `clone-full-state-replay`; the corrected FMR SHA-256 is
`9438c8e1b73881e10a4bd7e5bc3d58fdf0ace67260b2f35aa57c40a6e7626809`.

Root subsequently corrected replay setup in `gmmain.c`: `nativeReplay_Open`
now runs after `db_GetGameLaunchButtonState` and `HSD_InitComponent`, immediately
before `nativeReplay_Seed`. Boot PAD reads no longer consume the first canonical
replay sample. The corrected full-state check **passed all 4,000 samples**,
consumed the entire recording and exited 0 without RNG or semantic-state
divergence; it includes actual timed-match completion at sample 3,837. Evidence
is `.local/replay-bootstrap-check/{verify.log,result.json,match.fmr}`; the
recording SHA-256 is
`72ecaea8dade30e951daa3a838858eee6afe2afc2ff0398f617026f250de0cd9`.
This accepts the corrected 4,000-sample self-replay only. The full 26,750-sample
recording was not replayed successfully, and all historical failure logs remain.
