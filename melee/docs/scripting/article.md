# Article declaration

An `article` block replaces the behavior of an existing article kind in the
fighter's DAT. It does not create a new engine item kind. The article kind,
state count, and animations must match content present in the fighter package.
Its callbacks use the [Item API](item.md).

For independently updated parts, see [linked articles](article_links.md).
For immutable root import and full local transforms, see [article models](article_models.md).
For binding an existing article to a held or auxiliary reference without
pickup side effects, see [fighter presentation](presentation.md#article-references).

## Explicit creation

### `articles.create_unowned_at_position(ArticleName, x, y, z, vx, vy, vz, facing)`

Available in fighter scripts. Returns an Int creation ID, or -1 when the live
native constructor cannot allocate/create the item. `ArticleName` must name an
article declared in this package; it is resolved when the script is prepared.
All seven numeric arguments accept expressions and are converted to Float.

Both gameplay parents, the damage owner and the original fighter creator are
absent from construction onward, including `on spawned` and `on created`.
The calling package still selects the code, assets and immutable parameters;
it does not depend on a live fighter owner for later item callbacks.
Use this for autonomous entities which should not credit damage to their author.

```text
articles.create_unowned_at_position(Debris, position.x, position.y, 0, -facing.direction * attrs.launch_speed, attrs.vertical_speed, 0, facing.direction)
```

Current and previous position use the supplied point. Velocity and facing are
set before construction and `on spawned`; `item.launch_origin` and the initial
attachment facing are recorded before `on created`. Collision group and
creation value start at zero, and initial collision setup is enabled. The
constructor preserves its normal airborne/hold-kind defaults. It does not
force a motion state, advance physics/animation, call `on launched`, or supply
an authored lifetime. Lifecycle scripts own those choices.

Non-finite inputs and missing/cross-package declarations are errors. Retirement
of the calling actor, removal of the new item, or a callback fault stops script
continuation without publishing a handle. Native side effects already performed
are retained; this operation does not implicitly roll back gameplay. A healthy
created item remains attached to its package if the author later retires.


`articles.create_held_at_center(ArticleName, previous_x, previous_y, previous_z,
raw_bone, facing)` creates a zero-velocity authored item at the fighter's common
item center, with the supplied previous position for its initial collision sweep.
`ArticleName` selects one exact declaration when the source is prepared.
It runs `on created` **before** the pickup callback, attaches to the supplied
runtime bone index and records the ordinary held item. It returns a creation ID
or -1 on failure. It does not notify the fighter's pickup handler; call
`items.notify_pickup(...)` when the move requires that notification.
The saved creator remains unchanged on pickup or reflection.
See the [held-at-center factory reference](held_center_factory.md) for callback
ordering, raw index validation and failure behavior.

`articles.create_at_position(kind, x, y, z, facing, origin_at_owner)` is a
fighter callback expression returning the item's creation ID, or -1 on failure.
Creation ID `0` is valid. Test a handle against `-1` or use the appropriate
live-article query; do not treat zero as a null pointer or a failed creation.
It may also be used as a statement. `kind` must name an Article declaration in
this package; the remaining five arguments accept expressions. It creates
an authored article from this fighter's package at the requested position.
Initial velocity is zero. When `origin_at_owner` is false, current and previous
positions both use the supplied point. When true, current position uses the
fighter's item center; previous position remains the supplied point for the
initial collision sweep. `item.launch_origin` records that current spawn position.

Package assets and authored callbacks are bound before model construction
and the engine's `on spawned` callback. The constructor then supplies the
launch origin and attached fighter identity and calls `on created`. It does
not implicitly choose a motion state, advance animation, initialize lifetime,
update physics or call `on launched`. The script owns those steps. Missing
authored behavior/assets or non-finite position/facing means no item is made;
creation never falls back to a stock article handler.

The eight-argument positional form appends `collision_group, creation_value`:
`articles.create_at_position(kind, x, y, z, facing, origin_at_owner,
collision_group, creation_value)`. Both positional forms use zero velocity and
initial collision enabled. The twelve-argument velocity form below controls
those values explicitly. Collision groups accept integral native 32-bit words,
including signed representations such as `-1` for `0xffffffff`; Float and
wider values reject creation. Obtain a new
group with `articles.new_collision_group()` and reuse it for parts that should
share the engine's hit-contact grouping. Zero leaves the ordinary ungrouped
spawn behavior. The finite scalar `creation_value` is exposed as
saved actor-owned `item.creation_value` after `on spawned`, before `on created`, so instances can initialize their
index, delay or role without a later notification. The six-argument form uses
zero for both values. Neither argument creates links between items; use
[`articles.link_owned()`](article_links.md#linking-existing-owned-articles).

`articles.create_with_velocity(kind, x, y, z, vx, vy, vz, facing,
origin_at_owner, collision_group, creation_value, initial_collision)` performs the same creation
sequence with an explicit initial velocity. Velocity is installed before model
construction and both creation callbacks. The Article identifier is literal;
the other eleven arguments accept expressions. `initial_collision` is an explicit
Bool controlling the native spawn descriptor's initial collision setup. It does
not select grounded spawning or change collision after creation.
Collision groups preserve all
32 bits and reject wider values instead of truncating them;
non-finite components reject creation. Use this when a callback needs the
launch velocity immediately, rather than assigning it after construction.

Inside an article callback, `item.create_at_position(kind, x, y, z, facing)`
creates another authored article with zero velocity. Its current and previous
positions match the supplied point. The new item keeps this item's package
author even after reflection; its damage owner is this item's current owner,
and its secondary parent is the creating item. It returns an integer creation
ID or −1 when the native constructor returns no item. The two items are not linked.
The Article name is a literal declaration in the same package; coordinates and
facing may be expressions. Attachment control is inherited separately from
immutable original-creator provenance and mutable damage ownership.

`item.create_child(kind, x, y, z, facing, damage_owner, collision_group,
creation_value, backlink)` creates a child with explicit damage ownership and
collision group. It returns an integer creation ID, or −1 on failure. Keep the
result in an [integer local](language.md#callback-locals) or `state int` field.
The child starts airborne with zero velocity and matching current/previous
positions. `damage_owner` is a live fighter target ID; 0 means no damage owner.
An invalid nonzero ID rejects creation.

The child's `backlink` slot (0–3) points to the parent's full creation ID before
`on created` runs. Its creation value and original fighter-creator bookkeeping
are also initialized first. The creating package and original creator remain
independent of damage ownership, including after reflection. No reciprocal
parent slot is consumed, and the child is not registered as a held item.
Non-finite numeric components or unavailable authored assets reject creation.

Creation callbacks run synchronously. Creation value and backlink are installed
after `on spawned`, before `on created`; neither factory implicitly selects a
state or advances animation. Original creator and any live captured attachment
and damage owner must survive construction. An already unavailable optional
attachment remains absent, even if its old slot has a new occupant. Clearing
the parent's attachment during a callback does not reconnect the child later.

Retirement or a script fault after the constructor returns a live item prevents
handle publication and stops the caller;
it does not undo creation side effects already performed. Removing the child
in `on created` therefore differs from ordinary allocation failure. No native
pointer or newly reused object is substituted. The native constructor's NULL
result also covers a child removed or faulted during `on spawned`; with a live
caller and captured relationships this publishes −1, just as allocation failure
does. Read an existing child's state
through [article references](article_references.md), which validate each read.

Use the configured [projectile launch](#launch-initialization) operations for
the ordinary launch sequence, and explicit creation when its ordering or
origin must be controlled by the script.

```text
article It_Kind_Luigi_Fire states 1:
    state bool bounced
    projectile appearance -1 state 0 speed launch_speed angle 0 lifetime lifetime auto_lifetime on

    state 0 anim 0:
        on animation:
            item.life = item.life - 1
            if item.life <= 0: return true
            return false
        on physics:
            item.gravity()
        on collision:
            item.stage_collision()
            item.state.bounced = item.bounce_stage()
            if item.state.bounced: item.spawn_effect_on_bone(1288, 0, 1)
            return false
    on shield_bounced:
        return item.shield_bounce()
```

The example is shortened from [Luigi's Fireball](../../examples/luigi_wip.fms).
That file contains the complete state and interaction callbacks.

### `articles.new_collision_group() -> int`

Allocates one group from the existing match-wide item collision-group counter.
The native item system starts the counter at one and skips zero when it wraps.
This operation does not create an article, allocate memory, draw random numbers
or change fighter state. It is available in fighter callbacks and helpers;
article callbacks retain their `item` namespace.

The returned Int preserves the native 32-bit word. Values above `0x7fffffff`
appear negative, and can still be passed unchanged to article creation. A group
is a contact identity, not an item handle or fighter-owned variable. Store it
in an ordinary state field when several projectiles should share it:

```text
state int projectile_group

func begin_volley():
    state.projectile_group = articles.new_collision_group()
```

Allocate another group only when the move intends to reset shared contact
grouping. The interpreter and the reference exporter use the same native
allocator; there is no separate custom-fighter counter.

## Local variables

Article callbacks accept nested `if / elif / else` blocks as well as one-line
`if` statements. Indent each nested body by four spaces. Empty blocks, orphaned
`else` clauses and a second `else` are rejected. Branches prepare as typed
bytecode. Article callbacks retain their separate item context; use a validated
`creator.helpers.Name(...)` or `owner.helpers.Name(...)` receiver to call a
package fighter helper. See [article helpers](article_helpers.md).

```text
    on signal choose:
            if signal.value < 0:
                item.state.direction = -1
            elif signal.value > 0:
                item.state.direction = 1
            else:
                item.state.direction = 0
```

Declare `state bool NAME`, `state int NAME`, or `state float NAME` inside the
article, before its first state or callback. Read and assign it as
`item.state.NAME`. Each live instance has its own variables; two projectiles
of the same kind do not share a counter or flag. Values start at zero on
creation, persist across article states and reflection, and disappear when
the item is removed. Use `on spawned` for other initial values. For values
that depend on a configured projectile launch, use [`on launched`](#launch-initialization).

```text
article It_Kind_Mario_Fire states 1:
    state int remaining_bounces
    state bool hit_stage
    state float previous_speed
    on spawned:
            item.state.remaining_bounces = 3
    state 0 anim 0:
        on collision:
            item.stage_collision()
            item.state.hit_stage = item.bounce_stage()
            if item.state.hit_stage: item.state.remaining_bounces = item.state.remaining_bounces - 1
            if item.state.remaining_bounces <= 0: return true
            return false
```

An `int` is a signed 32-bit integer, a `float` is 32-bit, and a `bool` occupies
one byte. The compiler checks names and reserves aligned storage, with at
most 64 declarations and 512 bytes per article. It rejects duplicate names,
undeclared fields, and fields declared after behavior begins. Storage is
part of the existing item state captured by rollback; callbacks allocate
nothing. There is no stock-fighter union or DAT offset behind these variables.

Fixed arrays use `state TYPE NAME[COUNT]`, where the type is `bool`, `int` or
`float` and the count is 1 through 128. They use the same per-instance storage:

```text
article It_Kind_Mario_Fire states 1:
    state int write_index
    state float history_x[16]
    state 0 anim -1:
        on physics:
            item.state.history_x[item.state.write_index] = position.x
            item.state.write_index = (item.state.write_index + 1) % 16
```

Each array counts as one declaration. Reads and assignments require an index.
The compiler rejects an out-of-range constant index. Dynamic indices convert
to signed 32-bit integers and evaluate once: out-of-range reads return zero or
false, and writes leave storage unchanged. An attached owner's declared fighter
arrays are also accessible through `owner.state.NAME[index]`; absent-owner
reads return zero or false and writes do nothing. The declaration metadata
records one name, type, count, stride and byte offset for the whole array.

`item.state.NAME` is separate from [fighter variables](language.md):
`owner.state.NAME` accesses a declared variable of a valid attached owner.
Use `creator.state.NAME` or `creator.state.NAME[index]` when behavior must
address the original creator after damage ownership transfers. Creator access
resolves the saved fighter creation ID against the live fighter registry and
requires its active authored definition to match the article's immutable
package definition. Removed, recycled or differently authored fighters cannot
be read as this script's state. Failed reads yield zero or false; failed writes
and `creator.call(FunctionName, arguments...)` do nothing. `owner.state` and
`owner.call` use the same validation and additionally require the current
damage owner to remain that creator. See [Item API](item.md#creator-context).

[`attrs.NAME`](language.md#fighter-attributes) remains an immutable package
parameter. The separate [holdable-item declaration](../native_fighter_items.md)
still has eight float scratch slots; it does not use article declarations.

Article expressions support integer remainder (`%`), useful for a recurring
effect counter such as `(item.state.effect_step + 1) % 3`. Use integer operands
and a nonzero divisor. Integer remainder by zero is a script error. See
[numeric semantics](language.md#callback-locals) for wrapping, conversions and
other checked operations.

## Attachment creation

Declare attachment creation settings before the first callback or state:

```text
attachment origin owner plane on initial_collision off held off
```

`origin owner` uses the fighter's current world position. `origin bone` uses
its attachment bone's world position. `origin center` uses the fighter's
collision-box center: the current position with the midpoint of the top and
bottom ECB offsets added to Y. It is distinct from the model origin.

By default, the initial previous position equals the selected origin.
The optional `previous bone` retains the attachment bone's world position as
the previous position while the current position uses `origin`. This lets the
initial collision sweep begin at the hand and end at the collision center.
`previous origin` explicitly selects the default.

```text
attachment origin center previous bone plane off initial_collision on held on
```

`plane on` sets both selected positions' Z to zero; `off` preserves Z.
`initial_collision` controls the engine's initial collision spawn flag.
`held on` registers the special held-item slot; `off` leaves that slot alone.
Both choices attach the item to the requested bone and run its pickup callback.
Creation installs the selected current/previous positions before `on created`,
then runs pickup; special-held registration follows pickup. Scripts retain
control of their own bookkeeping and launch sequence.

Without a declaration, creation uses the bone position for both positions,
preserves Z, enables initial collision, and registers the special held item.
The fighter creates it with
[`articles.spawn_bone_attached()`](fighter.md#articlesspawn_bone_attached).
These settings are immutable module data, not per-instance state.

## Accessory callback

`on accessory` declares an optional effect callback. Install it explicitly
with [`item.track_accessory()`](item.md#itemtrack_accessory); declaring it alone
does not install it. State changes clear the live callback. Scripts decide
whether to install it again. The direct interpreter saves the declared
callback's code ID in the Item actor and clears the native callback pointer.

```text
    on signal fire:
            item.state.effect_pending = true
            item.track_accessory()
    on accessory:
            if not item.state.effect_pending: return
            item.state.effect_pending = false
            item.spawn_directional_effect(1166, RightThumb, 0, 1.2325, 4.2636, 0)
```

This example belongs inside an article that declares `state bool
effect_pending`. Tracking requires an accessory callback body.

## Parameters

Article tuning belongs to the fighter package's `fighter_parameters.fmp`,
just like [fighter parameters](language.md#fighter-attributes). Qualify each
name with its article kind so separate articles can reuse a local name:

```text
# Illustrative authored tuning, not stock defaults.
float It_Kind_Luigi_Fire.launch_speed = 3
float It_Kind_Luigi_Fire.lifetime = 120
float It_Kind_Luigi_Fire.minimum_bounce_speed = 0.1
bool It_Kind_Luigi_Fire.can_bounce = true
```

Callbacks read these as `item.params.launch_speed` or `item.params.can_bounce`.
The compiler resolves the scope and typed storage once; callbacks do no string
lookup or allocation. Values remain with the creating package after reflection
or ownership changes. Use [`item.state`](#local-variables) for mutable counters.
For shared root tuning, read [`program.attrs.NAME`](program.md). This selects
the executing package's immutable root parameter even if its original fighter
has been removed. It does not change article-local parameter scope.
Changing a value needs a reload/repackage, but no behavior rebuild. Changing its
name, type or position requires rebuilding, as with fighter parameters.

Projectile speed, angle and lifetime accept a scoped float parameter's local
name or a finite numeric literal. Launch initialization reads the current owned
values; it does not bake defaults into C or read source DAT tuning.

Stock translation **drafts** can declare up to 32 article attributes using
`attribute float NAME OFFSET` or
`attribute int NAME OFFSET` inside an
article to identify an original DAT field and read it as `item.attr.NAME`.
[`import-parameters`](language.md#fighter-attributes) copies these defaults into
the qualified parameter store, replaces offsets with scoped literal
`parameter` declarations, and changes reads to
`item.params.NAME`. Float/int offsets are word aligned; bool imports one byte.
The converted script contains no article tuning offsets. Shared stock templates
retain only the source fighter's article blocks (e.g. Fox, not Falco's variants).
An unmapped kind cannot import another fighter's attribute bytes. Direct draft
compilation remains a translation aid; use owned parameters for authored content.
The Workshop imports offset declarations in all move files, appending new values
to its Parameters editor while preserving existing owned values. Conflicting
names leave the original source untouched. Save and build the converted draft.

## Constant tables

Declare a small, read-only float table before the article's first callback:

```text
table float recoil_distance = [0, -0.5, -1, -0.5]
```

Read an entry as `table.recoil_distance[item.state.recoil_frame]`. Indices are
converted to signed 32-bit integers; negative or out-of-range indices return
zero. The index expression is evaluated once. Tables contain finite numbers,
with at most 64 entries per table and 16 tables per article. They become
constant module data, so each item instance only stores its changing index.

The [gun model example](../../examples/fox_falco_blaster.fms) uses tables
for joint translation, scale, and rotation while its ordinary named variables
control opening and recoil. A table is useful whenever several animation
states need the same sequence of values; it is not a special blaster feature.

## Signals from the fighter

Signals let a fighter tell an attached article to do something without knowing
the article's private variables. The article decides what the signal means:

```text
article It_Kind_Fox_Blaster states 1:
    state int recoil_frame
    on signal recoil:
            item.state.recoil_frame = int(signal.value)
    state 0 anim 0:
```

The fighter sends this with
[`articles.signal_owned(handle, DeclaredArticle, "recoil", 1)`](fighter.md#articlessignal_owned).
The callback runs immediately. `signal.value` is a float, available only inside
that signal callback; use `int()` to assign an integer. A signal callback has
no return value and may use `return` to stop early. It can select a state that
ends the article's lifetime; check that the handle still exists before later
requests rather than treating the signal as a new persistent reference.

Names are identifiers of up to 47 characters. An article can declare up to
16 distinct signals. Their lookup table is immutable module data, and dispatch
allocates nothing. The compiler rejects duplicate names and colliding signal
identifiers within an article. Signals are private to the live owned article:
a stale handle, changed owner, stock item, unknown signal, or nonfinite value
does not call a callback. This is separate from ordinary item interaction
callbacks, which the engine invokes on collisions or handoffs.

## Sharing state callbacks

Different states can use the same callbacks with different DAT animations:

```text
state 0 anim 0:
    on physics:
            velocity.y = velocity.y - 0.1
state 1 anim 1 use 0:
state 2 anim 2 use 1:
```

`use INDEX` selects all three callbacks from an already declared state. It
keeps the new state's own animation index. Chained sharing resolves to the
original callbacks at build time: the state table stores their function
pointers directly. A shared state cannot also define its own callbacks.
Forward references and cycles are rejected. The source only needs to have
been declared earlier; its numeric index may be larger than the new state's.

## Projectile appearance

```text
projectile appearance APPEARANCE state STATE speed SPEED angle ANGLE lifetime LIFETIME auto_lifetime on|off
```

This declaration configures a projectile created with
`articles.spawn_authored_projectile(KIND, BONE, VARIANT)` in the fighter's
move script. `APPEARANCE` is an existing DAT appearance index, or `-1` for
none. `STATE` is the initial declared article state. `SPEED` is the launch
speed, `ANGLE` is in radians, and `LIFETIME` is the initial life value. Each
value can name an attribute or be a finite numeric literal.

The optional flags default to `on`; each can be declared once, in any order:

| Option | Meaning |
| --- | --- |
| `auto_lifetime on|off` | Keep or clear the engine's world-interaction flag. The existing name refers to its automatic lifetime handling for ordinary item kinds; it also gates pickup eligibility. Fighter projectiles above the ray-gun kind manage expiry in their own callbacks. |
| `initial_velocity on|off` | Calculate initial velocity from launch speed/angle, or leave the spawn velocity at zero so the article can calculate it in its own update. |
| `initial_update on|off` | Run the common post-spawn update (`it_802750F8`), or leave it to the article's ordinary phases. This does not disable later simulation ticks. |

For example, Dr. Mario's pill clears world interaction; Luigi's Fireball
keeps it. The blaster laser leaves initial velocity and the common initial
update off, matching the original article's setup. These are behavior
choices, not fighter-specific switches.

## Launch initialization

`on spawned` runs during creation, before the fighter's launch operation has
set up state, speed, angle, lifetime and appearance. `on launched` runs after
those steps, once for each projectile created through an authored launch
operation. It is a void callback and requires a `projectile` declaration.

The launch properties are read-only snapshots; reflecting the item does not
rewrite them. Copy them into local variables if the article changes its
trajectory during play:

```text
article It_Kind_Fox_Laser states 2:
    state float angle
    state float speed
    projectile appearance -1 state 0 speed 5.5 angle 0 lifetime 35 initial_velocity off initial_update off
    on launched:
            item.state.angle = item.launch_angle
            item.state.speed = item.launch_speed
    state 0 anim 0:
    state 1 anim 1:
```

`item.launch_angle` is a normalized world-space angle in radians;
`item.launch_speed` is the configured speed. `item.launch_origin.x/y/z` is the
owner position supplied to the item's spawn collision initialization, which
can differ from the muzzle position (`position.x/y/z`). The
[laser example](../../examples/fox_falco_laser.fms) uses that origin for its
first stage sweep and thereafter stores the preceding physics position.

Use [`articles.launch_projectile()`](fighter.md#articleslaunch_projectile)
when the firing script chooses angle, speed, state and bone-local muzzle
offset at runtime. The simpler `spawn_authored_projectile()` uses the
declaration's defaults, a facing-relative angle, and zero bone offset.
A source file may contain only article declarations; it does not need a
placeholder fighter handler to compile.

## States and callbacks

`on out_of_bounds:` runs in the item's ordinary bounds check before the stock
kind-specific bounds behavior. Its presence replaces that stock behavior, even
when it deliberately does nothing. The callback belongs to the module that
created the article, independently of its current damage owner. It can change
state or remove the item; removal immediately ends this bounds operation.

`state INDEX anim INDEX:` declares one reachable item state and its DAT
animation index. State callbacks run in the item's animation, physics, and
collision phases. Item-wide callbacks such as `on reflected`, `on clanked`,
`on hit_shield`, `on shield_bounced`, and `on interaction` handle contacts and
ownership changes. A state-changing callback can call
[`item.change_state()`](item.md#itemchange_state) when the new state is declared.

For boolean callbacks, `true` asks the engine to remove the item; `false`
retains it. A stock translation should enumerate and compare every reachable
path before it is considered complete. Do not assume an omitted authored
callback reproduces the stock article's decision.

### Stage response callback

[`item.respond_to_stage()`](item.md#itemrespond_to_stage) requires a declared
`on stage_contact:` body. The shared routine updates contacts and performs
its stage response, then calls that void handler on contact. The handler
chooses effects, state changes, expiry or other behavior and is bound directly
to the compiled callback.

### Calling a local signal

`item.signal("name", value)` synchronously calls a signal declared in the same
Article. The load-time compiler resolves the literal name to owned callback
code, and the interpreter delivers its F32 `signal.value`. It does not queue an
event or search another item. Unlike targeted `articles.signal`, self delivery
preserves all F32 payloads, including NaN and infinity, as the original local
call did. The Bool result reports successful entry; caller retirement stops
the script. Static call-graph checks reject recursive signal/helper chains.

Use it to share a response across collision, expiry and combat callbacks:

```text
on signal expire:
        item.begin_expiry()
        item.stop_motion()
        item.change_state(2)
on stage_contact:
        item.signal("expire", 0)
```

Signals and helpers share bounded interpreter call depth and register storage;
a local signal can hold a shared response without duplicating its body.

## See also

- [Script structure and callbacks](language.md)
- [Item API](item.md)
- [Package-authored holdable items](../native_fighter_items.md) — a separate
  declaration type for pickups and throwable items

## Unattached creation and model instances

[`articles.spawn(kind, state)`](fighter.md#articlesspawnkind-state) creates an
owned article at the fighter's world position. Its `on created` notification
runs after owner metadata and the initial animation state are ready, before
the constructor's first update. Attached and projectile factories deliver the
same notification at the end of their setup. `on spawned` is earlier, inside
engine creation; `on launched` remains the projectile launch notification.
Immediate removal is forbidden in `created`, `spawned`, `picked_up`, and
`destroyed` callbacks because their caller still uses the item.

Declare `render standard` before callbacks to use the common article renderer
instead of the inherited kind's custom C renderer. This is appropriate when
scripts replace a kind whose stock renderer reads private item variables.

An article can load its declared package-owned roots into three additional slots,
using [`item.load_model(slot, name)`](article_models.md#itemload_modelslot-name---bool). Each instance has
an independent world transform. `item.joint("path")` addresses the primary
model; `item.joint(1, "path")` addresses slot one. Instances draw after the
primary model and are released automatically with the item. They use HSD's
tracked object pools, with fixed pointer storage in each item.

See [Article models](article_models.md) for declarations, raw quaternion
transforms, draw gates, named joint/material/shape animation components and
lifetime rules. The direct interpreter supports `model_animation` declarations
and the shared model animation operations documented there.
