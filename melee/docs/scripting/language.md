# Script structure and callbacks

An `.fms` file may contain named `move` blocks, `action` blocks, `article`
blocks, [functions and recipient exports](functions.md), and fighter state declarations. Workshop projects can keep each
special move in its own file and fighter variables in a separate editable
fighter-state file. The
build joins those sources into one owned source program.

Properties and variables are values: read `velocity.x`, `guard.front_only`, or
`state.charge` directly. Parentheses belong to function calls, such as
`sqrt(value)` or `vectors.angle(ax, ay, az, bx, by, bz)`. Calling a scalar value
as a function, for example `velocity.x()`, is a script error. This applies to
fighter and article expressions; ordinary grouped expressions remain valid.

[Synchronous callback dispatch](synchronous_callbacks.md) explains
`targets.call`, source lifetime checks, expression forms and the interpreter's
evaluation order.

## Independent behavior

```text
fighter:
    behavior independent
```

This declaration precedes functions, moves, actions and articles and marks
behavior as owned by the source program. Ordinary source packages require
independent behavior; the catalog base does not supply missing fighter-specific
handlers. Declare the block once. Comments and blank lines are allowed within it.

Independent behavior clears the base Definition's callbacks, item and article
behavior tables, and model-attachment ownership before installing authored
handlers. Supply `on load` and `on load_special_attrs` explicitly; `pass` is
allowed when no initialization is needed. Stock entry and stock article
delegation are rejected. Omitted special entries are disabled.

Preparation checks every extended motion in the supplied owned table. Each
non-NULL animation, interrupt, physics or collision callback must have an
authored replacement, through a supported move phase, `on state_N_PHASE`, or an
explicit [common motion policy](#common-motion-policies).
An intentional NULL callback needs no replacement. Missing coverage rejects
the program, including an uncovered phase in a later motion. Motion metadata
and the shared camera callback remain copied.

The directive does not prove source parity or remove asset dependencies.
Owned rig mappings, article assets, effects, sounds and demo roots still need
complete metadata. Common fighter policies and lifecycle ordering still need
source comparison and runtime validation before promoting a template.

## Common motion policies

```text
motion_policy 341 common 122
```

This top-level declaration binds an owned extended motion to the animation,
interrupt, physics and collision callbacks of a common engine motion. For
example, common motion `122` supplies the shared Beam Sword swing behavior.
The package keeps its animation ID, motion flags, attack metadata and camera
callback. The owned motion table installs shared engine callbacks directly; it does not
delegate through a donor fighter or add a callback wrapper.

Declare each target once, before functions, moves, actions and articles. The
common motion must be `0..340`, and the target must belong to the package's
extended motion table. An invalid target or missing engine table rejects the
program. Later `on state_N_PHASE` handlers can replace individual bound phases.
For independent behavior, only non-NULL phases actually supplied by the common
policy count toward coverage; binding an empty policy cannot bypass the guard.

Use this for genuinely shared engine behavior. Fighter-specific move decisions
still belong in scripts. No animation, attributes or fighter state is borrowed
from the common policy.

### `on item_swing_motion:`

Return the owned motion ID for the fourth held-item swing variant. The shared
entry supplies integer `event.swing_type` (`0..5`: Beam Sword, Home-Run Bat,
Parasol, Fan, Star Rod, Lipstick) and `event.swing_variant` (`3`). These event
fields are available directly in this callback; pass them as typed arguments
to helpers. Other swing variants retain the common motion table.

Falling through or returning a negative/out-of-range ID leaves this variant
unsupported and retains the existing error boundary. There is no fallback to
another fighter's swing animation.

### `state.taunt(right_motion, left_motion)`

Enter a taunt using a common motion or an owned extended motion. The direct
interpreter operation takes two integral IDs from the selected source's exact
motion table; `state.taunt(264, 265)` selects the ordinary common taunts. It calls
the shared native entry: clear the interrupt flag, select the left motion only
when facing is exactly -1 and its animation's x8 is nonzero, then change motion
at frame 0, rate 1, flags 0. It does not start animation commands. Missing/stale
owned tables, invalid left animation indices and retirement during entry fail
without publishing a result. This operation is Fighter-only.

## Moves and subactions

```text
move Fireball:
    on special_n:
        state.enter(NeutralSpecial)
    on special_air_n:
        state.enter(NeutralSpecial)
    on ground_animation:
        if animation.finished: state.wait()
    on air_animation:
        if animation.finished: state.fall()
    on neutral_special_cue:
        articles.spawn_authored_projectile(It_Kind_Luigi_Fire, LeftHand, 0)
        effects.spawn(1287, LeftHand)
```

`on special_n` and `on special_air_n` respond to grounded and airborne special
input. The animation, interrupt, physics, and collision callbacks own the
move's decisions while it is active. A fully authored special supplies animation,
interrupt, physics and collision callbacks for each supported ground/air entry.
Ground-only and air-only specials are allowed; an omitted entry is disabled,
and its phase callbacks must also be omitted. The compiler rejects an incomplete
supported phase set rather than retaining a stock move callback.

`on neutral_special_cue` responds to the existing subaction cue. The action's
subaction still determines *when* that cue occurs; this callback determines
what to do when it occurs. For example, Luigi's cue creates an authored
Fireball. See the [Fighter API](fighter.md) for its spawn call,
[Articles](article.md) for its declaration, and the
[Item API](item.md) for its update callbacks.

Other action timing, hitboxes, and animation commands remain editable in the
Workshop Action Editor. A script does not replace the subaction timeline.

### Special accessory callbacks

Any authored special can declare `on accessory`. It runs through the engine's
fighter accessory update slot, independently of the move's animation callback.
Use it for an attachment or one-shot effect. Clear the callback with
`callbacks.accessory(None)` when finished.

```text
func sing_effect():
    if not effects.active:
        effects.spawn_on_part(1238, 5)
        effects.active = true
    effects.pause_during_hitlag()
    articles.stop_cue()

move Sing:
    on accessory:
        sing_effect()
```

This excerpt from [Sing](../../examples/purin_wip.fms) uses existing shared
effect primitives. [`callbacks.accessory(sing_effect)`](fighter.md#callbacksaccessoryhelper)
reinstalls the helper when a motion transition clears the callback. Mario and
Dr. Mario's Cape templates also use `on accessory`.

## Integer-valued queries

[`on throw_victim_motion`](fighter.md#on-throw_victim_motion) returns a common
victim motion ID. `event.motion` is the pending holder throw, and
`event.victim_motion` is the engine default victim pose. The holder has not
entered the throw yet: `motion.id` still describes its previous state. Return
`event.victim_motion` when keeping the default; falling through also retains
it. This is an integer query, unlike the boolean policies below.

[`on cpu_special_candidate`](cpu_policies.md#on-cpu_special_candidate) receives
`event.angle` and `event.distance`, returns a boolean indicating whether it
consumes the decision, and can assign `cpu.selected_command`. Returning true
without assigning preserves the command; false continues common selection.
[`on cpu_projectile_hit`](cpu_policies.md#on-cpu_projectile_hit) is an integer
contact-classification query using an already-computed prediction horizon.

## Predicate callbacks

[`jab_start`](common_policies.md#on-jab_start) returns integer motion flags;
it is not a Bool predicate. The
[auxiliary-item presentation hooks](common_policies.md#auxiliary-item-presentation)
are Void callbacks at the shared item attach/removal boundaries.

The void common-entry hooks [`wait_enter`, `crouch_wait_enter` and
`aerial_attack_enter`](common_policies.md#common-entry-completion) finish shared
motion/animation setup. They differ from the per-tick interrupt predicate
`wait_input` below. The [aerial input policies](aerial_actions.md)
`aerial_action_manual` and `aerial_action_automatic` also return Bool;
`grounded_motion_setup` is a void setup hook before common grounded resets.

`on wait_input`, `on action_status`, `on idle_variants` and
`on motion_is_angled`, `on jab_restart`, `on dynamics_wind`,
[`on dynamics_ground_collision`](common_policies.md#on-dynamics_ground_collision) and
[`on body_rebound`](common_policies.md#on-body_rebound) return booleans,
unlike ordinary event callbacks. Use `return true` or `return false`;
reaching the end returns `false`.
Returning a value from a void callback, or using a bare `return` in a predicate,
is a compile error.

The [CPU classification/decision callbacks](cpu_policies.md#callback-summary)
also return booleans. Their return value either answers a motion query or
controls whether shared planner logic continues; consult the policy reference
before using the same result for both purposes.

`wait_input` runs in the standard grounded Wait interrupt sequence, after
ordinary attack checks and before ordinary taunt, jump and movement checks.
Return `true` after entering your custom action to stop subsequent checks.
Return `false` to allow them to continue. A declared handler replaces the
stock radio-taunt input check for that fighter; it does not call it afterward.
In source-owned fighters an absent hook returns false and continues common
checks; it never inherits another fighter's radio-taunt handler. A fault or
retired source stops the input sequence before further engine decisions.

`action_status` extends the engine's action-status test used by player-stat
bookkeeping. Shared checks for the standard taunt action IDs run first. This
handler is a query, not a per-frame update or motion entry callback.
The ordinary interpreter schedules both hooks directly. Absent, faulted or
retired action-status hooks return false; standard common-state shortcuts
remain unchanged.

`idle_variants` runs when the current grounded idle animation ends and an idle
choice table exists. Return `true` to use the normal weighted variant selection,
or `false` to repeat the current idle. With no choice table the engine repeats
regardless. Use ordinary fighter state, parameters and
[`items.held`](model_attachments.md#properties) to define your policy.
For example, the stock Fox template allows variants while holding an item;
Falco repeats that idle. Defining this callback replaces those stock-kind
exceptions. It does not select an animation directly or consume selection RNG.

`motion_is_angled` classifies a motion for the common player-statistics system.
Its `event.motion` integer is the queried motion ID, which can differ from
[`motion.id`](fighter.md) on the current fighter. Return whether it counts as
an angled attack. A declared callback replaces the stock classification,
including its range check. This changes bookkeeping, not hitbox direction or
attack selection. Helpers can receive `event.motion` as an ordinary typed argument. It is also available
in [`jab_restart`](common_policies.md#on-jab_restart), where it identifies the
requested follow-up attack. [Common action policies](common_policies.md)
describes jab/wind queries and the two shield-entry events.

```text
action IdlePolicy:
    on idle_variants:
        return not items.held
    on motion_is_angled:
        return event.motion == 51 or event.motion == 55
```

These are synchronous queries. Keep their decisions free of state writes,
random draws and transitions; do not use them as tick callbacks. Both script
forms support up to 512 fighter/action handlers within the 512 KiB source limit.
Ordinary source scripts run through prepared interpreter hooks at these shared
decision points. Idle selection is not called while the animation has frames
remaining or no choice table exists. Lifetime/program/storage changes or VM
failure stop native continuation; they do not fall back to a donor decision.

```text
action StageInteraction:
    on wait_input:
        if not input.down_held and input.down_release_age == 1:
            if stage.event("conversation.available", 0):
                state.enter_at_speed(370, 0, 0, 1)
                return true
        return false
```

This shows callback placement; it is not a complete radio implementation.
See [stage events](stage.md), [input history](fighter.md#player-and-down-input-history),
and the [Fox example](../../examples/fox_wip.fms) for the complete
phase, statistics and cancellation behavior.

## Motion-state callbacks

The [`on trail`](trail.md) callback supplies weapon-ribbon data in either script
form. Its write-only `trail` outputs and local expressions cannot change fighter
state or call engine functions, helpers, or RNG. Use ordinary simulation callbacks
for the decisions that change the pose or start an action.

An `action` block can replace selected animation, interrupt, physics, or
collision callbacks for a numbered motion state without taking ownership of
the other callbacks. It can also author fighter-wide events such as
[`on knockback_enter`](fighter.md#fighter-event-callbacks). For example,
Luigi's Green Missile uses
[`state.switch_to_ground(motion, flags)`](fighter.md#stateswitch_to_airmotion-flags-and-stateswitch_to_groundmotion-flags)
when its airborne charge state lands:

```text
action GreenMissilePhases:
    on state_350_collision:
        if collision.landed: state.switch_to_ground(344, 0x0c4c5286)
```

The callback runs on the current fighter. Here the collision test is true
only if the airborne move reaches the ground; the state operation then enters
the grounded Hold motion at the same animation frame. Green Missile's other
callbacks are authored in the same script. Super Jump Punch is also scripted;
Cyclone is also scripted. Green Missile's
[translation ledger](../luigi_green_missile_translation.md) records the
stock-versus-clone paths tested so far.
For either-wall checks in an airborne collision callback, see
[`collision.wall_contact`](fighter.md#collisionwall_contact).

An `on state_change` action handler can initialize
[fighter state](#fighter-state) during an entry transition. The move must
register it with `callbacks.track_state_change()` before entering the motion.
Green Missile uses this one-shot hook to make its random misfire roll at the
same point as stock. A `callbacks.track_deal_damage()` call similarly
registers an [`on deal_damage`](fighter.md#callbackstrack_deal_damage) handler.
See the [Fighter API](fighter.md#method-summary) for the state-change,
hitbox, effect, and motion-transition operations these callbacks use.
For a single-state special with grounded and airborne callbacks, see the
[movement and collision API](movement.md) and
[Luigi's Super Jump Punch](../../examples/luigi_wip.fms).

## Callback locals

Fighter callbacks, fighter helpers and `article` callbacks can declare ordinary
locals. Their lifetime ends with the callback or helper invocation; use state
fields for values that must survive between calls.

```text
let int handle = item.creation_id
let bool active = handle != 0
let float distance = 2.5
let temporary = distance
```

`let int` stores a signed 32-bit integer. `let bool` normalizes any nonzero
numeric value to true. `let float` stores a 32-bit float; an untyped `let name`
infers its type from the expression. Integer locals preserve IDs such as `16777217`,
high-bit IDs and the `-1` sentinel without passing through float storage.
Converting an ID to a float first loses this guarantee, including when it is
later cast back with `uint()`.

Persistent Bool state and Boolean engine flags use the same truth conversion.
For example, storing `input.attack_pressed` retains true when the A-button
mask is nonzero; it does not truncate that mask to a byte. Article writes to
their creator's Bool state follow the same rule. Store raw masks in Int
variables when the individual bits must remain available.

Use integer locals for bit masks as well. For example, `let int flags =
0x0c4d508e` preserves every collision flag; an untyped local rounds that value
to `0x0c4d5090` before the engine ever receives it. The compiler does not infer
integer storage merely because the literal is hexadecimal.

Integer declarations, reassignments, integer helper arguments and integer
callback returns reject implicit float conversion. Integer and bool state,
parameters and locals retain their declared types. Explicit `int(value)` and
`uint(value)` casts permit deliberate conversion. Casting a float discards its
fraction toward zero. Nonfinite or out-of-range float-to-integer conversions
stop execution with a script error rather than relying on C undefined behavior.

Arithmetic uses the interpreter's resolved numeric types. Integral operands stay integral;
`7 / 2` is `3`, while `7 / 2.0` is floating point. Mixing a float with an integer
promotes the expression to floating point. Comparisons and logical operators
produce bool; arithmetic on bool promotes it to int. Remainder, bitwise and
shift operators require integral operands. `uint()` makes an unsigned 32-bit
operand. Integer addition, subtraction and multiplication wrap at the resolved
integer width, including signed arithmetic. Integer division and remainder by
zero, and shifts outside the operand width, are script errors. Signed right
shift preserves the sign. The minimum signed integer divided by -1 wraps to
that minimum; its remainder is zero. Floating-point division retains floating
semantics and can produce infinity or NaN.

Decimal integer and hexadecimal bit-pattern literals become typed constants.
For example, `int(uint(0x80000001))` preserves that bit pattern in an int local.
Engine operations have checked signatures; an unknown operation is a preparation
error, not an implicitly floating-point query.

Each callback/helper has space for 512 local declarations, including its arguments.
Duplicate names in the same scope are rejected. Indented branches and loops introduce
scopes; their locals cannot be read after that scope ends. These declarations
apply to `article` callbacks, including article signals; legacy `item Name`
holdable callbacks keep their existing syntax.

## Fighter state

Fighter-local state is declared with a type and name, then accessed through
`state.Name`. It is stored per fighter in rollback-tracked state. Use it for a
move's charge, cooldown, or other value that persists between callbacks.
Article callbacks can access declared owner state through `owner.state.Name`
when they have a valid owner. See [Item API](item.md#properties).

Fixed arrays use the same scalar types:

```text
state int history_index
state float speed_history[16]
state bool active[2]

action RecordVelocity:
    on state_341_physics:
        state.speed_history[state.history_index] = velocity.x
        state.history_index = (state.history_index + 1) % 16
```

An array has 1 through 128 elements and requires an index when read or assigned.
A constant index outside its declared range is a compiler error. Dynamic
indices are converted to signed 32-bit integers and evaluated once; an index
outside the range reads as zero or false and an assignment leaves storage
unchanged. Arrays stay inline in each fighter's state and allocate nothing
in callbacks. Each array is one declaration, regardless of its element count.

## Constant tables

Use a typed immutable table for values selected by a computed index:

```text
table float recoil = [0, -0.51, -1.02, -1.53]
let float offset = table.recoil[state.recoil_step]
```

Tables belong to the script root or an entity scope. An entity's `table.NAME`
prefers its local table, while `program.table.NAME` explicitly reads the root.
Bounds are checked; tables cannot be written at runtime and do not add mutable
actor state. See [constant tables](tables.md) for types, expressions and limits.

## Fighter attributes

Declare fighter-owned tuning in the script with typed `parameter` defaults.
Read a root parameter as `attrs.Name` in fighter callbacks and move files:

```text
parameter float missile_start_decel = 0.94
parameter int extra_jumps = 1
parameter bool starts_charged = false
```

The project can override these defaults in `fighter_parameters.fmp`. The
Workshop's Scripting sidebar edits that same file. An override must match a
declared name and type; the sidecar cannot introduce an undeclared parameter:

```text
# fighter_parameters.fmp
float missile_start_decel = 0.94
int extra_jumps = 1
bool starts_charged = false
```

Scripted item and article callbacks read root tuning as `program.attrs.Name`.
These are the owning package's immutable values, including after an item changes its holder
or is reflected; they do not change to the new holder's parameter table.

```text
action GreenMissilePhases:
    on state_343_physics:
        physics.ground_decelerate(attrs.missile_start_decel)
        physics.ground_to_self()
```

Script declarations define the parameter layout. Editing sidecar values keeps
that layout; adding, removing or changing declarations requires preparing the
updated source. The ordinary loader prepares scripts without a C compiler. Values
are bound once when a fighter loads, and are immutable during a match. Practice
preview can apply unsaved value edits at its normal scene reset. Save and Install
include the parameter file, and Build behavior saves it before compiling. Use
[`state`](#fighter-state) for charges, cooldowns, and other changing values.

A package has at most 256 parameters across fighter and article scopes. The
parameter text is bounded to 64 KiB, and each line is shorter than 256 bytes.
Names are at most 127 characters, including an article qualifier. The 257th
value is rejected by both direct parsing and import; it cannot become an
unbound callback read. Behavior source retains its separate 512 KiB limit.

Existing stock translation drafts may still declare an imported DAT field by
byte offset. The script can read it as `attrs.Name`:

```text
attribute float missile_start_decel 28

action GreenMissilePhases:
    on state_343_physics:
        physics.ground_decelerate(attrs.missile_start_decel)
        physics.ground_to_self()
```

Offset `28` is `0x1C`, the deceleration float in Luigi's Green Missile
attributes. This is useful while translating a stock fighter, but new authored
values should use literal parameter defaults and optional project overrides.
The compiler checks offset alignment and maximum attribute-block capacity but
cannot prove that a DAT offset has the intended meaning or type. Check the
fighter's attribute layout when declaring one. See the
[`physics.ground_decelerate()`](fighter.md#physicsground_decelerateamount-and-physicsground_to_self)
reference for the operation using this value.

Opening a Workshop draft with offset declarations in its state or move files copies those values
into the Parameters section and replaces the offsets with typed, literal
`parameter` declarations in their original scope. Save and build behavior to
commit the conversion. The DAT stays
unchanged for common engine routines that still read it. The source checkout
also provides an explicit import command:

```text
native_content_tool import-parameters PROJECT_DIRECTORY SOURCE.fms OWNED.fms
native_content_tool script-build PROJECT_DIRECTORY OWNED.fms
```

The command creates `fighter_parameters.fmp` and a new converted script; neither
output may already exist. It supports declared primitive fighter and article
attribute fields. Article names are scoped as `It_Kind_Name.field` in the same
file and read through [`item.params.field`](article.md#parameters). Shared
stock templates retain only the current source fighter's mapped articles.
Float defaults preserve their original binary32 values.

An import draft may also supply a literal owned default without a DAT offset:

```text
parameter float chain_direction_threshold = 0.550000012
parameter int maximum_charges = 3
parameter bool starts_charged = false
```

`import-parameters` retains these declarations and copies their values into
`fighter_parameters.fmp`. Article-local defaults receive qualified sidecar
names while keeping local declarations inside the article. Imported defaults
must be finite typed literals; duplicate names are rejected. The converted
script is self-contained and its sidecar remains editable.

`attribute int` reads an aligned signed 32-bit word; `attribute bool` reads
one byte containing 0 or 1 and imports it as an owned bool. A bool byte need
not be word-aligned. This supports byte flags such as reflector ownership
without exposing a particular fighter's attribute struct in the scripting API.

`attribute byte` is an import-only declaration for an unsigned `0..255` byte.
It produces an ordinary owned `int`, useful for packed colors and other byte
fields. Run `import-parameters` before compiling a draft that uses it. See the
[weapon-ribbon example](trail.md#import-stock-defaults) for a source layout.

## Article callbacks

An [`article`](article.md) declares states and item-wide callbacks. Each
state may define `on animation`, `on physics`, and `on collision`. Item-wide
callbacks include `on reflected`, `on hit_shield`, and `on shield_bounced`.
Callbacks that return a boolean use `true` to request item removal and
`false` to retain it. Calls such as
[`item.shield_bounce()`](item.md#itemshield_bounce) return the engine's own
result, which can be returned directly.

## Build and test

The Workshop's **Build script** action validates the active draft. The
installed [agent tools](agent_tools.md) provide `scripts.check` for editable move
files and their parameters, with diagnostics tied to each file and line.
`native_content_tool script-build PROJECT_DIRECTORY SCRIPT_FILE` accepts a
combined source file and saves it as the draft's validated behavior after a
successful check. Use the [move checks](authoring_checks.md#move-logic) to
verify the behavior in practice.

### Callback work limit

Each callback can use up to 65,536 bytecode instructions. Entering a function reserves the length of its compiled body. Repeating a loop uses the length of the section it repeats. Helpers and synchronous callbacks started by that callback share its allowance. If the allowance runs out, the interpreter stops with a diagnostic at the function or loop that used it up. Changes the callback already made still apply.

### State storage limits

The current package bank admits up to 2,048 state/parameter declarations across
its owners. Each fighter actor has a fixed 8,192-byte state capacity; its
prepared root layout must fit. Article owners have their own checked storage
capacity. These are reserved pools, not allocations per callback. Snapshots
record occupied authored state rather than treating immutable parameters as
mutable state.

Preparation lays out each owner's declared fields with their types, counts and
alignment. Floats and 32-bit integers consume four bytes per element; bools
consume one. Every new actor clears its own active state footprint. Rebuilding
a program reconstructs the layout from its source declarations; it does not
append offsets to an inherited base's state. See the
[typed storage checkpoint](../native_script_frontend.md).

## Explicit special entry

Put `entry explicit` immediately below `move Name:` to declare fully script-owned
entry setup. It is accepted once, directly inside a move, before its callbacks,
and requires an authored special entry callback. Malformed or misplaced policies
are errors.

The interpreter runs that callback directly. It adds no command-0/throw-flag
resets or automatic accessory assignment. This also applies when the directive
is omitted: write required setup in the script. State changes still perform
their normal shared engine reset behavior.

An explicit entry can call a script function that selects and enters its state
using `state.enter_at_speed(...)`; it need not contain a literal `state.enter_flags(...)`
in the entry callback itself. Every path is the author's responsibility. Supply
all four phase callbacks for each supported entry, and omit the other entry and
its phases for a ground-only or air-only move.

```text
move Dash:
    entry explicit
    on special_s:
        command(2) = 0
        state.enter_flags(347, 0)
        animation.start()
```

Put shared accessory logic in a helper. A move's `on accessory` callback can
call the helper. After a motion transition, install that helper at the intended
point with [`callbacks.accessory(helper)`](fighter.md#callbacksaccessoryhelper).
Install or reset only the callbacks and flags that the move requires.

See [Action command variables](command_variables.md) for timed move cues and aerial landing windows.
