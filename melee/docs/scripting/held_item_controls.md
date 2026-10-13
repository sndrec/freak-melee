# Fighter item controls

These operations change the fighter's held or auxiliary item slots. They do not
select a stock fighter's behavior. Item handles are full 32-bit creation IDs;
signed representations preserve their bits, and `-1` means absent. Float handles
and wider values that do not fit reject rather than selecting another item.

| Operation | Result | Behavior |
| --- | --- | --- |
| `items.held_id` | int, read-only | Current held item's creation ID, or `-1` when absent. |
| `items.auxiliary_id` | int, read-only | Current auxiliary item's creation ID, or `-1` when absent. |
| `items.held_kind` | int, read-only | Current held item's kind, or `-1` when absent. |
| `items.held_motion` | int, read-only | Current held Item motion field, or `-1` when absent. |
| `items.held_frame` | float, read-only | Current held Item animation-frame field, or `0.0` when absent. |
| `items.held_use_category` | int, read-only | Current held item's common use category, or `-1` when absent. |
| `items.heavy` | Bool, read-only | Whether the current live held item's native heavy flag is set; false when absent. |
| `items.clear_held()` | Void | Clear the held slot without deleting its item. |
| `items.clear_auxiliary()` | Void | Clear the auxiliary slot without deleting its item. |
| `items.bind_held(handle)` | Bool | Store a live source or ordinary engine item originally created by this fighter in the held slot. |
| `items.bind_auxiliary(handle)` | Bool | Store that item in the auxiliary slot. |
| `articles.bind_held(handle)` / `articles.bind_auxiliary(handle)` | Bool | The same creator-gated slot operations, available under the article namespace. |
| `items.bind_existing_held(handle)` | Bool | Restore a live existing item, including a foreign or ordinary engine item. |
| `items.bind_existing_auxiliary(handle)` | Bool | Restore that item to the auxiliary slot. |
| `items.attach(handle, joint, notify_holder)` | Bool | Attach an originally created source or ordinary engine item to a raw joint of the current owned rig using the common pickup routine. |
| `items.inherit_owner_scale(handle, remember)` | Bool | Multiply the item's scale by its current damage owner's instance Y scale; optionally save the resulting throw scale. |
| `items.set_visible(handle, visible)` | Bool | Show/hide a live item's model using the common engine visibility operation. |
| `items.set_paused(handle, paused)` | Bool | Set/clear a live item's common pause flag. |
| `items.notify_drop(handle, flag)` | Bool | Run the holder's common drop notification in native order, then clear its held slot. |
| `items.notify_pickup(flag)` | Void | Run the common pickup notification for the temporary pickup slot, or the held slot, in native order. |
| `items.notify_held_neutral()` | Bool | Invoke the held source article's `holder_neutral` hook; false for an absent item or absent hook. |
| `items.reopen_blocked` | Bool, read/write | Read or set the fighter's native item reopen-block flag. |
| `items.reject_healing_pickup` | Bool, read/write | Exclude items with a nonzero native healing amount from the common pickup searches. |
| `items.throw_held(motion)` | Void | Enter the common held-item throw motion, including its speed, facing, animation and accessory setup. |
| `items.release_held(vx, vy, vz, multiplier)` | Void | Immediately release the current held item through its common drop routine. Supply world velocity and the item damage multiplier; does nothing if no item is held. |
| `items.spawn_food(x, y, z)` | Bool | Spawn ordinary engine Food with its native availability, previous-position/current-center, unowned-parent and success-counter rules. |
| `items.create_engine(kind, x, y, z, vx, vy, vz, facing, group, damage)` | int | Construct an ordinary common engine item, returning its full creation-ID word or `-1` at the native NULL boundary. |

`items.heavy` observes the calling owned Fighter's current held slot through
the native `itIsHeavy` operation. It accepts a live owned, foreign or ordinary
engine Item without imposing creator ownership. A missing or retired physical
slot reports false; a malformed live Item or missing attributes diagnoses
before publishing. The read retains the Item identity, attributes and exact
held-slot pointer. It allocates nothing and invokes no authored callback.

Created-item operations admit both same-bank source Articles and ordinary
engine items using the actual retained native creator/generation/kind. No source
specification is fabricated for engine items. These operations use original
creator identity; they do not substitute
the current damage owner or the revocable attachment owner. Restoration is an
explicitly different operation. Missing or unowned targets return false and
preserve the previous slot. A queued item can remain live during its destruction
callback; live cleanup-window admission follows the native ordering. Physical
retirement, VM failure or an identity change stops the caller before publishing
a result.

`items.reject_healing_pickup = true` changes the Fighter's pickup eligibility
flag immediately. It does not drop a held item or run a pickup callback. The
common searches consult `it_8026B47C`, which reports the healing amount for
Heart, Tomato, Food, Lucky Egg and Whispy healing apples; a zero amount remains
eligible under this flag. `items.block_ground_pickup` controls a separate flag.
The healing policy accepts canonical Bool values and can also be accessed on a
retained physical Fighter receiver, including stock and foreign source Fighters,
without requiring the caller's parameter or state layout.

`joint` is a raw index in the imported costume rig. To use a semantic part,
convert it explicitly with `bones.index(part)`. Invalid joints return false.
Attachment preserves the existing control relation; it does not reactivate
revoked attachment ownership. A pickup callback that clears that relation keeps
it cleared. `notify_holder` repeats the native debug-display inheritance step;
it is not an additional fighter pickup callback.

Scale inheritance follows the current damage owner, which can be a different
fighter after reflection. Both that owner and the item are retained through the
native scale update. It does not replace the creator or attachment relation.

The four queries inspect the current slots directly; foreign held items are
valid. A live slot with the wrong physical class or faulted script is rejected.
The ID's signed value preserves the entire native word, including its high bit.

Visibility, pause and drop notification also accept foreign or ordinary engine
items. They do not grant creator or attachment ownership. An absent handle
returns `false`; visibility additionally returns `false` for an item without a
model. A synchronous callback that faults or retires either participant stops
the caller before publishing its result. Drop notification is not an item
factory, throw command or deletion command.

Pickup notification uses `ftpickupitem_80094818`. Its temporary `x1978` slot
takes precedence and receives the ordinary pickup statistics without a fighter
pickup hook. Otherwise, a held item receives the fighter's interpreted
`item_pickup_ext` hook before statistics. With neither slot, the operation is a
healthy no-op. It does not perform item attachment or invoke `picked_up` again.
The native routine retains the caller and held item across that hook; a failure
stops its cached continuation.

Neutral notification invokes the held source item's prepared `holder_neutral`
code ID directly. It does not borrow the physical item kind's native authored
policy. Foreign source items remain valid recipients. A missing hook, absent
item, ordinary engine item or initially queued removal returns false. A hook
that changes the held slot returns false; a captured actor that faults or
retires rejects publication. Creator and attachment relationships are preserved.

Throw admission is the native common table's inclusive
`ftCo_MS_LightThrowF` through `ftCo_MS_HeavyThrowLw4` span, also checked against
the published motion layout. Arbitrary special motion rows are rejected before
entry: the common accessory routine indexes that throw table. A missing held
item is a healthy no-op. The common routine preserves native command clearing,
item damage-multiplier speed, facing selection, motion change, animation and
accessory order. Both participants must survive each synchronous continuation.
This operation does not define a custom throw recipe.

Engine creation uses the explicitly selected common item family and its engine
assets, independently of the fighter's source recipe declarations. It does not
select another fighter's behavior. Position and previous position both receive
the supplied coordinates; velocity, facing and the full 32-bit collision-group
word are copied. Initial collision remains enabled, and the engine prefunction
selects airborne creation and the native hold kind. Finite damage within the
native signed-16-bit domain is truncated toward zero when stored. Missing
engine assets or allocation/constructor cleanup produce `-1`. A native NULL
return cannot distinguish allocation failure from immediate callback cleanup.
If the caller retires after a surviving creation, the operation publishes no
handle and preserves that completed item side effect.

All Bool inputs require canonical zero or one. Collision-group words admit
signed and unsigned integral representations within the native 32-bit domain;
Float identities and wider words are rejected.

See [article references](article_references.md), [article instances](article_instances.md)
and [callback lifetime](synchronous_callbacks.md) for the related identity rules.

`items.spawn_food` is a separate native contract from engine creation. Supplied
XYZ is the previous position; current position is the caller's position plus the
ECB midpoint Y offset. Food has facing -1, zero velocity/damage/group, enabled
initial collision and no primary or secondary parent/creator. Availability uses
the enabled-items bitmap, stage policy and item frequency. Successful creation
increments the ordinary food counter. Arguments evaluate once in their written
order; caller retirement withholds the Bool and preserves completed creation and
counter effects. This does not replace Article `item.spawn_food()` dispersion.

### Immediate held-item release

`items.release_held(vx, vy, vz, multiplier)` calls the shared native drop/release
routine (`Item_8026ABD8`); it does not start an item-throw animation. Its velocity
is in world coordinates and the native item applies its own throw-speed factor.
The multiplier initializes the item damage field before its drop sound, detach
and Dropped callback run. A source article runs its `on dropped` handler; an
ordinary engine item runs its own dropped callback. The common damage ownership
and collision setup then continue as in the native routine.

All four arguments must be finite numbers. Zero and negative values are retained
without extra clamping. An empty held slot is a no-op. A dropped callback may
destroy its item; this is a normal terminal outcome, and the old item is not
accessed again. A surviving item with faulted script storage or replaced identity
rejects the operation, as does a retired/faulted calling fighter.
