# Fighter pose graph contract proposal

Status: source-only design against the live checkout. No engine, compiler,
template, fixture extraction, build or gameplay changes are included here.
`source_manifest.json` pins the reviewed inputs and actual function signatures.
The existing Sheik `pose_contract.md` supplies the ordered move policy; this
proposal supplies its reusable engine boundary. It does not replace that policy
with a native Chain helper.

## Owned inputs and an important distinction

The current stock Sheik layout declares article slots 0–3 and joint graph slots
4–5 (`article_mask=0x00f`, `joint_graph_mask=0x030`). The character importer already
decodes each graph with `DAT_JOINT` into the character arena; `fighter_items[slot]`
is the legacy view of that allocation. These graphs are immutable descriptors,
not live JObjs, articles, animation clips or newly inserted runtime parts.

The existing ISO receipt reports 58 physical/runtime parts, 52 semantic bindings
and **zero omission records** for Sheik. Stock `FtPart_TransN` in the Chain calls
is the direct array start 1, which also happens to match Sheik's semantic TransN
mapping. It is not an omission index. Do not add an omission to make the new API
work. `ftParts_8007506C` still matters for other rigs: it returns an omission
mask for a runtime part, and the shared pose functions skip such parts while
consuming ordinary descriptor nodes. The descriptor ordinal and runtime part
index are therefore different coordinate systems.

`model_graphs.install(slot, omission)` is already supported, but serves a
different contract: it creates one reserved runtime joint and its alternate,
attaches them according to an omission recipe and publishes DObjs. It must not
be used to load or blend the Chain's whole immutable pose graph.

The current rig receipt traversed the Nr costume; a future integration must
validate all five actual costumes and both graphs. This proposal does not
upgrade that earlier scope to all-costume acceptance.

## Actual shared semantics

| Function | Contract that must survive direct binding |
|---|---|
| `ftAnim_8006F4C8(Fighter*, bool, FigaTree*)` | Install tracks into live (`false`) or alternate (`true`) joints. Consumes clip nodes/tracks, `x594_bits`, omission masks, and parts flags b0/b1/b2/b5. Does not select the current motion, write `x590`, request a frame, evaluate, or clear unrelated animations. Track installation may replace/allocate AObjs through `lbAnim_8001E6D8`; preserve the original call order. |
| `ftAnim_80070710(HSD_JObj*, float)` | Request the given frame recursively with animation flags 1. Evaluation is separate. |
| `ftAnim_8006FA58(Fighter*, Fighter_Part, HSD_Joint*)` | Copy descriptor pose into live joints from a direct runtime start. Traverses descriptors and skips omission parts. Handles b0/b5 and the special compensation part `ft_data->x8->x10`; that part also receives inverse model scaling via `ftCommon_8007F6A4`. |
| `ftAnim_8006FB88(Fighter*, Fighter_Part, HSD_Joint*)` | Alternate-skeleton counterpart of the previous copy, including b0/b5 and compensation. Certain b0 paths explicitly clear quaternion mode. |
| `ftAnim_80070108(Fighter*, Fighter_Part, float, float, HSD_Joint*)` | Blend descriptor pose into the alternate skeleton; b0/b5 skip, b4 forces descriptor copy, otherwise `lb_8000C868`. First weight multiplies descriptor pose; second multiplies the existing alternate pose. |
| `ftAnim_80070010(Fighter*, Fighter_Part, float, float, HSD_Joint*)` | Same descriptor blend into the live skeleton, with the same flags and weight ordering. |
| `ftAnim_8006FE9C(Fighter*, Fighter_Part, float, float)` | Iterate from direct start through the active runtime part count. For b1 and neither b0 nor b5, blend alternate into live with `lb_8000C490`, or copy if b4. First weight belongs to alternate, second to existing live. |
| `ftAnim_8006FF74(Fighter*, Fighter_Part)` | Copy alternate into live over that runtime range for b1 and neither b0 nor b5. |

The two supplied weights are independent authored Float expressions. Do not
normalize them, clamp them, infer the complement or collapse a full-weight
blend into a copy: the actual copy and blend paths differ in flags, scale and
quaternion handling. The scripts keep ordinary frame and weight arithmetic.

## Minimal proposed script surface

The following syntax is **pending**, not an available API:

```text
pose_graph GroundRest slot 4 path "0"
pose_graph AirRest slot 5 path "0"

pose.load_animation(Pose.blend, animation_id)
pose.request_frame(Pose.blend, frame)
pose.copy_costume(Pose.blend, raw_start)
pose.evaluate(Pose.blend)
pose.blend_graph(Pose.blend, raw_start, GroundRest, descriptor_weight, pose_weight)
pose.blend_to_live(raw_start, blend_weight, live_weight)
pose.copy_to_live(raw_start)
pose.blend_graph(Pose.live, raw_start, GroundRest, descriptor_weight, pose_weight)
pose.copy_graph(Pose.live, raw_start, GroundRest)
```

`Pose.live`/`Pose.blend` are compile-time destination constants, not user-visible
pointers or handles. Select the actual whole live root `GET_JOBJ(gobj)` or
alternate root `fighter->x8AC_animSkeleton` for request/evaluate. A direct part
start is an Int expression, checked against the active runtime part count.
No semantic bone remapping is silently applied. Graph names are immutable
definition metadata selecting an owned declared slot and child path; `"0"`
selects its first child. They do not allocate, install, or mutate a graph.

For copying costume pose, resolve the descriptor corresponding to the chosen
start using actual `ftAnim_8006F994` and the live/alternate joint for that start,
then call the chosen real copy function. Do not universally use
`costume_root->child`; that is only the verified Sheik start-1 case. Arbitrary
imported graph names already identify the intended descriptor subtree and are
not resolved through the costume traversal.

The existing `bones.copy_costume_pose(start)` remains a real supported live
operation. It already uses `find_costume_pose` and `copy_bone_pose`. No renamed
compatibility entry or forwarding wrapper is needed; a new destination-aware
operation covers alternate pose work and direct graph selection.

## Exact integration requirements

Reuse these existing actual Host bindings:

- `animation_clip(Fighter*, int)` → `ftData_80085E50`.
- `find_costume_pose(Fighter*, HSD_JObj*, HSD_Joint*)` → `ftAnim_8006F994`.
- `copy_bone_pose(Fighter*, Fighter_Part, HSD_Joint*)` → `ftAnim_8006FA58`.
- `model_advance_animation(HSD_JObj*)` → `HSD_JObjAnimAll`.
- `bone_count(Fighter*)` for the active runtime bound.

Add only direct bindings for the remaining actual operations:

```c
void (*load_pose_animation)(Fighter*, bool, FigaTree*);      // 6F4C8
void (*request_pose_frame)(HSD_JObj*, float);               // 70710
void (*copy_blend_pose)(Fighter*, Fighter_Part, HSD_Joint*); // 6FB88
void (*blend_graph_live)(Fighter*, Fighter_Part, float, float, HSD_Joint*); // 70010
void (*blend_graph_alternate)(Fighter*, Fighter_Part, float, float, HSD_Joint*); // 70108
void (*blend_pose_to_live)(Fighter*, Fighter_Part, float, float); // 6FE9C
void (*copy_pose_to_live)(Fighter*, Fighter_Part);           // 6FF74
```

One substantive runtime asset resolver is also required, for example:
`HSD_Joint* pose_graph(Fighter_GObj*, int32_t named_graph_index)`. It resolves
the active definition's immutable graph recipe against its own runtime slot and
imported character arena, not a donor kind or an arbitrary `void*`. A typed
`FtCharacterAssets` graph view can alias the **same** imported allocation; no
second decode or graph copy. Validate declaration count/slot/path, declared
joint mask, no overlap with article/visibility masks, active package/ft_data,
non-NULL selected node and bounded traversal. Missing asset/query failure means
no operation and no partial pose mutation. Generated C may retain the resolved
pointer only for that synchronous operation; scripts never store it.

Before entering an actual shared traversal, validate the complete target range
and source descriptor/clip topology against active parts, omission masks and
costume mapping, including live/alternate JObjs required by that operation.
Part-start bounds alone do not make the existing unbounded inner skip loops
safe. Preserve the shared algorithm rather than interpreting its math in a
new native policy helper. Any real malformed-input hardening belongs in those
actual shared functions/callers with explicit failure semantics.

Clip/frame/weights must be finite and clip ID must refer to the own animation
bank. Do not substitute `change_motion_state`/`start_animation`: they change
current motion bookkeeping and perform unrelated animation setup. Pose loading
specifically leaves those fields alone.

`pose.evaluate` uses the existing actual HSD evaluation call. It drains AObj end
callbacks; its generated operation requires the same full generation guard as
other synchronous callback-bearing operations **before subsequent pose access**.
Do not hold the resolved graph, Fighter or JObj pointer across that guard.
Load/copy/blend paths still need an audited callback-effect classification;
their absence from today's lifetime list is not evidence of safety. Integration
must add exact required-Host checks, ABI fingerprinting, fighter-only context
validation and correctly typed Int/Float arguments. No article/model-effect
context should accidentally receive a `fighter` dereference.

## Sheik policy demonstration and remaining boundary

For nonzero smoothed magnitude, preserve this precise order: load selected
305/308 clip into alternate → request computed frame → copy costume pose into
alternate → evaluate alternate → conditional selected-graph blend into alternate
→ conditional alternate/live blend or alternate/live copy. With zero magnitude,
only conditional selected-graph/live blend or selected-graph/live copy occurs.
The script selects slot 4/5, updates smoothing, computes
`0.0556 * angle + 4`, chooses all branches and supplies the two weights.

No native helper should implement `ftSk_SpecialS_80110610` as a delegated move
handler. Its parameter `arg2` is an explicit phase weight; its threshold choices
and zero/nonzero magnitude branches belong to owned script state and parameters.

Future source acceptance should compare the eight actual shared operations,
descriptor resolution and ordered generated policy with healthy and malformed
topologies, all relevant flags, omission masks, compensation part, live versus
alternate destinations, clip track alignment, frame boundaries, quaternion
paths and callback invalidation. Compare all five real Sheik costumes and both
imported pose graphs. These are planned checks, not fixtures implemented here.

Pose tooling does not close the separate Chain capsule current/previous-position
contract, resource solver, linked rendering or full independent Sheik activation.
The accepted Sheik fixture currently proves Vanish; it does not prove these new
pose operations, Chain visuals or the complete move.
