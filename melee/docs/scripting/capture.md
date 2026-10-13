# Attached captures

Use these operations for a command grab which attaches a victim to the holder,
permits mashing to escape, and eventually releases or throws the victim.
The script owns the holder's states, timing, directions and repeat policy.
The common engine owns the victim attachment, input mashing, damage and
knockback bookkeeping. No operation calls a stock fighter's special handler.

## Capture settings

`capture.victim_damage` reads the currently linked victim's damage percentage,
or zero when there is no victim. It is read-only. Test `capture.has_victim`
when absence should have a different meaning from an undamaged victim.

```text
if capture.has_victim and capture.victim_damage < attrs.throw_damage_threshold:
    state.prefer_light_throw = true
```

These writable properties belong to the holder's common capture state and are
saved with the fighter during rollback. Give them named, editable fighter
parameters; they are independent of extended DAT offsets and private move unions.
Configure them before the grab can connect.

| Property | Meaning |
| --- | --- |
| `capture.timer_damage_multiplier` | Multiplier passed to the common grab-timer initializer. |
| `capture.timer_decay` | Timer reduction per captured animation update. |
| `capture.mash_reduction` | Additional timer reduction from the common mash detector. |
| `capture.mash_pause_frames` | Duration used by the victim's mash animation update. |
| `capture.mash_animation_rate` | Playback rate selected when a mash is detected. |
| `capture.wiggle_scale` | Scale applied to the victim's detected stick movement. |
| `capture.wiggle_limit` | Limit on the victim's local vertical/depth wiggle. |

For example, `capture.timer_decay = attrs.grab_decay` selects a package-owned
parameter. Scripts can set these properties independently; there is no opaque
seven-number configuration command. The timer and wiggle algorithms are the
existing common Melee algorithms, including their animation-update ordering.

All seven settings above now have direct interpreter read/write bindings. They
are Float32 engine fields, preserving native float bits without normalization or
clamping. Assigning them changes the current Fighter's settings; it does not
retroactively replace settings already copied to a victim or perform a capture.
The holder must retain its exact owned program, storage and physical lifetime.
The [capture scheduler fixture](../../tests/native/script_capture_scheduler/README.md)
checks all seven fields and a real interpreted configuration helper. Geometry,
timer progression and complete command-grab gameplay remain separate acceptance.

Captured-item observation, attachment and release also have direct interpreter
bindings. The enclosure settings, victim visibility/body status, enclosure release
and caught-shape/constraint operations below are also implemented. Held/thrown
motion entry and shared throw ticks are interpreted too. Special-capture release
variants remain separate work; a native/reference implementation alone does not
establish interpreted support.

## Victim motion entry

### `capture.set_victim_capture_motion(motion: int)`

Restart the attached victim's common capture animation at frame 0 and speed 1.
The existing engine routine selects `FreezeState` when the victim requires it,
uses the holder as its animation donor, installs the attachment update, excludes
other grab targets, and performs the initial animation update. This does not
choose the holder's state or decide whether to bite again.

```text
if airborne:
    capture.set_victim_capture_motion(Motion.captured_damage_air)
else:
    capture.set_victim_capture_motion(Motion.captured_damage_ground)
```

### `capture.initialize_hold(motion: int)`

Initialize the victim's shared shouldered/held state. The native common damage
percentage timer is converted to an integer exactly as in Melee, mash bookkeeping
is reset, and the victim enters the requested animation using the holder as its
animation donor. Movement is cleared and the attachment update is installed when
the animation changes. Reinitializing an existing `Motion.held_wait` resets the
timer without restarting its animation, preserving the native condition.

```text
if capture.has_victim:
    capture.initialize_hold(Motion.held_wait)
```

Both operations return no value. An absent victim is a no-op. A present victim
must have a reciprocal live holder link and the requested motion in its actual
installed table. Stock and independently scripted victims are supported; the
holder's package does not supply the victim's motion table. Invalid signatures,
retired actors/storage/programs, broken links, or nonfinite/out-of-range hold timer
data produce a script diagnostic instead of publishing success.

### `capture.set_victim_hold_motion(motion: int)`

Change an already held victim's animation using the shared shouldered-motion
routine. It uses the holder as its animation donor, clears movement and installs
the attachment update when the motion changes. It preserves the native
already-held-wait exception and does not reset the victim's mash timer.

### `capture.set_victim_throw_motion(motion: int, rate: float)`

Enter a common thrown-victim animation with the requested playback rate. The
engine attaches the victim, copies the holder's facing, initializes thrown state
and performs the common throw-victim setup hook, statistics and initial animation
update. The holder's declared `on throw_victim_setup:` hook owns its fighter-specific
policy. Rates are finite floats; zero and negative rates are not silently clamped.

### `capture.set_victim_attached_throw(motion: int, flags: uint, start: float, initialize: bool)`

Enter an attached throw pose using explicit native motion flags and starting
frame. The victim faces the holder's current direction and uses its animation as
the donor. `initialize` requests the initial animation update; false is useful
when preserving a transferred frame during a ground/air transition. This operation
does not run the ordinary thrown-victim setup/statistics path above.

```text
capture.set_victim_attached_throw(next_pose, 0x0c4c5080, capture.victim_frame, false)
```

### `capture.sync_victim_facing()`

Copy the holder's current facing to its attached victim without restarting an
animation or changing either actor's position. An absent victim is a no-op.

### `capture.victim_frame`

Read the linked victim's current animation frame as a Float, or zero when there
is no victim. This is read-only. It observes a retained victim; it does not seek
the victim or replace the holder's own `frame` value.

All victim-motion methods validate the recipient's own installed motion table and
reciprocal link before entry. Attached throw requires native-width unsigned flags,
a finite starting frame and a Bool initial-update option. Each operation retains
both actor/program identities across common callbacks. Invalid input and retired
or replaced relations report a diagnostic without publishing success.

For cue-driven release/pause and movement on the holder, see
[`throws.update()`](fighter.md#throwsupdate) and
[`physics.throw_motion()`](fighter.md#physicsthrow_motion).

The [capture scheduler fixture](../../tests/native/script_capture_scheduler/README.md)
compares these common entry bodies with their pre-change source, including freeze
flags, animation donor, exact Fighter fields and engine call order. Motion/pose
evaluation is controlled in this fixture; complete animated command-grab gameplay
still needs match testing. See [held fighter transitions](fighter.md#held-fighter-transitions)
for the holder-side workflow.

## `capture.arm(mask)`

Arm the shared grab detector using the script's `on grab_catch:` and
`on grabbed:` events. The mask determines which common grab targets can be caught. This
works in helper functions as well as special entries and state transitions.
Both handlers are required when the script arms a grab. An optional `on item_catch:`
handler receives catches of existing items; scripts without one install no item
catch callback. `capture.has_item` reads the captured item link, separately from
the ordinary held-item slot.

In the ordinary interpreter, both events execute on the **capturing fighter**,
using that fighter's program and variables. Both events receive a retained
`victim` object payload; it does not execute the holder's program on the victim.
The engine calls `grab_catch` first, checks both lifetimes, and then calls
`grabbed`. The first event runs before the victim establishes its reciprocal
capture link. An ordinary native grab which installs engine callbacks takes
precedence over the script hooks. `mask` must be an Int in 0..32767.

The arm, entry, attachment, observation and throw-release paths are implemented and covered by the
[capture scheduler fixture](../../tests/native/script_capture_scheduler/README.md).
The fixture compares entry ordering and fields with the original common Falcon
capture routine; it is not full-match acceptance of every command grab.

## `capture.begin(motion, same_facing, force_air, track_position, holder_animation)`

Call from the holder's `on grabbed` to initialize its currently selected victim
and establish the victim's reciprocal link. This performs the common cleanup,
reciprocal link, captured motion, capture mask, animation initialization and
movement reset. It does not run a stock fighter's move decisions.

`same_facing` selects the holder's facing or its opposite. `force_air` establishes
the attachment even for a grounded victim, then switches the victim to air
**after** entering its captured motion. Otherwise the normal ground/air link
policy applies. `track_position` installs the common follow-holder accessory
for an airborne link, before the forced-air transition. `holder_animation`
selects the holder as the animation donor when true; false selects the victim
without a donor. All five choices are explicit.

The motion must belong to the victim's own imported motion table. A stock victim
admits common captured states; a scripted victim also admits its owned custom
states. The engine checks both retained lifetimes between callback-capable steps.

## Attachment and release

`capture.clear_movement()` resets the current fighter's common capture movement
bookkeeping. It does not pick a motion or change facing.

`capture.attach_to_victim()` constrains the **holder's** semantic `XRotN` to the
victim's `TransN2`. It preserves the common already-attached check and does not
change either fighter's ground/air state. This replaces the misleading old
exporter spelling `victim_to_air`; there is no alias.

`capture.follow_victim()` installs the common accessory-4 position update on the
holder. Each update samples its retained victim's current world position.

`capture.ground_link` is a readable/writable Bool describing the common capture
ground-link policy. Assign it directly: `capture.ground_link = true`.

`capture.release_throw()` applies the holder's throw hit to its linked victim,
then performs the common thrown-victim damage response. Applying damage unlinks
the pair, so the second step retains the original victim identity rather than
looking up a new link. It does nothing when no victim is linked. Retired actors
stop the continuation and report a script fault.

## Other capture contracts awaiting interpreter migration

The following descriptions include retained native/exporter contracts. Their
presence here does not establish that those APIs are admitted by the ordinary
interpreter; check the current [migration checkpoint](../native_script_capture_scheduler_checkpoint.md).

`capture.armed` reads or sets whether this fighter's grab detector is armed.
`capture.freeze_state` reads the captured fighter's freeze-state policy; it is
read-only. These are engine lifecycle flags, distinct from authored variables.

## `capture.set_attached_throw(motion, flags, start, initialize)`

Enter a holder-linked thrown pose on the **current fighter**, using the common
attached-throw routine. It follows the holder's facing, resets the attached
pose bookkeeping, installs the follow accessory and capture mask. `initialize`
also starts the selected animation. Use the separate
`capture.set_victim_attached_throw(...)` when calling from the holder instead.

## `capture.release_damage_fall()`

Apply the holder's throw damage to its linked victim, then enter the common
DamageFall policy on that victim. The engine retains the victim reference
through damage application, which clears the reciprocal links. This differs
from `capture.release_damage(...)`, which launches the victim with throw
knockback. Without a victim the operation does nothing.

## `capture.begin_attached(motion)`

Initialize the selected victim's reciprocal capture link, timer, facing,
airborne bookkeeping and attachment, then select the supplied victim motion.
The capturing fighter owns the script, variables and parameters throughout;
`airborne` describes that fighter. `on grabbed:` receives the victim as its
explicit `victim` argument. This operation requires a live selected victim and
a motion admitted by that victim's owned animation tables. Cleanup or animation
callbacks that retire either actor or its program stop entry and further script
execution; a retired victim cannot be mistaken for a recycled actor.

```text
on grabbed:
    if airborne:
        capture.begin_attached(Motion.captured_damage_air)
    else:
        capture.begin_attached(Motion.captured_damage_ground)
```

## Victim poses

`capture.set_victim_capture_motion(motion)` selects a captured damage pose and
restarts its animation while retaining the capture. The common pose's ending
state transitions to its captured wait pose. It does nothing without a victim.

`capture.set_victim_attached_throw(motion, flags, frame, initialize)` sets the
victim's attached throw pose. Pass `initialize = true` for a fresh animation;
use `false` to preserve a transition's frame and subaction flags.
`capture.victim_frame` reads the current victim frame, or zero without a victim.
`capture.victim_freeze_state` reads the victim's motion-state freeze flag, or
false without a victim. This is the flag used with `Ft_MF_FreezeState` during
attached pose transitions, not hitlag or paused gameplay. It never reads the
holder's flag, even inside `on grabbed:`.
These operations choose no holder state or throw direction themselves.

| Constant | Common victim animation |
| --- | --- |
| `Motion.captured_damage_ground` | Captured ground damage/bite pose. |
| `Motion.captured_damage_air` | Captured aerial damage/bite pose. |
| `Motion.captured_throw_forward` | Attached grounded forward throw. |
| `Motion.captured_throw_back` | Attached grounded back throw. |
| `Motion.captured_throw_air_forward` | Attached aerial forward throw. |
| `Motion.captured_throw_air_back` | Attached aerial back throw. |

These are the common motions historically named `CaptureDamageKoopa` and
`ThrownKoopa*` in the decomp. Other fighters may reuse those victim poses and
the parameterized attachment engine. They do not inherit Bowser's move code.
Ordinary throws and shouldered carries have their
[own initialization methods](fighter.md#held-fighter-transitions).

## Releasing

`capture.release_damage(track_holder, upward)` applies the current throw
capsule, unlinks the capture and starts the victim's common damage response.
`track_holder` installs the common post-release holder tracking callback;
`upward` selects the common upward release-angle policy. Both are explicit
booleans. Guard with `capture.has_victim` when making additional decisions
around release; a release removes the relationship.

`capture.drop_victim()` unlinks the captured fighter and enters its normal fall,
without applying throw damage. It does nothing without a victim, and does not
change the holder's state. A holder may explicitly fall afterwards.

`capture.mask(mask)` and `capture.clear_movement()` expose the common
collision-mask and movement-clear operations. They do not choose a move or
advance animation. Song immunity is a separate fighter property:
[`damage.song_immune`](fighter.md#damagesong_immune).

The ordinary interpreter implements `capture.arm` and `capture.mask`; the latter uses the native exclusion
mask semantics described in [common tether prerequisites](tether_controls.md).
Other attached-capture operations on this page remain native/exporter contracts
unless their ordinary implementation is documented separately.

## Input and movement

`input.previous_x` is the previous sampled left-stick X value. Compare it with
`input.x` and an owned threshold to detect a crossing rather than repeatedly
responding to a held direction.

`physics.ground_animation_facing_motion()` runs the shared ground-animation
movement step using the animation's saved facing. This differs from
`physics.ground_animation_motion()`, which uses current facing; a throw can
reverse facing midway through an animation without reversing its root motion.

This operation now runs directly in the ordinary interpreter. When native
animation-driven movement is enabled, it takes TransN Z displacement multiplied
by the saved animation facing as ground velocity. Otherwise it applies ordinary
ground deceleration. Both branches project movement along the contacted floor;
the saved animation facing may differ from the fighter's current facing.

See the [Bowser translation](../bowser_script_translation.md) and its
[authored example](../../examples/koopa_wip.fms) for a complete holder policy.

## Capturing existing items

`capture.attach_item(raw_bone)` makes the captured item inert and attaches its
model's attachment bone to the supplied holder bone. It retains the existing
item object, replaces its motion table with the common inert state, disables
contacts and automatic lifetime, and preserves the common owner bookkeeping.
An interpreted item retains its authored program, editable state and lifecycle
callbacks; its normal animation, physics and collision phases stop. The raw bone
must exist in the current holder's rig, and the item must have a valid model
attachment bone. This does not disable the shared graph update or render path.

`capture.has_item` is a read-only Bool for this captured-item link. It is separate
from the normal held-item slot and does not imply the item is already attached.
It returns false without a selected item; a retired or inconsistent relation
rejects the operation instead of masquerading as an empty slot.

`capture.release_item(raw_bone, clear_destroy_type)` detaches it, samples the
holder bone for its release position, performs item statistics, unlinks the old
item and destroys it. It does nothing without an item.
The boolean selects whether to clear the engine's destruction category before
removal. These operations do not create an authored article or use the normal
held-item pickup/throw flow.
Destruction can run an authored callback. A newly selected capture target is
preserved, and a retired holder stops the native continuation. Both operations
require the current owned Fighter's exact physical/program/storage lifetime.

```text
on grab_catch:
    if capture.has_item:
        capture.attach_item(bones.index(TransN2))
```

The [connected capture-item fixture](../../tests/native/script_world_feedback/README.md)
executes real interpreted helpers against stock and owned Items. Its graph,
statistics and teardown leaves are controlled; the separate
[Item scheduler fixture](../../tests/native/script_item_scheduler/README.md)
checks actual inert phases, actor-state save/load and destruction. Real rendered
attachment geometry and whole-match rollback remain separate acceptance.

`capture.victim_visible` reads or assigns the linked victim's visibility.
`capture.set_victim_body_status(status)` applies a common body hit status to
that victim: Int `0` normal, `1` invincible, `2` intangible. The body-status
operation runs the existing common collision status and color-animation update.
Visibility writes are no-ops without a victim; reads return false. Body-status
changes also do nothing without a victim. A stale/inconsistent victim faults
instead of changing a newly allocated actor.

## Timed enclosures

Configure these holder settings before `capture.release_into_enclosure()`:

| Property | Meaning |
| --- | --- |
| `capture.enclosure_model_slot` | Imported accessory joint slot, not an item kind. |
| `capture.enclosure_launch_x/y/z` | Initial world-space victim velocity. |
| `capture.enclosure_damage_behavior` | Common damage mode used while enclosed. |
| `capture.enclosure_shrink_fraction` | Fraction removed from the starting model and accessory scales. |
| `capture.enclosure_shrink_frames` | Updates over which those scales return to their saved values. |
| `capture.enclosure_lifetime` | Value passed to the common grab-timer initializer. |
| `capture.enclosure_damage_time_scale` | Timer reduction per point of incoming damage. |
| `capture.enclosure_escape_x/y/z` | World-space velocity applied on escape. |
| `capture.enclosure_escape_invincibility_frames` | Invincibility duration in frames applied on escape. |

All thirteen settings have ordinary interpreter read/write bindings.
`enclosure_model_slot` and `enclosure_escape_invincibility_frames` are native-width
Ints; the remaining settings are Float32 values. Assignment preserves the
native fields rather than clamping or normalizing their values. Configure sensible
values for your move: storing a model-slot number does not validate or load an
accessory. Assignments do not immediately change an already enclosed victim.
`capture.release_into_enclosure()` is also implemented in the ordinary interpreter.
It applies the holder's configured throw damage, releases the captured pair and
enters the common timed-enclosure state on that same victim. With no victim it
does nothing. Missing model descriptors, invalid source-owned rig mappings or
retired actors fault and stop the calling script's continuation. Joint mappings
needed by throw damage are checked before applying it; both actor identities
remain retained after the grab links are intentionally cleared.

The common entry checks identities again after motion selection and accessory
creation. It never continues by looking up a replacement victim from a new grab
link. A fault stops continuation; it does not undo changes an earlier native step
or callback already made. The [capture scheduler fixture](../../tests/native/script_capture_scheduler/README.md)
exercises this path with both stock and owned victims. Its damage/model/constraint
leaves are controlled; full rendered enclosure gameplay remains separate acceptance.

The existing timer decay and mash settings also apply. Release snapshots all
settings, the accessory descriptor and facing before throw damage clears the
capture links. The common enclosed-victim state then owns timer/mash updates,
shrinking, damage, collision and escape. Later holder edits do not change an
already enclosed victim. Settings and active state participate in rollback.
No stock fighter special callback or global fighter-attribute table is consulted
while the victim updates. A missing victim is a no-op; a missing accessory
descriptor rejects release.

## Ordinary grab presentation hooks

The ordinary interpreter implements `capture_wait_setup`, `capture_wait_enter`,
`catch_pull_frame`, `throw_victim_setup` and the related
[catch policy hooks](catch_hooks.md). These declarations prepare as typed owned
hooks and run from the shared native capture schedulers.

These callbacks run on the **holder**, with its own script variables and
parameters. They customize shared grab/caught poses independently of special
command grabs.

`on capture_wait_setup:` runs when the victim's shared captured-wait or
captured-damage pose needs its presentation rebuilt, including transitions
between ground and air. `on capture_wait_enter:` runs only after initial
captured-wait setup. Keeping them separate lets a script rebuild a hurt
shape repeatedly without reinstalling an initial attachment constraint.

`capture.enclose_caught(radius)` hides the linked victim, installs the common
follow-holder accessory, makes its hurt capsules intangible and initializes
its first capsule around its semantic `XRotN` joint. That capsule is not
grabbable and has zero endpoint offsets; its radius is divided by the
victim's common model scale. The script supplies the radius and decides when
to request this shape. This operation does not create an enclosure article
or release the capture.

`capture.constrain_caught()` establishes the existing common attachment of
the victim's `XRotN` to the holder's `TransN2`. The common constraint routine
preserves its already-attached check. Both operations do nothing without a
linked victim.

Both operations and `capture_wait_setup` run in the ordinary interpreter.
They retain the holder, selected victim and their admitted joint mappings;
actor/program/storage changes stop the script's remaining work. The enclosure
radius must be finite and nonnegative, and the victim's common model scale
must be finite and positive. Invalid geometry rejects before modifying the
victim. An already established constraint keeps the common attachment policy.

```text
on capture_wait_setup:
    capture.enclose_caught(attrs.caught_radius)
on capture_wait_enter:
    capture.constrain_caught()
```

`on catch_pull_frame:` returns a **float** start frame when standing Catch
(motion 212) enters CatchPull (motion 213). It does not run for the later
CatchPull-to-CatchWait transition or the dash-catch branch.
The callback may explicitly adjust the current animation before returning
its remapped frame. The common engine still selects the next motion and
performs the capture transition; this hook does not replace the entire grab.
The return value remains a float, including fractional start frames. An
undeclared hook leaves the native start-frame policy intact; a declared hook
which reaches its end without returning uses the event's zero-frame default.
`frame` reads the holder's current animation frame, not a supplied event argument.

`on throw_victim_setup:` runs on the holder just after the shared thrown-victim
motion change, before common throw statistics, capture flags and animation
initialization. Use victim operations such as
`capture.set_victim_body_status(Hurt.intangible)` for extra pose policy. It
runs on the holder's script, not the victim's installed fighter definition.
Undeclared hooks retain the stock fighter's policy.
Both schedulers retain the holder and victim across callback execution. A script
fault, actor/program/storage retirement, or changed capture relation stops the
remaining shared transition instead of continuing with stale actor data. Grab
entry preserves the victim's existing relation, which can precede establishment
of its reciprocal holder link; throw setup requires the established mutual link.

### `on throw_presentation`

This Void event runs on the holder after its motion and initial animation,
capture-mask initialization, and the victim's **complete** thrown setup. It
runs before the common throw-invincibility grant. Read-only `event.motion`
(Int) identifies the holder's selected throw motion; `event.rate` (Float) is
its actual playback rate, including the common weight-dependent choice.

Use this event to initialize an accessory or other presentation that needs the
victim's initialized pose. It is distinct from `on throw_victim_setup`, which
runs earlier, and `on throw_animation`, which handles subsequent animation
updates. Calling a victim-pose primitive directly does not run this holder
entry event: the common full throw entry dispatches it once.

Event values are scoped to the callback. Pass them as typed arguments to
helpers rather than reading `event` inside a helper. For example:

```fms
state int selected_throw
state float selected_throw_rate

on throw_presentation:
    state.selected_throw = event.motion
    state.selected_throw_rate = event.rate
```

A missing event is a healthy no-op for an interpreted holder. It does not
dispatch a stock fighter-specific accessory constructor. Native holders keep
their existing native policy. The holder and victim must retain their physical,
program and reciprocal capture identities across entry and callback execution.
A script fault, retired actor/schema or broken capture link stops continuation
before the invincibility grant. No pointer is passed to the script.

The connected capture scheduler tests all four throw motions, stock and owned
victims, playback rates, missing hooks, callback faults/retirement, and common
entry observation boundaries. Motion/animation setup and stock accessory
construction are controlled leaves; this is not rendered accessory acceptance.

## Damage release

`capture.release_damage(retain_collision_credit: Bool, force_upward: Bool)`
applies the holder's throw hit, unlinks the capture, and enters the victim's
normal thrown-damage reaction. With `force_upward` true, the common reaction
uses 90 degrees; otherwise it uses the angle stored by the throw damage.
The first option keeps the holder as the reaction's follow-up collision-credit
reference. It does not change who initially dealt the throw damage.

`capture.release_damage_fall()` applies the same throw hit and unlinks the
capture, then enters DamageFall directly rather than the normal launched
damage reaction. This is a different transition, not an upward-launch option.
Neither operation changes the throw hit's authored damage or knockback values.
Both are Fighter-only and do nothing when no victim is linked. A present
victim must have the established mutual capture relation. Actor, program or
storage retirement stops result publication and the following script work.

```text
capture.release_damage(false, false)
```

## `capture.drop_victim()`

Release the linked victim without applying a throw hit, then enter its ordinary
Fall state. The common unlink routine restores the attached model and resolves
its release position against the floor using the original capture policy.
It also handles the reversed holder/attachment roles used by grounded captures.
This is not equivalent to clearing the victim reference manually.

It takes no arguments, is Fighter-only, and does nothing without a victim.
A present victim must have the established mutual link. Both retained actor
identities are checked after unlinking and across Fall entry; retirement stops
the remaining script work. The unlink routine marks a hitlag-end flag but does
not itself run the hitlag-end callback.

## `capture.armed`

Read or set the executing Fighter's common capture-eligibility flag as Bool.
Clearing it prevents another capture after the move accepts a victim. Unlike
`capture.arm(mask)`, setting this flag does not assign a capture mask, install
callbacks or initialize capture state. Use `capture.arm` to set up a new grab;
use this property for subsequent policy changes.

This is an owned Fighter property. It retains the actor's exact program,
storage, native data and scene generation and rejects a stale actor before
reading or writing. It is not a substitute for unlinking a captured pair.
