# Article terrain queries

This namespace queries stage geometry and applies shared item surface responses. It is
available in authored article callbacks and article-local helpers. The island
bounds query also works in Fighter callbacks and supports a four-Float X/Y form;
see [stage queries](stage.md#floor-and-line-island-reads).
It does not expose an engine island pointer or invoke a tether solver.

## Read line island bounds

```text
article It_Kind_Link_Hookshot states 1:
    state int line
    state float first_x
    state float first_y
    state float first_z
    state float last_x
    state float last_y
    state float last_z
    state 0 anim 0:
        on physics:
            let bool found = stage.read_line_island_bounds(item.state.line, item.state.first_x, item.state.first_y, item.state.first_z, item.state.last_x, item.state.last_y, item.state.last_z)
            if found:
                velocity.y = item.state.last_y
```

`stage.read_line_island_bounds(line, first_x, first_y, first_z, last_x, last_y,
last_z)` returns Bool. `line` must be an Int expression; an explicit `int(...)`
conversion can be used when needed. Each destination must name a distinct
declared writable scalar Float state field or typed Float local. Arrays,
parameters, computed destinations and undeclared fields are rejected. No
implicit Float-to-line conversion is performed.

The input expression is evaluated once, before any destination writes. The
read uses the actual `mpIsland_8005AB54(line)` lookup and copies its `x8` and
`x14` Vec3 values, in that order. These are the engine's stored endpoints; the
operation does not sort, normalize, clamp or replace them with a lookup using
the current item position. Missing island returns false and preserves all six
destinations in the interpreter. The older article C exporter wrote zero to all
six on failure. A negative or otherwise invalid line can therefore be
handled with the same boolean check.

Use the call as a boolean expression or as a statement when only the output
values are needed. The expression's temporary integer and two vectors have
automatic storage; no gameplay allocation or mutable pointer escapes occur.

## Collision mask constants

`Collision.floor_mask` and `Collision.wall_mask` are Int constants resolving
directly to the public engine `Collide_FloorMask` and `Collide_WallMask` values.
They can be combined with an article resource's full-width
`item.resource(index).collision.environment` channel, when that resource is
declared by the article:

```text
            let int contact = item.resource(i).collision.environment
            let int relevant = Collision.floor_mask | Collision.wall_mask
            let bool touching = (contact & relevant) != 0
```

Keep masks and line IDs in typed Int locals/state; untyped `let` defaults to
Float. The constants are read-only and take no arguments. Other `Collision.*`
names are not implicitly recognized.

## Item wall and ceiling contacts

```text
let int wall = item.wall_contact_flags()
let int ceiling = item.ceiling_contact_flags()
let bool bounced = item.reflect_from_wall()
item.align_to_floor()
```

These zero-argument operations use the article's current collision result; they
do not sweep the stage. Call the appropriate collision sweep first. Both contact
queries return Int and publish the contacted line in the item's shared contact
slot. `wall_contact_flags()` returns 8 for a left-facing wall, 4 for a
right-facing wall, or 0 for no wall. When both are present, the right-facing wall
wins, including the published line. `ceiling_contact_flags()` returns 2 for a
ceiling or 0 for none. These compact response codes are distinct from the
`Collision.*` environment bit masks.

`reflect_from_wall()` returns Bool. A wall contact publishes its line, mirrors
velocity only when it points into the wall's normal, then multiplies all three
velocity components by the item's surface restitution. It returns true for
wall contact even when no mirroring was necessary. With no wall it preserves
velocity and returns false. The same right-wall precedence applies.

`align_to_floor()` returns no value. With floor contact it aligns the primary
model through the shared item orientation operation, including facing direction
and the item's existing orientation-axis policy. No floor means no change.

See [item properties](item.md#surface-response-properties) for restitution and
the stop-on-surface flag. Authored code owns the order of these operations;
querying a ceiling after a wall can replace the published contact line.

## Validation scope

The dedicated `article_terrain_compiler_test` covers the real generator, strict
output types/aliases, missing fields, invalid contexts, exact constant names,
Host requirement emission and single line evaluation. The separate
`article_terrain_source_fixture.py` executes the actual bounds entry, island
lookup and line-validity predicate against controlled collision-line graphs.
Its frozen25 run covers 913 cases and 4,553 assertions, including missing and
invalid lines, exact endpoint copies and 64 multiple-island floor chains.
This establishes the backend contract on those graphs; stage construction,
malformed cyclic geometry and authored tether gameplay remain outside it.


## Native projectile contact and landing

`item.hits_stage()` performs Melee's `it_8026E058` ground/left-wall sweep and
returns its removal/contact decision as a Bool. It updates native Item position
and floor information as that routine does. It is a fresh sweep, not a read of
stored `collision.floor_contact`, and does not run a stage hook.

`item.land_on_stage()` performs `it_8026E414`: ground/wall/ceiling handling,
shared impact response and the normal grounded/contact reset. When it lands,
it invokes the Article's `on stage_landed` callback synchronously. Use that
callback for authored landing decisions. A retired/faulted Item stops the caller
before later statements or outputs can access its replacement.

Both calls require a live owned Item and its native collision actor/attributes.
They take no arguments and do not delegate to a stock fighter's article handler.
The controlled native-pair fixture checks landing under 16 floor/wall/ceiling
combinations; full stage solving and live authored gameplay remain separate.
