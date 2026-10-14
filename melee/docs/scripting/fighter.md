# Fighter API

## `fighter.creation_id`

Read-only Int identifying this live fighter generation. The full 32-bit value
is retained; do not narrow it to a player slot or assume it stays positive.
Use it with [target queries](targets.md) and retained article ownership checks.
It changes when an entity is recycled and is not a persistent profile ID.

## `engine.debug_level`

Read-only Int containing the live engine debug level. Compare it with
`DebugLevel.master`, `DebugLevel.no_debug_rom`, `DebugLevel.debug_develop`,
`DebugLevel.debug_rom` or `DebugLevel.develop`. This lets a script reproduce
debug-only presentation decisions without a fighter-specific API. It is not
a character variable or a writable engine setting.

These namespaces are available in authored fighter move callbacks. The
compiler accepts operations only in phases where they make sense and reports
an error for unsupported syntax. The examples here come from
[Luigi's neutral special](../../examples/luigi_wip.fms); see
[Script structure](language.md) for where the callbacks run.

## Individual input checks

`items.held_kind` reads the ordinary held item's engine kind, or -1 when empty.
`items.throw_held(motion)` enters the common held-item throw motion selected by
the script. This is distinct from the descriptive `items.hold_kind` category.

`controls.try_side_special()`, `try_up_special()`, `try_neutral_special()`,
`try_down_special()`, `try_catch()`, `try_side_smash()`, `try_up_smash()`,
`try_down_smash()`, `try_side_tilt()`, `try_up_tilt()`, `try_down_tilt()`,
`try_jab()`, `try_spotdodge()`, `try_guard()`, `try_dash()` and
`try_air_special()`, `try_taunt()`, `try_crouch()`, `try_turn()` and `try_walk()`
all use the `controls.` prefix. Each runs
the shared input check and can **change state**; return true when it accepts
the input. Call them in the intended interrupt priority order, stopping after
success. They are not passive button queries. A script can gate the checks
without calling a stock special-move handler.

`try_guard()` attempts the shared shield entry, `try_dash()` attempts the shared
dash entry, and `try_air_special()` attempts the common airborne-special input
check. Their bool results have the same state-changing contract and must be
tested in the move's intended priority order.

`try_turn()` and `try_walk()` attempt their actual common transitions. They
differ from `controls.turn_requested()` and `controls.walk_requested()`, which
only test input eligibility. Individual attempts let a script preserve exact
interrupt ordering around its own combo or movement decisions.

`animation.id` reads the first animation-part slot's current index;
`animation.rate` reads the frame-speed multiplier. `action.category` and
`action.move_id` read legacy action metadata, separately from `motion.id`.
`action.flag(bit)` reads metadata bits 4 through 7 using a literal index. Some
stock flag meanings remain unnamed; these are exposed raw metadata, not new
fighter-specific flags. Prefer named script state for custom move decisions.

## Method summary

| Method or expression | Use |
| --- | --- |
| [`state.enter(NeutralSpecial)`](#stateenter) | Enter the move's authored motion state. |
| [`state.wait()`](#statewait-and-statefall) / [`state.fall()`](#statewait-and-statefall) | Finish into ordinary grounded or airborne action. |
| [`controls.ground_actions()`](#controlsground_actions-and-controlsair_actions) / [`controls.air_actions()`](#controlsground_actions-and-controlsair_actions) | Try standard interrupts when the subaction permits them. |
| [`controls.taunt_actions()`](#controlstaunt_actions) | Run the common taunt interrupt policy with its own interrupt gate and input order. |
| [`physics.ground_friction()`](#physicsground_friction-and-physicsair_drift) / [`physics.air_drift()`](#physicsground_friction-and-physicsair_drift) | Apply shared movement physics. |
| [`physics.air_gravity_and_friction()`](#physicsair_gravity_and_friction) | Apply the shared airborne gravity/friction step. |
| [`physics.ground_decelerate(amount)`](#physicsground_decelerate-and-physicsground_to_self) / [`physics.ground_to_self()`](#physicsground_decelerate-and-physicsground_to_self) | Decelerate grounded movement, then copy it to self velocity. |
| [`physics.fall(gravity, terminal_velocity)`](#physicsfall-and-physicsair_decel) / [`physics.air_decel(amount)`](#physicsfall-and-physicsair_decel) | Apply bounded vertical fall and horizontal air deceleration. |
| [`state.to_air()`](#stateto_air-and-stateto_ground) / [`state.to_ground()`](#stateto_air-and-stateto_ground) | Change the move's ground/air variant after collision. |
| [`state.switch_to_air(motion, flags)`](#stateswitch_to_air-and-stateswitch_to_ground) / [`state.switch_to_ground(motion, flags)`](#stateswitch_to_air-and-stateswitch_to_ground) | Switch to a particular motion while retaining the current animation frame. |
| [`state.enter_at(motion, flags, frame)`](#stateenter_at) | Enter a numbered motion with explicit preservation flags and start frame. |
| [`articles.spawn_authored_projectile(kind, bone, variant)`](#articlesspawn_authored_projectile) | Create an article owned by this script. |
| [`articles.launch_at_position()`](#articleslaunch_at_position) | Launch from an explicit world position with a facing and hit group. |
| [`bones.world_position()`](#bonesworld_position) | Read a skeleton joint position into declared float variables. |
| [`articles.launch_projectile()`](#articleslaunch_projectile) | Launch an authored article from a bone-local offset with runtime trajectory and state. |
| [`effects.spawn(effect, bone)`](#effectsspawn) | Spawn a fighter effect at a named bone. |
| [`effects.active`](#effectsspawn) | Read or set the shared effect-active flag; scripts decide when to mark a spawn. |
| [`visuals.apply_part_animation(group, variant, duration)`](#visualsapply_part_animationgroup-variant-duration) | Apply an owned partial-body animation with explicit blend duration. |
| [`hitbox.state(index)`](fighter_hitboxes.md) / [`hitbox.set_damage(index, damage)`](fighter_hitboxes.md) | Read a live attack capsule's state and recalculate its damage. |
| [`callbacks.align_velocity_to_facing()`](#callbacksalign_velocity_to_facing) | Install the common facing-aligned velocity callback. |
| [`callbacks.track_deal_damage()`](#callbackstrack_deal_damage) | Run the authored `on deal_damage` callback after a hit. |
| [`visuals.hide_group(group)`](#visualshide_group) | Hide a fighter model-part group, including during death reset. |
| [`input.horizontal_smash_age`](#inputhorizontal_smash_age) | Read the horizontal stick-smash timer. |
| [`stats.count_as_thrown_item()`](#statscount_as_thrown_item) | Mark the current action for the game's thrown-item statistic. |
| [`stats.action_count(slot, id)`](#statsaction_count-and-statsnotify_taunt) / [`stats.notify_taunt()`](#statsaction_count-and-statsnotify_taunt) | Read action bookkeeping and notify a taunt entry. |
| [`callbacks.track_death1()`](#callbackstrack_death1-and-callbacksclear_death1) / [`callbacks.clear_death1()`](#callbackstrack_death1-and-callbacksclear_death1) | Register and clear the transient damage/death cleanup callback. |
| [`state.wait_blend(frames)`](#statewait_blend) | Enter shared Wait with an explicit animation blend. |
| [`player.slot`](#player-and-down-input-history) / [`player.secondary`](#player-and-down-input-history) | Identify the player slot and secondary fighter. |
| [`input.down_held`](#player-and-down-input-history) / [`input.down_release_age`](#player-and-down-input-history) | Read down-taunt button history. |
| [`stage.event(channel, value)`](stage.md) | Query or notify the active stage's event provider. |
| [`collision.wall_contact`](#collisionwall_contact) | Test either wall after a collision update, regardless of facing. |
| [`throws.update()`](#throwsupdate) | Process the common throw's facing, release and animation-pause cues. |
| [`physics.throw_motion()`](#physicsthrow_motion) | Apply the initialized common throw's grounded or airborne movement. |
| [`facing.animation_direction`](#facinganimation_direction) | Read the direction captured when the current animation began. |
| [`on throw_finish`](#on-throw_finish) | Own the common throw's end-of-animation transition. |
| [`on throw_presentation`](capture.md#on-throw_presentation) | Initialize throw presentation after the victim's complete thrown setup. |
| [`capture.has_victim`](#held-fighter-transitions) | Test whether a fighter is still captured before changing its held pose. |

`input.attack_pressed` reads the newly pressed Attack/A button, alongside
`input.special_pressed` for Special/B. Both are edge events, rather than held
input. `input.shield_pressed` reads the engine's synthesized fresh shield-trigger
event, including analog triggers; it is not the raw physical L/R button mask.
[Guard volumes and reactions](guard.md) expose interception without
a fighter-specific handler.

Use [`controls.try_roll()`](movement.md#interrupt-checks-and-platform-dropping)
to attempt the common grounded escape before processing a move's shield-cancel
logic. [Secondary damage callbacks](combat_visuals.md#damage-callback-lifecycle)
and [terrain queries](stage.md#terrain-queries) expose shared engine behavior.

[Common action policies](common_policies.md) let the fighter own jab-chain
restart decisions, shield-entry presentation and dynamic-bone wind overrides.

Additional [combat and presentation methods](combat_visuals.md) cover live
hit groups, model scale, arbitrary motion rates, effects and reflection.

## Method descriptions

### `state.enter(...)`

Enters the chosen special motion and starts its animation. Choose the motion
name for the fighter's current grounded or airborne state:

| Special | Grounded motion | Airborne motion |
| --- | --- | --- |
| Neutral | `NeutralSpecial` | `AirNeutralSpecial` |
| Side | `SideSpecial` | `AirSideSpecial` |
| Up | `UpSpecial` | `AirUpSpecial` |
| Down | `DownSpecial` | `AirDownSpecial` |

These names select the callbacks and clip assigned to that motion.
In an airborne move handler, names such as `UpSpecial` also select its airborne
variant. Use the explicit `Air...` names in reusable helpers to make the choice
clear.

Changing
ground/air state with `physics.ground_to_air()` changes movement; enter an
airborne motion afterward to select its animation and callbacks:

```text
physics.ground_to_air()
state.enter(AirUpSpecial)
```

Numeric motion entry selects a particular action ID. The script supplies the
move's animation, interrupt, physics and collision decisions after entry.
See [motion callbacks](motion_callbacks.md) for callback order and
[fighter hitboxes](fighter_hitboxes.md) for adjusting newly created hitboxes.

### `state.wait()` and `state.fall()`

Return to ordinary wait or fall after the animation finishes. For example:

```text
on ground_animation:
    if animation.finished: state.wait()
on air_animation:
    if animation.finished: state.fall()
```

### `state.wait_blend(frames)`

Enters common grounded Wait using the engine's explicit blend operation.
`frames` is a float expression; `-1` selects the engine's default blend
duration. Stock radio taunts use this transition rather than the ordinary
[`state.wait()`](#statewait-and-statefall) entry. It does not run move-specific
logic or decide whether the fighter should be grounded.

### Player and down-input history

`player.slot` is the current fighter's player index. `player.secondary` is
true for a secondary fighter entity, such as Nana sharing Popo's player slot.
Both are read-only.

`input.up_held` and `input.z_held` are read-only bools for physical D-pad-up
and Z in the fighter's current held-button input. They are available in fighter
callbacks/helpers. D-pad-up is separate from analog up; Z is separate from
analog shield pressure. These observations leave the move's decisions in your
script.

`input.l_held` and `input.r_held` are read-only bools for the individual
physical L and R digital trigger bits. They are independent of Z and the
engine's combined logical shield bit read by `input.shield_held`. They do
not measure analog trigger pressure. Use them when the distinction between
the triggers matters, including through a retained fighter receiver:

```text
if input.l_held:
    state.timer = state.timer + 1
```

`input.down_held` tests the physical D-pad-down button in the current held
input, not downward analog-stick input. `input.down_release_age` reads the
engine's down-button release timer: `1` is the first frame after release.
Use these in a [wait-input predicate](language.md#predicate-callbacks) to
implement a short-tap action without displacing normal held-down behavior.

### `stats.action_count(...)` and `stats.notify_taunt()`

`stats.action_count(slot, id)` reads an existing engine action counter for
player slot `0..5`. It does not increment it. `id` is an integer expression
in the engine's action-counter namespace, not a motion-state ID. Fox/Falco
use `0x72` to query their radio-taunt bookkeeping across all six slots.
Out-of-range slots are invalid; use a bounded loop, as in their example.

`stats.notify_taunt()` calls common taunt-entry bookkeeping for the current
player and its primary/secondary entity. It does not enter an action or
start dialogue. The [Fox script](../../examples/fox_wip.fms) calls
it after the motion transition, preserving stock order.

### `callbacks.track_death1()` and `callbacks.clear_death1()`

`track_death1()` installs the script's `on death1` transient cleanup handler
in the engine's first damage/death callback slot. The compiler requires that
handler to exist. `clear_death1()` removes the installed handler. This is
distinct from the fighter-wide `on death` lifecycle event and the existing
`on death2` callback slot; it runs at the original engine call sites.

```text
action Cleanup:
    on death1:
        stage.event("conversation.cancel", 0)
        callbacks.clear_death1()
```

Registration is explicit so an inactive move does not cancel a stage sequence
owned by another fighter. Motion transitions can clear transient callbacks;
reinstall one when its behavior is needed, using the source move's lifetime
as the guide. See [stage events](stage.md) and
[predicate callbacks](language.md#predicate-callbacks).

### `callbacks.death1(Function)`, `death2(Function)` and `death3(Function)`

Install a named, zero-argument authored fighter function directly in the
corresponding engine cleanup slot. Use the `callbacks.` prefix for each slot;
`callbacks.death3(None)` clears the third slot, and the other two accept
`None` too. Binding does not invoke the function. These slots remain separate
from fighter-wide `on death` and retain their engine call sites and reset rules.

```text
func release_article():
    # Update declared state and remove the authored article here.
    state.article_live = false

move SideSpecial:
    on ground_start:
        callbacks.death3(release_article)
```

This form is useful when multiple phases share a named cleanup function.
The existing `callbacks.track_death1()` form instead selects `on death1`;
it is still available when that event is the intended handler.

### `state.basic_landing()`

Enter the shared basic landing transition, including its ordinary worn-item
selection. The script supplies preceding cleanup and ground/air handling.
This call performs no collision test and does not select special landing lag;
use [`state.landing_special(...)`](combat_visuals.md#statelanding_specialinterruptible-lag)
when that behavior is required.

### `controls.ground_actions()` and `controls.air_actions()`

Run the engine's ordinary action-interrupt check for the current ground or
air state. The script decides whether the subaction has enabled interrupts,
usually by reading `command(0)` first.

### `controls.taunt_actions()`

This Fighter-only, zero-argument Void operation runs the shared
`ftCo_AppealS_IASA` policy. It first requires `allow_interrupt`; then it tries
side/up/neutral/down special, grab, forward/up/down smash,
forward/up/down tilt, jab, escape and guard in the native order. The first
successful check ends the chain. This is the common taunt policy, distinct from
the general Wait/ground interrupt routine. It invokes no character taunt handler.

Checks may invoke interpreted decisions or change motion. A false check that
retires, faults or replaces the source stops the native continuation before the
next check; the binding fails without publishing a result. Valid false checks
continue normally. No runtime allocation is performed.

### `animation.clip_length(index)`

Returns the resident owned animation clip's F32 frame count. The integral index
may be computed at runtime; it must be nonnegative and inside the selected
source's exact animation table. A valid row without a resident clip returns 0.
An invalid index, float index or changed/missing table fails. The query does not
load a clip, allocate storage or select an animation from another fighter.

### `physics.clear_kinetics()`

Reset the Fighter's movement channels through the shared engine operation:
self velocity/acceleration, ground velocity/acceleration, knockback velocity,
attacker shield knockback and the additional velocity channel. This does not
change the action, animation samples or ground/air state, and does not run
callbacks or allocate memory. Use it when an authored move intentionally starts
with clean movement; preserve any channels you want to restore in script locals.

### `physics.ground_friction()` and `physics.air_drift()`

Apply the engine's common ground friction or airborne drift in the move's
physics callback. The move remains responsible for choosing which physics
function runs in each phase.

### `physics.air_gravity_and_friction()`

Applies the common airborne gravity and friction step used by several stock
specials. It corresponds to the engine's `ft_80084EEC` operation, whereas
[`physics.air_drift()`](#physicsground_friction-and-physicsair_drift) calls the
different drift operation. Luigi uses it in the charge and launch phases of
Green Missile:

```text
on state_350_physics:
    physics.air_gravity_and_friction()
```

For the grounded counterpart, use
[`physics.ground_friction()`](#physicsground_friction-and-physicsair_drift).

### `physics.ground_decelerate(amount)` and `physics.ground_to_self()`

`ground_decelerate` applies the common grounded deceleration operation to
`fighter.gr_vel`. `ground_to_self` copies the resulting grounded movement
into the fighter's self movement. Call them in that order when translating a
stock state that does both; these are distinct from
[`physics.ground_friction()`](#physicsground_friction-and-physicsair_drift).
`amount` is a float expression, commonly a named DAT
[attribute](language.md#fighter-attributes).

```text
on state_343_physics:
    physics.ground_decelerate(attrs.missile_start_decel)
    physics.ground_to_self()
```

### `physics.fall(gravity, terminal_velocity)` and `physics.air_decel(amount)`

`fall` updates vertical speed using the given gravity and terminal-velocity
limit. `air_decel` applies the common horizontal self-velocity deceleration.
Luigi's Start phase applies fall only after subaction command 0 has been set,
then decelerates horizontally on every tick:

```text
on state_349_physics:
    if command(0): physics.fall(attrs.missile_start_fall_speed, air.terminal_velocity)
    physics.air_decel(attrs.missile_start_decel)
```

`air.terminal_velocity` comes from the fighter's common attributes. End
instead calls `fall` unconditionally using its own
[`attrs.missile_end_gravity`](language.md#fighter-attributes).

### `state.to_air()` and `state.to_ground()`

Switch between the authored move's ground and air motion when collision
reports `collision.left_ground` or `collision.landed`. They do not decide when
a fighter leaves the stage or lands; the collision callback does.

### `state.switch_to_air(motion, flags)` and `state.switch_to_ground(motion, flags)`

Switch ground/air physics and enter the specified motion at the **current
animation frame**. Use these when a move has several ground/air pairs, such as
start, charge, and launch, and a collision must remain in the matching phase.
The first argument is a motion-state ID in this fighter's table. The second
argument is the engine's motion-preservation bitmask; decimal and `0x` hex
integers are accepted. Both arguments are compile-time constants. These
operations are valid in motion-state collision callbacks and require the
corresponding ground/air engine functions.

```text
action GreenMissilePhases:
    on state_344_collision:
        if collision.left_ground: state.switch_to_air(350, 0x0c4c5286)
    on state_350_collision:
        if collision.landed: state.switch_to_ground(344, 0x0c4c5286)
```

The mask above preserves Luigi's hit status, sound, graphics, and other
motion details during Green Missile charge. It comes from the stock
`transition_flags1` value and is specific to that transition; copying it to
an unrelated move can preserve the wrong data. The method itself is generic.
For a special with only one ground/air pair, prefer
[`state.to_air()` or `state.to_ground()`](#stateto_air-and-stateto_ground).
For entering a motion from its beginning, use [`state.enter(...)`](#stateenter).

### `state.enter_at(motion, flags, frame)`

Enter a numbered motion at `frame` with the given motion-preservation mask.
`motion` and `flags` are compile-time nonnegative integers; `frame` is a float
expression. The engine evaluates that frame during the change and uses normal
playback speed. This does **not** change the fighter between grounded and
airborne physics. Call `physics.ground_to_air()` first if the move needs that
change.

Green Missile enters airborne Flight at its current launch frame while
retaining graphics and hitbox state:

```text
state.enter_at(351, 0x0c4c508a, frame)
```

Use [`state.enter(...)`](#stateenter) for a fresh motion at frame zero or
[`state.switch_to_air/ground(...)`](#stateswitch_to_air-and-stateswitch_to_ground)
when collision changes the ground/air variant at the current frame.

### `state.enter_flags(motion, flags)` and `animation.start()`

Enter a numbered motion at frame zero with explicit preservation flags.
Unlike `state.enter(UpSpecial)`, this operation lets the script choose among
several entry variants. `animation.start()` then runs the shared initial
animation advancement used by stock special entries. It does not enter a
different motion or change ground/air physics.

For example, Sing chooses its left or right clip before starting animation:

```text
on special_hi:
    if facing.direction == -1: state.enter_flags(365, 0)
    else: state.enter_flags(367, 0)
    animation.start()
    command(0) = 0
```

Both arguments to `state.enter_flags` are compile-time integers. The script
owns its branch coverage; every entry path should enter the intended motion.
An independent authored entry owns this decision and its callbacks; there is no
fallback to the stock fighter's special handler.

### `callbacks.accessory(helper)`

Install an owned zero-argument, Void helper for the accessory phase. Call it
after a motion transition that clears the fighter's accessory callback. The
helper runs every eligible frame until replaced or cleared; installation does
not call it immediately. This works in both moves and numbered actions.

```text
func release_feedback():
    effects.spawn_on_part(attrs.release_effect, 0)
    callbacks.accessory(None)

func begin_release():
    state.enter_at_speed(343, 0, 1, 1)
    callbacks.accessory(release_feedback)
```

Use `callbacks.accessory(None)` to finish a one-shot helper. For a helper that
should run only when a subaction cue is set, use
[`callbacks.accessory_cue`](motion_callbacks.md). Explicit installation gives
each phase of a move its own timing rather than assuming the entry state's
accessory callback applies to every later state.


### `articles.spawn_authored_projectile(kind, bone, variant)`

Spawns `kind` at a named fighter bone using the associated
[article declaration](article.md#projectile-appearance). `variant` is an
integer expression passed to the article appearance setup. The current cue
grammar accepts `LeftHand`, `RightHand`, or `Hip` as the bone. The article's
script then owns its state and contact callbacks.

```text
on neutral_special_cue:
    articles.spawn_authored_projectile(It_Kind_Luigi_Fire, LeftHand, 0)
```

Use this for a stock-style DAT article with authored behavior. For a holdable
pickup created during a move, see [package-authored items](../native_fighter_items.md).

### `articles.launch_projectile()`

```text
articles.launch_projectile(kind, bone, offset_x, offset_y, offset_z, angle, speed, state)
```

Create a package-authored projectile with explicit runtime launch data.
In the interpreter, `kind` names an Article declaration in this fighter's
source bank, such as `CustomBolt` or `It_Kind_Fox_Laser`. Its owner is resolved
when preparing the script; runtime kind/parameter selection is not yet admitted.
`bone` is
one of the named fighter bones (`Top`, `Hip`, `LeftHand`, `RightHand`,
`RightThumb`, `LeftFoot`, `RightFoot`). The three offsets are local to that
bone; the engine transforms them to the muzzle's world position. As with
stock projectile spawning, the initial item's Z position is set to zero.

`angle` is world-space radians, normalized before spawning. Facing is
chosen from that angle; it is not multiplied by the owner's facing again.
`speed` is a finite float and `state` must name a declared article state.
Lifetime comes from the article's
[projectile declaration](article.md#projectile-appearance). Invalid launch
values or unavailable article data reject the operation. The bone-offset form
passes appearance variant zero and collision group zero; use the positional
form below when those values need to be chosen explicitly.

```text
articles.launch_projectile(It_Kind_Fox_Laser, RightThumb, 0, 1.2325, 4.2636, pi_half * 2, 5.5, 0)
```

After state, lifetime and model setup, the article's
[`on launched`](article.md#launch-initialization) callback can copy the angle,
speed and collision origin into its own named variables. There is no
fighter-specific projectile behavior inside this operation.

### `effects.spawn(effect, bone)`

Spawn a joint-only effect on a semantic bone role such as `Hip` or `LeftHand`
from an authored Fighter callback. The role is mapped through the owned rig.
The ID must be admitted by the native effect's joint-only argument signature;
see [Joint effects](joint_feedback.md). The operation does not set
`effects.active`: the script owns that shared flag separately.

```text
if not effects.active:
    effects.spawn(1290, Hip)
    effects.active = true
effects.pause_during_hitlag()
```

To defer marking effects active until the accessory phase, register an ordinary
helper through [the callback API](callback_slots.md). Registration does not run
it immediately. A motion change or `articles.stop_cue()` can clear it.

```text
func mark_effect_active():
    effects.active = true
    effects.pause_during_hitlag()

# After entering the new motion:
callbacks.accessory(mark_effect_active)
```

### `visuals.apply_part_animation(group, variant, duration)`

Immediately apply a variant from one of the fighter's five owned part-animation
groups. Group indices are 0-4; variant indices are 0-7, and the selected variant
must exist in this fighter's imported data. These are partial-body layers, not
main action clips or attachment slots.

`duration` is a Float blend duration. Zero applies the fixed pose with a zero
blend rate. A nonzero duration uses the first animated node's end frame divided
by that duration, exactly as the native operation does. It is not a playback
speed multiplier. Finite negative durations retain native semantics; nonfinite
durations or blend rates reject before changing the pose.

```text
visuals.apply_part_animation(3, 2, 0)
visuals.apply_part_animation(4, 2, 0)
```

The operation validates the selected owned group, variant, imported tree and
rig traversal before applying any bone. Missing assets or a retired source
produce a diagnostic rather than borrowing another fighter's data.

### `hitbox.state(index)` and `hitbox.set_damage(index, damage)`

`hitbox.state(index)` returns the exact native state of attack capsule `index`.
Compare it with `Hitbox.enabled` to select only enabled capsules.
`hitbox.set_damage(index, damage)` passes a numeric damage value
through the common hitbox update operation, including fighter scaling and
attack staleness. Both accept capsule indices `0` through `3`; `damage` may be
a numeric expression. The script does not create the capsule here: the
[subaction event timeline](language.md#moves-and-subactions) does that.

```text
if hitbox.state(0) == Hitbox.enabled:
    hitbox.set_damage(0, state.missile_charge * attrs.missile_damage_slope + attrs.missile_damage_base)
```

See [Fighter hitboxes](fighter_hitboxes.md) for numeric conversion, damage
scaling and invalid-index rules. Other native capsule states remain distinct.

### `callbacks.align_velocity_to_facing()`

Install the common callback that makes the horizontal ground and self
velocities follow the fighter's facing direction while preserving their
magnitudes. Green Missile installs it on entry to Flight, after setting the
launch velocity. This callback is an engine primitive; the script still owns
the launch-speed calculation.

The direct interpreter clears the current authored Reflect binding and installs
the native `ftCommon_8007F76C` function in `Fighter.x21F8`. Installation does not
change velocity immediately or invoke either callback. Later common reflection
handling chooses the installed native callback. Explicit
`callbacks.reflect(...)` or `callbacks.track_reflect()` takes over that slot
again; ordinary motion entry clears it under the existing engine rules.

This zero-argument operation is available to the current owned Fighter source.
It retains the actor, program, storage, physical slot and scene identity through
installation and allocates nothing. Article callbacks and receiver objects
cannot use it to modify another fighter.

### `callbacks.track_deal_damage()`

Install an authored `on deal_damage` callback for the current fighter. The
compiler requires a corresponding action handler. Green Missile uses it to
cancel horizontal speed and enter its airborne End state after a hit:

```text
on deal_damage:
    velocity.x = 0
    if velocity.y >= 0: velocity.y = 0
    state.enter(352, 0)
```

See [motion-state callbacks](language.md#motion-state-callbacks) for `action`
handler placement and [`state.enter_at()`](#stateenter_at) for a transition
that must retain its animation frame.

### `visuals.hide_group(group)`

Hides model-part visibility group `0` or `1` through the common fighter-part
operation. This affects the fighter's model state, not the Workshop editor's
temporary mesh visibility. Luigi's death callback hides group `0`:

```text
action FighterEvents:
    on death:
        visuals.hide_group(0)
```

The stock Luigi death handler also writes zero to a fighter-union field at
`0x2234`. No native source reader of that field was found, so the script
does not expose a fighter-specific operation solely for that write. A
stock-versus-clone death/respawn recording matches after replacing the
callback. The authored clone also explicitly clears its declared
[`state.cyclone_charge`](language.md#fighter-state) on death. See
[fighter-wide callbacks](#fighter-event-callbacks) for the other lifecycle
events.

### `input.horizontal_smash_age`

Readable and writable integer timer for the current horizontal stick-smash input. The
engine resets it to `0` on a fresh horizontal smash input, increments it
while the input is held, and uses `0xFE` when no qualifying input is active.
Luigi checks this against a DAT threshold when Green Missile starts charging.
Unlike `input.x`, this value records
input history rather than the current stick position.

A move can consume that input history explicitly with
`input.horizontal_smash_age = 0`, then let the engine advance it normally.
This is the shared input timer, so resetting it affects subsequent input tests
for this fighter; it is separate from a move's own charge or cooldown variable.
Writes accept integers from `0` through `254`, including the native inactive
sentinel `254`. Values outside that range produce a script error, with no
clamping or byte wraparound. Current stick coordinates and neighboring input
timers are unchanged.

### `stats.count_as_thrown_item()`

Sets the fighter's shared `count_thrown_items` action flag. The game uses
this flag when updating its player statistics. It does **not** spawn or
throw an item. Stock Luigi sets it when Green Missile begins with a recent
horizontal smash input; Link, Peach, and Pikachu also set the same flag in
their moves. It is a common engine operation, not a Luigi-specific counter.

```text
attribute float missile_smash_window 4

action GreenMissileSetup:
    on state_change:
        if input.horizontal_smash_age < attrs.missile_smash_window:
            stats.count_as_thrown_item()
```

This excerpt is the intended side-special setup pattern. The authored move
entry would install its `on state_change` handler before entering Start;
the compiler requires that handler when `callbacks.track_state_change()` is
used. The current Luigi clone owns this entry and its one-shot setup in
script. See [fighter attributes](language.md#fighter-attributes) for the
threshold declaration and [Luigi's translation ledger](../luigi_green_missile_translation.md)
for the callback timing.

### Common horizontal smash input values

| Expression | Value |
| --- | --- |
| `controls.horizontal_smash_threshold` | Common horizontal stick threshold. |
| `controls.horizontal_smash_window` | Common horizontal smash input window, returned as a scalar. |
| `controls.horizontal_smash_extension` | Common scalar extension used when comparing a recent smash input. |

These read immutable common engine tuning. They do not classify input or
change state. A move can compare them with `input.stick_x` and
`input.horizontal_smash_age`, retaining its own strict/inclusive comparisons
and any additional parameter-dependent window.

### `collision.wall_contact`

Read-only boolean for contact with either the left or right wall. It reads
the collision environment flags and is valid in a motion-state collision
callback. Unlike `collision.wall_ahead`, it does not use the fighter's
facing direction. Luigi's airborne Green Missile Flight checks it after
the landing test because the stock callback handles both conditions in that
order:

```text
on state_351_collision:
    if collision.landed:
        # Handle landing first.
        pass
    if collision.wall_contact:
        # Then handle either wall.
        pass
```

The snippet shows ordering, not the complete Flight callback. See
[Luigi's script](../../examples/luigi_wip.fms) for its executable transitions
and [script callbacks](language.md#motion-state-callbacks) for the surrounding
`action` block.

## Throw animation events

### `on throw_animation`

Runs in each of the four common throw animation callbacks, before the shared
victim attachment, release and end-of-motion logic. Declare it in an `action`
block. It replaces the fighter-specific throw operation; it does not replace
common throw physics, collision or victim bookkeeping. Without this callback
the existing stock operation remains in place.

```text
action ThrowEvents:
    on throw_animation:
        if motion.id == Motion.throw_up and cue.article.consume():
            # Spawn the move's authored article here.
            sound.play(110109, 127, 64)
```

### `on throw_finish`

Declare this predicate callback in an `action` block. It runs after common
throw cue bookkeeping when one of the four ordinary throw animations ends.
Return `true` after performing your own transition; return `false` to use
the common ground/air finish. A declared callback owns the finish policy,
including replacing any stock fighter's special finish decision. Without
one, the existing stock finish policy remains in place.

```text
action HoldEntry:
    on throw_finish:
        if motion.id == Motion.throw_forward and capture.has_victim:
            state.enter_at_speed(351, 0, 0, 1)
            capture.initialize_hold(Motion.held_wait)
            return true
        return false
```

This example assumes motion 351 has authored holding callbacks. Do not return
`true` without transitioning or otherwise completing the ended animation.
[`on throw_animation`](#on-throw_animation) still owns any per-frame fighter
operation; it is a separate event.

### `on heavy_pickup_finish`

Declare this predicate callback in an `action` block. It runs when the common
heavy-item pickup animation ends while an item is held. Return `true` after
entering your authored carrying state. Return `false` to enter the common
heavy-item wait state. A declared callback replaces the stock fighter's
pickup-finish decision; without one, stock behavior is retained.

```text
action ItemEntry:
    on heavy_pickup_finish:
        state.enter_at_speed(Motion.carry_idle, 0, 0, 1)
        return true
```

The common engine still attaches the item to the fighter's heavy-item bone
and processes pickup cues. This event owns the transition after pickup,
without delegating to a stock fighter's carrying handler. Author the carrying
state's callbacks too. Use
[`controls.try_heavy_item_throw()`](movement.md#interrupt-checks-and-platform-dropping)
where the carrying state permits ordinary item throws.

### `on heavy_throw_enter`

Runs after the common heavy-item throw motion, accessory callback and damage
callback have been installed, before its initial release-cue check. Declare it
in an `action` block. It owns the fighter's override of common throw movement;
the engine retains ordinary item-release bookkeeping. Use [callback binding](functions.md#binding-a-motion-callback)
to install authored physics or collision helpers. Without this event, stock
movement policy remains in place.

### `on heavy_item_damage`

Runs when common damage handling reaches a fighter holding a heavy item,
after its preceding capture-specific decisions. A declared event owns whether
to keep the item and enter a holding reaction or release it and take normal
damage. It does not run for a light item or every damage event.

```text
on heavy_item_damage:
    if damage.reaction_suppressed or damage.knockback_level < 3:
        damage.enter(Motion.carry_damage, 0)
    else:
        items.release_held(0, 0, 0, 1)
        damage.default_reaction(0)
```

This demonstrates a policy, not a mandatory rule for custom fighters. Author
the selected holding-damage state's callbacks as well.

| Operation | Meaning |
| --- | --- |
| `damage.reaction_suppressed` | Read the common zero-knockback/armor suppression test. It does not consume damage. |
| `damage.knockback_level` | Read the current knockback's common level, 0–3, after the engine's common knockback scaling. |
| `damage.enter(motion, facing)` | Initialize the shared damage reaction with the specified motion and facing argument. |
| `damage.default_reaction(facing)` | Let the common engine choose the normal damage motion. |
| `items.release_held(vx, vy, vz, multiplier)` | Release the held item with the given world velocity and the common item damage multiplier. Does nothing without a held item. |

Interpreter migration status: both reaction observations, `damage.enter` and
`damage.default_reaction` and `items.release_held` are available. See
[held-item controls](held_item_controls.md) for immediate release versus throw
motion entry, and [common reaction entry](combat_visuals.md#common-reaction-entry)
for the implemented semantics.

The facing argument is passed to the shared damage routine; zero preserves its
normal direction decision. These calls do not redefine damage, knockback or
hitlag calculations.

### Held fighter transitions

For attached command grabs and editable mash/escape settings, see
[Attached captures](capture.md). Those operations are distinct from shouldered
carry and ordinary throw initialization below.

`capture.has_victim` is true while the fighter has a live captured victim.
The following operations do nothing if there is none:

| Method | Effect |
| --- | --- |
| `capture.initialize_hold(motion)` | Initialize the victim's common shouldered/held state and select its pose. |
| `capture.set_victim_hold_motion(motion)` | Change the already held victim's pose. |
| `capture.set_victim_throw_motion(motion, rate)` | Initialize a thrown-victim animation with the supplied playback rate. |
| `capture.sync_victim_facing()` | Copy the holder's current facing onto the victim. |

All methods in this table have direct interpreter bindings to common engine
operations. [Attached capture and throw motion entry](capture.md#victim-motion-entry)
is also interpreted. Capture motion restarts a capture pose without reinitializing
the hold timer; attached throw entry accepts explicit flags and a starting frame.

These use shared capture bookkeeping, without choosing the holder's move,
timing or direction. `Motion.held_wait`, `Motion.held_turn` and
`Motion.held_throw_forward` name the common victim motions. The stock walk
variants and directional thrown variants are consecutive motions; the
[cargo example](../../examples/donkey_wip.fms) explicitly selects them.
[`throws.update()`](#throwsupdate) can release the victim, so check capture
existence again after it when necessary.

`damage.in_hitstun` reads the common hitstun flag. `damage.update_timer()`
runs the common damage-duration update, and `physics.damage_motion()` runs
common damage movement. They do not select a fighter-specific recovery or
release policy; an authored holding-damage state makes those decisions.

Both operations take no arguments and operate on the executing owned Fighter.
`damage.update_timer()` decrements a positive damage-duration counter by one.
When that counter reaches zero or less, it clears hitstun, initializes the
common post-hitstun recovery timer, calls `on knockback_exit`, and updates hammer
recovery when holding a hammer. Retiring the actor or its program in that
callback prevents the remaining recovery work and following script statements.
This is the damage-duration counter, not the hitlag timer.

`physics.damage_motion()` selects shared movement by ground/air and hitstun.
Airborne hitstun applies ordinary gravity, terminal-speed clamping and aerial
friction. After hitstun it uses normal airborne movement; grounded damage uses
ground friction and floor-slope movement. These operations preserve the engine's
existing velocity arithmetic rather than choosing a fighter-specific recovery
move. Their actor, program, storage and parameter identities must remain current
before a result can be published.

### `cue.article.consume()`

Read and clear the article cue bit emitted by throw subactions. It returns
true once per emitted cue. This differs from `cue.throw.consume()`, which
consumes the victim-release cue, and `cue.reverse.consume()`, which consumes
the facing-reversal cue. None of these seeks or advances the animation.
See [Fox/Falco's translation](../fox_falco_script_translation.md) for an
example that fires along an animated bone.

### `items.notify_pickup(flag)`

Run the common pickup notification at the current action cue, passing the
boolean flag to the engine. This updates the held-item callback/statistics
when a held item exists, or the existing player statistic otherwise. It
does not spawn, equip, remove or destroy an article. For owned attachments,
use the [article API](article.md).

### `throws.update()`

This operation has a direct interpreter binding to the shared throw cue processor.
Both captured actors remain retained across the release and animation-pause paths;
successful release deliberately removes their capture links. A replaced relation
or retired actor/program stops the common continuation and reports a script error.

Process the common throw's subaction cues: reverse facing, release the captured
fighter using the live throw capsule, or pause the thrower/victim animation for
animation-driven travel. This uses the existing shared throw bookkeeping and
statistics. It does not enter a throw, choose its direction, spawn an article,
or decide what state to enter when the animation ends.

Use it in an authored animation callback for a properly initialized common
throw. The operation may release the victim; do not assume that a capture
still exists afterwards. Its release cue is bit 3, which differs from the
[special capture's bit-1 cue](combat_visuals.md#cueconsumebit). Do not consume
the same cue separately before calling this operation.

```text
func throw_tick():
    throws.update()
    if animation.finished: state.wait()
```

### `physics.throw_motion()`

This operation has a direct interpreter binding to the common throw movement
routine. It changes only the thrower's movement; it neither selects a victim's
motion nor performs a throw release.

Apply shared throw movement. Grounded throws use ordinary animated ground
motion. Airborne throws use the common air-motion step, except during an
initialized throw's movement pause, when its saved animation delta drives
velocity. This reads the common throw engine state initialized by throw entry;
it is not a general replacement for air drift or gravity.

Cargo's airborne throw motions instead use ordinary `physics.air_drift()`.
The script chooses that policy in its physics callback. See
[DK's partial template](../donkey_script_translation.md) for both paths.

### `facing.animation_direction`

Read the direction saved when the current animation began. `facing.direction`
is the live gameplay direction and can change later, for example at a back
throw's reversal cue. The saved direction is read-only.

When switching between paired throw animations, a script may temporarily use
the saved direction for the state change, then restore the live direction.
This preserves the original animation orientation without undoing the throw's
gameplay turnaround. The state change itself updates the animation direction.

## Fighter event callbacks

### `on throw_victim_motion:`

An integer-valued query before the common throw initializes the victim pose.
`event.motion` is the pending holder throw, and `event.victim_motion` supplies
the common engine's default victim motion. `motion.id` still describes the
holder's previous state because initialization has not happened yet. Return
`event.victim_motion` when no alternate pose is needed. The engine accepts common victim motion IDs and retains the default
for an out-of-range result. The callback does not own throw damage, weight-based
playback speed or release timing.

`capture.victim_animation_family` reads the victim's animation compatibility
tag, or `AnimationFamily.none` when no victim exists. For an installed fighter this
comes from its own definition's animation source tag; it is asset metadata,
not a clone ID, an inherited behavior choice or a call to a donor handler.
`AnimationFamily.peach` and `AnimationFamily.zelda` name two stock asset tags.
`Motion.thrown_down_alternate` names their existing common down-throw pose.

This read-only Int query now executes directly in the ordinary interpreter.
It retains the source and the current capture relation before reading the
victim's own asset tag. Stock victims report their native kind; installed
scripted victims report their independently owned animation tag, even when
that tag differs from their runtime kind. Missing victims return
`AnimationFamily.none`; retired or inconsistent relations reject the read.

```text
on throw_victim_motion:
    if event.motion == Motion.throw_down:
        if capture.victim_animation_family == AnimationFamily.zelda:
            return Motion.thrown_down_alternate
    return event.victim_motion
```

That example chooses one stock-compatible pose. A custom fighter may use a
different rule or retain the default. Keep this query free of RNG, state writes
and transitions; it selects the pose once, before initialization.

### `damage.reverse_knockdown_pose`

Writable boolean which reverses the common floor-bound up/down pose selection
derived from the hip joint orientation. Configure it in `on load` for a rig
whose orientation requires that convention. It changes the pose choice, not
gravity, hitstun or knockback. Bowser and Yoshi set the corresponding stock flag.

The ordinary interpreter reads and writes this flag on the current owned
fighter. It does not replace the common knockdown calculations or require a
fighter-specific scripting operation.

An `action` block can author fighter-wide events independently of the four
special moves. Luigi currently scripts the events below, alongside all four
special moves:

```text
action FighterEvents:
    on load:
        fighter.copy_ext_attributes()
    on load_special_attrs:
        fighter.reload_ext_attributes()
    on death:
        visuals.hide_group(0)
    on item_pickup:
        fighter.item_pickup(event.flag, 1, 1)
    on knockback_enter:
        fighter.knockback_enter(1)
```

`on load` copies the fighter's extended DAT attributes into its live state.
`on load_special_attrs` refreshes that copy. Item pickup/drop callbacks receive
the event's original boolean `event.flag` and pass the stock item-visibility
arguments to the shared fighter operation. The `*_ext` pickup/drop callbacks
cover the corresponding extended event slots. `on item_visible` and
`on item_invisible` call the matching shared visibility operations.
`on knockback_enter` and `on knockback_exit` call the shared model response
with the fighter's existing argument.
`on death` runs Luigi's model-group reset before respawn.

`on landing` fires on the transition from air to ground, including transitions
that keep a special move active. `on landing_action` instead fires after the
common engine enters its landing or special-landing action and initializes
that action. Its read-only Bool `event.interrupt_allowed` captures the entry's
interrupt option. The latter is useful for resetting a move resource at the same
point as stock Mario's Tornado and Cape flags. Landing inside a special move
can reset a different set of variables in that move's collision callback.
Both events are optional and operate on ordinary declared fighter state.

These operations are common engine primitives; the event block selects which
ones a clone uses. [Luigi's source](../../examples/luigi_wip.fms) contains the
complete current event block. A stock-versus-clone recording verifies that
Luigi is damaged and enters knockback with this block installed. Another
recording reaches death, respawn, and post-respawn Fireball. Separate
recordings are still needed for pickup, drop, and visibility events.

## See also

- [Script structure and callbacks](language.md)
- [Article declaration](article.md)
- [Item API](item.md)

## Costume attachments and item poses

See [Costume model attachments](model_attachments.md) for `attachments.has_root`,
`attachments.load`, `attachments.add_dynamics`, `attachments.clear_all`,
`costume.index`, `items.heavy`, `items.hold_kind`, and the saved part-animation
selection/show/hide operations. These are shared engine operations; no fighter
union or named special-move state is exposed.

### `articles.spawn_bone_attached(kind, bone)`

Create an authored article attached to a semantic fighter bone, such as
`RightThumb`. Returns its creation-number handle, or `-1` when creation fails.
`kind` may be an integer expression, including an owned package parameter.
The article's [attachment declaration](article.md#attachment-creation) selects
initial position, collision, and special held-item registration.

```text
state.gun_handle = articles.spawn_bone_attached(attrs.gun_kind, RightThumb)
```

Store the handle in ordinary named fighter state. Check `articles.owned(handle)`
before further work when a previous callback could have removed the article.
This does not invoke a stock fighter's item creation handler.

### `articles.signal_owned(handle, DeclaredArticle, "name", value)`

Send a named signal to a live article attached to this fighter. `handle` is its
item creation number, usually saved from an article factory. `DeclaredArticle`
names the exact article block, just as it does for `articles.signal`.
`value` is a finite float expression. The name is a quoted identifier declared
by an [`on signal` article callback](article.md#signals-from-the-fighter).

```text
articles.signal_owned(state.accessory_handle, MyAccessory, "recoil", 1)
articles.signal_owned(state.accessory_handle, MyAccessory, "visibility", command(1))
```

This returns Bool and may also be used as a statement. It executes synchronously;
both the live attachment owner and current damage owner must be this fighter.
The recipient must use the same script bank and exact declared article. Missing,
detached, reflected or mismatched recipients return false; unknown names are
loading errors. A callback that removes its recipient still counts as delivered.
A recipient fault or caller retirement stops the calling script. Names resolve
at loading, with no per-frame string lookup. It never selects a stock handler or reaches
into a stock item's private union. For mutable values inside the same fighter,
continue to use ordinary `state.NAME` variables; signals are for communication
across the fighter/article boundary.

### `articles.signal(handle, DeclaredArticle, "name", value)`

Available in fighter and article callbacks. Articles can signal a linked peer
with `articles.signal(item.link_id(slot), DeclaredArticle, "name", value)`;
see [linked articles](article_links.md). Ownership-restricted
`articles.signal_owned()` remains fighter-only.


Dispatch a declared Article's signal from the owning fighter's script bank.
The direct interpreter currently supports Fighter callers for this targeted
operation; Article callbacks use `item.signal` for self delivery. Unlike
[`signal_owned`](#articlessignal_owned),
this does not require the recipient's current damage owner to be the caller.
Reflection can transfer damage ownership while the original script still needs
to notify its projectile. The recipient must have the exact declared Article
program and live owned storage in the same bank. Foreign programs, a different
Article declaration, and stock item handlers cannot receive this signal.

The handle is a full 32-bit integer creation ID. Its high bit is preserved;
`uint(0xFFFFFFFF)` is the absent-handle sentinel. Fractional handles are compile
errors. `name` must be a declared quoted identifier, resolved when loading the
script. Runtime delivery never hashes or parses the name. Nonfinite targeted
payloads return false, following native targeted delivery.
Signals execute synchronously with bounded dispatch depth. The compiler checks
[caller lifetime](synchronous_callbacks.md) before continuing after a signal,
because either participant may be removed or reused by the callback. This is a
Bool expression: true after entering its handler, including when that handler
removes the recipient; false when a live caller has no matching recipient.
Handler faults or caller retirement abort execution instead of returning false.
No new mutable pointers or private fighter fields are exposed.

```text
articles.signal(state.projectile, It_Kind_Zelda_DinFire, "detach_control", 0)
```

### `articles.spawn(kind, state)`

Create an unattached, owned article at the fighter's current position and
facing. Returns its creation-number handle, or `-1` if the kind is not authored
by this package, its resources/state are unavailable, or creation fails.
The script supplies the initial item state; the engine creates the item,
selects that state, delivers [`on created`](article.md#unattached-creation-and-model-instances),
and runs its initial update. An update may immediately remove the item, so
use `articles.owned(handle)` before later operations.

```text
state.trail_handle = articles.spawn(attrs.trail_kind, 0)
```

### Facing-directed ground and ledge checks

Run `collision.check_ground_ledge(direction)` with `-1` for left and `1` for
right. To follow the fighter's current facing, use:

```text
collision.check_ground_ledge(1 - 2 * int(facing.direction < 0))
```

Returns true on ground contact. If false, the script can
call `collision.cliff_catch()` to attempt the common ledge transition. This
query does not choose a fighter-specific landing or recovery state.

## bones.world_position

`bones.world_position(index: int, x: float output, y: float output, z: float output [, offset_x: float, offset_y: float, offset_z: float])`

Reads the indexed joint's world position into three distinct declared float
variables: local variables, `state.NAME` fields, or a mixture of both.
The index refers to the fighter package's skeleton, rather
than a stock fighter's named-part table. Invalid indices write zero. This
performs one joint calculation using stack storage.

```text
bones.world_position(48, state.muzzle_x, state.muzzle_y, state.muzzle_z)
state.muzzle_y = state.muzzle_y + 2 * model.instance_scale_y
```

With four arguments, this samples the joint origin. The optional three
offsets specify a point in the bone's local coordinates, transformed into
world coordinates by the joint matrix. Supply all three together:

```text
bones.world_position(attrs.hand_joint, state.muzzle_x, state.muzzle_y, state.muzzle_z, 0, 0, 4)
```

Local offsets rotate and scale with the joint. For a world-axis offset,
sample the origin first, then add to the output variables. The three output
arguments must be distinct float variables in either form; arrays and arbitrary
expressions are not output destinations. The index must be an integer;
`bones.index(Top)` resolves a semantic bone role to that integer. Inputs are
evaluated once, in index then X/Y/Z offset order. Missing joints also write zero.

Use locals when the position is only needed during this callback:

```text
let float x = 0
let float y = 0
let float z = 0
bones.world_position(bones.index(Top), x, y, z)
```

## articles.launch_at_position

`articles.launch_at_position(kind: ItemKind, x: float, y: float, z: float, angle: float, speed: float, state: int, variant: int, hit_group: int, facing: float)`

Creates an [authored projectile](article.md#launch-initialization) at the
specified world position. The explicit facing is preserved independently of
the world-space launch angle. The descriptor still decides whether initial
velocity/update and automatic lifetime are applied. If initial velocity is
enabled, angle uses the usual cosine-X/sine-Y basis. An article can instead
initialize and update its own velocity in callbacks.

The interpreter resolves `kind` to an Article declaration in this fighter's
source bank at preparation time. It does not accept a runtime item-kind variable
or another fighter's Article. The supplied XYZ position is the native constructor's
previous position (with Z set to zero); its current input position is the fighter's
ECB center. This preserves the shared projectile constructor contract. Created
and Launched hooks may then place the authored projectile as its behavior needs.

`variant` selects the descriptor's appearance entry; it has no visual effect
when appearance is `-1`. `hit_group` supplies the engine hit-group identifier
for grouping projectile hits. Obtain a fresh ID with `articles.new_collision_group()`;
store it in a declared `state int` and choose when to reuse or replace it. The
engine allocates no fighter-specific counters or charges. Signed high-bit IDs
retain all 32 collision-group bits; Float group values are rejected.

```text
state.hit_group = articles.new_collision_group()
articles.launch_at_position(It_Kind_Koopa_Flame, state.muzzle_x, state.muzzle_y, state.muzzle_z, 0, 0, 0, 0, state.hit_group, facing.direction)
```

This example needs an owned Flame article declaration. Only kinds bound to
the fighter package can be created. Invalid states, nonfinite launch values,
or missing article assets are rejected by the common constructor.

## damage.armor

`damage.armor: float` is a readable and writable shared damage-armor threshold.
Set it from an [owned parameter](language.md#fighter-attributes) in `on death`
or another appropriate lifecycle callback. The engine evaluates armor; the
script decides its threshold and when it changes.

## callbacks.track_death3

`callbacks.track_death3()` installs the script's `on death2` handler in the
engine's additional death-cleanup callback slot. The script owns its cleanup
policy; registering this callback does not clear variables immediately.
A move can register the same authored cleanup in both slots using
`callbacks.track_death2()` and `callbacks.track_death3()` when it needs both
engine lifecycle points. The compiler requires a declared `on death2` handler.

## `controls.interrupt_allowed`

Read and write the Bool IASA flag consumed by normal action interrupts.
Action subcommands can update the same flag during playback. A script can
clear it on entry and test it before trying normal ground actions; changing
the flag does not itself leave the current move. The interpreter reads and writes
the native Fighter IASA bit directly; this operation is Fighter-only.

## `input.attack_held`

Read-only Bool indicating that the Attack/GameCube A input is currently held.
Use a pressed-input query for a new press; use this property for continuing
charge or hold behavior.

## `damage.song_immune`

Read and write the shared Bool that prevents the common Song damage reaction.
It is independent of capture state. Set or clear it explicitly when the move
requires that immunity; the property neither enters a state nor installs a
damage callback.

## Previous vertical input

`input.previous_y` is a read-only number containing the previous sampled
vertical main-stick value. It accompanies `input.previous_x`; scripts can
compare consecutive samples without storing their own input history.

## `state.finish()`

Finish the current action with the shared engine policy: enter Fall if airborne,
or the common grounded idle entry otherwise. It takes no arguments and does not
test whether the animation has finished; the script makes that decision.
This uses the grounded finish entry, which is distinct from an explicit
`state.wait()` request. Actor/program/storage retirement during entry stops
the following script work.

```text
if animation.finished:
    state.finish()
```

## `facing.animation_direction`

Read-only Float holding the orientation recorded for the current animation.
It is separate from `facing.direction`, which can change during a move. For
example, a throw may temporarily restore the animation-facing value while
changing motion, then restore its logical facing. Reading this property does
not rotate the fighter or change either facing value.
