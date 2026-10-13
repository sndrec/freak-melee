# Bowser script translation

Updated: 2026-10-02. Bowser is an **independent scripted versus template**. All four specials,
Flame article logic, capture/throw decisions and identified fighter-wide
setup/item/knockback/CPU policies are authored. Its compiled Definition has no
stock behavior identity. Broader runtime acceptance is open; the closure
checkpoint below records the bounded independent package acceptance.

The [template](../examples/koopa_wip.fms) translates move decisions into
readable scripts using shared engine operations. It calls no stock Bowser
special handler and reads no stock motion-variable union or live DAT tuning.
`import-parameters` copies stock defaults once into the project's editable
`fighter_parameters.fmp`; the build uses those named owned parameters.

## Asset and motion layout

The `koopa_clone` base imports the retail `PlKp.dat`, `PlKpAJ.dat` and selected
costume independently. Source tables specify 316 actions and 14 demo entries;
both the common parts table and a traversal of `PlKpNr.dat` confirm 76 joints.
The fighter has 23 special motion states, but only 21 special animation entries:
side-special waiting states share clips. Motion IDs must not be treated as
animation-table indices. No skeleton, animation or texture was reauthored for
the comparison.

| Authored path | Stock source | Motion IDs |
| --- | --- | --- |
| Fortress entry, animation, steering and ground/air collision | `ftKoopa/ftkoopaspecialhi.c` | 359–360 |
| Shell visibility, hurtbox states and attached-item visibility | `ftKoopa/ftkoopaspecialn.c`, `ftKp_SpecialN_80135780` | Applied by Fortress callbacks |
| Bomb startup, root-driven rise, dive, contact/ledge checks and landing | `ftKoopa/ftkoopaspeciallw.c` | 361–363 |
| Dynamic-bone floor policy | `ftdynamics.c`, `ftCo_8009DD94` | Fighter-wide |

Fortress installs empty damage/death2 callbacks, as stock does. Its aerial
entry immediately invokes the ground animation callback. A cue-driven aerial
exit destroys effects and clears hitlag callbacks; the animation-ended exit
does not perform that extra cleanup. The script preserves those distinctions.
Stock's landing rewind condition tests `lower < elapsed < lower`, so it can
never be true. The script preserves the reachable same-frame landing and
comments on the omitted branch rather than inventing another bound.

Bomb's grounded startup switches to airborne physics before entering its
ground-start motion. It clears vertical TransN delta, later changes to the
aerial motion at frame 30, and enters the dedicated landing motion only after
its subaction cue permits it. Diving and landing effects receive different
argument layouts: a model joint versus a world position.

## Reusable scripting additions

- [Body capsule states and attached-item visibility](scripting/combat_visuals.md#hurtboxes-and-attached-item-visibility): normal/invincible/intangible states,
  per-bone changes, latched held-item visibility and attached-article visibility.
- [Position-based effects](scripting/combat_visuals.md#effectsspawn_at_positioneffect-x-y-z), alongside existing joint-based spawning.
- [Writable root-motion sample components](scripting/movement.md#animation-root-motion-deltas) and [collision-box unlocking](scripting/movement.md#collisionunlock_ecb).
- [Dynamic-bone floor policy](scripting/common_policies.md#on-dynamics_ground_collision), independent of stock fighter ID.

The initial run found a pose difference at the first landing, before either
special. Stock Bowser disables dynamic-bone floor collision by fighter kind;
the custom slot missed that exclusion. The explicit script policy fixes the
cause. Pose hashes remain part of acceptance; none were filtered out.

Compiled fixtures exercise both script forms, argument forwarding, component
writes, changing floor-policy results and rejection of invalid hurt statuses,
out-of-range bones and bones without capsules. New host operations point
directly to existing shared engine functions; there is no Bowser-specific API.

## Accepted runtime comparisons

`tests/native/bowser_special_parity.py` runs hidden clients with ordinary draw
callbacks and accelerated input timing. Each stock/script pair replays the
same 550 controller samples. Sixteen distinct cases match the complete traced
fighter state for both ports, actions, damage, body contacts, item state/events
and RNG:

- Fortress: grounded/aerial starts, left/right facings, neutral/opposite steering,
  grounded Zelda contact and Yoshi's Story slope.
- Bomb: grounded/aerial starts, left/right facings, grounded Zelda contact and
  Yoshi's Story slope. Grounded left-facing root motion carries stock Bowser
  off the left ledge; that case verifies the dive, KO and respawn path. The
  remaining Bomb cases reach the dive, landing and ordinary idle states.

The accepted recordings are in `.local/bowser-special-final` (14 non-contact
cases, including a corrected offline KO/respawn coverage check for the
left-facing Bomb) and `.local/bowser-special-contact-ready` (two fresh contact cases).
Both contact cases record actual damage to Zelda. The summary is
`.local/bowser-up-down-acceptance.json`. These local files contain extracted
disc assets and are intentionally untracked.

The first batch had the dynamic-bone mismatch. An initial Fortress contact
recording missed the target; extending its approach then put B inside the
noninterruptible RunBrake segment. Neither attempt is accepted contact evidence.
The corrected recording waits for idle before pressing B. All 34 focused
native fighter/script checks pass, as does the native build. Windows visibility
was independently observed as false during the batch.

Reproduce using the retail ISO:

```powershell
python tests/native/bowser_special_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso'
```

## Remaining ownership and acceptance

The compiled template authors the identified versus behavior, including common
throw victim-pose selection. Broader costumes, held-item/interrupt/death paths,
opponents, stage interactions, effects/audio and rollback/editor acceptance
remain beyond the selected recordings below. The uncompiled asset base still
uses stock callbacks; a draft without its authored module is not an independent
behavior template. Earlier dated checkpoints describe those inherited drafts.

Rook was rebuilt for the extended host layout with her script source unchanged;
verification retained 285 own-rig clips and zero borrowed clips, and project
installation replaced her previous package rather than adding a duplicate.

## Flame Breath and Flame article checkpoint (2026-09-29)

Neutral-B now owns all six start/loop/end motion callbacks, including the
minimum emission duration, looping-animation gate, frame-preserving ground/air
transitions, quake timing and per-tick breath depletion. The death and late-update
policies initialize and regenerate ordinary `breath_length` and `breath_size`
fighter variables from owned parameters. The script chooses appearances with
the original conditional RNG draw count, groups hits in twelve-flame batches,
and preserves voice-selection thresholds. It does not call a stock neutral-B
handler or access Bowser's fighter/motion union.

The owned Flame article translates `itkoopaflame.c`: random speed/angle,
integer-truncated strength scaling, first-animation/physics/sweep ordering,
per-instance hitbox radius, hitbox lifetime, model scale, root effect,
manual lifetime, contact-normal steering, reflection, shield bounce,
absorption cleanup and non-removing damage/clank/shield callbacks. Direction
and velocity normalization use shared engine vector operations; the script
owns when they run and the steering policy. The original shield-bounce
`atan2(y,x)` convention is retained even though normal physics uses sin-X/cos-Y.
Explicit intermediate assignments preserve float rounding before a later multiply.

### Native source correction

Bowser's `unk1` motion slots were typed as placeholder pointers and C booleans.
On the PC, the pointers widened and shifted counters that shared throw callbacks
read as 32-bit words. The supposed boolean at offset four actually contains
an item hit-group ID during breath. Both union views now use explicit word types,
with compile-time offset checks, and the stock Flame constructor accepts a
`u32` group ID without boolean truncation. This corrects the native representation;
it does not copy a PC layout bug into the authored behavior. Absorption now emits
a diagnostic item event when replay tracing is enabled, before the existing
callback; this adds no gameplay decision.

### General API additions

- [World joint positions, explicit world launches and hit groups](scripting/fighter.md#bonesworld_position).
- [Shared armor threshold](scripting/fighter.md#damagearmor).
- [Item contact normals and collision-box extents](scripting/item.md#stage-contact-and-collision-box).
- [Article hitbox reads, radius writes and disabling](scripting/item.md#article-hitboxes).
- [Synchronous article-root effects](scripting/item.md#itemspawn_root_effect).
- [Signed angles, normalization, vector angles and RNG](scripting/math.md).

All tuning is imported once into the package parameter file; neither the
fighter script nor the article reads mutable stock DAT tuning at runtime.

### Accepted comparisons

Thirteen full stock/script pairs match: tap and 400-frame sustained breath
from ground/air in both facings; depletion and a second pull after recharge;
actual Zelda contact; Yoshi's Story floor geometry; Fox reflection with
ownership transfer; and Ness absorption with six traced absorb callbacks.
Comparisons retain fighter poses/hitboxes/body contacts, item core/relations/
pose/visual state, damage, actions and simulation RNG. Runs draw normally but
use hidden windows and accelerated replay pacing.

Evidence: `.local/bowser-breath-first` for the eleven ordinary cases and
`.local/bowser-breath-defense-contact` for reflection/absorption.
`.local/bowser-breath-acceptance.json` records the corrected offline comparisons
for the first eleven. Their original runner mistakenly required item kind 111
(the stock function's legacy suffix); the actual Flame kind is 100. Corrected
comparison gates match the original retained logs; no state fields were removed.
The first defense recordings used the wrong pad offset, and a later attempt
put Bowser close enough to be hit by Fox's initial reflector before neutral-B.
Those are rejected coverage, not accepted defense evidence. The final recording
sets P2's actual pad slot and stops Bowser outside the startup attack.

Reproduce with the retail ISO:

```powershell
python tests/native/bowser_breath_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/bowser-breath-new
```

At this checkpoint, remaining acceptance included the full side capture path, fighter-wide setup/
cleanup/item/CPU ownership, broader collision surfaces and combined interactions,
costumes, direct pixel/audio review and editor/rollback integration. These
selected comparisons do not establish a completed whole-fighter clone.

The layout/lifecycle change was also checked against four selected up/down
regressions: aerial left-facing Fortress with steering, grounded Fortress
contact, grounded left-facing Bomb's KO/respawn path and right-facing aerial
Bomb's dive/landing path. All match; traces and coverage results are retained
in `.local/bowser-breath-up-down-regression`.

The final native build and all 34 focused fighter/script checks pass. Rook's
unchanged script was rebuilt and verified for the extended SDK: 285 own-rig
clips, zero borrowed clips, one installed package. Her current package is
`5a8088a0a87ff191ac98e94d904865736b3854e116fd6f600d431b3e86225954`.
Refresh report: `.local/rook-bowser-flame-sdk-refresh.json`.

## Koopa Klaw capture and throw checkpoint

The side special now owns its entry, catch, waiting, repeat bite, directional
throw, release and ground/air transitions in script. It preserves the stick
threshold crossing, B-button repeat request, throw reversal cue and the saved
animation-facing root motion. Bite damage is imported as an integer, matching
the stock attribute's word type. All side-special motion callbacks are
explicitly authored, including empty interrupt callbacks where stock has none.

The [attached capture API](scripting/capture.md) parameterizes the common victim
engine: timer/mashing/wiggle settings, attached victim poses, frame-preserving
throw transitions, dropping and damaging releases. The victim's capture
initializer receives the holder explicitly; no common callback reads Bowser's
DAT or calls a Bowser tuning getter. Stock Bowser and Giga Bowser initialize
the same common settings from their own attributes. Removed entry variants and
getters have their call sites updated directly.

The native captured-pose union also had a placeholder pointer at offset four,
where the engine actually shares a scalar timer. On x64 this shifted the mash
and wiggle fields. It is now a float with checked word offsets. The SDK
fingerprint covers the corrected fields and the new common capture settings.
This corrects the native data representation rather than changing move policy.

### Acceptance

Fourteen stock/script comparisons match, with 700 controller samples per pair:
ground/air misses in both facings, and ground/air forward throw, back throw,
waiting, repeated bites and mash-out. The aerial throws also exercise landing
while carrying the victim. Coverage gates require the intended capture/throw
states, actual victim damage where appropriate, and both fighters' escape
states for mash-out. Both ports' traced poses, hitboxes, body contacts, actions,
damage and RNG remain in the comparison.

Evidence: `.local/bowser-capture-final/results.json`. Earlier airborne attempts
which missed the capture are rejected coverage; the final recording places
the victim inside the startup grab bubble and checks entry before acceptance.
No mismatching state fields were removed from comparison.

Four selected regressions also match: breath contact, Flame reflection,
left-facing aerial Fortress with steering and aerial Bomb's dive/landing.
Evidence: `.local/bowser-capture-breath-regression` and
`.local/bowser-capture-special-regression`. The final build and all 34 focused
native fighter/script checks pass. Tests run hidden with drawing enabled; a
live Windows visibility check confirmed the test window was hidden.

Reproduce the capture batch with:

```powershell
python tests/native/bowser_capture_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/bowser-capture-new
```

Rook's unchanged script was rebuilt, bundled, verified and installed for this
SDK: 285 own-rig clips, zero borrowed clips and one installed package.
Package: `5d7470e05317b7134ea5daf94522f7f7ecbf98965d71b842ab8edc12cacea968`.
Refresh report: `.local/rook-bowser-capture-sdk-refresh.json`.

At the capture checkpoint, setup, item/knockback and CPU ownership remained
open; the next section records that work. Left-facing successful captures,
offstage drops, capture interruption, broader opponents/costumes, pixel/audio
review and workshop/rollback integration are not established by this batch.

## Fighter-wide and CPU policy checkpoint

The compiled template now owns load/special-attribute reload, held-item pickup,
drop/visibility, knockback setup/cleanup, CPU initialization/spacing, attack
and capture classification/decisions, pummel button choice, ranged eligibility/
candidates and attacker threat geometry. Common input planning, collision
queries and engine operations remain in C. The script uses ordinary named
parameters for spacing, probe distance and threat height. No move-specific
charge property or Bowser-only scripting primitive was introduced.

The common down-throw victim-pose exception is authored through
[`throw_victim_motion`](scripting/fighter.md#on-throw_victim_motion), using the
victim's animation compatibility metadata. Its `event.motion` describes the
pending holder throw, and `event.victim_motion` is the default victim pose.
The callback runs before the holder changes state. An initial translation
incorrectly inspected the current CatchWait motion; the Zelda/Peach coverage
checks caught the missing alternate pose. The corrected context is tested in
both script forms and full stock/script recordings.

### Accepted comparisons

Six 2,200-sample CPU pairs match: level-nine idle/shield opponents on Final
Destination, level-four idle on Final Destination, level-nine idle on
Battlefield, and a Zelda CPU defending against Bomb and up-smash. All keep
both ports' tracked fighter/pose/contact state, actions, damage, item state
and RNG. The CPU batch uses the preceding hook build; none of these traces
enters down throw, and the subsequent callback-context correction is covered
by the dedicated throw batch. CPU Koopa Klaw captures/pummels are not covered
by these six recordings; compiled fixtures check the policy results without
claiming that runtime branch.

Four 700-sample common-policy pairs also match with the corrected hook: down
throw against Zelda and Peach (alternate victim pose), down throw against
Mario (default pose), and held-bat pickup, Fortress and drop/throw. Throw
coverage requires the holder throw, the expected victim pose and real damage.
The item case requires pickup, Fortress and the held item's traced state.
Initial attempts with A plus down selected pummel rather than throw and were
rejected coverage. No pose or RNG fields were removed to obtain acceptance.

Evidence: `.local/bowser-cpu-final2`, `.local/bowser-common-context-fixed` and
`.local/bowser-policy-acceptance.json`. Tests use hidden windows with drawing
enabled. The native build and all 34 focused fighter/script checks pass.

```powershell
python tests/native/bowser_cpu_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/bowser-cpu-new
python tests/native/bowser_common_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/bowser-common-new
```

Rook's script source remains unchanged after the SDK rebuild, bundle,
verification and replacement installation: 285 own-rig clips, zero borrowed
clips, one installed package. The refresh report is
`.local/rook-bowser-policy-sdk-refresh.json`.

All identified Bowser-specific versus decisions are now represented in the
compiled template. Broader opponents/costumes, successful left-facing side
captures, offstage drops, capture interruption, direct effects/audio review
and workshop/rollback integration remain open. This checkpoint is selected
path acceptance, not proof that every whole-fighter interaction matches.

Audio metadata follow-up: the clone now selects stock Bowser's bank 15
(`koopa.ssm`) instead of bank 12. The clone metadata check compares all 14
supported bases against stock sound masks and action/demo counts. Earlier
simulation traces remain valid, but they were not audible-output acceptance.


## Owned rig and named model-policy candidate, 2026-10-02

The retail PlCo map contains 48 semantic roles and 76 runtime/physical joints,
with no omissions; all four costumes were checked. The existing 23 own states
cover all 83 nonnull stock phase callbacks. FortressModels owns the original
secondary-model visibility and both models' slope rotation. It reads command2
and the owned spin_floor_contact variable, preserving the separate command3
fighter-root decision. Flame/other particle constructors remain unchanged.

The rig fixture passes against the actual ISO's PlCo table, inverse map and
costume trees: `.local/donkey-bowser-closure/rig-validated.json`. The common
production-body group fixture passes catalog family/count, both members'
animation phases, secondary removal/recycling, atomic partial-allocation
failure, creator/module/rig changes, and deferred sampling. Its allocation and
typed constructor resource boundaries are explicit doubles.

The generated named model policies now pass 46,080 exact combined DK/Bowser
source comparisons and eight lifetime/registration rejection cases using
matching frozen snapshot19 binaries, SDK and compiler. Four Fortress stock/clone pairs pass on ground and in air in both
directions, comparing 2,200 controller samples and 4,400 fighter states.
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
