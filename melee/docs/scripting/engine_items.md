# Fighter engine-item operations

These methods create and manage ordinary engine items from fighter callbacks
or fighter helpers. Article callbacks use their own `item` namespace. These
methods are unavailable in article and model-effect update contexts.

## Create an item

`items.spawn_food(x, y, z)` returns Bool through the ordinary interpreter and
runs the native engine Foods
spawn operation. The supplied XYZ becomes the previous position; the current
position comes from the calling fighter's engine center. The operation checks
the enabled-item bitmap, stage item policy and item frequency, uses facing -1, zero velocity and damage, enables
initial collision, and supplies no primary or secondary parent. A successful
allocation increments the engine's ordinary food count. False means the
availability check or allocation failed.

Coordinates evaluate once in their written order. A synchronous spawn callback
can retire or recycle the caller. The binding validates its exact physical
identity, owned actor, program and storage before publishing the Bool. A live
food created before caller retirement remains in the world, including its
completed success-counter increment. The result reports a successful live
construction, not a retained item handle. The native NULL boundary includes
availability/allocation failure and immediate constructor cleanup. This is
distinct from article `item.spawn_food()`, which disperses food from an existing
ordinary Food item.

```text
let bool spawned = items.spawn_food(position.x, position.y + 5, position.z)
```

For package-authored articles, `articles.create_with_velocity(kind, x, y, z,
vx, vy, vz, facing, origin_at_owner, collision_group, creation_value,
initial_collision)` requires the final Bool. Passing true enables normal initial
collision setup. Passing false skips that setup; the article
script can then initialize its own attachment and collision state. This
controls the actual spawn descriptor rather than changing collision after
creation. Ordinary `items.create_engine` retains its standard engine setup.

```text
let int handle = items.create_engine(It_Kind_Bat, position.x, position.y, position.z, velocity.x, velocity.y, velocity.z, facing, int(uint(0xFEED1234)), 2.5)
```

The ten arguments are kind, position XYZ, velocity XYZ, facing, collision group
and damage. Kind is Int; collision group accepts integral signed/unsigned
32-bit word representations and rejects Float or out-of-width values. Position,
velocity, facing and damage use the language's ordinary Float conversions.
Arguments evaluate once, in their written order, before the direct shared
engine creation call.

The result is an Int creation handle, or `-1` on failure. Handles preserve all
32 bits; values above `2^24` and handles with the high bit set remain exact.
Keep handles and collision groups in Int variables. Float expressions are
rejected at those argument positions; explicit `int(...)` conversion is an
authored choice, not an implicit handle conversion.

Creation rejects nonfinite position, velocity, facing or damage. Damage must
lie within `INT16_MIN..INT16_MAX` (`-32768..32767`) before the engine narrows it
into the spawn damage field. There is no saturation or clamp. The normal C
narrowing discards a fractional damage component within that range. The
engine validates the source and created item across constructor callbacks;
scripts still decide what to do when creation returns `-1`.

## Attach and inherit scale

`items.attach(handle, rawpart, notify_holder)` returns Bool. The first two arguments must be Int; the
part is an actual runtime part-table index. Use `bones.index(Name)` to resolve
a semantic bone when appropriate. The engine rejects missing, foreign,
retired or mismatched item ownership and invalid parts. Attachment uses the
real shared held-item engine operation. `notify_holder` must be Bool. Set it
to `true` to repeat the native debug-display inheritance step after attachment
and pickup finish. This flag is not an additional fighter pickup callback.
Use `false` for attachment alone. The operation checks item and fighter
generations after callbacks and never publishes a held or auxiliary slot
itself; [bind those slots explicitly](#bind-and-clear-fighter-slots).

`items.inherit_owner_scale(handle, remember)` returns Bool. The handle is Int
and `remember` is Bool. It uses the item's **current damage owner's** scale;
that scale source can differ from its original creator. With `remember=true`,
the resulting item scale is also retained as its throw scale. Failure returns
false; it does not silently select another owner.

Creation and attachment can invoke synchronous callbacks. The interpreted
binding validates the calling fighter's exact pool/native identity, program and
live actor storage after those calls, before reading cached payload or publishing
a result. The VM propagates invalidation through helper callers. An invalidated
caller stops before assigning the result or continuing the callback. This
also handles reuse of the same address with a different generation.

## Bind and clear fighter slots

| Surface | Result and behavior |
| --- | --- |
| `items.bind_held(handle)` | Bool; bind an item created by this fighter in its held-item slot. |
| `items.bind_auxiliary(handle)` | Bool; bind an item created by this fighter in its auxiliary item slot. |
| `items.bind_existing_held(handle)` | Bool; bind any live item handle in the held-item slot. |
| `items.bind_existing_auxiliary(handle)` | Bool; bind any live item handle in the auxiliary item slot. |
| `items.clear_held()` | Void; clear the held-item binding. |
| `items.clear_auxiliary()` | Void; clear the auxiliary item binding. |
| `items.held_id` | Read-only Int handle for the live held item, or `-1` if absent. |
| `items.auxiliary_id` | Read-only Int handle for the live auxiliary item, or `-1` if absent. |

Binding and attachment are separate operations. Clearing is a healthy no-op for
a live fighter whose selected slot is already empty; it does not destroy the item. The ID properties take no parentheses and do not expose
pointers. Methods can be used as statements when the result is irrelevant.

Creator-restricted attachment, scale inheritance and `bind_held` /
`bind_auxiliary` admit both same-bank source Articles and ordinary engine items
created by the calling fighter. Source Articles retain their immutable owned
creator relation and exact program identity. Raw engine items use their actual
`native_creator`, native generation and kind; no source specification is
fabricated. The selected Item's physical identity, payload, creator and live
Article/model/attribute/logic peers remain stable through the operation.
Current damage ownership can differ after reflection. `bind_existing_*` remains
an explicitly different foreign-item restoration operation.
`articles.bind_held` / `articles.bind_auxiliary` use the same creator-guarded
binding policy.

The existing-item methods restore a previously saved binding without requiring
the original creator or current damage owner to match the receiving fighter.
They change only the fighter's slot. They do not reattach the model, rewrite
ownership, transfer the creator, show the item or send pickup notification.
Missing, removed or recycled handles fail without changing the slot.

## Temporarily hide and pause an item

`items.set_visible(handle, visible)` and `items.set_paused(handle, paused)`
return Bool. The handle must be Int and the second argument Bool. They resolve
the live instance and invoke the ordinary engine model visibility or item
pause/resume operation. Visibility requires a loaded model root; pause does
not. Neither changes attachment, owner, creator or the fighter's bindings.

For example, retain an item while a move temporarily uses another one:

```text
state.saved_item = items.held_id
if state.saved_item != -1:
    items.set_visible(state.saved_item, false)
    items.set_paused(state.saved_item, true)
    items.notify_drop(state.saved_item, true)
```

Restore it explicitly, preserving the intended operation order:

```text
let int saved = state.saved_item
state.saved_item = -1
if items.bind_existing_held(saved):
    items.set_visible(saved, true)
    items.set_paused(saved, false)
    items.notify_pickup(true)
```

## Notify the fighter of an item drop

`items.notify_drop(handle, normal)` returns Bool. This sends the ordinary
fighter drop notification using an Int live item handle and Bool flag. It does
not throw or destroy the item. The engine notification may clear the held slot
and synchronously call fighter callbacks; it retains its normal exceptions
and callback order.

The result is published only if the fighter's full retained identity, source
actor/program/storage and notified Item remain valid. A synchronous retirement,
VM fault or identity change stops the caller and preserves its previous output;
an absent handle is a healthy false. An Item already queued for removal can
remain logically live, but its captured removal state must not change during the
operation.

The connected compiler/VM/actor fixture compares the actual common pickup,
scale, throw setup and food-spawn bodies. Common constructor, immutable asset,
render/animation and statistics leaves are controlled explicitly. That proof
does not establish full engine DAT constructor or gameplay acceptance. The old
generated-C tests remain historical reference evidence only.
