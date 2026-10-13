# Item segment sweep

In an owned Article callback:

```text
item.sweep_segment(previous_x, previous_y, previous_z, normal_x, normal_y, normal_z, line, flags, hit)
```

The three position inputs are sampled once in written order. Previous and
current Item XYZ positions must be finite. The operation calls the unchanged
native `it_8026EA20` Item collision kernel against the Item's current position;
the stage intersection itself uses XY. A hit snaps the native Item position to
the returned position, including Z.

The last six arguments require distinct writable destinations with exact types:
three `float`s, two `int`s and one `bool`. The normal, line and flags begin with
their existing destination values. A miss preserves those five values and
writes `false` to `hit`; a hit copies the native normal, line and all 32 collision
flag bits and writes `true`. The call returns no value.

The interpreter accepts its supported scalar state, local and engine property
destinations. The older C exporter restricts this operation to declared
`item.state` fields; existing source uses those fields unchanged. Input/output
flags retain their native bits through signed `int` storage.

Script output publication requires the original physical Item, scene,
generation, owned Article program and storage identities to remain valid.
Output guards do not roll back the kernel's native position snap. The operation
performs no attachment, reflection, embed or expiry decisions; those remain in
the authored callback.

The focused [native comparison fixture](../../tests/native/script_item_sweep/README.md)
executes the real Item/map wrappers over a controlled intersection leaf. Full
stage geometry and complete fighter behavior remain outside that proof.
