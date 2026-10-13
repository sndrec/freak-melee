# Ness YoYo four-physics source checkpoint

`candidate.fms` is an **unregistered solver candidate**. It owns all four YoYo
physics callbacks and the previously accepted six-argument node initialization.
It is not installed in `examples/ness_wip.fms`, a catalog, or a Workshop default.
The accepted initialization/resource/Thunder namespaces are unchanged.

Run from the checkout with the matching frozen SDK and tool:

```powershell
python tests/native/ness_yoyo_solvers/run.py --snapshot .local/baseline34 --output .local/ness-yoyo-solvers34-review
```

This generates C with the snapshot tool and compiles the actual generated module
directly against that snapshot SDK with its Clang and `vcvars64 -vcvars_ver=14.44`.
It does not call `script-build` (which embeds live include paths), CMake, or a game.
Output must be outside the source namespace and immutable snapshot.

The frozen34 result is **5,075,363 checks, zero failures, 2,556 source pairs and
six defined lifetime/default-owner cases**. The per-phase pair counts are
585 / 723 / 657 / 591. Maximum observed absolute numeric difference is zero.
Float checks still use the explicit `2e-6 * max(1, abs(a), abs(b))` bound; this is
source behavioral evidence, not an assertion about all floating-point bit patterns.
The final receipt records all source, generated C and frozen input hashes.

## Authored bodies and field ownership

| Original callback/body | Authored script |
| --- | --- |
| `itNessyoyo_UnkMotion0_Phys`, `it_802BF900` | `IdlePose`: attachment matrix, depth and current-owner scale |
| `itNessyoyo_UnkMotion1_Phys`, `it_802BF28C` | `HeldChain` then `ChainPose` |
| `itNessyoyo_UnkMotion2_Phys`, `it_802BF4A0` and its tail adjustment | `AccelerateTip`, `ReleasedChain`, `AdjustTail`, then `ChainPose` |
| `itNessyoyo_UnkMotion3_Phys`, `it_802BF800`, `it_802BF180` | `RetractChain`, optional `RestoreHeld`, then `ChainPose` |
| `it_802BF030`, `it_802A3D90`, `it_802A42F4` | explicit history preparation, stage sweep, touching flags and history acceptance |
| `it_802BFAFC`, `it_802A6DC8` | `ChainPose`, `OrientBasis`, `WritePose` |
| `it_802C0010` | `RestoreHeld`: anchor position/history, pin, zero velocity, common state change, saved-hitbox position |

The linked-list head is resource index 0, the tip/tail is `link_count - 1`;
`next` becomes index - 1 and `prev` becomes index + 1. Node position, velocity,
collision history and model matrices use the generic resource API. Pin/touch/
contact flags are ordinary owned boolean arrays. `SetPinned` updates the node's
draw flag too, because the original `x2C_b0` controls both pinning and drawing.
Source motion and retraction step use ordinary owned fields. The original saved
fighter hitbox position is three named fighter floats; the creator is distinct
from the current damage owner.

There are 24 owned parameter values: 14 named fighter constants and 10 article
values. Article `attribute` declarations are import metadata; gameplay reads
`item.params` and never the donor DAT or fighter/item private unions. The candidate
uses 23 article fields / 392 item-state bytes and 23 directly compiled helpers.
Initialization uses `create(part, radius, category, initial_x, initial_y, initial_z)`.
The original raw attachment part 42 remains explicit.

The one missing frozen32 contract was current damage-owner instance Y scale.
Frozen34 `holder.instance_scale_y` resolves the live full-generation current owner
through `item_current_owner` and `target_instance_scale_y`, distinct from creator
scale and root-joint scale. Changed-owner cases use scale 2.25 while the creator
uses 0.5 or 1.75. No solver-specific native operation was introduced.

## Comparison boundaries and ordering

The runner re-extracts unchanged original callbacks/helpers from six hash-pinned
snapshot files. Both families run the same unchanged Dolphin C matrix/cross-product
bodies and `lbVector_Normalize`. Because SDK snapshots contain Dolphin headers but
not these three implementation files, `reference_math.h` retains only their
mechanically extracted bodies; `math_original_inputs.json` records the full source
file hashes, function names and fragment hash. No copied solver is handwritten.

The generated side uses the actual snapshot `native_article_resource.c` and
`native_model_joint.c`. HSD allocation/model construction/animation/rendering,
active Definition and UID registration, common item state transitions, and the
stage sweep oracle are explicit fixed-storage engine boundaries. The stage oracle
applies the same controlled floor/wall corrections to both families and records
every incoming current/previous position and node index. This tests actual resource
dispatch and solver ordering; it does not claim the oracle is actual stage geometry.

The matrix includes 2/8/90-node chains, both facings, up/down source motions,
different pin layouts, velocities below/equal/above limits, edited charge counts
and lengths, floor/wall/both/no-contact masks, changed owner, pure vertical and zero
directions, and 18 twelve-frame continuations. Assertions cover positions,
velocities, history, environment, pin/draw/touch/contact flags, world matrices,
model translations/flags, saved creator hitbox, common state transitions and every
ordered sweep. Branch gates require both partial/completed retraction and released
early/completed processing, floor and wall paths.

In particular, the repeated **current-node** sweep in released motion and the
double history update before its height-based sweep are retained. Retraction
resets the tip's collision history to the attachment before publishing the saved
hitbox position. Both unused return codes from the released helper stop the same
authored helper continuation; the callback still renders afterward. Pose matrix
assembly/scaling is committed through one generic matrix write: final matrices
and dirty flags are compared, not incidental counts of writes to a dirty joint.

Two authored-only removal cases destroy the item during the common state change
and verify no later saved-hitbox/model write. Four missing/dead/poisoned-owner cases
verify the generic scale default of 1 without dereferencing invalid userdata.
Original C dereferences these invalid owners, so those are defined authored
boundaries, not original-C pairs. The pose also stops if no node is pinned;
original C would walk past the list. Valid original states keep a pinned tip.

## Remaining activation boundary

The four physics solvers have source acceptance. The article animation callback,
release/retraction entry calculations, smash fighter phases/solver selection,
collision damage/rehit ordering, bat/YoYo common presentation and complete Ness
lifecycle composition still need their coherent source gate. This isolated module
has no animation policy or fighter smash hooks. Actual DAT model import, rendering
and drawn gameplay remain untested here. Do not publish these article logics beside
stock smash handlers that still mutate private union fields.
