# Movement and collision API

Fighter scripts use these engine operations to turn subaction commands and
controller input into movement. They are available in the relevant
[`move` callbacks](language.md#moves-and-subactions). The same operations
work for any fighter; named [`attrs`](language.md#fighter-attributes) read the
active fighter's owned parameters (or transitional stock import declarations).

The [shared fighter reference](common_fighter.md) covers threshold-facing,
ground/air lifecycle operations, platform skipping and common collision/model
observations implemented by the interpreter.

`position.delta.x/y/z` reads the engine's observed displacement for the current
tick. It differs from self velocity: collision, knockback and other movement
can contribute to displacement. These properties are read-only; for example,
`position.delta.y < 0` tests actual downward movement.

## Common locomotion attributes

These read-only Float properties belong to the current physical fighter. They
read its common attributes, including changes made in the Workshop; they do not
look up the stock fighter it was copied from or access special-attribute offsets.

| Property | Meaning |
| --- | --- |
| `ground.walk_acceleration_multiplier` | Stick multiplier for walking acceleration (`walk_accel_mul`). |
| `ground.walk_max_speed` | Walking horizontal speed limit (`walk_max_vel`). |
| `air.ground_jump_momentum_multiplier` | Horizontal momentum multiplier for a ground jump (`ground_to_air_jump_momentum_multiplier`). |
| `air.jump_horizontal_max` | Horizontal jump speed limit (`jump_h_max_velocity`). |

Use these as inputs to the [shared movement operations](common_fighter.md),
for example when a special move modifies ordinary walking or jumping physics:

```text
physics.ground_accel(ground.walk_acceleration_multiplier * attrs.mobility, ground.walk_max_speed * attrs.speed_limit)
```

Reading these properties does not apply acceleration or change ground/air state.
`air.ground_jump_momentum_multiplier` differs from
`air.jump_horizontal_multiplier`, which describes an **air** jump's horizontal
multiplier.

## `jump.update_animation()`

Advances the common aerial-jump animation policy. If no active joint has an
animation remaining, it enters FallAerial and initializes the common fall
selector. It queries the normal or blended pose joints according to the
fighter's current animation blend. This operation does not tick skeletal
animation, apply physics, process input or play a jump sound.

Call it from an animation callback when you want this common completion rule;
inspect `motion.id` afterward because it can change motion. The writable
[`motion.fall_animation`](common_policies.md#motionfall_animation) selector lets
an authored callback continue the common fall-blending policy. An owned costume
must supply each active pose joint. Actor, program or script-storage retirement
during the transition stops the callback before further script work.

## `jump.begin()`

`jump.begin(motion, x, y, z, reset_input_timer, animation_driven, worn_item_sound)`
enters an aerial-jump motion with the common nametag policy, sets initial
self velocity, enables the jump flag, increments jumps used, clears the
shared jump sound state and plays the fighter's jump sound. `reset_input_timer`
sets the upward-stick age to the common consumed value. `worn_item_sound`
permits the additional worn/head-item jump sound when the corresponding item is
present; the script decides this policy, independently of fighter kind.

When `animation_driven` is true, declare `on animation_driven_jump_physics:`.
After the state change, bookkeeping and sounds, entry installs that prepared
script callback as the current physics phase. The shared native animated-jump
behavior is reproduced by the two operations shown below: normal air drift,
then animation-driven vertical velocity including the worn-item modifier.
You can author a different policy using the same generic operations. A missing
hook rejects entry before changing motion. When false, the target motion's
ordinary physics phase is retained. This method does not convert grounded
velocity or choose a motion.

```text
action AirJump:
    on aerial_jump:
        physics.ground_to_air()
        command(0) = 1
        let motion = 27
        if input.x * facing.direction <= -controls.aerial_jump_reverse_threshold:
            motion = 28
        jump.begin(motion, input.x * air.jump_horizontal_multiplier, 0, 0, true, true, false)
    on animation_driven_jump_physics:
        physics.air_accel_normal()
        physics.animation_vertical_velocity_adjusted()
```

`air.jump_horizontal_multiplier` reads this fighter's common aerial-jump
horizontal multiplier. `controls.aerial_jump_reverse_threshold` reads the
engine's common `p_ftCommonData->x78` value unchanged. Native direction tests
compare `input.x * facing.direction > -controls.aerial_jump_reverse_threshold`;
equality selects the backward branch. The property does not negate, clamp or
normalize the stored threshold.
The example selects common forward/back aerial-jump motions; custom fighters
may choose their own valid motions. See [`on aerial_jump`](common_policies.md#on-aerial_jump).

The override uses the same interpreted phase IDs as motion callbacks. It survives
snapshot restoration, is replaced by a subsequent motion selection, and can be
explicitly replaced with `callbacks.physics(helper)` or cleared with
`callbacks.physics(None)`. It never installs a stock fighter handler.

## Explicit ground/air conversion

`physics.ground_to_air()` and `physics.switch_to_ground()` run the shared
lifecycle operations. They do not choose a motion or advance animation. Follow them with `state.enter_at_speed(...)` when a move owns its transition.
`physics.ground_to_air()` uses `ftCommon_8007D5D4`: it sets airborne mode,
clears ground velocity, shield knockback Z, position Z and self-acceleration Y,
sets jumps used to one, and locks the ECB for ten ticks. It sets this baseline rather than consuming all
jumps; `physics.force_air_and_use_jumps()` has a separate all-jumps policy.
`physics.switch_to_ground()` uses the shared `ftCommon_8007D7FC` landing path,
including its owned landing callback. Neither operation chooses a motion.

`collision.ground_with_nudge()` runs `ft_80084280`. A nonzero player nudge
against the fighter's facing direction uses ordinary ground collision; otherwise
it uses the nudge-aware probe with facing and stick X. Supported ground ends the
operation. Without support it tries edge balancing, then enters common Fall if
edge balancing also declines. Native source retirement or fault stops further
responses and result publication. Both methods are Fighter-only, zero-argument
Void operations over the selected source's owned motion/layout data.

`state.crouch_wait()` enters the shared crouching idle state.

`Transition.common_ground_air` preserves the standard ground/air command and
presentation policy. Combine integer flags with `|`: `Transition.keep_hurt_status`,
`Transition.skip_hitboxes` and `Transition.skip_model` preserve their respective
state-change behavior. These flags control the transition routine; they do not
disable drawing or attack processing. Use them deliberately rather than
assuming a timestamp alone preserves animation/subaction progress.

| Expression | Common fighter attribute |
| --- | --- |
| `ground.walk_acceleration` | Walking stick-acceleration multiplier. |
| `ground.walk_speed` | Maximum walking velocity. |
| `ground.mid_walk_point` | Read-only middle-walk velocity reference from the fighter's common attributes. |
| `air.jump_momentum` | Ground-to-air jump momentum multiplier. |
| `air.jump_horizontal_speed` | Maximum horizontal jump velocity. |
| `air.jump_horizontal_multiplier` | Fighter common `air_jump_h_multiplier` used to scale stick X for aerial jumps. |
| `controls.aerial_jump_reverse_threshold` | Common `x78`; compare facing-relative stick X against its negative. |
| `controls.tap_jump_threshold` | Common upward-stick threshold for tap jump. |
| `controls.aerial_action_down_threshold` | Common `x88`; compare stick Y against its negative for a downward aerial-action request. |

The four jump observations above are Fighter-only read-only F32 values. The
three control thresholds require live common data; missing data fails without
publishing a result. The horizontal multiplier belongs to the Fighter and does
not require common data. Reads preserve native values, including signed zero
and nonfinite bits; they neither consume input nor decide jump eligibility.
A retired source fails before publication.

These values can be multiplied by owned move parameters to author movement
without accessing the stock fighter's private attributes.

## Method summary

| Method or expression | Use |
| --- | --- |
| [`physics.ground_animation_velocity()` / `physics.air_animation_motion()`](#animation-driven-movement) | Choose the common TransN-driven travel path. |
| [`state.freefall(mode_a, mode_b, allow_interrupt, mobility, landing_lag)`](#statefreefall) | Finish an aerial special in common helpless fall. |
| [`motion.launch_angle`](#motionlaunch_angle) | Read or set the angle used by an animation-driven launch. |
| [`cue.reverse.consume()`](#cuereverseconsume) | Consume the subaction's turnaround cue. |
| [`facing.from_input()`](#facingfrom_input-and-bonesrootrotate_y) / [`bones.root.rotate_y(angle)`](#facingfrom_input-and-bonesrootrotate_y) | Apply a permitted turnaround and rotate the model root. |
| [`physics.ground_motion()`](#physicsground_motion-and-physicsair_motion) / [`physics.air_motion()`](#physicsground_motion-and-physicsair_motion) | Run common animation-driven ground or air movement. |
| [`motion.scale_velocity(factor)`](#motionscale_velocity) | Multiply all three self-velocity components. |
| [`physics.quick_air_decel()`](#physicsquick_air_decel) | Apply common quick aerial deceleration. |
| [`physics.air_decel_normal()`](#physicsair_decel_normal) | Apply ordinary aerial self-acceleration/deceleration without gravity. |
| [`physics.clear_kinetics()`](#physicsclear_kinetics) | Clear common movement and knockback vectors explicitly. |
| [`animation.clear_motion_samples()`](#animationclear_motion_samples) | Reset sampled root movement without changing animation playback. |
| [`facing.from_input_threshold()`](#facingfrom_input_threshold) | Turn only above the common horizontal input threshold. |
| [`collision.check_ground_ledge(direction)`](#collisioncheck_ground_ledgedirection) | Run the common ground/ledge check using a direction expression. |
| [`sqrt(value)`](#sqrtvalue) | Compute a single-precision vector magnitude component. |
| [`animation.delta.x/y/z`](#animation-root-motion-deltas) | Read the current TransN animation movement delta. |
| [`air.gravity`](#airgravity) | Read the fighter's ordinary gravity attribute. |
| [`physics.air_accel_friction(threshold, acceleration, speed_limit)`](#physicsair_accel_friction) | Apply stick drift with ordinary aerial friction. |
| [`physics.ascend(tap_speed, tap_gravity)`](#physicsascend) | Apply the common button-tap ascent step. |
| [`physics.ground_accel(stick_scale, speed_limit)`](#physicsground_accel-and-physicsair_accel) / [`physics.air_accel(stick_scale, speed_limit)`](#physicsground_accel-and-physicsair_accel) | Accelerate toward stick input with a move-selected speed limit. |
| [`physics.clamp_horizontal_speed(limit)`](#physicsclamp_horizontal_speed-physicsclamp_ground_speed-and-physicsclamp_fall_speed) / [`physics.clamp_ground_speed(limit)`](#physicsclamp_horizontal_speed-physicsclamp_ground_speed-and-physicsclamp_fall_speed) / [`physics.clamp_fall_speed(limit)`](#physicsclamp_horizontal_speed-physicsclamp_ground_speed-and-physicsclamp_fall_speed) | Clamp a selected velocity component after a move transition. |
| [`collision.air_standard()`](#collisionair_standard-and-collisionground_standard) / [`collision.ground_standard()`](#collisionair_standard-and-collisionground_standard) | Run ordinary air or ground collision. |
| [`collision.ground_box(...)`](#collisionground_box-and-collisionair_box) / [`collision.air_box(...)`](#collisionground_box-and-collisionair_box) | Test a custom ground or air collision box. |
| [`collision.air_special_landing(allow_interrupt, landing_lag)`](#collisionair_special_landing) | Check ledges and enter special landing when appropriate. |
| [`vectors.angle_xy(ax, ay, bx, by)`](#vectorsangle_xyax-ay-bx-by) | Measure the unsigned angle between two XY vectors. |
| [`collision.skip_soft_floor()`](#collisionskip_soft_floor) | Mark a contacted pass-through platform as skipped. |
| [`collision.floor_contact` / `collision.ceiling_contact`](#surface-contacts-and-normals) | Inspect the last collision result. |
| [`physics.animation_vertical_velocity()`](#physicsanimation_vertical_velocity) | Apply only the animation-driven vertical velocity step. |
| [`walk.enter(...)`](#authored-walking) | Initialize shared walking with the move's own lengths, speeds and acceleration. |
| [`jump.prepare(...)`](#authored-jump-startup) | Prepare common jump input, then let the script own startup timing. |
| [`state.landing(...)`](#authored-landing) | Initialize an authored landing motion through common landing bookkeeping. |
| [`animation.clip_length(action)`](#animation-lengths) | Read an owned animation's length without entering its action. |

## Method descriptions

### Authored walking

```text
walk.enter(first_motion, flags, frame,
           slow_length, middle_length, fast_length,
           slow_speed, middle_speed, fast_speed, acceleration)
```

The three consecutive motions represent slow, middle and fast walking.
Lengths are animation lengths; speeds and acceleration tune the shared
walking step. This initializes common walking state and selects its starting
variant. It does not invoke a stock fighter's carrying handler.

`walk.type` returns 0, 1 or 2 for the common slow/middle/fast selection.
`walk.update_animation()` advances the common walking animation policy;
`physics.walk()` applies its movement. `controls.walk_requested()` tests
ordinary walking input, while `controls.walk_stop_requested()` tests the
common stop condition. Each query leaves the script responsible for choosing
a transition. See the [cargo translation](../donkey_script_translation.md)
for animation phase preservation when changing walking variants.

These four walking operations run directly in the ordinary interpreter. Entry
requires three consecutive motions in the fighter's owned motion tables. Pass
an Int motion ID, UInt flags and finite Float parameters; the three reference
speeds must be nonzero because animation playback divides by them. Signed
reference speeds are allowed. Entry retains the source identity across motion
change and initial animation evaluation before storing the shared walk data.

`walk.type` reads the native velocity classification using the current walk
acceleration multiplier and common stick thresholds. It does not inspect the
current animation ID. `walk.update_animation()` requires the current motion to
be one of the three initialized walk variants. It uses the stored target speed
on slippery surfaces, otherwise current ground velocity; movement against
facing stops playback. `physics.walk()` applies native stick acceleration,
velocity taper, ground friction and slope projection. None calls a stock
fighter's special handler.

### Authored jump startup

`controls.jump_input()` returns the common jump-input classification:
0 for none, 1 for main-stick tap jump, and 3 for X/Y. The engine's enum reserves
2 for C-stick input, but this common query does not currently return it.
It observes the active player's controls, including tap-jump settings.

`jump.prepare(input, short_hop)` initializes common startup input and the
initial short-hop choice. The script owns its delay in an ordinary fighter
variable, chooses its startup motion and calls `jump.check_short_hop()` at
the appropriate interrupt phase. That operation observes common release
timing and the profile's short-hop binding.

`jump.launch(play_sound, multiplier)` launches through common jump physics;
the script must first select its airborne motion and switch to air.
`physics.jump_motion()` then runs common jump movement, preserving its
first-frame gravity behavior. These methods do not choose a startup duration,
carrying motion, landing delay or victim animation.

All four jump methods now execute directly in the ordinary interpreter.
Preparation takes an Int input classification from 0 through 3 and a Bool
initial short-hop choice. It only writes common startup data: it does not choose
a startup motion or start counting a delay. Short-hop detection can latch that
choice after a jump-button release, stick release or explicit short-hop input.

Launch takes a Bool sound option and a finite Float multiplier, preserves the
native horizontal momentum/clamp and full/short-hop vertical velocity, resets
the stick tilt timer and initializes the shared jump-physics flag. A soundful
launch requires the fighter's owned sound table. The first call to
`physics.jump_motion()` only advances that flag; later calls use common air
physics. Actor/program/storage retirement prevents result publication and the
next script statement.

### Authored platform drop

`state.platform_drop(motion, flags, frame)` enters an admitted motion through
the common platform-drop routine. Arguments are an Int motion ID, native-width
UInt flags and finite Float animation frame. It switches to air, clamps drift,
sets the common initial fall velocity, enters the motion, marks the current
floor as skipped and resets the vertical stick tilt timer. The source's actual
owned motion tables are checked before entry. If motion change retires the
actor/program, the native continuation stops before the floor/timer updates.

The script owns when to call it; use
[`controls.platform_drop_requested()`](#shared-movement-input-queries) for the
common input predicate. Custom motion selection, carrying animation and victim
pose remain separate decisions.

### Authored landing

`state.landing(motion, allow_interrupt, flags, frame, rate)` initializes a
selected landing motion through common engine bookkeeping. The script supplies
its motion, preservation flags, start frame and playback rate, and owns the
landing duration and next transition. This also delivers the normal authored
[`on landing_action`](fighter.md) event.

### `physics.air_accel_normal()`

Calculate ordinary horizontal air acceleration using the fighter's current
stick, horizontal velocity, aerial drift values and friction. This sets the
acceleration consumed by the shared movement step; it does not add gravity,
change vertical speed, move the fighter or change its motion state.

Use this when a move supplies its own vertical motion but still needs standard
air control:

```text
velocity.y = animation.delta.y
physics.air_accel_normal()
```

`physics.air_drift()` performs the normal air-physics routine, including its
vertical work. `physics.air_drift(acceleration_multiplier)` scales both parts
of the stick-driven horizontal acceleration. Gravity, fastfall, the target
horizontal speed and air friction use the fighter's usual values.
`physics.air_accel_friction(threshold, acceleration, speed)`
instead calculates horizontal acceleration from explicit tuning values. These
operations overwrite the current horizontal acceleration; calling normal
acceleration after custom acceleration preserves that deliberate order rather
than adding both values together. The [Samus template](../../examples/samus_full.fms)
does this to reproduce stock ground-started Screw Attack.

### Current model scale

`model.instance_scale_y` reads the fighter's current instance scale, including
temporary mushroom changes. It is distinct from a fixed costume or tuning
value. Scripts own when scale affects their custom movement:

```text
let carry_speed = attrs.carry_speed * model.instance_scale_y
```

This query does not resize the model or rescale an attribute. Common fighter
attributes can already include engine scaling, so avoid multiplying them again
unless the move requires it.

### Animation lengths

`animation.length` reads the current common animation length.
`animation.clip_length(action)` reads the length of an animation from the
active fighter's owned action bank, without changing its current animation.
Results/demo actors use their active owned demo bank instead of the gameplay
bank, including during load callbacks. A gameplay ID absent from that smaller
table returns zero, matching the stock clip lookup.
The action argument is a literal integer from 0 through 65535. A missing clip
returns zero; use positive lengths for walking and guard any division by a
length. The lookup uses the already loaded animation bank without allocating.

### Shared throw input thresholds

`input.attack_or_special_pressed` tests a fresh press of either attack or
special. `controls.throw_horizontal_threshold`,
`controls.throw_up_threshold` and `controls.throw_down_threshold` read the
engine's shared throw-input thresholds. Compare them with `input.x` and
`input.y` when authoring a directional throw selector. They are common control
settings rather than special attributes owned by one fighter. Direction,
priority, motion selection and capture transitions remain in the script.

The ordinary interpreter reads these three thresholds directly from the same
common control data used by native throw selection. The values are read-only
Floats, not offsets into a cloned fighter's special attributes. Missing common
control data rejects the observation rather than supplying a guessed default.

### Shared movement input queries

`controls.walk_requested()`, `controls.walk_stop_requested()` and
`controls.turn_requested()` run the native stick/facing tests without entering
a state. Their boundary comparisons match the common routines: walking accepts
the threshold, stopping tests below it or opposite-facing input, and turning
accepts its signed threshold. Do not replace these calls with a shared deadzone;
they intentionally have different comparisons.

`controls.platform_drop_requested()` checks downward stick input, the native
tilt timer and the contacted floor's pass-through flag. It does not skip that
floor or start falling. `controls.jump_input()` returns the native common jump
classification (0 none, 1 tap jump, 3 jump button). It honors the player's tap-jump
toggle and explicit short-hop binding; tap jump has priority when both inputs
qualify. These queries execute directly in the ordinary interpreter. Scripts
remain responsible for the resulting state/motion transitions.

### `state.freefall(...)`

`state.freefall(mode_a, mode_b, allow_interrupt, mobility, landing_lag)`
enters the engine's common helpless-fall state. The first two integers select
the original freefall modes; `allow_interrupt` is a boolean, and the final two
floats set horizontal mobility and landing lag. Match the mode values to the
stock behavior being translated. Luigi's Super Jump Punch calls it when its
animation ends:

```text
if animation.finished:
    state.freefall(0, 1, false, attrs.jump_freefall_mobility, attrs.jump_landing_lag)
```

This is a state transition, not a gravity step. While the special is active,
use [`physics.air_motion()`](#physicsground_motion-and-physicsair_motion) or
[`physics.fall()`](fighter.md#physicsfallgravity-terminal_velocity-and-physicsair_decelamount) as its source
requires.

### `motion.launch_angle`

A readable and writable float representing the fighter's current launch
angle. Super Jump Punch updates it before subaction command 0 changes the
move's movement phase. The example compares the absolute new angle with the
current one, retaining the larger steering input:

```text
if abs(angle) > abs(motion.launch_angle): motion.launch_angle = angle
```

### `cue.reverse.consume()`

Returns whether the current subaction requested a turnaround and clears that
cue. Consume it once in an interrupt callback. It does not change facing by
itself; the script decides whether the stick passes its threshold and then
calls [`facing.from_input()`](#facingfrom_input-and-bonesrootrotate_y).

### `facing.from_input()` and `bones.root.rotate_y()`

`facing.from_input()` runs the common facing update using current controller
input. `bones.root.rotate_y(angle)` rotates the model root around Y. Luigi's
Super Jump Punch applies both only when the reverse cue and stick threshold
allow a turnaround:

```text
let reverse = cue.reverse.consume()
if reverse and stick > attrs.jump_reverse_stick_range:
    facing.from_input()
    bones.root.rotate_y(pi_half * facing.direction)
```

The facing direction also matters to
[`callbacks.align_velocity_to_facing()`](fighter.md#callbacksalign_velocity_to_facing)
in other moves.

### `physics.ground_motion()` and `physics.air_motion()`

Run the common animation-driven movement step for the fighter's current
ground or air condition. A grounded motion may become airborne before its
motion-state callback changes, so check `grounded` and `airborne` when the
stock source does:

```text
on ground_physics:
    if grounded: physics.ground_motion()
    if airborne: physics.air_motion()
```

### `motion.scale_velocity()`

Multiply the fighter's X, Y, and Z self-velocity by a float expression. Use
it only when the stock move scales all three components. Luigi does so after
the launch command in airborne Super Jump Punch. If a move scales only one
component, assign `velocity.x` or `velocity.y` directly.

### `physics.quick_air_decel()`

Run the common quick aerial deceleration step. Luigi applies it in airborne
Super Jump Punch only before subaction command 0 fires, after
[`physics.fall()`](fighter.md#physicsfallgravity-terminal_velocity-and-physicsair_decelamount). It is
different from `physics.air_decel(amount)`, which takes a move-selected rate.

### `physics.animation_velocity_plain()`

Set horizontal self-velocity to the current TransN Z delta multiplied by facing,
and vertical self-velocity to its Y delta. This uses the common animation-motion
operation without rotating the sample by the launch angle. It does not advance
animation or apply gravity/drift. Bowser's initial Bomb ascent and carry throws
use this operation; a script can clamp or otherwise alter the resulting velocity
before the rest of its physics callback.

### Animation root-motion deltas

`animation.delta.x`, `animation.delta.y`, and `animation.delta.z` are readable
and writable floats from the current TransN movement sample. They describe animation
movement, not the fighter's world position or accumulated velocity. The common
[`physics.air_motion()`](#physicsground_motion-and-physicsair_motion) operation
turns that movement into velocity using the current launch angle and facing.

For example, a rising move can switch to gravity once its animation starts
descending:

```text
physics.air_motion()
if animation.delta.y < 0: command(2) = 1
```

These values are meaningful after the animation update for that simulation
frame. Reading them does not advance the animation or modify the pose.
Assigning a component changes that movement sample, without moving the fighter
or editing the animation asset. For example, `animation.delta.y = 0` clears
leftover vertical root movement when entering a move. Later animation updates
can supply a new delta; this is not a permanent root-motion toggle.

### `collision.unlock_ecb()`

Release the common environmental collision-box lock, clearing its remaining
frame count and locked-sample flag. This does not run a collision query or
change grounded/airborne state. A move may intentionally switch to air and
then unlock its collision box, as Bowser Bomb does when touching the floor
before its landing cue allows the landing action.

### `air.gravity`

Read the fighter's ordinary gravity acceleration. Pair it with
`air.terminal_velocity` when a move calls for the usual gravity step without
automatically applying drift or friction:

```text
physics.fall(air.gravity, air.terminal_velocity)
```

### `physics.air_accel_friction()`

`physics.air_accel_friction(threshold, acceleration, speed_limit)` applies
horizontal stick acceleration, clamps it toward the stick-selected target
velocity, and includes the fighter's ordinary aerial friction. All three
arguments are float expressions. When the stick's absolute X value is below
`threshold`, its acceleration and target are zero; friction still applies.
Call it from a physics callback.

```text
physics.air_accel_friction(0, air.drift_stick_mul * attrs.fall_mobility, air.drift_max * attrs.fall_mobility)
```

The existing `physics.air_accel(stick_scale, speed_limit)` uses the
no-friction engine operation with a zero threshold. Choose the operation that
matches the move being authored; do not apply friction twice.

### `physics.ascend()`

`physics.ascend(tap_speed, tap_gravity)` runs the shared ascent helper for an
airborne button tap. The arguments are
the move's upward speed and ascent gravity. A script should still decide
whether the button press is legal; Luigi's Cyclone checks its
[`state.cyclone_charge`](language.md#fighter-state), subaction command, and fresh
special press before calling this helper. See the
[Cyclone translation](cyclone.md#airborne-taps-and-charge).

### `physics.ground_accel()` and `physics.air_accel()`

Both methods take `(stick_scale, speed_limit)`. Apply the common
stick-directed acceleration step with a move-selected
multiplier and speed cap. The ground variant accelerates ground velocity;
the air variant uses the fighter's self velocity without extra friction.
Cyclone reduces its cap as command 0 ends the spin, then passes that computed
cap to the appropriate method. Use [`physics.ground_to_self()`](fighter.md#physicsground_decelerateamount-and-physicsground_to_self)
after the grounded step when the stock move does.

### `physics.clamp_horizontal_speed()`, `physics.clamp_ground_speed()`, and `physics.clamp_fall_speed()`

Each method takes a `limit`. Clamp horizontal self velocity, ground velocity,
or vertical falling speed,
respectively. They are distinct engine operations; choose the one the
source move uses. Cyclone clamps self velocity on entry, fall speed when it
leaves the ground, and ground velocity when it lands.

### `collision.air_standard()` and `collision.ground_standard()`

Run the ordinary air or ground collision routine in a collision callback.
These methods update collision state; they do not change the fighter's motion
to a new special phase. See
[`collision.air_special_landing()`](#collisionair_special_landing) for the
descending special-landing path.

### `collision.ground_box(...)` and `collision.air_box(...)`

`collision.ground_box(top, bottom, left_x, left_y, right_x, right_y)`
and `collision.air_box(...)` test a six-value move collision box. They
return booleans for their respective ground/air contact tests and are only
valid in a collision callback. Luigi's Cyclone uses the same box in both
conditions:

```text
if collision.air_box(12, 0, -6, 6, 6, 6):
    state.cyclone_charge = false
    state.to_ground()
```

The box is a stage-contact test, not an attack hitbox. See the
[Cyclone translation](cyclone.md#landing-and-transition-flags) for the
complete landing sequence.

### `collision.ground_box_edges(...)`

Takes the same six Float32 coordinates and returns whether the Fighter is
supported after the engine's grounded edge-support query. This selects
`ft_80082978` / collision mode 2, including the existing-floor edge test.
`ground_box` selects `ft_80082888` / mode 0. Yoshi's and Jigglypuff's rolling
moves select the edge variant at low speed; custom fighters can choose their
own policy without installing either fighter's handler.

All three box queries mirror asymmetric left/right X coordinates when facing
is negative, retain the native collision-cache and position updates, and reject
nonfinite coordinates before entering the solver. They keep the exact physical,
program and storage identity across the query; a retired or changed source fails
without publishing a result. Completed native collision updates are not undone.

### `collision.air_special_landing()`

Run the shared airborne special collision and ledge check. If the fighter
lands, enter common special landing with the given lag. The boolean is passed
to the common landing transition as its interrupt option; it does not disable
the ledge check. Luigi uses this only after launch,
while descending:

```text
if airborne and command(0) and velocity.y < 0:
    collision.air_special_landing(false, attrs.jump_landing_lag)
```

See [Luigi's script](../../examples/luigi_wip.fms) for the full grounded and
airborne branches, or [script callbacks](language.md#moves-and-subactions)
for where the collision handler runs.

## Interrupt checks and platform dropping

| Operation | Result and effect |
| --- | --- |
| `controls.turn_requested()` | Test the common turn-input threshold; does not change motion. |
| `controls.try_ground_jump()` | Attempt the common grounded jump and return whether it entered. |
| `controls.try_roll()` | Attempt common forward/backward roll input and return whether it entered. Direction comes from stick input relative to facing. |
| `controls.try_spotdodge()` | Attempt a spot dodge from fresh downward stick input while a shield button is held. |
| `controls.try_guard_spotdodge()` | Attempt a spot dodge while already shielding; also accepts the common downward C-stick check and does not require held shield. |
| `controls.try_air_jump()` | Attempt the common aerial jump and return whether it entered. |
| `controls.try_heavy_item_throw()` | Attempt the common heavy-item throw input and return whether a throw began. Chooses the ordinary directional/smash item-throw state using the engine's shared input thresholds. |
| `controls.platform_drop_requested()` | Test fresh downward input while standing on a pass-through platform. |
| `state.platform_drop(motion, flags, frame)` | Enter an airborne motion while skipping the current platform. |
| `state.finish()` | Finish the action into the common ground idle or airborne fall. |
| `bones.root.rotation_y` | Read root part Euler rotation Y in radians, using its Euler copy when the rendered joint uses a quaternion. |

Jump methods perform a transition when successful. Call them once in the desired
interrupt order and return from the callback when appropriate; they are not
passive button queries. A platform-drop query only tests input and collision.
The heavy-item throw check may also be used as a standalone statement when
the callback has no lower-priority interrupts. It uses common item throwing,
not a fighter's custom carry/throw handlers. The script decides when to allow
the attempt, such as only after a carry turnaround has completed.
The separate state operation applies shared drop velocity, clamps drift, marks
the floor to skip and preserves the requested animation frame. The script
chooses its destination motion and transition flags.

[Reflector](../../examples/fox_wip.fms) uses these primitives to express
turn, jump and drop priorities. Its scripted air physics preserves the gravity
delay and uses `air.terminal_velocity`, the common fighter attribute.

## Animation-driven movement

### `physics.clear_kinetics()`

Clear the common self, ground, knockback, shield-recoil and auxiliary movement
vectors, including self acceleration. This is the shared `ftCommon_8007E2FC`
operation. It does not change position or enter another motion. Preserve any
velocity that the move should keep explicitly before calling it:

```text
let x = velocity.x
let y = velocity.y
let z = velocity.z
physics.clear_kinetics()
velocity.x = x
velocity.y = y
velocity.z = z
```

### `animation.clear_motion_samples()`

Reset the fighter's accumulated translation samples, current TransN position
and translation offset, and its movement stick angle. It does not remove
animation keys or reset the animation frame/rate. Scripts that replace animated
movement with explicit travel can use this after updating the bone pose.

### `physics.air_decel_normal()`

Apply the fighter's common aerial self-acceleration/deceleration step using its
ordinary attributes. It does not apply gravity. This is distinct from
[`physics.quick_air_decel()`](#physicsquick_air_decel) and from the
[move-selected deceleration rate](fighter.md#physicsfallgravity-terminal_velocity-and-physicsair_decelamount).

### `facing.from_input_threshold()`

Update facing only when the horizontal stick reaches the common turnaround
threshold. Below it, retain the previous facing. `facing.from_input()` instead
uses the stick's sign directly. A move may choose the thresholded form when
its direction defaults to straight up for a nearly neutral stick.

### `collision.check_ground_ledge(direction)`

Run the common grounded/ledge collision check and return its Boolean result.
`direction` is an integer expression, usually `facing.direction` (`-1` or `1`)
or `0` when the caller requests the neutral form. This check updates collision
state; it is not a read-only contact query and does not itself select a new
special-move motion. Use it once in the intended collision callback.

### `sqrt(value)`

Return the engine's single-precision square root. A negative input produces
the math implementation's NaN; the script chooses whether to guard it. For a
stick magnitude:

```text
let magnitude = sqrt(input.x * input.x + input.y * input.y)
```

See [vector angles](#vectorsangle_xyax-ay-bx-by) for comparing directions rather
than their lengths.

### `physics.ground_animation_velocity()`

Use the common ground movement routine that derives speed from TransN's
current translation offset when the active motion enables it. Otherwise apply
normal ground friction. It also converts grounded movement to self velocity.
This is the `ft_80085088` engine path, distinct from `physics.ground_motion()`.

### `physics.air_animation_motion()`

Set horizontal self velocity to TransN's Z offset multiplied by facing, and
vertical self velocity to its Y offset. This is `ft_80085134`; it does not apply
ordinary gravity/deceleration. Scripts choose this path for animated travel
and switch to normal air physics at their chosen recovery transition.

`bones.root.rotation_x` reads the current root X rotation in radians, useful
for recording an article's pose history alongside world position.

### `vectors.angle_xy(ax, ay, bx, by)`

Return the unsigned angle in radians, from zero through pi, between two
vectors in the gameplay XY plane. The common engine implementation clamps
the normalized dot product before `acos`; a zero-length vector returns zero.
This is different from `atan2_float(y, x)`, which gives a signed direction.
For example, compare a travel velocity against a contacted surface normal:

```text
let impact = vectors.angle_xy(floor.normal.x, floor.normal.y, velocity.x, velocity.y)
```

### `collision.skip_soft_floor()`

Return true when the current floor is a pass-through platform and mark that
floor as skipped by subsequent collision checks. Return false for solid floors.
This call **changes collision state**; evaluate it only when the move intends
to pass through the platform. Short-circuit conditions matter:

```text
if state.travel_age < attrs.platform_grace:
    let can_land = not collision.skip_soft_floor()
```

### Surface contacts and normals

`collision.floor_contact` and `collision.ceiling_contact` read the last
collision environment flags. Existing `collision.left_wall` and
`collision.right_wall` test the two wall masks. Normal components are
`floor.normal.x/y/z`, `ceiling.normal.x/y/z`, `wall.left.normal.x/y/z`, and
`wall.right.normal.x/y/z`. Check a corresponding contact before using its normal;
these expressions do not perform a fresh collision query.

### `physics.animation_vertical_velocity()`

Apply the common TransN-derived vertical velocity update without also running
the horizontal animation step. It does not advance the animation or move the
fighter immediately. Use it when a rebound animation supplies Y motion while
a separate deceleration rule governs X. `physics.animation_velocity()` runs
both engine steps and can have different dynamics.

## collision.ground_standard_box

`collision.ground_standard_box(top, bottom, left_x, left_y, right_x, right_y)`
runs the normal grounded collision/floor correction with this authored box.
It accepts six float expressions. It differs from `collision.ground_box()`,
which performs the lower-level sweep/query without that full grounded policy.
See [`on roll_enter`](common_policies.md#on-roll_enter) for a practical use.

## collision.try_wall_jump

`collision.try_wall_jump() -> bool` runs the shared wall-jump test and, when
allowed, enters its normal jump state. Use it as an ordered interrupt test;
a successful call already changes state.

## Vector velocity transforms

`physics.rotate_velocity_unit_axis(axis_x, axis_y, axis_z, angle)` rotates
Fighter self velocity using `lbVector_RotateAboutUnitAxis`. Supply a unit-length
axis and a radian angle. All three velocity components participate. The script
owns the axis and turn decision; the engine primitive neither normalizes the
axis nor modifies facing, position, ground speed or knockback velocity.

```text
physics.rotate_velocity_unit_axis(0, 0, 1, target_angle - current_angle)
```

For a copied vector rather than the fighter's velocity, use
[`vectors.rotate`](math.md#vectorsrotatex-y-z-axis-angle-out_x-out_y-out_z).
All ceiling and wall normal namespaces expose `.x`, `.y`, and `.z` on fighters
and articles. These are stored native `CollData` values, including signed zero
and nonfinite values; a read neither refreshes collision nor changes state.


`physics.mirror_velocity(normal_x, normal_y, normal_z)` mirrors the fighter's
self velocity against the supplied normal using the shared Melee vector routine.
It leaves position and knockback velocity unchanged. Supply the normal for the
surface you intend to reflect against; the script owns that choice.

`physics.rotate_velocity_unit_axis(axis_x, axis_y, axis_z, angle)` rotates self
velocity around a **unit** axis by an angle in radians. It uses Melee's original
rotation routine and does not normalize the axis for you. These operations change
velocity immediately but do not advance physics or animation.

```text
physics.mirror_velocity(floor.normal.x, floor.normal.y, 0)
physics.rotate_velocity_unit_axis(0, 0, 1, state.turn_angle)
```

## `state.freefall_blend(normal_air_limits, force_special_landing, allow_interrupt, mobility, landing_frames, blend)`

Enter the shared special-fall state with explicit engine policy flags, air-drift
multiplier, special-landing duration and motion blend. This exposes the same
shared entry used by stock special moves, rather than a fighter-specific recovery
decision. A nonzero first argument uses normal terminal velocity and drift limits;
zero uses fast-fall terminal velocity and the supplied mobility cap. A nonzero
second argument always selects special landing; zero allows an ordinary landing
when downward speed is below the engine threshold. The third argument controls
interrupt permission passed to special landing. The script decides these values
and when to enter the state.

This blended entry preserves the current used-jump count for an already airborne
fighter; the ordinary special-fall entry consumes the remaining jumps. Use
`state.freefall(...)` when that existing policy suffices. Both entries can
invoke motion callbacks; generated code checks the caller's lifetime before
continuing after the transition.

## `state.down_bound()`

Enter the shared grounded down-bound/landing decision. Its normal engine logic
chooses the corresponding common motion and sets down-spot state; it does not
author a recovery move or test whether a particular impact deserves this state.
Call it only after your script decides the transition is appropriate. As with
other motion transitions, a removed or replaced fighter ends the caller's
continuation before further state writes.

## Forced airborne entry

`physics.force_air_and_use_jumps()` consumes remaining jumps, sets airborne
kinetics, clears grounded velocity and vertical self-acceleration, and locks the
ECB for five ticks. It retains current self velocity. Use this existing operation
when those semantics are required rather than plain `physics.ground_to_air()`.

## Collision surface properties

`floor.is_platform` is a read-only boolean from the current floor line's platform
flag. `floor.normal.x/y/z`, `ceiling.normal.x/y/z` and
`wall.left.normal.x/y/z` / `wall.right.normal.x/y/z` read the actual three-component
collision normals.

## World position

`position.x/y/z` are writable ordinary fighter properties. An assignment writes
that component of the current world position directly; it does not infer a
velocity, update the ECB or perform collision detection. Their right-hand side
is evaluated once, and synchronous source-lifetime guards run before the write.
Use these deliberately inside authored physics when position changes separately
from self velocity. Model-effect callbacks cannot write fighter position.

## Explicit drift and animated vertical velocity

`physics.air_accel_from(horizontal_velocity)` calculates standard air drift
acceleration using the supplied horizontal speed rather than `velocity.x`. It
updates the normal acceleration fields; the script chooses how to combine that
acceleration with animation-driven movement. This uses the same engine drift
calculation as `physics.air_accel_normal()`.

`physics.animation_vertical_velocity()` applies the animation's vertical
translation delta directly. `physics.animation_vertical_velocity_adjusted()`
applies the common equipped-helmet multiplier when present. Both use the engine's
existing animation-motion operations; neither advances the animation.

## Fastfall state

`movement.fastfall` reads or writes the fighter's fastfall flag. Assignment requires a Bool; clearing it can reset an authored hovering or recovery action without running a fastfall input check. See [Aerial actions](aerial_actions.md) for landing callbacks and shared aerial input selection.

## `state.catch_ledge()`

Enter the common ledge-catch state from the current collision ledge information.
This Fighter-only operation takes no arguments and returns Void. It performs
facing adjustment, airborne setup, catch animation and ledge bookkeeping using
the ordinary engine path. It does not test whether a ledge can be caught; use
`collision.cliff_catch()` when you need that Bool query and conditional entry.

The interpreter requires the selected fighter's owned program and motion layout.
A retired or faulted caller stops the native continuation. See the
[ledge entry checkpoint](../native_source_ledge_entry.md) for comparison evidence
and the controlled fixture boundaries.

### Canonical aerial-jump and owned ledge checks

`controls.try_aerial_jump()` is the existing zero-argument Bool attempt through
`ftCo_800CB870`. It checks the Hammer veto, then chooses the enabled multi-jump
path or ordinary aerial-jump input. Profile/tap-jump rules and the used-jump
limit remain native. The Screw Attack item alternative precedes ordinary
source entry; an authored `on aerial_jump` owns its jump when present, and an
absent hook retains the generic common fallback. `jump.begin(...)` performs
explicit authored entry/bookkeeping, while `controls.aerial_jump_actions()`
runs a wider interrupt chain. Neither substitutes for this narrow input call.
Link and Young Link use the canonical name; no `try_air_jump` alias is provided.

For an owned source, `collision.check_ground_ledge(direction)`,
`collision.cliff_catch()` and `controls.try_aerial_jump()` now retain the exact
Fighter allocation, generation, scene lifetime, selected program, storage and
layout/parameter identities across the native operation. They publish their
Bool only while that original ownership remains current. Legitimate motion
changes are allowed, and completed native mutations are not rolled back.
This strengthening is scoped to owned sources; existing foreign Fighter
behavior remains unchanged. Full engine setup and authored catch/tether policy
closure remain separate from these checks.


## Platform flags and velocity reflection

`floor.is_platform` returns Bool from the stored floor-contact `LINE_FLAG_PLATFORM`
bit. It does not perform a new collision query or infer a platform from its
material index. Use it with the current contact/grounded state when deciding
whether a move may pass through the floor.

`physics.mirror_velocity(normal_x, normal_y, normal_z)` applies the native
`lbVector_Mirror` routine to this fighter's self velocity. The native routine
reflects X/Y about the supplied unit normal, leaves Z unchanged and does not
normalize the input. Its exact zero and non-unit-vector behavior is preserved.
This changes velocity only; it does not change facing, rotate the model, choose
an action or change ground/air state.

`state.down_bound()` enters the shared native knockdown/bounce continuation
`ftCo_80097D40`. It selects the normal up/down knockdown state through the
existing engine policy. It requires the exact live owned fighter, motion table
and script storage and rechecks that ownership after the native transition.
This API has source/build coverage in the
[common-state checkpoint](../native_script_ness_common_state_checkpoint.md);
live Ness knockdown gameplay remains to be checked once that package prepares.
