# Functions and loops

Use a function when several callbacks need the same behavior. Functions
prepare as typed bytecode inside the fighter program. They use the
current fighter, shared engine primitives and the same rollback-tracked state
as their caller. Invocations use the interpreter's fixed call/register pools
without allocating memory. Generated C remains a comparison tool; normal
fighter packages execute through the interpreter.

## Declaring a function

```text
state int charges

func spend_charge(int amount, bool allow_empty, float impulse):
    if state.charges < amount and not allow_empty: return
    state.charges = state.charges - amount
    if state.charges < 0: state.charges = 0
    velocity.y = impulse
```

Argument types include `float`, `int`, `uint` and `bool`. Values are passed by value; assigning
an argument changes only the function's local copy. A declaration without a
return annotation has no result. `return` exits the current function, so a callback that calls a helper
continues after that call unless it also returns.

Call a function from a fighter callback or another function:

```text
on tick:
    spend_charge(1, false, 2.5)
```

Each invocation receives the calling fighter automatically. There are no
pointers to declare, and another fighter's variables remain independent. Use
`state.Name` for values that should survive callbacks; `let` variables and
arguments are temporary locals. Read immutable owned values with `attrs.Name`.

Definitions are global to the combined fighter source. Forward calls work,
including across move files. Put shared helpers in `fighter_state.fms`, or keep
a move-specific helper beside that move in its `.fms` file. A standalone
action script still begins with its `base` declaration. Duplicate function or
argument names are rejected, as are unknown calls and incorrect argument counts.

## Returning a value

Add `-> bool`, `-> int` or `-> float` after the argument list:

```text
func spend_charge(int amount) -> bool:
    if state.charges < amount: return false
    state.charges = state.charges - amount
    return true

func launch_speed(float strength) -> float:
    return strength * 1.25
```

Use the result in a declaration, assignment, condition or another helper call:

```text
let bool spent = spend_charge(1)
if spent: velocity.y = launch_speed(2.0)
```

A typed helper must finish with an unconditional `return value`, even if earlier
branches also return. Integer returns reject floating values; Boolean returns
use the ordinary nonzero truth conversion. A void helper cannot return a value.
Typed helpers cannot be installed in engine callback slots that require void
callbacks. These rules also apply to model-effect helpers.

Arguments containing helper calls or synchronous engine callbacks are evaluated
once in written order before the outer helper is called. For several side effects
in arithmetic operands, use separate local declarations to specify their order.
If a synchronous callback removes or recycles the source, lifetime checks stop
the current helper and its calling chain before later writes. The VM withholds
its return value. Recipient calls apply the documented
[retirement result rules](targets.md#typed-recipient-results).

[Model-effect updates](model_effects.md) can call helpers using the same syntax.
Those calls compile the helper for the current effect, with read-only creator
properties and effect joint controls. A helper using `effect.*` or `creator.*`
belongs to that context: it cannot be exported, installed as a fighter callback,
or called through an article's `owner.call`/`creator.call`. A shared helper must
contain only operations valid in every context from which it is called.

## Exporting a recipient callback

Use `export func Name():` to let another authored fighter or article request a
named operation on this fighter. The recipient supplies the implementation:

```text
state int held_resource

export func clear_held_resource():
    state.held_resource = -1
```

An export must be a root fighter helper with up to 30 typed arguments.
Parameters may be `bool`, `int`, `uint` or `float`. It may return
`bool`, `int` or `float`, or omit the return annotation for a void helper.
Article-local and effect-local helpers cannot be exported. A prepared program
supports up to 128 exports. Its body uses this package's
fighter state, attributes and engine binding, with the recipient fighter as
its current object. It can call ordinary typed helpers, be called locally, or
be registered in an ordinary callback slot if it has no arguments and no result. A private `func` is never available
through recipient dispatch.

The compiler prepares an immutable table of recipient-local function IDs,
sorted by 32-bit name hash. Exact name comparisons distinguish colliding hashes.
Duplicate function names are rejected; private helpers do not consume the
128-export capacity. A script with no exports has an empty table. Literal name
spans remain pinned in the immutable source; dispatch allocates no memory.

Required engine operations include export bodies and their helper calls even
when no local callback invokes the export. Missing operations reject program
preparation. A package must remain bound to its own state layout while live;
recipient dispatch does not reopen or rebind the program.

Recipient requests use `targets.call(handle, "Name", arguments...)`.
A successful request means the recipient had the named authored export and it
was invoked; it does not report a return value from that function. Missing
handlers require an explicit authored branch. There is no stock fallback or
access to another package's private state. See the
[recipient dispatch and lifetime contract](../native_source_recipient_export_plan.md)
for the integration scope and validation gates. Recipient calls being available
does not establish a fighter's resource construction or gameplay acceptance.

Use [`targets.call_bool`, `targets.call_int` or `targets.call_float`](targets.md#typed-recipient-results)
to receive the value of a matching typed export. These requests validate the
export's result type before invoking it.

```text
export func apply_impulse(float horizontal, float vertical):
    velocity.x = horizontal
    velocity.y = vertical
```

An Article with a retained fighter creation ID can request that operation:

```text
targets.call(item.current_owner, "apply_impulse", 1.5, 3.0)
```

The recipient's declaration determines conversion and arity. Bool parameters
require Bool values; Int/UInt accept integral values; Float accepts numeric
values. These follow ordinary helper assignment rules, including 32-bit
integer conversion. If a matching export exists but its arguments do not
match, the call faults before entering the helper. A missing recipient/export
still returns the documented missing result. Parameters use bounded call
buffers; no allocation or temporary fighter-state mailbox is introduced.

## Calling the owner from an article

An [article](article.md) can call a fighter function directly:

```text
func launch_from_blast(float center, float radius):
    velocity.y = attrs.launch_speed

article It_Kind_Samus_Bomb states 1:
    on accessory:
            if item.attachment_active and item.overlaps_owner(0):
                owner.helpers.launch_from_blast(position.x, hitbox.radius(0))
    state 0 anim 0:
```

`owner.helpers.Function(arguments...)` runs the named function synchronously on
the valid attached fighter. It does nothing after the attachment owner is
cleared or no longer matches damage ownership. Arguments use article
expressions; the function body uses fighter expressions and state. The
compiler checks the function name and argument count and prepares a local
helper ID. It performs no name lookup during gameplay. Normal typed conversion
applies at the function's parameter boundary.

If the fighter function declares `-> bool`, `-> int`, or `-> float`, an article
can use the result in an expression. `creator.helpers.Function()` follows the
original live creator; `owner.helpers.Function()` also requires that creator
to remain the current owner. These calls require the package's own program;
use `targets.call_*` when the recipient may belong to a different package.
Guard optional recipients with `creator.exists` or `owner.exists` before calling
their helpers. A captured receiver that retires or changes identity is an error;
the interpreter does not select a replacement through the same object address.
See [article helper results](article_helpers.md) for argument evaluation and
the lifetime rules when a synchronous call removes the calling article.

This can implement a blast-induced jump, a resource refund or an owner
reaction without putting fighter-specific decisions in the engine. A function
may change fighter state. If it destroys the calling article, do not access
that article again in the remaining callback.

## A real move example

[Jigglypuff's Sing](../../examples/purin_wip.fms) puts its effect callback into
a helper:

```text
func sing_effect():
    if not effects.active:
        effects.spawn_on_part(1238, 5)
        effects.active = true
    effects.pause_during_hitlag()
    articles.stop_cue()

move Sing:
    # Entry and motion callbacks omitted in this excerpt.
    on accessory:
        sing_effect()
```

The script also uses a small helper to reinstall its accessory callback after
ground/air transitions. These are editable script functions, not calls to a
stock fighter handler. See [accessory callbacks](language.md#special-accessory-callbacks).

## Binding a motion callback

Assign a zero-argument fighter function to the current motion's callback slot:

```text
func carry_physics():
    if airborne: physics.air_drift()
    else: physics.ground_friction()

action ItemEvents:
    on heavy_throw_enter:
        callbacks.physics(carry_physics)
```

`callbacks.animation(function)`, `callbacks.interrupt(function)`,
`callbacks.physics(function)`, `callbacks.collision(function)` and
`callbacks.accessory(function)` replace their respective callbacks. The
accessory slot is the ordinary motion accessory callback (slot 4). Pass `None`
to clear a slot. Unknown functions, functions with arguments, and numeric
addresses are rejected. Forward references work.

Assignment does not run the function. The engine calls it in the corresponding
phase; assigning the same function from inside itself is permitted. A motion
transition can replace the slots again, so install the desired callbacks after
entering that motion. This allows scripts to customize shared engine states
without delegating their behavior to stock fighter handlers.

### `callbacks.reflect(function)`

Bind a zero-argument helper to the engine's forced-turn completion callback.
Pass `None` to clear it. This follows the same validation rules as the motion
slots above. It runs after the common turn effect reverses facing and restores
the model orientation, for example after a Cape hit. It is **not** a notification
for every assignment to `facing.direction` or every ordinary turnaround.

Use it to update move-owned travel state after that forced reversal:

```text
func align_travel():
    velocity.x = facing.direction * abs(velocity.x)
    state.first_direction_x = facing.direction * abs(state.first_direction_x)

on state_357_animation:
    callbacks.reflect(align_travel)
```

The helper supplies the policy; the engine knows nothing about its variables
or which special owns them. Motion changes can clear the slot, so bind it after
the intended transition. See the [Quick Attack translation](../pikachu_pichu_script_translation.md).

### `callbacks.damage(function)`

Bind a zero-argument helper to the fighter's damage-received callback slot.
It runs when the engine dispatches damage while that registration is active;
it does not replace damage calculation. Passing `None` clears the slot.
Motion changes may clear registrations, so rebind after a transition when
the move needs to continue observing damage. This can update ordinary named
state such as whether an active move may catch its own projectile.

Use `callbacks.matches(Callback.damage, None)` or
`callbacks.matches(Callback.death2, None)` to test whether a slot is empty.
The second argument can also be an owned zero-argument Void helper. The query
compares the interpreted registration without running or changing it; native
handlers match neither a script helper nor `None`. See
[callback identity and saving](callback_slots.md#comparing-the-current-handler).

```text
func interrupted():
    state.can_catch = false

# Inside the move callback, after entering the motion:
callbacks.damage(interrupted)
```

### `callbacks.secondary_damage(function)`

Bind a zero-argument Void helper to the separate secondary damage slot, or
pass `None` to clear it. The engine dispatches it in its knockback-applied branch
after computing damage and knockback, before entering the normal damage
reaction. It does not run for every primary damage notification. Motion reset
clears this registration, so install it after entering the intended motion.
Use `callbacks.matches(Callback.secondary_damage, helper_or_None)` to inspect
its current registration. See [callback slots](callback_slots.md).

### `callbacks.state_changed(function)`

Register a zero-argument helper for the next motion transition, or pass `None`
to clear it. The engine invokes the helper after assigning the new motion and
attack category, before its ordinary attack-count update, then clears the slot.
This is a one-shot registration, not a persistent observer of every transition.
No motion argument is passed; read `motion.id` inside the helper if needed.
See [`on jab_start`](common_policies.md#on-jab_start) for attack bookkeeping.

## While loops

`while` repeats a block while its condition remains true:

```text
func clamp_counter(int maximum):
    while state.charges > maximum:
        state.charges = state.charges - 1
```

The condition and body use the same expressions and operations as a callback.
A one-line body is also allowed. Loops have no implicit frame advance: the
entire loop runs during the current callback. Ensure its body makes progress
toward a false condition. `break`, `continue` and loop `else` are not currently
part of the language.

## Current boundaries

The current package bank permits 512 functions/callbacks and 512 local
declarations per body, including arguments. A helper accepts up to 32 arguments;
an exported helper accepts up to 30 because dispatch also supplies a handle and
name. There is no generated-C line-count limit: source nodes, expressions and
bytecode use bounded preparation arenas. Overflow produces a diagnostic rather
than truncating a body. Direct and indirect helper recursion are rejected.
Runtime calls share 64 frames and 65,536 registers; synchronous engine reentry
uses the same pools. See [source lifetime](synchronous_callbacks.md).
Functions are available to fighter callbacks. Item/article callbacks retain
their separate execution context and can enter fighter helpers through the
validated `owner.call` or `creator.call` operations.

Callbacks with extra event arguments must pass the needed values explicitly,
for example a Boolean `event.flag` from an item-pickup callback. A helper does
not inherit that event argument implicitly.

Fresh special entries should enter their motion explicitly in `on special_*`;
the compiler uses that declaration to check full ground/air phase ownership.
The `collision.air_special_landing` form that declares an automatic landing
callback belongs directly in the motion collision handler. Other collision
tests and ordinary state transitions may be used in helpers.

Callback registrations in helpers still require their matching authored event
handlers. Engine dependency checks include helper bodies, so a program rejects
a missing required primitive before invoking it. Compilation preserves source
line numbers for diagnostics. See [fighter state](language.md#fighter-state),
[fighter callbacks](fighter.md) and [movement](movement.md).
