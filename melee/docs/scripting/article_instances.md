# Article instance scale and stage contacts

These members are available in item and article callbacks. They operate on the
current instance; authored parameters remain ordinary editable package values.

## Properties

| Property | Type | Description |
| --- | --- | --- |
| `item.scale` | Float, read only | Current engine instance scale. |
| `item.throw_scale` | Float, read only | Scale remembered for throwing, including scale inherited when the item was picked up. |
| `holder.instance_scale_y` | Float, read only | Instance Y scale of the live current damage owner, or 1 when none is live. This follows pickup/reflection ownership; it is separate from the original creator and the root joint scale. |

## Methods

### `item.set_instance_scale(scale)`

Returns nothing. Accepts a numeric scale and requires the item's loaded model
root. Updates the engine instance scale and root XYZ scale, then runs the
original geometry refresh sequence: hitbox/range setup, grab range scaling,
and collision extent scaling. It does not change the remembered throw scale.

Use this to restore an item's engine scale when it is thrown:

```text
on thrown:
    item.set_instance_scale(item.throw_scale)
```

`item.set_scale(scale)` changes the rendered model only. Choose the instance
operation when gameplay geometry must follow the engine's scale refresh.
The refresh preserves the existing engine's multiplication/order semantics;
it does not replace those mechanics with a new scaling solver.

### `item.respond_stage_mask(contact_mask)`

Returns nothing. Accepts an Int stage-contact result from
`item.sweep_stage_flags()`. Preserves all integer bits and invokes the normal
stage response, including collision-position correction and applicable sound,
effect and motion response. The script decides whether to change state or
remove the item afterward.

```text
let int contacts = item.sweep_stage_flags()
if contacts & item.params.surface_response_mask:
    item.respond_stage_mask(contacts)
    item.change_state(1)
elif contacts & item.params.retire_contact_mask:
    return true
return false
```

This contact result has its own low-bit encoding. It is separate from the
collision environment's `Collision.floor_mask` and `Collision.wall_mask`.
Keep the result in an Int rather than converting it to a Float or Bool.
Both methods reject wrong argument types and extra/missing arguments at build
time. `item.throw_scale` is not assignable.
