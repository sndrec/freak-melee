# Requested held-item visibility

`visuals.held_item_visible` is a read-only Bool property in Fighter context.
It reads the native Fighter `x221E_b3` requested-visibility flag, including when
the Fighter has no held item. It does not inspect the model, attachment, or
rendered meshes. An absent rig does not prevent reading it.

The ordinary shared `ftCommon_8007F5CC` helper updates this flag after its
visibility callbacks. Reading the flag runs no callback and changes no native
state. A source cannot assign the property; existing held-item controls retain
their own operation contracts.

```text
if visuals.held_item_visible and state.bow_visible_timer == 0:
    visuals.set_group(2, 0)
```

The interpreter requires the caller's exact live source Fighter, selected
source package, prepared program, storage and scene lifetime. Stock callers,
wrong contexts, stale generations, retired actors and invalid owned metadata
reject without publishing a result. Admission does not evaluate a model graph.

The focused fixture in `tests/native/script_held_item_visibility` executes the
actual compiler, Prepare/Publish, Fighter actor, VM, storage, property routing,
callback lifetime and fixed GObj pools over controlled source assets. It pins
the exporter's exact field mapping, compares against the extracted actual
common visibility helper on its callback-free branches, and checks complete
Fighter records remain unchanged. Native callback branches, rendered meshes,
retail DAT assets and full Link/Young Link behavior are outside this proof.
