# Held article policies

A held article can define a status query and an action request. Both callbacks
belong to the article's package, even when another fighter picks it up. Shared
engine item transitions dispatch to the actual held instance; a saved creator
handle does not stand in for the held slot.

## `on held_status:`

Returns an **Int** status, or `-1` when the article supports no current held
status. This is a read-only query: conditions, local numeric computation and
returns may read item state and parameters. It cannot change authored state,
run engine actions, consume randomness or call a potentially mutating helper.
Shared callers may query twice while checking a transition.

## `on held_request:`

Receives `event.action` (**Int**), `event.frame_rate` (**Float**) and
`event.duration` (**Float**). It returns **Bool**: `true` accepts the request;
`false` refuses it. Refusal never invokes the stock item handler. The callback
owns the animation state, sound, rate and accessory decisions.

Both handlers must be declared together. Numeric statuses/actions are ordinary
script data. Existing engine held-animation commands have their own numeric
convention; an authored item implements that convention where it participates.
No fighter-specific tuning lives in the API.

```text
article It_Kind_Peach_Parasol states 3:
    state 0 anim -1:
    state 1 anim -1:
    state 2 anim -1:
    on held_status:
        if item.motion_state == 1:
            return 4
        if item.motion_state == 0 or item.motion_state == 2:
            return 6
        return -1
    on held_request:
        if event.action == 4:
            # Add this item's animation/rate policy here.
            return true
        return false
```

The example is a protocol sketch, not a complete Parasol implementation. Duration
zero conventionally uses the supplied frame rate directly. Other durations can
use an owned clip length while retaining the intended arithmetic order.

## Observe a holder's current item

| Query | Type | Missing/stale result |
| --- | --- | --- |
| `items.held_id` | Int | `-1` |
| `items.held_kind` | Int | `-1` |
| `items.held_motion` | Int | `-1` |
| `items.held_frame` | Float | `0.0` |

These read-only properties inspect the Fighter's actual held slot. In article
callbacks use `holder.items.held_id`, `holder.items.held_kind`,
`holder.items.held_motion` and `holder.items.held_frame`, guarded by
`holder.exists`. The holder must currently hold this article for that relation
to exist. `current_owner.items.*` can instead observe the current damage
owner's held slot, including a different held item, guarded by
`current_owner.exists`. The same native observations are available through
other retained Fighter relations without matching the caller's authored bank.
Ordinary engine items and Fighters without interpreted programs are supported.

Motion and frame are the actual Item `msid` and animation-frame fields, not
named script variables or the original creator's item. Float frame bits,
including signed zero and nonfinite values, are preserved. Creation IDs preserve
all 32 bits in signed Int representation; do not convert them to Float.

An absent, unlinked or logically removing held item returns the table's native
default. An invalid live slot or replacement of a captured source/slot faults
without publishing a value. A held Item's unrelated script fault does not erase
its physical fields; action and authored protocol dispatch retain their own
stricter status checks. An absent Fighter relation itself must be checked with
`.exists` before dereferencing these properties.

Resolution checks live fighter and item membership before dereferencing retained
addresses. Authored protocol dispatch additionally checks immutable author,
matching logic, generations and the actual held slot. Requests retain ordinary
[synchronous callback guards](synchronous_callbacks.md); command/transition
callers stop their cached continuation if a request retires or replaces them.

The old `targets.held_*(id)` and direct `holder.held_*` spellings belong to the
historical generated-C reference interface; they are not interpreter aliases.
The connected compiler/VM/receiver and runtime identity tests are console-only.
These contracts do not by themselves complete a fighter's held-item gameplay.

## `on held_motion:`

Selects a motion before the shared held-item open/fall entry publishes it.
`event.motion` is the requested common motion and `event.previous_motion` is
its captured predecessor. Return an **Int** common or package-owned motion.
A handler without an explicit return retains `event.motion`; an invalid returned
ID also retains the common request. A callback that retires or replaces its
fighter stops the entry before animation/timer setup.

```text
on held_motion:
    if state.held_article != -1:
        if event.motion == Motion.held_open:
            return attrs.owned_open_motion
        if event.motion == Motion.held_fall_special:
            return attrs.owned_fall_motion
    return event.motion
```

The ordinary variable/attribute names in this example belong to your fighter.
The API does not prescribe storage or a particular article kind. Use
[`motion_policy`](language.md) to bind the chosen owned rows to shared
engine callbacks when their gravity, drift and input behavior should remain common.

## `state.held_open(blend)`

Runs the real common open entry with an animation blend. It captures the previous
motion, runs `held_motion`, changes motion and starts animation, then initializes
the common open flags/timer. Automatic opening uses blend `0`; the original
special-startup path uses `10`. Both use the common timer tuning, including its
original Float-to-Int conversion. This operation does not create a held article.
Its synchronous source guards also stop subsequent script statements if a nested
callback retires or recycles the fighter.

| Common motion symbol | Meaning |
| --- | --- |
| `Motion.fall_special` | Ordinary special fall |
| `Motion.held_open` | Shared held-item opening |
| `Motion.held_fall` | Shared held-item fall |
| `Motion.held_fall_special` | Shared held-item special fall |

## Raw `motion.id` publication

`motion.id = value` accepts an **Int** common/package motion ID. It changes only
the published motion identity: no animation reset, callback replacement or union
setup occurs. IDs must fit the selected owned motion layout and the native
16-bit domain. Invalid writes fault execution without changing the current
identity. Existing typed recipient reads of `creator.motion`, `holder.motion`
and `owner.motion` remain available. Use an identity write when
reproducing an entry boundary which intentionally publishes identity before the
real shared entry, rather than replacing `state.enter_at_speed(...)` during normal moves.

```text
motion.id = Motion.fall_special
state.held_open(10)
```

This ordering reproduces the original common-open predecessor check while the
actual open transition still owns animation, callbacks and timers.

The interpreter validates its actual owned motion/animation tables and the
common timer's defined Float-to-Int range before open entry. Undefined native
NaN/out-of-range timer conversions are rejected rather than reproduced.

## Neutral and landing cleanup

`on neutral_enter:` runs after ground setup, before the shared neutral entry
changes to Idle. `event.blend` is the requested animation blend, a Float. The
handler owns fighter-specific cleanup at this boundary; the shared entry still
owns the Idle transition, animation and camera. This is distinct from
`on wait_enter:`, which runs after that setup.

```text
on neutral_enter:
    let bool notified = items.notify_held_neutral()

on landing_action:
    if event.interrupt_allowed:
        let bool notified = items.notify_held_neutral()
```

`event.interrupt_allowed` is a read-only Bool available directly inside
`on landing_action:`. It captures the landing entry's interrupt option before
your callback runs; changing motion does not change this captured value. It is
distinct from the animation's IASA flag. Pass it as an argument to a helper when
needed. Helpers cannot implicitly read their caller's event payload.

`items.notify_held_neutral()` calls the current held item's authored
`on holder_neutral:` handler. The article's immutable author selects that
handler, including when a different fighter holds it. It can run without a
`held_status`/`held_request` pair. It returns false if there is no authored
notification or if the item or source identity changes during dispatch. A
false result does **not** authorize running a donor's cleanup. The synchronous
guard stops subsequent statements if the source fighter was retired/recycled.

```text
article It_Kind_Peach_Parasol states 1:
    state 0 anim -1:
    on holder_neutral:
        item.remove()
```

The article kind above is an example; the notification itself has no
fighter-specific kind or storage requirement.

## Held timer expiration

`on held_timer_expired:` returns Bool after the common held-item timer reaches
zero and its active flag is cleared. Return true when the script handled the
transition. Return false, or fall through, to run the ordinary common item-throw
entry. A declared handler never falls through to a donor's special item policy.
`items.reopen_blocked` is the shared held-open lock flag, a readable/writable
Bool. The hook does not rearm the timer automatically.

## Airborne motion after a size change

`on air_resume_motion:` returns an Int motion category to the shared mushroom
size-change entry. `event.previous_motion` is the captured motion before the
size change. Falling through or returning an invalid common/owned ID retains
that predecessor. The shared switch resumes ordinary aerial fall for
`Motion.fall_aerial`, special fall for `Motion.fall_special` or
`Motion.held_fall_special`, and ordinary fall otherwise. Returning an owned
motion does not directly enter that motion; map it to the intended common fall
category. This hook selects the shared resume path, not an arbitrary callback.

```text
on air_resume_motion:
    if event.previous_motion == attrs.owned_fall_motion:
        return Motion.held_fall_special
    return event.previous_motion
```

Removal, recycling, fighter-kind changes or Definition replacement stop the
common entry's cached continuation after these synchronous callbacks.
