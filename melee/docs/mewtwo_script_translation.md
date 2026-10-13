# Mewtwo script translation

Mewtwo has an independent copied-asset base and an authored template covering
its character-specific specials, articles, CPU decisions and common policies.
[`mewtwo_wip.fms`](../examples/mewtwo_wip.fms) owns Shadow Ball's ten fighter
phases, Disable's two fighter phases, Teleport's six fighter phases, Confusion's
two fighter phases, and both
combat articles. Their authored
callbacks call shared engine operations directly; none delegates to a stock
Mewtwo special or article handler. All 20 special fighter states are authored.
Forward-throw Shadow Ball creation, CPU charge decisions, root-animation flags,
dynamics policies and ordinary aerial-jump entry are also authored. Their
original-C comparison and all 45 hidden, muted game paths are accepted below,
including the shared worn-item jump sound policy.
Kirby cloning/copy abilities remain deferred.

## Copied assets and owned tuning

The USA v1.02 archives were inspected through `native_content_tool`: Mewtwo
has 314 action entries, 14 demonstration entries, four costumes and a
68-joint default costume. The 20 character motion states begin at 341. The
fighter root is `ftDataMewtwo`; the costume root is
`PlyMewtwo5K_Share_joint`, with material-animation root
`PlyMewtwo5K_Share_matanim_joint`. Sound bank
20 is verified against `lbl_803BB3C0` and `ssm_files` in
[`lbaudio_ax.static.h`](../src/melee/lb/lbaudio_ax.static.h).

The fighter's article slots are Disable at slot zero and Shadow Ball at slot
one, as registered by stock `ftMt_Init_OnLoad`. Article assets are copied into
the project. The template imports 46 tuning words and one behavior byte into
named project-owned parameters: 33 fighter words and one fighter byte, two
Disable words and eleven Shadow Ball words.
It also owns editable joint indices for the release/attachment joint and
Disable's spawn joint and Teleport's effect joint. Forward throw owns separate
shoulder/hand parameters, initially 35 and 32, preserving the source's direct
`parts` accesses. The root-animation flag owns a separate direct joint-index parameter, initially
1. Stock's enum spelling `TransN` is used as a raw ordinal at this call site;
semantic `TransN` resolves to joint 2 on the actual Mewtwo rig.

Import removes the attribute declarations and replaces `item.attr.*` reads
with `item.params.*`. Runtime decisions use named parameters and ordinary
fighter/article variables, with no storage in Mewtwo's native union. The
source template's numeric attribute offsets are import inputs only.

## Shadow Ball fighter phases

The source contract is
[`ftmewtwospecialn.c`](../src/melee/ft/kinds/ftMewtwo/ftmewtwospecialn.c).

| Path | Authored behavior |
| --- | --- |
| Ground/air start, 341/346 | Clear commands and motion latches; ground entry resets vertical velocity, air entry halves it. Register damage, death2 and death3 cleanup and begin the animation. |
| Held creation | Command 3 creates an article at the shoulder joint's local `(0,0,2)` offset, projected onto the gameplay plane. Its handle and charge are fighter variables. |
| Start completion | Full/stored charge selects release; otherwise select the ordinary charging loop. The aerial start clears the release-request latch before deciding, preserving the source asymmetry. |
| Charging loop, 342/347 | Decrement the initial release lockout, then create the attachment, update charge sound and advance the charge timer. Charge increases only when the timer is **greater than** the interval. |
| Full loop, 343/348 | Preserve the original ground `true`/air `false` charge-stop latches. Full charge triggers rumble, the Top-joint effect and color 92 without inventing an automatic cancel. |
| Loop interrupts | Ground roll has first priority, then A, then B, then shield cancel. A/B respect release lockout in the ordinary loop. Shield cancellation removes the held article while keeping charge. |
| Cancel, 344/349 | Remove the held article and finish in Wait/Fall. Reentry retains stored charge. |
| Release, 345/350 | Command 1 launches synchronously through an authored article signal, applies ground/air recoil using the pre-reset charge, clears the handle/charge, then plays the source sound. |
| Ground/air collision | Preserve the animation frame and common ground/air transition flags, adding KeepSfx in charging/full loops. Rebind cleanup after the transition. |
| Release completion | Ground release returns to Wait; aerial release chooses Fall or FallSpecial according to the owned landing-lag parameter. |

The release sound test intentionally remains after the charge reset. Thus the
ordinary stock maximum of seven also takes the normal release sound branch.
This is a translation of the source order, not a correction to its apparent
intent. Charge-sound thresholds preserve the original strict comparisons and
the initial small positive latch; command 2 is not consumed by that helper.

Damage removes the held articles while retaining a fully charged Shadow Ball;
partial charge clears the charge/color/effects. Death2 and death3 always clear
charge. The shared generic `callbacks.track_death3()` targets the authored
death2 handler, matching stock's callback registration. Color reset restores
the full-charge indication from ordinary fighter state.

## Disable fighter and article

The source contracts are
[`ftmewtwospeciallw.c`](../src/melee/ft/kinds/ftMewtwo/ftmewtwospeciallw.c)
and [`itmewtwodisable.c`](../src/melee/it/kinds/itmewtwodisable.c).

Ground and air entries reset the throw cue, command and owned handle; air
entry also zeros vertical speed. Command 0 creates the projectile through the
ordinary accessory update. The spawn offset uses the owned X/Y tuning,
facing and fighter instance scale. Landing and leaving ground retain the
animation frame, restore the accessory and cleanup callbacks, and preserve
stock's airborne vertical reset and horizontal clamp. Ground physics uses
ordinary friction; air physics uses Disable's gravity and terminal speed
with normal horizontal deceleration.

The one-state article owns speed, lifetime, hitbox scaling, debug ownership,
bounds removal and destruction notification. Its lifetime checks zero
**before** decrementing, unlike Shadow Ball's decrement-first lifetime.
Wall/ceiling collision, damage, shield, clank, absorb and shield bounce remove
Disable; reflection calls the common item reflector operation. Neither fighter
cleanup nor article destruction calls the stock Disable handler.

## Shadow Ball article

The source contract is
[`itmewtwoshadowball.c`](../src/melee/it/kinds/itmewtwoshadowball.c).
The authored 18-state table retains held state 0, charge-dependent flying
states 1-8, throw flight state 9 and explosion states 10-17.

Held creation preserves the two initial RNG draws, initializes visual wobble
state and attaches to the owner. Held animation checks owner motion/cancel/
detachment, reads the owner's ordinary charge and applies the same model scale
and captured-rest-position centering. Release selects charge+1, computes the
same charge-dependent speed/scale/damage values and initial half-to-full speed
factor, detaches while preserving any unrelated held item, and resets the
child scale. Subsequent flying physics preserves the source's full-speed
overwrite beginning on the second update.

Visual wobble is ordinary article state: timer, angle, speed, acceleration,
Y/Z offsets and update count. At a period boundary it consumes the same
floating RNG draw and bounded integer sound choice in the same order. The
child translation receives the oscillation, while physical travel uses the
launch angle. Reflection adds pi to that angle; shield bounce mirrors velocity
and recalculates the angle. Stage impact hides the model, switches to the
charge+10 explosion state, clears the accessory, spawns the impact effect and
stores the scaled hit radius. Explosion animation updates an enabled hitbox's
radius; explosion physics zeros all three velocity components.

State 9's animation/physics/collision callbacks are translated, but the native
forward-throw creation path that enters it is not. The state is not reached by
the authored neutral special. Forward-throw projectile initialization and its
joint-to-joint launch-angle policy remain untranslated.

## Teleport fighter phases

The source contract is
[`ftmewtwospecialhi.c`](../src/melee/ft/kinds/ftMewtwo/ftmewtwospecialhi.c)
and the ordinary surface-angle checks in
[`inlines.h`](../src/melee/ft/inlines.h).

| Phase | Ground | Air | Authored behavior |
| --- | --- | --- | --- |
| Startup | 353 | 356 | Stop ground momentum or divide air momentum; own startup gravity, accessory effect and ground/ledge transitions. |
| Travel | 354 | 357 | Enter frame 35 and freeze playback; consume jumps, set body intangibility and visibility, decrement the owned travel timer and process surface exits. |
| Recovery | 355 | 358 | Capture momentum after the motion transition, restore visibility, apply end momentum, then wait or enter common freefall/landing. |

Fourteen imported Teleport parameters cover both startup velocity divisors,
gravity/terminal speed, integer travel duration, platform grace, stick minimum,
speed multiplier/addition, end drift, integer surface-angle clamp, end momentum,
freefall mobility and landing lag. The effect joint is an owned index 5,
matching stock's direct `parts[FtPart_WaistN]` access.

Direction selection preserves the source's asymmetric comparisons: grounded
selection accepts strength equal to the minimum, while airborne selection
requires strictly greater strength. Grounded travel also checks the angle to
the floor normal and conditionally marks soft floors skipped. Otherwise it
enters airborne travel. Neutral airborne input uses straight-up travel with
unit strength; horizontal facing updates preserve their original thresholds.

Air travel increments its own collision age before the ground/ledge query and
uses that age to gate landing on pass-through platforms. Ceiling and both wall
angle checks run independently in source order, including any change to
velocity caused by an earlier check ending travel. Ground travel separately
handles wall exits and walking off the floor. The transition masks retain the
source's body-status preservation and explicit zero animation rate.

The startup effect samples the waist joint and emits effect 1256 once. Recovery
samples that joint and marks the effect flag but emits no end effect, matching
the live source. Both install ordinary effect hitlag callbacks and clear the
accessory callback. Body status and render visibility use generic engine
operations; all travel decisions and temporary momentum remain ordinary named
fighter variables. No Teleport handler is delegated to stock.

The full 37-word module compiled and its isolated bundle was accepted under
`.local/mewtwo_teleport_preflight`. All ten Teleport recordings passed exact
700-sample stock comparisons using the isolated copied tool/host/UI/assets
snapshot. Evidence is in `.local/mewtwo_teleport_matrix_2/results.json` and its
stock/clone logs. Each path compared 1,398 fighter states and 699 RNG samples,
including pose/hurtbox/visibility traces and any damage events.

The accepted paths cover ground and air direction selection, neutral input,
diagonal travel, walking off the floor during travel and downward travel
landing. They enter all six authored motion states. These Final Destination
recordings do not exercise the ceiling/wall angle exit or soft-floor grace
branches; those are source-translated, with runtime branch coverage still
pending. The existing 15 accepted Shadow Ball/Disable comparisons remain the
verified first batch.

## Confusion capture and reflection

The source translation follows
[`ftmewtwospecials.c`](../src/melee/ft/kinds/ftMewtwo/ftmewtwospecials.c),
[`ftCo_CaptureMewtwo.c`](../src/melee/ft/kinds/ftCommon/ftCo_CaptureMewtwo.c)
and [`ftCo_ThrownMewtwo.c`](../src/melee/ft/kinds/ftCommon/ftCo_ThrownMewtwo.c).
Ground motion 351 and air motion 352 own entry, animation, physics, collision,
the accessory reflector cue and capture callbacks. Confusion has no article.

Nine imported words own air boost and the reflector's joint, maximum damage,
offset, radius, damage multiplier and speed multiplier. Its descriptor behavior
is a single byte at offset 60, imported as a named integer. Raw stock values are
boost 1.5, joint 2, maximum damage 50, offset (0, 8.5, 10), radius 10, damage
multiplier 1.5, speed multiplier 1 and behavior zero. Descriptor behavior and
ownership retention are separate engine flags; Confusion sets retention after
creating its bubble and restores it across ground/air transitions.

The captured fighter uses the holder's facing and forced air kinetics. Common
capture cleanup and linked motion initialization enter capture pose 301/302,
then run force air, mask 511, animation start and movement clear in stock order.
The existing shared attached-throw core enters pose 303/304 with the holder as
motion owner and installs the common thrown-position callback. FreezeState is
selected from the captured fighter's ordinary capture flag. No Mewtwo-specific
capture initializer is delegated to stock.

Holder catch clears movement, disarms further grabs and marks special capture.
A nonzero release command only consumes when a live victim exists. Release
caches that victim before applying throw damage, which clears the reciprocal
grab links, then enters common DamageFall through its engine operation. This
preserves parasol handling, drift clamp and rumble rather than approximating
release with normal fall or the ordinary throw launch.

The reflection cue preserves its source ordering: create and retain ownership,
then disable and clear the no-op reflect callback only when latched. Collision
transitions preserve the reflection latch and rebuild neither the descriptor
nor the bubble. A persistent ordinary boost-used variable gates aerial entry;
primary death, common landing-action initialization and Confusion's air-to-ground
transition reset it at the stock policy points.

This batch compiled, imported all 47 fields exactly and was accepted as package
`93b41c1ae08293e168a93f3ee521a31badf805adde1456392b3c35ee71c9b12e`.
All ten stock comparisons passed 700 samples each, including 1,398 fighter
states and 699 RNG samples per case. They require actual ground/air/left-facing capture and DamageFall release,
capture through a shield, repeated aerial use and landing reset, and a reflected
Mario fireball reversing horizontal velocity while retaining its owner. The
fireball reverses from -1.26 to +1.26 at sample 214 with owner P2 throughout.
Repeated air entry reaches frame one at samples 181, 236 and 401: the second
entry retains downward momentum, while the first and third apply the stock
boost, confirming consumption before landing and reset afterward. Ground and
air capture require target poses 303/304 and common DamageFall 38, plus damage.

Accepted evidence is `.local/mewtwo_confusion_matrix_2/results.json` with its
stock/clone logs. The initial matrix exposed a corrected physics selection
(ordinary air drift instead of stock gravity/friction) and two fixtures that
missed aerial capture/reflection. A higher airborne victim and closer fireball
shooter now enter the required branches. The repeat fixture checks fighter
frame-one snapshots because the action logger omits same-motion restarts.
The previous 25 accepted paths retain their earlier evidence. Their combined
regression initially exposed a shared compiler defect: article owner-state
parsing reserved two internal bytes while this fighter program reserved one.
Three consecutive booleans moved the article's integer offsets, so its held
Shadow Ball read the charge timer as charge and changed joint scale at sample
199. All ten Shadow Ball paths failed on pose; Disable and Teleport matched.
The previous template rebuilt against the same tool/host still matched,
isolating state layout rather than a host or stock translation change.

The compiler now receives the selected fighter program's reserved byte count
when parsing article owner state. A regression checks three consecutive
booleans followed by integer and float fields, including owner reads/writes,
for both special and base callback programs. No state declarations were
reordered to hide the defect. Failed evidence remains in
`.local/mewtwo_confusion_regression/results.json`; the old-template isolation
is preserved in `.local/mewtwo_shadow_regression_isolation`.

The repaired current template, with explicit captured-animation donor policy,
imports all 47 fields (46 words and one descriptor byte) exactly. Package
`efc75d21e6ee6010297c5111895e1c14170b1af1235735f0af5348eab4723944`
passed all 35 paths against one freshly copied host/tool snapshot: each case
matches 700 controller samples, 1,398 fighter states and all 699 RNG samples,
along with article, pose, visual and damage traces. Complete evidence is
`.local/mewtwo_repaired_matrix/results.json`; the accepted project/template
and binary hash manifest are in `.local/mewtwo_repaired_preflight`, and the
frozen runtime is `.local/mewtwo_repaired_runtime`.

## Verification and remaining scope

[`mewtwo_special_parity.py`](../tests/native/mewtwo_special_parity.py) provides
45 accepted stock comparisons, including seven forward-throw/aerial-jump paths
and three worn-item jump paths:
short A/B release, left-facing release, shield cancel
and reentry, roll and reentry, full charge release/storage, aerial transitions,
shield/reflector responses and grounded/aerial/left-facing Disable, including
Disable shield contact and target stun, plus ten Teleport and ten Confusion
paths. It first
compares every imported tuning word bit-for-bit with raw fighter/article DAT
data, then builds/packages a fresh isolated draft and compares identical
recorded inputs against stock. The shared runner uses hidden windows, accelerated simulation and drawing
enabled. Current runs mute host output while retaining
normal audio callbacks. The runner preserves fighter, article,
visual and RNG traces and requires the named motions to occur.

The integrated compiler generated and compiled the complete owned-parameter
module, and packaging/acceptance succeeded. The first batch's 23 imported words
matched the independent raw DAT values. Its 15 cases match stock across 700 recorded
samples each, including fighter/item state, visuals, damage and all 699 RNG
samples per case. Grounded short-B release covers 139 article states and one
damage event; full-charge release covers 272 article states and two damage
events. The reflector case requires the live projectile's owner to transfer
from P1 to P2. Shield cases require Zelda's guard-setoff motion 181. The close
Disable target requires one damage event and stun motion 300.

These comparisons diagnosed two corrected translation errors: the copied
costume required its material-animation root, and Shadow Ball's wobble-period
attribute at offset 32 is an integer. Reading that integer as a float preserved
its raw import bytes but produced an immediate post-release visual/hitbox
divergence. The corrected integer parameter and timer preserve stock behavior.
The left-facing recordings use a short walk turn to stay on stage; the
reflector recording starts Zelda's special before projectile arrival. Disable
contact recordings place the target nearby through the shared offline fall
fixture, then wait for landing before the move. These assertions prevent
defensive recordings from passing without the intended contact.

Accepted evidence is preserved under `.local/mewtwo_matrix` (the original
12 passing cases), `.local/mewtwo_matrix_recordings_fixed` (left-facing cases
and actual reflection) and `.local/mewtwo_disable_contact` (shield contacts
and Disable stun). `.local/mewtwo_validation_summary.json` combines the 15
passing results. The matrix records failures and logs rather than treating
move-entry markers as runtime parity.

The template now authors the relevant Mewtwo load, item, knockback, idle,
angled-motion, dynamics and ordinary aerial-jump policies. Common engine
movement and animation-command handling remain shared. Primary death,
landing boost reset and article cleanup continue to use ordinary owned state.
The worn-item sound hook below now covers common Screw Attack and multijump
entry. Kirby copy behavior remains deferred. Accepted cases establish the
listed behavior; they do not exhaust every stock gameplay interaction.

## Forward throw and common lifecycle candidate

Forward throw limits its callback to common ThrowF motion 219, consumes
command 3 once, reads direct shoulder/hand world
positions, projects both onto z=0, and computes the launch angle with float
`atan2`. An ordinary owner flag distinguishes forward-throw creation from
normal held creation, including angle zero. The synchronous article callback
enters state 9 before clearing commands, initializes the exact source fields,
sets manual lifetime, and uses the throw speed for both launch and wobble
speed. Allocation failure still consumes the cue and plays sound 200115.
Forward-throw construction draws no RNG; ordinary held construction retains
both original RNG draws in their original order. Existing normal Shadow Ball
states and signals remain intact.

The CPU hook preserves the actual stock charge branch and command queue:
full charge in motions 342/343 taps R and finishes; zero charge outside 342
centers the stick, taps B and finishes; cancellation in 342/343 taps R then
waits without adding a finish command. Ordinary policies preserve no-op item
callbacks, part mode 2 for knockback, idle variants with a held item, angled
motions 51..61 excluding 53, and the stock dynamics-ground predicate.

Load copies extended attributes into fighter-owned storage, enables direct
animation copy on stock joint 1, and enables hip-root compensation. The copied
base declares Disable/Shadow Ball article asset slots 0/1; shared asset loading
performs their registration. Reload restores extended attributes; immutable
named Disable offsets apply the source scale at spawn rather than mutating
raw tuning memory. Every dynamics set begins interaction forces at part 8.
Ordinary aerial jump enters air, sets command 0, chooses motion 27/28 using
the strict stock reverse threshold, and calls the shared common jump operation
with horizontal input times the common multiplier, zero vertical velocity,
input-timer reset, animation-driven physics and bunny-hood sound suppression. The source checks
`x197C` (bunny hood), rather than `item_gobj` (ordinary held item).

[`mewtwo_stock_policy_compare.py`](../tests/native/mewtwo_stock_policy_compare.py)
compiles mechanically extracted original stock C alongside
[`mewtwo_stock_policy_test.c`](../tests/native/mewtwo_stock_policy_test.c).
The initial candidate reported 22,504 checks with zero failures, but its
identity semantic-bone stub was insufficient for OnLoad. The enhanced fixture
now parses actual Mewtwo joint maps from ISO `PlCo.dat`, compiles the actual
`ftParts_GetBoneIndex`, and executes the actual extracted ground-to-air reset.
It rejects the initial module with exactly one direct-animation-flag failure;
the corrected named direct-joint parameter then passes **22,510 checks with
zero failures**, including negative command-3 cues in other throw motions.
Coverage includes both facings, three model scales, sixteen throw angles,
allocation success/failure, every motion and charge boundary for CPU queues,
all angled-motion/dynamics predicates, load flags, all tested dynamics set
indices and original aerial-jump entry. Jump coverage includes threshold
equality and adjacent floats, three velocity multipliers, bunny-hood presence,
jump counts, timer resets, selected motion, exact velocity, physics callback,
voice/item sounds and ordered engine calls. The original pre-refactor common
jump core from `598f7e889` also matches the refactored actual common core for
Mewtwo, Ness and Mario under both timer policies and both bunny-hood conditions.
Host stubs record only shared operation boundaries; production script handlers
do not call the extracted reference functions. Load comparison asserts rig
flags and attribute copying; asset-slot registration occurs in the copied
asset loader, so its callback trace is not claimed identical to stock OnLoad.

The corrected candidate imports all 47 raw tuning fields exactly and bundles
as `7f6102fb15a81272ba69856931313a3c2e2dd4a0c0fd00dd86dde3d4d53be406`.
Frozen tool/host/UI/assets and the matching SDK are preserved in
`.local/mewtwo_lifecycle_fixed_runtime` and `.local/mewtwo_lifecycle_fixed_sdk`,
with SHA-256 manifest `.local/mewtwo_lifecycle_fixed_snapshot.json`. Comparison
logs and extracted-source/module hashes are in
`.local/mewtwo_lifecycle_fixed_direct`; package, owned source and imported
raw-field checks are in `.local/mewtwo_lifecycle_fixed_preflight`. The copied content tool embeds its live
SDK path, so the candidate DLL was recompiled against the frozen SDK headers.
The 35 previously accepted game cases retain their evidence above. The first
seven new throw/jump attempts were rejected: the right throw enters
ThrowF 219 and creates article 112, yet the first idle pose differs at sample
52: stock `5d8289f704f54fd9`, candidate `8f5c6ef175d345d0`, with the other
traced fields identical. The raw joint-1 versus mapped joint-2 flag explains
this pre-move difference. Failed logs remain in
`.local/mewtwo_lifecycle_muted_newcases`. The left grab target was too distant;
the revised fixture uses the same 12-unit spacing as the right case. Airborne
placement consumed all jumps, so revised jump fixtures use ordinary ground
jump followed by a second recorded jump. These source/fixture corrections
passed all **42 hidden, muted, fast, drawn comparisons** in one complete
regression at `.local/mewtwo_lifecycle_fixed_matrix/results.json`. Each case
matches 700 input samples, 1,398 fighter states, 699 RNG samples and every
applicable article, pose, visual and damage trace. Both facing directions reach
forward throw 219 and spawn article 112, matching eleven article states and
six/right or seven/left damage events. All five double-jump cases enter their
required 27/28 motions. The complete 35-case special/article regression also
passes on this same module/host snapshot. The raw joint-1 fix resolves the
initial sample-52 pose divergence; the real-map C fixture prevents the prior
identity-map assumption from accepting it again.


## Worn-item jump sound policy

`on worn_item_jump_sound` returns false for the copied Mewtwo. The shared
Screw Attack and multijump entries consult this optional authored predicate;
ordinary authored `jump.begin` keeps its explicit false sound argument. This
policy targets the bunny-hood object at `x197C`, not the ordinary held item at
`item_gobj`. Shared common entry handles motion, velocity, jump count, timer,
voice and turn behavior; the predicate only supplies the source sound choice.

The expanded original-C comparison passes **23,495 checks with zero failures**.
It mechanically extracts original `ft_800D2E7C` and `ftCo_800D74A4` from
`598f7e889`, compares the current common functions using a custom fighter kind
and the authored predicate, and retains the prior 22,510 checks. Additional
coverage includes hood presence, both facings, timer/jump-count variations,
multijump stick directions and counts, exact velocity scaling, ordered sound
calls, turn countdown/facing and actual shared turn-core root rotation.

Three hidden, muted, fast, drawn gameplay fixtures passed against candidate
`1a7f1816708dcca98d811b0c6ea4858f86024134f351da2b0c453a0dd4c3472b`:
bunny hood with ordinary double-jump, held Screw Attack with double-jump,
and the combined hood/Screw Attack case. They require actual item state 2 with
owner P1 through the second jump and motions 27 or 154/155. Each matches
700 input samples, 1,398 fighter states and 699 RNG samples. Item traces also
match 600, 570 and 1,130 samples respectively; the combined case matches one
damage event. Sound calls are checked directly in C, since gameplay traces
do not contain an audio-event stream. The first attempted Screw fixtures used
an incorrect required motion 156; raw enum inspection confirmed 154/155, and
the corrected rerun is accepted. Failed assertion logs remain preserved.

Evidence is `.local/mewtwo_worn_direct_owned`, `.local/mewtwo_worn_matrix_2`,
`.local/mewtwo_worn_preflight`, and the matching frozen
`.local/mewtwo_worn_runtime`/`.local/mewtwo_worn_sdk` with manifest
`.local/mewtwo_worn_snapshot.json`. The final 42-case regression against this
same current module/host passed in `.local/mewtwo_worn_regression`. Together
with the three worn-item cases, all **45 comparisons** accept this complete
template revision. `.local/mewtwo_worn_acceptance.json` combines their results,
source hashes and evidence paths. The reusable checked-in stock-C runner also
passes all 23,495 checks against this same candidate.


## Fully authored behavior: baseline 6

The template now declares `fighter: / behavior independent`. Its generated
definition uses `Ft_Kind_None`: stock callbacks, special entries, item/article
logic and model-attachment behavior are cleared before authored handlers are
installed. The untouched catalog base still provides fallback for raw drafts.
The module owns all 80 nonempty animation/input/physics/collision callbacks
across its twenty special states. Its lifecycle, common-action and CPU policy
audit found no remaining fighter-specific gameplay callback dependency.

Owned metadata includes all 44 semantic bone pairs for the 68-part rig, two
article bindings, effect bank 13, sound-bank choices and four demo archive roots.
Animation-family tags remain immutable clip metadata. Shared engine primitives
and the common camera callback remain native C.

All 45 hidden, muted, accelerated, drawn gameplay cases passed against stock
with this independent definition: 31,500 controller samples. The comparison
explicitly validates stock versus custom behavior identities before normalizing
only the `kind` and `behavior` action fields. All fighter, pose, hitbox, hurtbox,
item, visual, damage and RNG traces remain compared. The matching generated
module also passed all 23,495 direct original-C comparisons.

Accepted package:
`9994476a51d55c99109ba013afd5fe7044439bd21005368a5769da6183a1a5dd`.
Evidence and hashes are in `.local/mewtwo-independence6-acceptance.json`,
`.local/mewtwo-independent-runtime6` and `.local/mewtwo-independent-direct6`.
Baseline 6 is frozen at `.local/baseline6`; all 58 focused checks pass, including
96 compiled ownership-mask combinations. These gameplay traces do not project
each script variable into the stock union layout; direct C fixtures provide
the corresponding variable comparisons.
