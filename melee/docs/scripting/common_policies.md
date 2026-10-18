# Common action policies

These callbacks customize shared action decisions and presentation without
depending on a stock fighter ID. They compile into interpreted bytecode callbacks; the
shared engine still performs input tests, animation setup and physics.

[Callback syntax](language.md) | [Fighter API](fighter.md) |
[Combat and presentation](combat_visuals.md) |
[Marth/Roy example](../marth_roy_script_translation.md)

## `on jab_start:`

Return integer motion flags for shared jab entry. With no handler, entry uses
zero flags. The callback runs after common pickup arbitration and before the
jab motion is selected, so it can register a state-change callback at this point.
Faults or fighter removal stop the entry before animation setup continues.

```text
on jab_start:
    callbacks.state_changed(record_jab)
    return Transition.skip_attack_count
```

These are engine motion flags, not an action or animation ID. Returning
`Transition.skip_attack_count` suppresses that transition's attack-count update.

## Auxiliary-item presentation

`on auxiliary_item_attach:` runs after the engine assigns a bunny-hood or
flower slot and its timers. `on auxiliary_item_visibility_reset:` runs before
their removal or expiration handling. Both callbacks return no value.

The engine preserves the stock gate: a callback runs if either auxiliary slot
is empty and is skipped only when both are occupied. This is separate from
ordinary held-item pickup/drop and visibility callbacks. Scripts own their
visual-group decisions; missing handlers do not dispatch stock fighter code.
The engine stops subsequent common work if a handler faults or removes the
fighter.

## `on action_metadata:`

An integer query while the engine enters an action. `event.metadata` is the
incoming action descriptor in the engine's canonical word format;
`action.move_id(event.metadata)` reads its move classification. Return the descriptor to commit,
or return `event.metadata` to retain it. An omitted return also retains the
incoming descriptor. This event does not change the animation's separate move
ID, subactions, hitboxes or motion selection.

The shared attack counter observes the original incoming classification first.
Your query then runs, followed by the common held-item classification and
statistics setup. Keep this query free of transitions, RNG and side effects.
If a callback removes or recycles the fighter, both this operation and its
motion-entry caller stop before continuing with the old fighter.

### `action.move_id(metadata) -> int`

Read the low eight bits of a canonical descriptor. This pure operation does
not read the fighter's current action, so it can inspect the incoming word
before the engine publishes that action.

### `action.metadata(move_id, category, animation_flags, statistics_flags)`

Construct a descriptor without depending on host byte order or C bitfield
layout. All four arguments are integer expressions, evaluated once in order.
Move and statistics fields keep their low eight bits; category and animation
flags keep their low four bits. The reserved upper byte is zero. To preserve
an existing descriptor unchanged, return `event.metadata` instead.

Category and flags are engine action metadata, separate from transition flags.
Their values can be ordinary named project parameters. For example, Luigi's
damaging taunt changes its move classification from 113 to 99 and its category
to 2, with animation flags 4 and statistics flags 0:

```text
parameter int taunt_move = 113
parameter int damaging_taunt_move = 99
parameter int damaging_taunt_category = 2
parameter int damaging_taunt_animation_flags = 4
parameter int damaging_taunt_statistics_flags = 0

action Classification:
    on action_metadata:
        if action.move_id(event.metadata) == attrs.taunt_move:
            return action.metadata(attrs.damaging_taunt_move, attrs.damaging_taunt_category, attrs.damaging_taunt_animation_flags, attrs.damaging_taunt_statistics_flags)
        return event.metadata
```

The native engine decodes canonical descriptors into named fields explicitly.
Earlier code reinterpreted the console word as a Windows memory layout, which
lost the low-byte move classification and bypassed this taunt rule. The fix
restores intended metadata semantics; matching that representation error is
not a fidelity requirement.

## `on jab_start:`

Return the integer preservation flags for the common first-jab transition.
This runs before that transition, so it can also register a one-shot
[`callbacks.state_changed`](functions.md#callbacksstate_changedfunction)
helper. The result is an integer bitmask, not a boolean.

```text
func record_jab():
    stats.record_attack()
    stats.refresh_attack()

action JabPolicy:
    on jab_start:
        callbacks.state_changed(record_jab)
        return Transition.skip_attack_count
```

`Transition.skip_attack_count` skips the transition's automatic attack-count
update. The helper above performs the stock bookkeeping in its original order
after entering the new motion. Other fighters can return `0` for ordinary
transition behavior. This hook does not replace input or item-pickup checks.

## Common entry completion

`on wait_enter:`, `on crouch_wait_enter:` and `on aerial_attack_enter:`
run after the shared entry has changed motion and initialized its animation.
Wait and crouch also finish held-item animation selection and their ordinary
friction setup first. Aerial entry clears its common interrupt and command
flags before changing motion. None of these callbacks replaces those steps.

Use these hooks to install package-owned callbacks or finish an accessory's
presentation. For example, an aerial entry can inspect `motion.id` and install
its own collision callback for a downward attack. The callback receives no
event arguments. It runs once per accepted entry, rather than every tick.

Wait and crouch declarations replace the stock fighter-specific completion
branch; an absent declaration retains that branch. Aerial completion adds
package setup after the common aerial entry. Generation, kind and definition
checks stop entry if a nested motion or animation callback retires or replaces
the fighter. The entry does not access the fighter after the final hook.

```text
parameter int down_air_motion = 69

action CommonEntry:
    on wait_enter:
        restore_accessory()
    on crouch_wait_enter:
        restore_accessory()
    on aerial_attack_enter:
        if motion.id == attrs.down_air_motion:
            callbacks.collision(down_air_collision)
```

The example helpers must be defined in the package. Both source forms support
these events. They are separate from `wait_input`, which decides interrupts
during grounded Wait.

## `on forward_smash_start:`

Runs after the ordinary forward-smash transition and animation setup. Use
it for post-entry setup such as `effects.pause_during_hitlag()` or installing an
authored interrupt with `callbacks.interrupt(helper)`. It receives no event
arguments. Common input acceptance, angled motion selection, charging, physics
and subactions remain in place unless the callback explicitly replaces a phase.
Fighters with a different entire smash implementation should use the complete
entry hooks below, then their own action handlers.

## Complete smash entry

`on forward_smash_enter:`, `on up_smash_enter:`, and `on down_smash_enter:`
replace the accepted attack's entire entry policy. Common input checks and
held-item throw, swing, or firing priority run first. The callback then owns
its motion transition, animation initialization, charge setup, article creation,
and callback installation. No stock fighter smash handler runs first.

Forward entry receives `event.angle`, the selected stick angle in radians.
The common input path has already selected `facing.direction`. Up and down
entry have no event arguments. The forward replacement takes precedence over
`on forward_smash_start`; it does not invoke that post-entry hook afterward.

```text
action SmashEntry:
    on forward_smash_enter:
        begin_side_smash(event.angle)
    on up_smash_enter:
        begin_up_smash()
    on down_smash_enter:
        begin_down_smash()
```

These illustrative helpers must be defined in the fighter's script. Shared
[smash charge](smash_charge.md) properties let them reuse the common charging
mechanism while owning the move's decisions. Without a replacement declaration,
the existing entry policy applies. Both `action` and `state` source forms support
these hooks; independent modules clear undeclared inherited callbacks.

## Auxiliary item visibility

`on auxiliary_item_attach:` runs at the shared auxiliary-item attachment point.
`on auxiliary_item_visibility_reset:` runs when that path restores visibility.
These slots are separate from ordinary held-item pickup/drop notifications.
The engine retains the existing attachment conditions and ordering; a declared
hook replaces only the fighter-specific additional work.

```text
on auxiliary_item_attach:
    visuals.set_group(1, -1)
    visuals.set_current_group(1, 0)
on auxiliary_item_visibility_reset:
    visuals.set_group(1, 0)
    visuals.set_current_group(1, 0)
```

This selects the same saved/live visibility rows as Pikachu. Group indices
belong to the authored costume, so other fighters must choose their own rows
or use `pass` when they need no additional change.

## `on jab_restart:`

A boolean query before the common jab chain enters its requested second or
third attack. `event.motion` is the requested motion: `45` for Attack12 or
`46` for Attack13. It is not the current fighter motion.

Return `true` to restart the ordinary first jab, including its item-pickup
test, animation initialization and input-window reset. Return `false` to
enter the requested follow-up using the standard engine behavior. The
callback replaces the stock kind exception; it does not start an action by
itself. It runs only when the existing subactions and input checks request a
follow-up. Giving a single-jab fighter this callback does not add more jabs.

The interpreter calls this query from both follow-up paths. An absent hook
uses the normal follow-up without borrowing a stock fighter's exception. A
query fault or fighter retirement stops before the requested transition.
The focused scheduler fixture compares the repeating third-jab decision with
native Marth and the ordinary third jab with native Mario; it also exercises
second-jab restart, the missing-hook default and retirement from either query.
Item pickup, first-jab entry and motion initialization are controlled leaves
there, so this does not establish complete jab gameplay parity.

```text
action JabPolicy:
    on jab_restart:
        return event.motion == 46
```

This is Marth's two-hit repeating chain. A fighter with an ordinary third
jab can return `false` for both requests. Keep this query free of state
writes, RNG and transitions. Helpers can receive `event.motion` as a typed
integer argument; it is available only in callbacks that receive a motion.

## `on shield_start:` / `on shield_reflect_start:`

Events after the shared shield-entry setup completes. `shield_start` runs for
ordinary shield entry; `shield_reflect_start` runs for powershield entry.
At this point the engine has already entered the appropriate shield motion,
initialized its collision/analog state, and created its standard effect and
sound. A declared callback replaces additional fighter-specific entry
presentation. It does not replace the shared shield simulation or a fighter's
entire shield implementation.

```text
action ShieldPresentation:
    on shield_start:
        visuals.set_current_group(1, 1)
        sound.play(190115, 127, 64)
    on shield_reflect_start:
        visuals.set_current_group(1, 1)
        sound.play(190115, 127, 64)
```

These values reproduce Marth's stock presentation. Other fighters can select
their own groups, effects and sounds, or use `pass` for no additional work.
Interpreted fighters run common setup first and then their own presentation;
an absent hook adds no stock-kind presentation. Retirement during common setup
stops before the hook. These hooks do not replace the separate full
`shield_transition` policy used by unusual shield implementations; its
interpreter migration remains open.

## `visuals.set_group(group, value)`

Change the saved model-visibility selector for a later visibility reset.
Both arguments accept expressions, including `costume.variant`. It does not
immediately rebuild the visible display-object list. Group and value bounds
are the same as the current-selector operation below.

## `visuals.set_current_group(group, value)`

Change a live model-visibility selector immediately and rebuild the visible
display-object list if the value changed. This differs from `visuals.set_group`,
which sets the saved selector for a later visibility reset. The current-only
change does not alter that saved selector.

Both arguments may be expressions. The group must exist in the current
fighter's visibility table and fixed selector storage; the value must be
`-1..127` (the native selector is a signed byte). A value absent from a channel
hides all its alternatives; an empty row still records the selector, as in the
native engine. Invalid arguments or unowned tables fault instead of changing
state. There is no allocation. Group numbers
refer to the costume's visibility rows, not skeleton bones or hitbox groups.

## `on dynamics_wind:`

A boolean query opting the fighter's existing dynamic-bone chains into the
engine's global wind override. Return `true` to let global wind temporarily
take precedence over their keyed animation, then restore the animation when
the wind ends. Return `false` to retain the ordinary keyed/dynamic policy.
The engine's explicit dynamics-disable flag still takes precedence.

```text
action ClothPolicy:
    on dynamics_wind:
        return true
```

This matches Marth/Roy's stock policy without selecting them by kind. The
query creates neither bones nor a wind field. Local attack wind effects are
separate from the global wind activation used here. Keep this query free of
state writes, RNG and transitions; it may run while changing an animation or
updating a fighter's dynamic chains.
Both native wind-reset and animation/dynamic-chain selection paths consult
this query. An absent interpreted hook opts out. A failed or retiring query
stops before the wind or bone operations continue with that fighter.

## `on dynamics_ground_collision:`

A boolean query immediately before the engine updates the fighter's dynamic
bone chains. Return `true` to constrain those chains against the floor, or
`false` to skip their floor collision. This is separate from the fighter's
environmental collision box, hurtboxes, keyed animation and global wind.

```text
action DynamicBonePolicy:
    on dynamics_ground_collision:
        return grounded and model.instance_scale_y == 1
```

That example enables floor collision for an ordinary-size grounded fighter.
The [Bowser template](../../examples/koopa_wip.fms) returns `false`, matching
stock Bowser's unconditional exclusion. A declared callback replaces the
stock kind/scale policy at this point. Keep it free of state writes, RNG and
transitions; declaring it does not create dynamic bones or their colliders.

## `on dynamics_interaction_start:`

An integer query before each dynamic bone set is updated. `event.set` is its
zero-based index and `event.set_count` is the number of active sets. Return the
first chain element that should receive interaction forces. Elements before
that index still use the shared dynamics solver; they skip its interaction
force step. Floor constraints are controlled separately by
[`on dynamics_ground_collision`](#on-dynamics_ground_collision).

```text
action DynamicInteractions:
    on dynamics_interaction_start:
        if event.set == event.set_count - 1: return 0
        return 3
```

This example uses different policies for the last set and earlier sets.
Returning `8` for every set reproduces Mewtwo's stock policy. No callback
retains the stock policy. The query must not change state, consume RNG or
create bones; it only selects the chain index used by the existing solver.

## `on aerial_jump:`

A void entry callback when the shared aerial-jump input check accepts a jump,
after held-item jump interception and before stock fighter-specific entry.
Declaring it replaces that entry. The script owns ground/air conversion,
motion selection and any jump-specific flags; the common input check still
enforces jump count and controller conditions. See
[`jump.begin()`](movement.md#jumpbegin) for shared jump bookkeeping.

## `on body_rebound:`

A boolean query on the fighter whose guard intercepted an inert body attack.
Return `true` to allow the attacking fighter's ordinary rebound transition,
or `false` to suppress that transition. The callback runs on the intercepting
fighter, so `motion.id` and fighter variables refer to that fighter, not the
attacker. It does not apply counter damage or create a guard volume.

```text
action CounterContactPolicy:
    on body_rebound:
        return motion.id == 369 or motion.id == 371
```

This reproduces the stock Marth/Roy counter-state exception for Peach's
side-special without testing the custom fighter's stock kind. A different
fighter can use its own motions or ordinary state variables. Keep the query
free of state writes, RNG and transitions. The attacking move still owns its
rebound animation and physics; declaring this query alone does not add a
body attack to another fighter.

## Verification

Compiled fixtures exercise both script forms, motion-query context, changing
wind decisions and shield presentation arguments. The gameplay runner checks
real jab/shield transitions and uses Corneria for global-wind coverage. Its
fighter traces include visibility selectors and wind activation alongside
pose, collision and RNG digests. See the [translation ledger](../marth_roy_script_translation.md)
for accepted recordings and the remaining broader scope.

## on roll_enter

Runs after shared forward/backward roll initialization. A fighter can install
its own animation/collision callbacks while preserving the common roll input,
physics and turnaround behavior. It owns any extra shape or presentation
state, for example an ordinary `state.morph_hurtbox` flag.

```text
action RollPresentation:
    on roll_enter:
        state.morph_hurtbox = false
        callbacks.animation(roll_pose)
        callbacks.collision(roll_collision)
```

[`Samus's translation`](../samus_script_translation.md) demonstrates this
hook. [`collision.ground_standard_box()`](movement.md#collisionground_standard_box)
uses the shared grounded collision algorithm with an authored box, including
its floor correction and edge behavior.

## on worn_item_jump_sound

This predicate controls the worn/head-item sound for shared item-induced aerial
jumps and multiple-air-jump entry. Return true to permit the ordinary sound or
false to suppress it. It changes no voice, movement, jump count or item state.
Absent callbacks preserve the stock kind policy. The explicit final bool in
[`jump.begin`](movement.md) remains the authored
normal jump's own decision; this callback does not override that argument.

```text
action JumpPresentation:
    on worn_item_jump_sound:
        return false
```

Mewtwo and Ness suppress this sound in stock behavior. Other custom fighters
can choose their own policy without a donor-kind exception in shared C.
The ordinary interpreter dispatches this query in both the shared worn-item
launch and native multiple-jump entry. Actor/program/storage and cached jump
attributes must remain current before the caller continues into jump entry
or post-entry bookkeeping. A script-owned `aerial_jump` handler replaces the
multiple-jump entry earlier; its explicit `jump.begin` sound argument remains
its own policy, as with any other authored jump.

## Whole shield transitions

`on shield_transition:` replaces a fighter's entire entry policy for the
requested shield transition. `event.mode` identifies the request:
`Shield.start`, `Shield.hold`, `Shield.release`, `Shield.hit`,
`Shield.reflect_continue`, or `Shield.reflect_start`. `event.special_hit` is
the existing shared shield-hit entry's boolean argument. The callback owns
its motion change, animation setup, volume creation and presentation; it must
perform them explicitly. Declaring this callback takes precedence over the
post-entry `on shield_start` and `on shield_reflect_start` hooks above.

The shared input checks still decide when to request a transition. Shared
release bookkeeping before the callback and shared special-hit work after
it retain their original ordering. Use this hook for an authored shield
shape or states, rather than adding stock-kind exceptions to common C.

This callback now runs in the ordinary interpreter at all six entry requests,
including the hold/release paths used after shield stun and powershield startup.
A fault does not run the stock entry instead. The shared special-hit continuation
runs only after a successful callback with the same live fighter, program and
state storage. Scripts without this hook retain ordinary shared entry behavior
and may use the narrower post-entry hooks above.

`on shield_scale:` returns a **float** radius multiplier used by the common
shield-joint scale calculation. `on shield_alpha:` returns whether the
ordinary shield alpha policy applies. These are queries, not transitions;
keep them free of RNG and state changes.
Both queries run in the ordinary interpreter. The radius result is applied only
while the same fighter, program, storage and shield joint remain live. A failed
query does not substitute stock policy. Alpha updates likewise stop on failure
or retirement. A script without these hooks retains the shared stock policy.

### Shield engine operations

The following guard operations run in the ordinary interpreter. Shared state
and tuning fields are documented in [Guard volumes](guard.md#shared-shield-state).

| Operation | Shared engine work |
| --- | --- |
| `guard.install_shield()` | Install the ordinary shield volume. |
| `guard.install_reflector()` | Install the common powershield descriptor with the script's `on reflect_hit` callback. That callback must be declared, even if its body is `pass`. |
| `guard.consume_release_input()` | Apply the shared held-input/release request policy. |
| `guard.drain()` | Apply shield depletion and return its shared result. |
| `guard.tick_powershield()` | Update the shared powershield windows. |
| `guard.track_sdi()` | Install the shared shield SDI hitlag and post-hitlag callbacks. |
| `guard.enter_hold()` / `guard.enter_release()` | Request the shared hold or release transition, including the authored transition hook. |
| `guard.scale_joint()` | Update the shield joint scale using the radius query. |

These input, physics and collision operations also run in the ordinary
interpreter. Input checks return whether they accepted a transition. A callback
fault or retirement stops the enclosing powershield input traversal.

| Operation | Shared engine work |
| --- | --- |
| `controls.guard_reflect_actions()` | Run the common powershield interrupt checks. |
| `controls.try_powershield()` | Test the shared powershield entry condition. |
| `controls.try_guard_item_throw()` | Test the shared shield item-throw action. |
| `controls.try_guard_spotdodge()` | Test spot-dodge input from shield, without requiring the shield button to remain held. |
| `controls.try_spotdodge()` | Test the same fresh downward input, requiring a held shield button. |
| `controls.try_roll()` | Test shared horizontal roll input, including C-stick input, and choose forward/backward using facing. |
| `controls.try_guard_platform_drop()` | Test the shared platform drop from shield. |
| `physics.guard_motion()` / `physics.guard_hit_motion()` | Run common shield or shield-hit physics. |
| `collision.guard_standard()` / `collision.guard_hit()` | Run their common collision callbacks. |

These operations do not call a stock fighter shield handler. The script
chooses their order, owns its timers and selects its own animations.

### Shield state and tuning

Writable engine properties include `guard.elapsed`,
`guard.release_requested`, `guard.minimum_hold_frames`, `guard.reflect_frames`,
`guard.powershield_frames`, `guard.input_lock_frames`, `guard.interrupt_mode`,
`guard.delayed_action_frames`, `guard.volume_enabled`, `guard.new_powershield`,
`guard.reflect_window`, `guard.powershield_window`, and `shield.health`.
`input.analog_shield_age` and `input.horizontal_tilt_age` are writable shared
input timers; the latter is distinct from `input.horizontal_smash_age`.
They count frames since the respective native analog-shield or horizontal-tilt
input test last reset them. The engine advances and saturates them at 254;
scripts need not increment them. Writes accept integers from 0 through 254.
Float values and integers outside that range are rejected before narrowing.

Read-only shared values are `shield.light_amount`, `shield.bone`, and
`shield.initial_size`. `shield.bone` is the shield attachment's
[bone ID](bone_queries.md#bone-ids). Common tuning queries are `shield.start_health`,
`shield.minimum_hold_duration`, `shield.pushback_damage_scale`,
`shield.pushback_base`, `shield.pushback_velocity_scale`,
`shield.reflect_duration`, `shield.reflect_radius`,
`shield.reflect_damage_multiplier`, `shield.reflect_speed_multiplier`, and
`shield.powershield_duration`. Fighter-specific adjustments should be named
package parameters; ordinary script variables hold mutable move state.

## `on spotdodge_enter:`

Runs when the shared spotdodge entry is accepted, before common motion
initialization. The common spotdodge still initializes after a successful
callback; this hook can restore a shape or visibility policy first. It does not
replace the whole entry. `on roll_enter` runs after common forward/backward roll
initialization, allowing authored animation/collision callbacks to remain
installed. Both hooks run in the ordinary interpreter; source faults or
retirement stop the remaining initialization/statistics work.

## `motion.fall_animation`

Reads or assigns the common fall-animation selector while the fighter is in
Fall or FallAerial. This selector controls common animation blending; it is
not the current skeletal clip ID. Other motions return âˆ’1 and ignore writes.
Use a declared fighter variable for your own countdowns or move phases.

This read/write property runs in the ordinary interpreter. Values must fit the
native signed 32-bit `Int`; out-of-range integer writes reject before narrowing.
[`jump.update_animation()`](movement.md#jumpupdate_animation) can initialize it
when the aerial-jump animation completes. Both operations require the current
owned fighter, program and script storage.

When a callback calls a common motion update, inspect the resulting motion
before continuing. That update may enter FallAerial and initialize this
selector. An authored callback can explicitly preserve a subsequent common
selector change without relying on overlapping stock motion-union fields.

## Grounded grab policies

These callbacks belong to the grabbing fighter. Catch/CatchDash are its grab
startup states; CatchPull draws the opponent in, and CatchWait holds them.
CaptureWait is the caught fighter's state, a separate policy boundary.
All hooks are available in both `action` and `state` source forms.

| Handler | Result | When it runs |
| --- | --- | --- |
| `catch_allowed` | Bool | After held-item throw priority, before normal standing/dash grab input checks. False vetoes the grab. |
| `catch_start` | Void | After common acceleration/counter reset, before standing/dash grab motion initialization. |
| `catch_frame` | Bool | Replaces both tether-specific animation decisions in standing/dash startup. True consumes the frame; false continues common animation completion. |
| `catch_exit` | Void | Replaces tether cleanup before falling out of a standing/dash grab. |
| `catch_wait_release` | Void | After successful pummel or directional-throw entry. A successful pummel short-circuits the throw-input check. |
| `catch_pull_anchor` | Bool | After pull start-frame selection, before throw-flag reset and motion initialization. True retains the anchor the script bound; false selects the common capture joint. |
| `catch_pull_ready` | Bool | Replaces the complete pull readiness decision. True requests common CatchWait entry. |
| `catch_pull_follow` | Bool | After pull motion initialization. True installs common positional following; false clears the follow callback. |

Without a declared override, the existing policy remains. A declared frame,
allowed, anchor, or ready policy is exclusive: a false result does not run a
stock fighter's tether decision afterward. `catch_start` adds setup work;
it does not replace the ordinary capture callback installation. Define mutable
grab counters as ordinary fighter variables, and put timing/geometry tuning in
named parameters.

An authored tether can bind its endpoint through
[`bind_capture_anchor()`](article_resources.md), then return the bind result
from `catch_pull_anchor`. Return false when the chain is absent. This provides
a defined common-anchor fallback where a stock tether handler would dereference
a missing chain. `catch_pull_follow` can keep the chain's own solver authoritative
without relying on hidden stock-kind exclusions in the common follow callback.

Continuations check the source generation, kind and Definition after scripts.
Pull and capture-wait callbacks also validate a retained linked fighter before
continuing; removal, recycling or replacing the retained link stops the tail.
An invalidated `catch_allowed` source consumes the input chain, even when the
script returned false. Cleanup callbacks that terminate the function need no
later source reads.

## `on item_swing_motion:`

An Int query selecting the fighter's custom held-item swing motion. The shared
swing entry calls it for variant 3; the ordinary variants retain the common
motion table. Read `event.swing_type` (0-5) and `event.swing_variant` (3 in this
caller). Return a motion present in the fighter's owned layout. Falling through
returns -1 and cancels entry. A missing hook on an interpreted fighter also
cancels variant 3 instead of borrowing a stock fighter's special swing policy.

```text
action SwingSelection:
    on item_swing_motion:
        return 341 + event.swing_type
```

The entry validates the owned motion tables and retains the source through the
query. Removing or replacing the source stops before reading swing rates or
calling the selected swing entry. The query chooses an animation; it does not
change the shared item's hit detection or swing physics.

## Aerial input and grounded motion setup

See [Aerial actions](aerial_actions.md) for the manual/automatic input predicates and grounded setup hook. These hooks own fighter decisions while retaining the shared callers and collision solver.


Shared smash startup preserves ordinary angled-motion selection and animation
startup before calling `forward_smash_start`. Motion setup and animation may run
nested callbacks; if those callbacks remove, recycle or replace the fighter or
its Definition, startup stops before the next operation. Up- and down-smash
common startup apply the same continuation rule. The focused
`native_common_entry` fixture executes these actual source bodies with controlled
motion and animation boundaries; it does not claim whole-match coverage.
