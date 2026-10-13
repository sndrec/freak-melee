# Linked articles

Articles can keep relationships with other articles from the same fighter
package. Use links for a controller and its visible model, or for a chain of
independently updated segments. Each item has four fixed link slots, numbered
0–3. Links use creation IDs rather than reusable object addresses. The existing
item state carries these slots through rollback; these operations allocate no
relationship storage.

The operations below run in the interpreter. Use exact declared
[article references](article_references.md) for linked position, normal and bone
observations. The old
`item.spawn_link()`, `item.link()` and `item.signal_link()` syntax is not an
available interpreter API. Use the supported ID and signal operations below.

## Creating linked articles

`item.create_linked(DeclaredArticle, slot, backlink, x, y, z, facing) -> int`
creates an ordinary source Article from this item's package. It returns its
creation ID, or −1 when allocation fails or the slot already names a live item.
The declaration is resolved at preparation time; it is not a dynamic ItemKind.
Slots are integral indices from 0 to 3. Position and facing must be finite.

```text
let int visual = item.create_linked(Visual, 0, 0, position.x, position.y, position.z, facing)
if visual == -1:
    return true
```

The parent slot and child's backlink are installed **before** the child's
`on created` callback. `on spawned`, which runs inside the native constructor,
precedes that linking step. The child receives its parent's current damage
owner, retained creator and attachment/control owner. Reflection changes damage
credit without changing the package author, assets or retained control.

It starts with zero velocity, collision group and creation value, at the supplied
position and facing. The position also initializes `item.launch_origin`.
No animation state or physics tick is implicit; Created initializes them.
If a callback removes the child or retires the caller, no live handle is
published. A stale parent slot can be reused. Creating a child with independently
chosen damage owner, group and creation value remains available through
[`item.create_child()`](article.md).

## Linking existing articles

`articles.link_owned(source_handle, slot, target_handle) -> bool` connects two
live source articles belonging to the calling fighter. Both their current damage
owner and their attachment owner must be the caller. Both must use the caller's
published program bank and immutable package author.

```text
articles.link_owned(state.previous_segment, 0, state.current_segment)
```

This creates a one-way link, without creating an item or running a callback.
Issue another call for a reciprocal link. Reinstalling the same link succeeds.
A slot pointing to a different live source article cannot be overwritten; a
stale slot can be reused. Missing/stale handles, ownership mismatches and
self-links return `false` without changing either item. Invalid slot or handle
width is an execution error. Handles and slots must be integral expressions.

## Reading links

| Method | Result |
| --- | --- |
| `item.link_id(slot) -> int` | Live linked source article's creation ID, or −1 if absent. |
| `item.link_reciprocal(slot, backlink) -> bool` | Whether the linked article's backlink resolves to this article. |

Both methods are available in article callbacks. Slots are integral values from
0 to 3. IDs preserve all 32 bits; an ID with its high bit set is returned as a
negative signed Int, distinct from the −1 absence sentinel.

A target must remain a healthy, published source article in the same program
bank and scene, with the same immutable package author. Removed, faulted or
unrelated targets are absent. Reflection changes damage credit; it does not
invalidate an existing relationship. Reuse of a freed object's address does not
make an old link point to its replacement.

## Sending commands through a link

Use `articles.signal(handle, DeclaredArticle, "name", value)` in fighter **or
article** callbacks. The declaration and signal name are resolved once when the
source is prepared; the runtime does not interpret names or compile code.

```text
article Controller states 1:
    on signal follow:
        articles.signal(item.link_id(0), Visual, "follow", signal.value)
    state 0 anim -1:

article Visual states 1:
    on signal follow:
        item.state.distance = signal.value
    state float distance
    state 0 anim -1:
```

The recipient must belong to the exact declared article in the caller's program
bank. A missing recipient returns `false`. A recipient that deletes itself
inside the callback still counts as delivered, provided the caller remains live.
Caller retirement stops the caller; recipient script errors propagate rather
than allowing subsequent statements to run as if the command succeeded.

`articles.signal_owned()` remains fighter-only and additionally checks current
and attachment ownership. See [signals](fighter.md#articlessignal).

## Copying collision state

`item.copy_link_collision(slot) -> bool` copies the linked article's complete
native collision record into the calling article. This includes contact and
sweep history; it does not run a new collision query or copy position, velocity
or surface-normal fields. Read those fields separately through an Article
reference. A missing link returns false and leaves the source unchanged.
Both items must be live, non-removing members of the same published script bank.
The operation runs no callbacks and allocates no memory.

## Removing links

`item.unlink(slot)` clears this slot and all reciprocal slots on its live linked
article. It does not delete either item or run any callback. An absent link is
safe to clear. It can run from `on destroyed` while this article still owns live
script storage, including after the native removal marker is set.

```text
on destroyed:
    item.unlink(0)
```

The script decides whether a disconnected child should expire, remove itself,
or continue independently. See [article lifecycle](article.md) for callbacks.
