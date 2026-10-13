# Move behavior language: discussion sketches

For current buildable syntax and cross-linked API entries, use the
[fighter scripting reference](scripting/index.md). This file records earlier
design sketches and may contain syntax that the compiler does not accept.

Status: the larger examples below remain design sketches. The working
subset accepts up to four named move blocks per file, one each for neutral,
side, up and down special. Paired ground/air entry handlers can call
`stock.enter()` or `state.enter(NeutralSpecial)` (respectively
`SideSpecial`, `UpSpecial`, or `DownSpecial`). Authored entries accept ordered
local values, state and command assignments, nested conditions, motion changes
and general fighter operations on either side of `state.enter(...)`. They can
add or set facing and vertical speed, or scale horizontal speed, using known
attributes and expressions. A ground handler can select its paired aerial
motion with `state.enter(AirDownSpecial)` while remaining grounded.
An inherited `stock.enter()` must be first and accepts motion changes after it.
Authored state entry owns the paired ground/air collision transitions instead
of rearming Mario's original accessory callback.
An optional `on neutral_special_cue:` handler responds
to the original subaction's throw flag. It can call `state.once()` to suppress
duplicate cues until the move exits, `stock.neutral_article()` to spawn the
inherited Mario fireball, and optionally add facing speed or airborne vertical
speed for an impulse, or set either axis directly. An article-only cue needs
no dummy zero-speed command. In the cue,
`state.once()` precedes the article and speed.
The generated C builds an exact-client DLL through `native_content_tool
script-build`. It uses the existing motion scheduler and per-Fighter rollback
state; it does not link game symbols by name. A live practice match and a
corrected rollback probe exercise this subset. These operations do not yet
author new article types or general state graphs, and do not implement the
projectile and charge examples below. `stock.enter()` and
`stock.neutral_article()` explicitly reuse stock Mario behavior.
An authored special can now provide all eight ground/air animation,
interrupt, physics and collision callbacks. The compiler rejects an incomplete
phase set rather than silently retaining a Mario callback in a script-owned
move. Its cue can create a Mario fireball and effect from named bones without
calling Mario's original special handler. See `tests/native/mario_fireball.fms`
for the current neutral-special source. It compiles and its generated callbacks
have fixture coverage; a 3,670-sample local comparison now matches all 276
traced stock Mario action, damage and Fireball events after the clone animation
source tag was corrected. The compiler also emits independent authored state
and phase callbacks for side, up and down special. A mixed neutral-plus-side
fixture checks that both sets bind to their own motion states. Super Jump
Punch's entry, interrupt, physics, collision and completion paths are authored
in the fixture; Cape and Tornado remain untranslated. The
compiled module preserves the source hash of the whole file, and the package
carries the exact source. `tests/native/multi_special.fms` exercises authored
handlers across all four special slots; separate fixtures exercise
explicit stock inheritance.

Air entries can additionally scale horizontal velocity with
`motion.scale_horizontal_speed(number)` or a known fighter attribute such as
`motion.scale_horizontal_speed(attrs.specialhi.vel_x)`. An animation phase can
finish through the common freefall state with explicit flags and editable
attributes, for example
`if animation.finished: state.freefall(0, 1, false, attrs.specialhi.freefall_mobility, attrs.specialhi.landing_lag)`.
These map to general engine operations; the script selects the values and
timing. The current attribute namespace describes Mario's DAT attributes,
since Mario is the initial fully cloneable fighter target.

Entry, animation, physics and interrupt phases now share a bounded code-block compiler: up to
20 ordered lines per phase, with local `let` values, assignments, nested
conditions and named fighter state. Available physics calls include
`physics.ground_motion()`, `physics.air_motion()`,
`physics.fall(attrs.specialhi.grav)`, `physics.quick_air_decel()`, and
`motion.scale_velocity(attrs.specialhi.vel_mul)`, alongside the existing
ground-friction and air-drift operations. Fall uses the fighter's common
terminal velocity. `tests/native/multi_special.fms` uses these primitives to
express Super Jump Punch's ground and air physics branches in their original
order. Its collision phases select ordinary airborne collision, the common
falling-special landing check, or grounded collision using the same command,
vertical-speed and ground/air conditions as the original. The landing callback
supplies the clone's editable landing-lag attribute. Collision branches are
mutually exclusive for one phase call, even if a collision routine changes
ground/air state.
The shared code-block compiler lets animation and physics use the same nested conditions,
locals and fighter state as interrupts. An animation callback can update a
persistent charge or other move state before changing to wait, fall, or
freefall. The mixed-special fixture checks that animation callbacks leave
state alone while the animation continues, then update it and complete the
transition when the animation ends. A fresh Fireball-plus-Up module built
through this path matched stock Mario on all 3,669 P2 fighter-state samples
in the targeted steering replay. Collision and article cues still have their
narrower phase-specific grammars.

Interrupt phases accept ordered `let` variables, assignments, and nested
`if` blocks. Expressions can read horizontal stick input, animation command
0, the current launch angle and facing direction, and known Mario attributes;
they support arithmetic, comparisons, `and`, `or`, `not`, `abs`, and `radians`.
The script can consume the reverse cue, update facing from input, and rotate
the root bone. This lets the script own Super Jump Punch's steering and reverse
decisions while calling only general engine primitives. The generated callback
does not invoke Mario's original Up-special interrupt handler.

Scripts can declare up to 16 fighter-local `state bool Name` and
`state float Name` fields before their move blocks. Entry, animation, interrupt and physics phases read and
assign them as `state.Name`. The new fields start after any state owned by the
base module, with one byte reserved for the neutral cue's once flag; float
fields use `memcpy` accessors, so no unaligned C casts are needed. The compiler rejects duplicate/unknown fields,
rejects a base plus script that exceeds the 256-byte fighter limit, reports the
generated state size, and keeps the values in each fighter's
rollback-tracked allocation. This gives Cape/Tornado a place for charge and
reflector state without globals; their full logic still needs translation.
The same callbacks can read or assign the four animation command values as
`command(0)` through `command(3)`, and can test a fresh B press with
`input.special_pressed`. This makes Tornado's animation cues and tap input
available to the script without depending on Mario's original handlers.
The script also reaches the existing common fighter movement functions for
ground/air stick acceleration, grounded-to-self movement, horizontal and
fall-speed clamps, basic fall, and tap ascend. A compiled fixture checks the
call order and editable attribute arguments. These are general engine
operations; the script must still choose when to use them for Tornado.
Stock Mario Tornado enters its aerial motion from a grounded down-special
input while the fighter remains grounded. The entry grammar therefore permits
`state.enter(AirDownSpecial)` in a ground handler, selecting the aerial motion
without changing the ground/air flag. The compiled down-entry fixture checks
that exact state choice; it is not yet a full Tornado implementation.

An isolated Fireball-plus-Up clone and stock Mario both completed a targeted
3,670-sample replay with P2 human pressing Up+B and holding right during the
jump. All 3,669 sampled P2 fighter-state lines match exactly, including raw
float words for launch angle, motion frame, facing, velocity, and position.
This is evidence for that recorded input path, not every possible move state or
input combination. Cape and Tornado still need their move-specific behavior
translated before Mario's specials are fully script-owned.

Cape's item code ordinarily calls Mario's special reset and removal handlers
when the article ends. The native fighter definition now accepts a paired
reset/removal callback from an authored module and routes a custom fighter's
Cape item events there. The pair is layout-checked and its fighter-local state
survives rollback in the module fixture. The readable compiler does not yet
generate those callbacks or Cape's entry/physics/reflector behavior, so this
boundary alone does not make Cape script-owned.

A compact authored side block:

```text
move SideDash:
    on special_s:
        state.enter(SideSpecial)
        motion.add_facing_speed(0.5)
    on special_air_s:
        state.enter(SideSpecial)
        motion.set_facing_speed(-0.25)
        motion.set_vertical_speed(1.5)
    # A fully authored move also defines eight ground/air state phases.
```

The editor's resource inspector selects the clips, subactions, bones and
articles referred to by name below. Named cues/flags map to existing subaction
fields through explicit bindings; naming a cue does not invent another event
system or change the original command encoding. Completion lists available
names and descriptions. The final spelling of the API remains open.

## A projectile special

The subaction timeline already signals when to release the projectile and
when interrupts are allowed. Behavior responds in the engine phases that
normally handle those operations. Hitbox windows remain subaction edits.

```text
move Ember:
    parameter recoil: speed = 0.8

    state Ground:
        on enter:
            cues.release.clear()
            flags.allow_interrupt = false
            animation.play(clips.ember_ground)

        on animation:
            if animation.finished:
                finish.idle()

        on interrupt_check:
            if flags.allow_interrupt:
                controls.try_ground_actions()

        on physics:
            physics.ground_friction()

        on collision:
            if collision.left_ground():
                transition Air preserving playback

        on accessory:
            if cues.release.consume():
                articles.spawn(EmberBall, at=bones.hand_right)
                motion.add_facing_speed(-recoil)

    state Air:
        # Its animation, interrupts, physics, collision and accessory handlers
        # are shown here by the template, rather than hidden in a magic tick.
```

The incomplete Air section keeps this a small readability example. A real
template must contain both paths, including what happens on landing and at
animation end. In the complete ground-to-air transition, preserving playback
must mean a documented native transition policy, including command progress,
not simply reusing the same animation timestamp. State changes terminate the
current authored handler under a proposed rule that must be checked against
the required engine behavior before the language is fixed.

The compiler resolves `EmberBall`, clips and bones before the match. Speed uses
game-world units per simulation tick; facing-relative operations are explicit.
The author never sees a numeric article ID or fighter pointer. There must be
no allocation for parsing strings or looking up these names during gameplay.

`on accessory` is a precise initial phase name. The editor can explain its
purpose beside the handler. Renaming it to a friendlier generic update event
must not obscure when it runs relative to subactions, hitlag and physics.

## Persistent charge and interruption

This second fragment tests whether variable ownership and resource cleanup
remain approachable. Exact charging rate and event order are provisional.

```text
fighter variable charge: number = 0
    reset on death, match_start

move StoredShot:
    variable held_orb: article_handle
        release on move_exit

    state Charge:
        on enter:
            held_orb = articles.attach(ChargeOrb, to=bones.hand_right)

        on physics:
            charge = min(charge + 1, parameters.maximum_charge)
            physics.air_drift()

        on interrupt_check:
            if controls.special_pressed:
                transition Release

    state Release:
        on accessory:
            if cues.release.consume():
                held_orb.launch(power=charge)
                held_orb.detach_from_move()
                charge = 0
```

Here charge belongs to the fighter, while the held object belongs to the move.
Leaving Charge for Release must preserve move-owned data; damage, a grab or
another move ends that ownership. Detaching transfers ownership to the active
article, which remains registered and rollback-tracked until destroyed. The
compiler/runtime must make that transfer explicit and prevent a stale handle
from referring to an unrelated recycled object.

This fragment omits the full movement and completion paths. Its purpose is to
expose the design questions before implementation: state versus move lifetime,
held versus launched objects, hitlag scheduling, reset events and interruptions.
The finished example needs tests for all of them, not only the normal release.

## Review with the first prototype

- Can an author find the release cue in Subactions from the script, and jump
  from the cue back to its consuming handler?
- Can they discover the available operations without memorizing this syntax?
- Do diagnostics explain the meaning of an invalid bone, type or operation at
  the actual script line rather than emitting generated C errors?
- Are lifetimes visible enough that changing states or being grabbed does not
  leave surprising effects, charge resets or disappearing projectiles?
- Can these operations compile to the existing engine behavior without adding
  a second physics system or changing unrelated stock moves?

Use answers from a working example to revise these sketches. Do not expand to
a large standard library or an automatic C converter before validating them.
