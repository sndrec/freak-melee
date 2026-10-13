# Program parameters

`program.attrs.NAME` reads a root parameter from the package executing the
current script. It is available in fighter, article, ordinary item and model
effect callbacks. The declaration must be a root-owned immutable parameter or
an imported root attribute; it cannot be mutable fighter state.

```text
parameter int hand_attachment = 32
parameter float launch_speed = 3

article Projectile states 1:
    parameter float launch_speed = 1
    on created:
        item.attach_to_owner(program.attrs.hand_attachment, false)
        item.velocity.x = program.attrs.launch_speed
```

In this example, `program.attrs.launch_speed` is the root value `3`.
`item.params.launch_speed` is the article-local value `1`. Local `attrs.NAME`
keeps its existing scope; it never silently falls back to root parameters.
See [article parameters](article.md#parameters) and
[fighter parameters](language.md#fighter-attributes) for declarations and
Workshop editing.

## `program.entity_params.ENTITY.NAME`

Read a declared entity's immutable parameter from the executing package. The
entity can be an Article, ordinary Item or model Effect. The entity name is the
literal name of its source declaration, and `NAME` is a `parameter` or imported
`attribute` declared within it. The compiler resolves both names and the exact
scalar type before the package runs.

```text
state float spin_rate

article YoYo states 1:
    parameter float released_spin_rate = 0.4
    state int remaining_hits
    state 0 anim -1:

func UpdateSmash():
    state.spin_rate = program.entity_params.YoYo.released_spin_rate
```

A read needs
no live YoYo actor, no Item handle and no surviving creator. It also works from
another entity in the same package. `program.attrs.NAME` continues to select
root parameters only; `item.params.NAME` and local `attrs.NAME` select only the
current entity's declarations.

Entity parameters are read-only, including inline defaults and resolved imports.
They share the package's existing immutable parameter table and add no actor
state or rollback storage. This path cannot read `state` declarations, select a
foreign package, call a helper or change a parameter. Missing, ambiguous and
unresolved declarations fail rather than silently choosing another scope.

## Ownership and lifetime

The compiler resolves the name and exact scalar type when preparing the
package. During execution, the read uses the current actor's pinned immutable
parameter table. It needs no live creator, holder or damage owner. Reflection,
dropping an item or removal of its original fighter cannot redirect the read
to another fighter's tuning. A later package reload does not substitute its
values into an existing actor's pinned program.

Use `creator.attrs.NAME` when you explicitly want a live creator's parameters,
with the appropriate receiver existence check. Use `program.attrs.NAME` when
the authored behavior belongs to the creating package even after that creator
has gone away.

## Restrictions

- Root parameters are read-only. Assignment is a compile error.
- Root mutable state and another article's local parameters are inaccessible.
- Parameters are typed scalars; subscripting a parameter is unsupported.
- A retired executing actor, a mismatched parameter layout, an unresolved
  imported value or a malformed type fails the read. No default value is
  invented and no output is published on failure.

Game & Watch's authored articles use this namespace for shared attachment
parts, motion IDs and move tuning. Their live palette and motion queries
remain explicit receiver operations.
