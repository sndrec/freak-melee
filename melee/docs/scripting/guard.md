# Guard volumes

Guards intercept fighter and article attacks using Melee's shared collision
system. A script supplies the volume, its active window, and the reaction.
These operations do not call a particular fighter's counter handler.

[Script callbacks](language.md) | [Combat](combat_visuals.md) |
[Reflectors](reflector.md) | [Marth/Roy example](../marth_roy_script_translation.md)

## `guard.create(bone, x, y, z, radius, helper)`

Attach a guard sphere to a costume joint. `bone` is a numeric joint index;
use [`bones.index()`](#bonesindexname) to resolve a semantic bone instead.
The offset is in the joint's local coordinates. The call activates the volume,
resets its reaction flags, and binds the supplied root helper to the shared
ShieldHit callback slot. The helper takes no arguments and returns no value.
Use `None` for a volume with no script reaction. Creating another volume replaces
that binding; recreate it after a state change if the guard should stay active.
Invalid joint indices or negative/non-finite radii leave the existing volume and
callback unchanged. Joint indices accept integer expressions without narrowing
wide values into a different valid joint.

```text
func GuardReaction():
    facing.direction = guard.hit_facing
    state.saved_damage = guard.hit_damage
    if airborne: state.enter_at_speed(372, 0, 0, 1)
    else: state.enter_at_speed(370, 0, 0, 1)

func enable_guard():
    guard.create(bones.index(Hip), 0, 0, 0, attrs.guard_radius, GuardReaction)
    guard.reactive = true
```

Declare `saved_damage` as integer fighter state and `guard_radius` as a fighter
tuning parameter. The example state IDs belong to the swordsman templates.
Replace them with
your fighter's reaction motions. The script owns how intercepted damage
affects its attack: it can use fixed damage, multiply the captured damage,
or implement a different reaction entirely.

## Properties

| Property | Access | Meaning |
| --- | --- | --- |
| `guard.active` | Read/write bool | Enable or disable collision with this volume. |
| `guard.reactive` | Read/write bool | Select the engine's reactive-guard collision treatment, including article contact. Reset by `guard.create()`. |
| `guard.front_only` | Read/write bool | Accept guard contact only from the fighter's facing side. Independent of `guard.active` and `guard.reactive`; reset to false by `guard.create()`. |
| `guard.filtered_attacks_only` | Read/write bool | Restrict contact to attacks carrying the engine's guard-eligibility hitbox flag. This is separate from the attack's reflectable flag. |
| `guard.skip_contact_details` | Read/write bool | Suppress the shared guard contact-detail calculation and its article contact follow-up selection. It does not disable the guard volume. |
| `guard.melee_hitlag` | Read/write float | Nonzero override for fighter contact hitlag; zero leaves the ordinary calculation. |
| `guard.article_hitlag` | Read/write float | Nonzero override for article contact hitlag. |
| `guard.hit_damage` | Read-only int | Largest intercepted damage recorded by the guard collision pass. Read in the reaction helper before the engine clears the contact data. |
| `guard.hit_facing` | Read-only float | Facing direction recorded for the intercepted attack. Read in the reaction helper. |

State changes normally clear active collision state. Recreate a guard if a
ground/air transition must preserve its window. Melee's collision pass still
controls contact detection and callback timing; changing these properties
does not immediately trigger a reaction.

Set `guard.front_only = true` after creating a volume to filter fighter and
article attacks by their source's horizontal position. Facing left rejects
sources to the right; other facing values reject sources to the left. Equal
horizontal positions pass. This follows the shared collision pass's position
test, rather than an attack's velocity or hitbox orientation. Setting the
filter does not activate a guard or enable its reactive treatment.

`filtered_attacks_only` applies the same eligibility flag to fighter and item
hitboxes. It does not mean "projectiles only" or "reflectable attacks only".
`skip_contact_details` is an independent collision policy. Both flags are reset
by shared guard creation; set them after installing the volume when needed.

## `guard.update_position()`

Permit the collision system to refresh the guard's joint-relative position.
Call this in the physics phase while an animated guard is active, as the
stock counter does. It does not advance the animation or simulate a hit.

## `bones.index(Name)`

Resolve a semantic bone to the current fighter's costume joint index. Available
names are `TransN`, `XRotN`, `YRotN`, `Top`, `Hip`, `LeftHand`, `RightHand`,
`RightThumb`, `LeftFoot`, `RightFoot`, `LeftShoulder`, and `RightShoulder`.
This uses the current fighter's bone mapping rather than a donor's joint number.
It can also supply the part argument to
[`effects.spawn_with_facing()`](combat_visuals.md).

## `callbacks.clear_hitlag()`

Clear the fighter's pre-hitlag and post-hitlag callback bindings. It does not
clear the current hitlag timer. This is useful when a new reaction replaces
effect pause/resume behavior established by the previous motion.

## Shared shield recoil tuning

`shield.pushback_light_min` and `shield.pushback_light_max` are read-only floats
from the match's common shield attributes. They define the endpoints used when
the shared recoil calculation interpolates by lightshield amount. They are
separate from `shield.pushback_damage_scale`, `shield.pushback_base` and
`shield.pushback_velocity_scale`. A script owning a shield policy can use these
common constants with its own fighter parameters; no DAT offsets are required.

## Shared shield state

These fields are available to the ordinary interpreter. Guard timers and input
modes occupy motion-local state shared with the common guard routines; other
motions reuse that storage. Shield health and light-shield amount belong to the
fighter independently of its current motion.

| Property | Type / access | Meaning |
| --- | --- | --- |
| `guard.elapsed` | Float, writable | Time spent in the guard motion. |
| `guard.release_requested` | Bool, writable | Whether guard release has been requested. |
| `guard.minimum_hold_frames` | Float, writable | Shared minimum-hold timer. |
| `guard.reflect_frames` | Float, writable | Remaining reflector window. |
| `guard.powershield_frames` | Float, writable | Remaining powershield window. |
| `guard.input_lock_frames` | Int, writable | Shared guard input-lock state. |
| `guard.interrupt_mode` | Int, writable | Shared guard interrupt mode. |
| `guard.delayed_action_frames` | Int, writable | Shared delayed-action timer. |
| `guard.volume_enabled` | Bool, writable | Enables the shared shield volume. |
| `guard.new_powershield` | Bool, writable | Marks powershield startup. |
| `guard.reflect_window` | Bool, writable | Enables the reflector window. |
| `guard.powershield_window` | Bool, writable | Enables the powershield window. |
| `shield.health` | Float, writable | Current shield health. |
| `shield.light_amount` | Float, read-only | Current analog light-shield amount. |
| `shield.bone` | Unsigned 32-bit, read-only | Raw shield joint index from the fighter data. |
| `shield.initial_size` | Float, read-only | Fighter's base shield size. |

Read-only Float tuning values come from the game's common data:
`shield.start_health`, `shield.minimum_hold_duration`,
`shield.reflect_duration`, `shield.reflect_radius`,
`shield.reflect_damage_multiplier`, `shield.reflect_speed_multiplier` and
`shield.powershield_duration`. These supplement the pushback values above.
Use named fighter parameters for your own tuning rather than altering global
common data. [Fighter variables and parameters](program.md) describe that split.

```text
guard.elapsed = 0
guard.release_requested = false
guard.minimum_hold_frames = shield.minimum_hold_duration
guard.reflect_frames = shield.reflect_duration
guard.powershield_frames = shield.powershield_duration
```

Integer guard fields retain signed 32-bit values. Direct backend calls reject
values outside that range; Bool fields require canonical true/false values.
Float fields preserve native Float values, including signed zero. Bone and
common-tuning queries require their corresponding engine data to be present.

## Shared shield operations

These methods operate on the shared shield state above. They are available to
ordinary interpreted Fighter callbacks and helpers; none selects a stock
fighter's shield policy. The script chooses their order and its own motions.

| Method | Result | Shared work |
| --- | --- | --- |
| `guard.scale_joint()` | Void | Set the owned shield joint's uniform scale, using `on shield_scale` when authored. |
| `guard.install_shield()` | Void | Create the common shield volume on the data's shield joint, enable it, and install shared shield-hit entry. |
| `guard.install_reflector()` | Void | Create the common powershield volume and bind the authored `on reflect_hit`. |
| `guard.consume_release_input()` | Void | Update the shared release request from the held input. |
| `guard.drain()` | Bool | Advance minimum hold, deplete shield health, and enter shared shield break when necessary. |
| `guard.tick_powershield()` | Void | Advance the shared reflection and powershield windows and disable expired reflection. |
| `guard.track_sdi()` | Void | Replace the hitlag/post-hitlag slots with shared shield SDI callbacks. |
| `guard.enter_hold()` / `guard.enter_release()` | Void | Request shared entry, honoring `on shield_transition` when authored. |

`guard.install_shield()` uses the common descriptor; use `guard.create()` when
you want to author a different shape or contact helper. Likewise,
`guard.install_reflector()` uses common radius and multipliers rather than the
custom descriptor accepted by [`reflector.create()`](reflector.md#reflectorcreate).
It requires an authored `on reflect_hit`, even if the handler only contains
`pass`. It binds that callback after construction, which clears the old slot.

`guard.drain()` returns true after shield break. Stop the current guard policy
in that case; the fighter has entered the break transition. Its depletion and
minimum-hold ordering are the native shared rules. `guard.tick_powershield()`
advances existing timers; it does not start a new window. `guard.track_sdi()`
replaces any previous interpreted post-hitlag handler as well as the native
callback slots, so that an earlier move's callback cannot run in its place.

```text
func advance_guard():
    guard.consume_release_input()
    if guard.drain(): return
    if guard.release_requested: guard.enter_release()
    else: guard.scale_joint()
```

Scaling and common volume installation require the fighter's exact imported
joint table. Retired/replaced actors and a rig replaced during a radius query
stop the operation before publishing a result or using the old joint. These
operations use existing engine storage and do not allocate script state.
See [whole shield transitions](common_policies.md#whole-shield-transitions) for
the radius and transparency query contracts.

## Shield inputs, physics and collision

All of these operations run in the ordinary interpreter:

| Method | Result | Work |
| --- | --- | --- |
| `controls.try_powershield()` | Bool | Accept fresh digital shield input within the common startup and analog-input age windows. |
| `controls.try_guard_item_throw()` | Bool | Accept attack input with a held item, selecting the delayed dash-throw branch when its timer is nonzero; otherwise decrement that timer. |
| `controls.try_guard_platform_drop()` | Bool | Drop from a pass-through platform on fresh downward input while shield is held. |
| `controls.guard_reflect_actions()` | Void | Run native powershield interrupt checks in their original priority order. |
| `physics.guard_motion()` | Void | Apply common ground friction/movement and permit shield and reflector geometry refresh. |
| `physics.guard_hit_motion()` | Void | Apply the same ground movement and shield refresh, without the reflector refresh. |
| `collision.guard_standard()` | Void | Run common guard ground collision. |
| `collision.guard_hit()` | Void | Choose ordinary guard collision or the SDI collision path using the live `allow_sdi` flag. |

[Roll and spot-dodge checks](movement.md#interrupt-checks-and-platform-dropping)
are separate methods. Inside a shielding policy, use
`controls.try_guard_spotdodge()`: releasing the button must not add an eligibility
condition absent from the stock shield check. Outside shield, use
`controls.try_spotdodge()` when the move requires held shield input.

These calls can change motion and run authored entry hooks. Once a Bool check
returns true, stop the old move's interrupt policy. A faulted or retired source
never publishes a successful result or continues the powershield priority chain.
Collision methods execute the existing stage collision algorithms; they do not
give a script direct ownership of stage geometry.

## Comparison boundary

The [direct-interpreter checkpoint](../native_source_guard_checkpoint.md) records
the current connected VM, native constructor and ShieldHit delivery tests. Its
fixture boundaries are separate from the earlier gameplay recordings below.

The compiled guard fixture exercises both script forms, callback binding,
descriptor values, position refresh, damage/facing reads, flag writes, callback
clearing, and invalid volume inputs. Gameplay comparison uses real stock and
scripted counter attacks; see the [translation ledger](../marth_roy_script_translation.md)
for the specific recordings and outstanding branches.

The front-filter source fixture separately checks the real generated reads and
writes, both actual fighter/article facing-filter predicate slices, and the
complete shared guard-creation reset. This covers equality and both facing
directions while keeping active/reactive and auxiliary flags distinct. It does
not exercise the full collision traversal or establish a new gameplay result.
