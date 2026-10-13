# Common horizontal smash controls

These read-only Float properties are available to a live source Fighter:

| Property | Native common data |
| --- | --- |
| `controls.horizontal_smash_threshold` | `dash_smash_stick_threshold` |
| `controls.horizontal_smash_window` | `dash_smash_window`, converted from signed Int to Float |
| `controls.horizontal_smash_extension` | `x44` |

The values preserve the existing `common_attribute` Float ABI. In particular,
the window's stored integer is converted to Float with the same native
conversion. Reads do not clamp, normalize, or alter the immutable common data.
They do not execute any fighter move or input algorithm.

```text
if abs(input.x) >= controls.horizontal_smash_threshold and input.horizontal_smash_age < controls.horizontal_smash_window + controls.horizontal_smash_extension:
    state.smash_requested = true
```

The exact selected source Fighter, program, storage and scene lifetime must be
live, and the native common-data pointer must be available. An absent rig or held
item does not prevent these observations. Reads reject assignments, Article
context, stock callers, stale generations and retired or mismatched metadata
without publishing output.

The connected fixture in `tests/native/script_held_item_visibility` retains its
held-item visibility cases and adds all three control properties through the
actual compiler, Prepare/Publish, Fighter actor, VM, storage and production
property routing. It extracts the actual `common_attribute` source as the
oracle, compares Float bits including signed zero, infinities and NaNs, and
covers signed-integer conversion boundaries for the window. Source assets and
common-data inputs are controlled; no renderer or game process launches, and
complete Link/Young Link input or move behavior is not proved here.
