# Samus script translation

Current Workshop direction, 2026-10-11: fresh Samus drafts now select
[`samus_full.fms`](../examples/samus_full.fms), with independent interpreted
behavior. All 282 bodies prepare with zero diagnostics and no legacy attribute
imports. This combines the four specials, four combat articles, complete grapple
resource solver, capture/throw presentation, CPU/roll/item policies and ordinary
load/death cleanup. Stock callbacks remain construction references in the copied
asset Definition; independent preparation clears them rather than forwarding.
The dated WIP records below describe earlier stages, not the current selection.
See the [gameplay checkpoint](native_source_samus_gameplay_checkpoint.md) and
[closure record](samus_clone_closure.md) for evidence and remaining boundaries.

The broader independent replay batch exposed a Charge Shot handle error:
`owner.state.shot_handle <= 0` copied a native pointer-null assumption into a
numeric creation-ID check. Creation ID zero is valid; only -1 means absent.
The script now tests `< 0`, so the first orb survives. No engine special handler,
ID offset, compatibility shim or altered gameplay timing is involved.

Current interpreter checkpoint (2026-10-10): the stock asset draft now has its
retail semantic rig map, and all 176 bodies in the main WIP template prepare.
The separately composed grapple/lifecycle candidate prepares all 279 bodies; its
remaining diagnostic is the intentional missing independent-Fighter declaration.
The [grapple interpreter checkpoint](native_source_samus_grapple_interpreter_checkpoint.md)
records constructor geometry comparisons and the remaining runtime boundaries.
The [lifecycle checkpoint](native_source_samus_lifecycle_checkpoint.md) adds
owned load/reset hooks and scale-derived values, compared through the VM with
the stock load, attribute-reload and primary-death bodies.
The [grapple policy checkpoint](native_source_samus_grapple_policy_checkpoint.md)
adds the input code sequence, air wall-contact path and modified hang timer.
The code sequence matches the whole stock helper in 8,200 VM comparisons;
240 comparisons match the native hang-timer slice. The wall-contact branch
has source preparation proof only.
The [homing checkpoint](native_source_samus_homing_checkpoint.md) adds a generic
nearest-available fighter query and authored grapple steering. Capsule creation
and its modified-mode transition remain open; homing has no live solver acceptance.
This remains a partial authoring base,
not an accepted independent fighter. See the
[authoring checkpoint](native_source_samus_authoring_checkpoint.md) and
[article feedback checkpoint](native_source_samus_article_feedback_checkpoint.md) and
[action/motion checkpoint](native_source_samus_action_motion_checkpoint.md) for the exact
scope, changes and receipts. The [interruption checkpoint](native_source_samus_interruption_checkpoint.md)
records the cleanup sequence and remaining targeting/overlap failures.
The [ahead-query checkpoint](native_source_samus_ahead_checkpoint.md) records
native target-search comparisons and the now-preparing missile physics body.

Samus is an independent copied-asset base with a **partial** authored template.
[Charge Shot, missiles, Screw Attack and bombs](../examples/samus_wip.fms)
own their entry, animation, interrupt, physics and collision decisions.
The three combat articles and CPU policies are also authored. Grapple,
throw attachments and broader lifecycle runtime still require verification;
this is not a fully script-owned Samus clone yet. Kirby cloning and copy
abilities remain deferred.

## Copied assets and editable values

The base copies 313 action entries, 14 demonstration entries and a 60-joint
costume. Its 18 fighter-specific motion states begin at 341. Samus's stock
costume has no material-animation public root, so that metadata is empty.
Her four stock article slots retain bomb, charged shot, missile and grapple
beam assets. Registering those assets does not translate their behavior.

The template's stock attribute declarations are import inputs, not runtime
offset dependencies. Importing the template creates named, project-owned
parameters for ground launch speed, drift acceleration, horizontal limit,
air launch speed, freefall drift, turn threshold and landing lag. The move's
turn latch and effect flag are ordinary script state. Charge Shot adds owned
maximum-charge, charge-interval, air-recoil and landing-lag parameters, plus
editable skeleton indices for creation and release. Its charge, timer,
air-start latch and handle are ordinary fighter variables. Article lifetime,
speed and model-scale limits become owned article parameters.

## Screw Attack callback translation

The source of truth is
[`ftsamusspecialhi.c`](../src/melee/ft/kinds/ftSamus/ftsamusspecialhi.c).
No scripted Screw Attack callback calls a stock Samus special handler.

| Path | Authored behavior |
| --- | --- |
| Ground entry | Enter motion 353, bind cleanup and effect hitlag callbacks, switch to ground, reset commands and turn latch, start animation, spawn effect 1154 on the rotation part. |
| Air entry | Enter motion 354, consume air jumps, initialize vertical speed, clamp horizontal speed, reset commands and turn latch, start animation and effect. |
| Ground-started physics | On the launch command, become airborne and set horizontal launch speed. In air, use animation vertical displacement, calculate tuned air acceleration, then overwrite it with normal air acceleration, matching the original call order. |
| Air-started physics | Run normal air physics, then calculate tuned horizontal acceleration. |
| Interrupt | During the permitted command window, opposite stick input over the threshold turns once and rotates the root to the new facing. |
| Animation end | Clear the effect, remain airborne with jumps consumed, then enter Fall or FallSpecial according to landing lag. |
| Collision | While rising, run the stock shared ground check. While descending, land with special lag or catch a ledge and clear the effect. Grounded frames use standard ground collision. |

Screw Attack introduced reusable
[`physics.air_accel_normal()`](scripting/movement.md#physicsair_accel_normal).
It computes horizontal acceleration without changing gravity or vertical
velocity. Ground-started Screw Attack deliberately performs both acceleration
calculations in their original order; they are not additive.

Held-item pickup, drop, visibility and invisibility policies use generic fighter
operations with Samus's original flags.

## Charge Shot callback translation

The source of truth is
[`ftsamusspecialn.c`](../src/melee/ft/kinds/ftSamus/ftsamusspecialn.c) and
[`itsamuschargeshot.c`](../src/melee/it/kinds/itsamuschargeshot.c).
No authored Charge Shot callback calls a stock Samus special handler or
stock Charge Shot article callback. Shared engine routines still handle
animation commands, collision, attacks, hitlag, sound and effect dispatch.

| Path | Authored behavior |
| --- | --- |
| Start | Ground motion 343 or air motion 347; clear commands, retain stored charge, reset the interval timer and bind cleanup. Create the article on the command at the hand's bone-local offset, then attach it to the hand. |
| Charge | Ground motion 344; advance charge only when the timer exceeds its limit, choose looping sound by charge fraction, clamp at full charge and enter cancel motion 345. |
| Cancel/resume | Shield/trigger input enters cancel and removes the held article without clearing stored charge. The common escape query runs first. Next neutral-B reuses the saved charge. |
| Release | Ground motion 346 or air motion 348; signal the owned article at the command frame, sample the release joint, preserve any ordinary held item, apply air recoil, then clear stored charge and handle. |
| Ground/air transition | Preserve frame and transition flags, rebind cleanup and retain whether the move began in air. Ground charging that leaves the floor enters air release. |
| End | Return to Wait on ground. In air, enter Fall or FallSpecial according to landing lag. |
| Article hold | Animation state 0; follow owner charge and scale the model grandchild. Remove when the owner leaves the move, cancels, loses its handle or enters an action that detaches items. |
| Article flight | Select animation state 1-8 by charge; calculate speed/size in stock order, release attachment while retaining damage ownership, update velocity, decrement lifetime and remove on directional stage contact. |
| Contact/accessory | Hit, clank, absorb and shield contact remove the article; reflection and shield bounce update facing/angle. Full charge emits two root effects every three accessory callbacks. |

New reusable operations cover [bone-local world positions](scripting/fighter.md#bonesworld_position),
[live attachment and release](scripting/item.md#itemattach_to_ownerbone_index),
[effects on article joints](scripting/item.md#itemspawn_effect_on_jointeffect-path),
[stage sweeps allowing landing](scripting/item.md#itemstage_collisionstay_airborne),
and [effects taking position and facing](scripting/combat_visuals.md).
The script owns its charge counter; there is no charge-specific host API.
Integer remainder supports the recurring effect counter.

The native article stores an interpolated damage value that no code reads.
The script uses the copied DAT attack states, as the original engine does,
and omits that unused field.

## Lifecycle boundary

Load enables wall jumping, copies generic instance attribute scratch and
initializes owned handles. Load and special-attribute reload compute ten scaled
values from immutable owned parameters. Bomb movement, spawn height and morph
collision consume these values directly. Repeated reloads never scale an
already-scaled value. Primary death restores model group 0 and resets owned
shot, missile, spin and (in the composed candidate) grapple bookkeeping.

The composed damage/death2 callbacks remove Charge Shot, reset charge, clear
spin effects, remove the grapple, then clear death2, secondary-damage and
primary-damage slots. This matches `ftSs_Init_80128428` / `ftCo_800D9C98` order.
The earlier statement that stock secondary cleanup leaves those registrations
intact was incorrect. Color reset still uses owned charge.

The [lifecycle cohort](native_source_samus_lifecycle_checkpoint.md) verifies
load, attribute reload and primary-death VM bodies. Damage/grapple cleanup has
preparation proof; real resource/borrower cleanup, reflection, absorption and
full stock-loss behavior still need runtime comparisons. Samus remains partial
until those boundaries and modified grapple/throw attachments are closed.

## Runtime checkpoint - 2026-09-30

Eight 700-sample stock/script pairs match: ground and air entry, each facing
direction, mid-move turnaround and actual victim contact. Comparisons include
both ports' traced state, pose, hitboxes, hurtboxes, attachments, visibility,
item state and RNG. Contact cases require actual Zelda damage and nonzero
attacker hitlag during Screw Attack, not merely identical noncontact movement.
Left-facing cases require the correct facing during the move.

Every recording reaches FallSpecial after Screw Attack. Seven then land with
special landing lag. Ground-started turnaround travels off the stage and
catches the ledge; that recording instead requires ledge catch and ledge wait.
These end-path gates were checked against the completed recordings without
rerunning the simulation. A blanket landing gate initially rejected that
valid ledge path and was corrected in the batch runner.

The first airborne contact attempt missed Zelda in both runs and is excluded
from acceptance. The accepted case uses the generic offline airborne fixture
to put Zelda near the spinning fighter; the normal combat engine handles the
subsequent hits. Ground and air contact record 11 and 12 victim damage events.

The build and all 38 focused backend checks pass, including copied metadata,
asset imports, package roundtrips and both script syntaxes for the new generic
operation. Rook's unchanged source was rebuilt for the host API, retaining
285 own-rig clips and zero borrowed clips, and replacing her previous installed
package. There is no new Rook behavior in this checkpoint.

Evidence is in `.local/samus-spin-acceptance.json`, with accepted log hashes,
coverage gates, build/test logs and the SDK refresh receipt. The batch runs
hidden and accelerated with drawing enabled:

```powershell
python tests/native/samus_spin_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/samus-spin-new
```

## Remaining work

Translate bombs/morph movement, grapple, remaining article
callbacks, CPU decisions and remaining fighter policies into generic scripting.
Compare cross-move cleanup, incoming interruption, scaled attributes,
nondefault costumes and additional opponents. Direct pixel/audio review,
Workshop usability and rollback integration are not established by this batch.

## Charge Shot runtime checkpoint - 2026-09-30

Nine accepted 1,100-sample stock/script pairs match both ports' traced state,
pose, attacks, articles, article pose/visual digests and RNG: partial and full
charge in both directions, cancel/resume, airborne firing in both directions,
incoming damage while charging, and stored charge carried through Screw Attack
then fired. Actual victim damage is required in the contact cases: 11 for
partial charge, 25 for full charge and 18 after cancel/resume. Incoming damage
requires a hit on Samus and removal of the held article on that damage sample.
Stored-spin requires a later flying state-6 shot; full-left requires a flying
state-8 shot, exercising the full-charge accessory path without an immediate
victim hit. Full-right hits immediately and has no post-release item sample;
its 25-damage event provides the meaningful contact coverage gate.

The initial runner confused the logic-table label 108 with actual item kind
94. It rejected completed runs at the coverage gate before comparing them.
Correcting that gate and comparing existing recordings establishes the first
seven cases; a follow-up batch supplies full-left and extended stored-spin.
The earlier stored-spin recording did not fire afterward and is excluded
from the nine-case acceptance.

The build and all 38 focused backend checks pass. The article fixture checks
rig bounds, missing owners, held-slot preservation versus ordinary release,
throw-speed damage, effect paths, both stage-sweep choices, integer remainder
and missing host-operation rejection. These are mocked engine operations;
the recorded pairs supply runtime evidence. Rook's unchanged source was
rebuilt for the host API, preserving 285 own-rig clips and zero borrowed clips,
and replacing her installed package.

Evidence: `.local/samus-charge-acceptance.json`. Tests run hidden and
accelerated with drawing enabled:

```powershell
python tests/native/samus_charge_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/samus-charge-new
```

## Missile callback translation - 2026-09-30

The authored Side-B and article callbacks reproduce
[`ftsamusspecials.c`](../src/melee/ft/kinds/ftSamus/ftsamusspecials.c), the
missile accessory in
[`ftsamusspecialn.c`](../src/melee/ft/kinds/ftSamus/ftsamusspecialn.c) and
[`itsamusmissile.c`](../src/melee/it/kinds/itsamusmissile.c). They do not call
stock missile fighter handlers or article callbacks.

| Path | Script-owned decisions |
| --- | --- |
| Entry | Select normal or smash missile from horizontal-smash input age; decelerate ground/air velocity, clear cues and enter motion 349-352. |
| Accessory | Consume the article cue, increment an owned serial, sample the muzzle joint, create kind 95 with its variant value, emit the launch effect and clear the accessory hook. |
| Flight | Normal missiles query fighter then eligible item targets, steer using XY math and coast after the homing interval. Super Missiles accelerate after their delay and clamp speed. |
| Lifetime/serial | Update flight counters in stock order. Expire the oldest projectile after two newer shots unless reflected; use unsigned differences across serial wraps. |
| Explosion | Hide the model, disable common physics, initialize common expiry, stop motion, clear effects, emit the variant's explosion/sound and enter state 2 or 3. |
| Contacts | Stage contacts, hits, clanks and shield contacts choose explosion; reflection and shield bounce choose velocity, orientation and effects. |

New generic API covers [target queries](scripting/targets.md),
[XY normalization, angles, rotation and unsigned arithmetic](scripting/math.md),
[stage response and expiry](scripting/item.md#itemrespond_to_stage), and
[direct article-local signals](scripting/article.md#calling-a-local-signal).
All fighter/article tuning values are imported into named owned parameters.

Twelve 900-sample stock/script pairs match both ports' state, pose, attacks,
article core/pose/visual digests and RNG: both missile variants, ground and air,
both facings, two homing-target fixtures and two repeated-shot recordings.
Coverage checks require flight in the intended direction, the appropriate
explosion state and eventual removal. Target fixtures require nonzero vertical
steering in both directions. The two ground-right cases require victim damage;
repeat cases require three separate launches. Two follow-up pairs match after
making serial wrap arithmetic explicitly unsigned.

The first batch called the repeated-shot cases `limit`; they have been renamed
`repeat`. Their spacing does not keep three flying missiles alive together,
so they do **not** prove serial-limit expiry. Reflection, shield bounce,
target-item fallback, active-shot limits, stock loss and cross-move interruption
remain runtime coverage gaps. This is a partial Samus template.

The release build and all 38 focused backend checks pass. The article fixture
checks query arguments and missing-target clearing, XY steering, stage callback
dispatch through a shared local signal, unsigned wrap/comparison and missing
host-operation rejection. The compiler rejects recursive local signals and
aliased query outputs. These fixture checks supplement the recorded pairs;
they are not runtime proof of the uncovered combat branches.

Rook's unchanged source was rebuilt for the host API, retaining 285 own-rig
clips and zero borrowed clips and replacing her installed package. Evidence
is in `.local/samus-missile-acceptance.json`. The batch runs hidden and
accelerated with drawing enabled:

```powershell
python tests/native/samus_missile_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/samus-missile-new
```

## Bombs and morph movement - 2026-09-30

Down-B entries and callbacks (355/356), bomb-induced morph motions (341/342),
and the kind-93 article now reproduce
[`ftsamusspeciallw0.c`](../src/melee/ft/kinds/ftSamus/ftsamusspeciallw0.c),
[`ftsamusspeciallw1.c`](../src/melee/ft/kinds/ftSamus/ftsamusspeciallw1.c), and
[`itsamusbomb.c`](../src/melee/it/kinds/itsamusbomb.c) in scripts. No stock
Samus special or bomb decision callback is invoked by these translations.

| Path | Authored behavior |
| --- | --- |
| Entry/accessory | Ground/air entry, crouch-start frame, hop, command reset, own-rig muzzle sampling and bomb creation. |
| Morph | Command-driven spherical hurtbox, restore timing, air/ground movement, custom collision box, crouch exit and interrupt priority. |
| Bomb jump | Geometric owner overlap followed by action/status eligibility, clamped launch angle, shared damage cleanup and morph motion entry. |
| Article | Fuse and companion half-life initialization, model animation rate, velocity/facing, floor follow/bounce/slide, explosion, expiry and contacts. |

The reusable additions are [owner function calls](scripting/functions.md#calling-the-owner-from-an-article),
[hurtbox definition](scripting/combat_visuals.md#editable-hurtbox-capsules),
[item collision transitions](scripting/item.md#ground-following-and-transitions),
[kinetic conversion](scripting/movement.md#explicit-groundair-conversion), and
[individual shared input checks](scripting/fighter.md#individual-input-checks).
Tuning values are imported once into named owned fighter/article parameters.

Twelve 900-sample stock/script pairs match both ports' traced state, animation
pose, attack/hurtbox state, article core/pose/visual digests and RNG: ground,
air, crouch entry, drift and repeated bombs in both directions, plus two
contact fixtures. Every recording requires creation, explosion and eventual
removal. The contact recordings require damage to the opponent and show
2 damage followed by 4 explosion damage, with identical bomb-jump timing.
Ordinary stationary recordings also exercise owner overlap and motion 342.

The first draft used incorrect decimal entry IDs; the first comparison batch
exposed this before acceptance. The corrected draft then differed in its
companion half-life timer. Using the existing lifetime initializer fixes
that missing initialization, without relaxing the comparison digest.

Ground-slide state 2, reflection, shield bounce, clanks, arbitrary parameter
changes, cross-move interruptions and stock loss still need runtime coverage.
Samus remains partial because grapple, CPU and remaining lifecycle decisions
are native. The owned missile serial's primary death reset remains pending.
These recordings establish the covered paths, not complete fighter acceptance.

The release build and 38 focused backend checks pass. Compiler/fixture checks
also cover typed article-to-owner calls, missing/cleared owners, overlap gates,
stage callback dispatch, absent host dependencies and missing callback/function
rejection. Rook's unchanged source was rebuilt for the host API, retaining 285
own-rig clips and zero borrowed clips, and replacing her installed package.
Evidence: `.local/samus-bomb-acceptance.json`.

```powershell
python tests/native/samus_bomb_parity.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/samus-bomb-new
```

Tests run hidden and accelerated with drawing enabled. Kirby remains deferred.

## CPU decisions and roll checkpoint

The authored CPU policy now owns spacing, attack/grab classification,
charge-shot decisions/cancellation, aerial bomb candidacy, Screw Attack
threat classification and recovery input queues. The common target, terrain,
held-item, cooldown and probability gates remain engine primitives.
[`on cpu_special_candidate`](scripting/cpu_policies.md) supplies the angle
and distance, writes `cpu.selected_command`, and returns a Bool indicating
whether it consumed the decision, rather than a stock-kind
special decision. Tuning and charge state are ordinary owned values.

The scripted roll installs its morph hurtbox and authored collision box
through [`on roll_enter`](scripting/common_policies.md#on-roll_enter).
Comparing it exposed a native union-layout defect: two one-byte C bools
occupied the same word as the shared item-throw timer. Decrementing the timer
corrupted the morph flag. The roll flags now occupy separate words, matching
the union's intended aliasing; a regression check covers the countdown.

Accepted hidden, fast, drawn pairs:

- 1,800 samples each: Final Destination level4 and level1 CPU, Battlefield
  level9 CPU, and Screw Attack threat classification. Both ports' fighter
  state, pose, collision, damage, articles and RNG match.
- 350 samples with seed1: grounded charging/release, required charge-shot
  motion and opponent damage. Longer runs reach the unported grapple.
- 320 samples in both recovery directions: required airborne Screw Attack
  and landing match. Longer runs now pass the roll mismatch but reach grapple
  divergences at samples377/794.

The angle boundary is intentionally tested at adjacent float values: the
stock comparison uses a double literal slightly above its nearest float;
the authored float predicate accepts that nearest value to preserve the
same reachable inputs. This changes neither probabilities nor RNG calls.

Full fights are not accepted yet: grapple/throw attachments, primary-death
reset and remaining lifecycle decisions still require translation. The
candidate query has fixture coverage; the aerial bomb-selection branch
still needs a dedicated runtime recording. Evidence is under
`.local/samus-cpu-confirmed`, `.local/samus-cpu-charge-confirmed`,
`.local/samus-cpu-recovery-confirmed`, and `.local/samus-roll-layout-confirmed`.
The complete matrix reports failures honestly when it enters unported paths.

```powershell
python tests/native/samus_cpu_parity.py --iso '..\Super Smash Bros. Melee (USA) (v1.02).iso' --out-dir .local/samus-cpu-new
```

Explicit article creation preserves all requested XYZ coordinates. Samus's
stock missile and bomb constructors intentionally flatten their initial
position to the gameplay plane, so these scripts pass literal zero for Z.
This policy belongs to those constructors rather than the generic API.

Typed recipient arguments are now available; `bomb_jump` is exported with its
two Float parameters. Bomb overlap and retained control-owner dispatch remain
unconnected. See [the argument checkpoint](native_source_recipient_arguments_checkpoint.md).

Bomb overlap and retained control-owner recipient dispatch are now connected.
The 172-body audit has no body failures, but the WIP still has no independent
package acceptance. See [the overlap checkpoint](native_source_samus_overlap_checkpoint.md).
