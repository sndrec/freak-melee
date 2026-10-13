# Transient fighter accessories

The `accessory` namespace controls the fighter's standard transient model.
It uses that fighter's own imported asset recipes and the normal HSD rendering,
animation and motion cleanup. It is not an Article or an item and does not
acquire collision or item callbacks. See [costume attachments](model_attachments.md)
for persistent costume additions and [throw presentation](capture.md#on-throw_presentation)
for initializing a model after complete victim setup.

All methods require a live interpreted fighter and retain its source, program,
rig and accessory identity across engine operations. Selectors are Int values;
transforms and timing are finite Float values. There are no script pointers,
stock fighter IDs or native special-handler calls.

## Asset recipes

An imported recipe contains a root joint tree, an optional base animation, and
up to 16 animation variants. Base and variant entries have independent joint,
material and shape channels. Missing channels remain absent. The recipe owns
typed graphs in its fighter DAT import arena, independently of serialized
pointer offsets or host structure packing.

The existing grapple archive format normalizes its structured model at slot 4
into this schema. The importer is generic; the current archive-format decoder
still defines which slots contain recipes. A plain joint or arbitrary article
slot is not an accessory recipe. This does not yet provide a Workshop editor
for creating new recipe declarations.

## Methods

| Method | Result | Purpose |
| --- | --- | --- |
| `accessory.create(slot)` | Bool | Construct the owned recipe's root model. Returns false if a model already exists or allocation fails; it does not replace an existing model. |
| `accessory.add_animation(slot, selection)` | Void | Add the recipe's base animation (`-1`) or a zero-based variant. Channels use the normal HSD AddAnim rules. This does not request a frame or advance animation. |
| `accessory.set_scale(x, y, z)` | Void | Set the model's local scale. |
| `accessory.set_geometry_animation_rate(rate, include_looping)` | Void | Set geometry animation clocks, including joint/reference/shape clocks. Material and texture clocks remain independent. False preserves every looping clock's current rate and loop flags; true also updates looping clocks. |
| `accessory.request_frame(frame)` | Void | Request a frame on the model's animation channels without evaluating them. |
| `accessory.evaluate()` | Void | Evaluate the requested animation using ordinary HSD behavior. |
| `accessory.follow_bone(index)` | Void | Use the common follow-matrix primitive with an actual joint index in this fighter's own rig. This installs the normal relationship rather than copying a world position once. |
| `accessory.clear()` | Void | Unpublish and dispose of the model. An absent model is a no-op. |

Except for `create` and `clear`, a missing live model is an error. Invalid asset
slots, animation selections, foreign recipes or unmapped joints are errors too.
Use `create`'s return value to keep subsequent work conditional on construction.
On retirement or a script fault, later operations do not execute.

For example, a fighter can choose one of its own variants and keep the model's
material loop independent of weight-dependent throw timing:

```fms
func CreateThrowModel(int variant, float rate):
    if accessory.create(4):
        accessory.set_scale(model.instance_scale_y, model.instance_scale_y, model.instance_scale_y)
        accessory.add_animation(4, -1)
        accessory.add_animation(4, variant)
        accessory.set_geometry_animation_rate(rate, false)
        accessory.request_frame(0.0)
        accessory.evaluate()
        accessory.follow_bone(51)
```

The slot, variant and bone are this example's asset choices, not engine rules.
Animations continue through the normal fighter accessory update. A motion
change normally disposes of this model; the existing KeepAccessory motion flag
preserves it. Explicit `clear` is also available for authored cleanup. Call `follow_bone` once
for a fresh model: each call installs new reference constraints, rather than
replacing an existing relationship.

Model creation, animation loading and reference-constraint installation use
tracked HSD allocation, just as the original transient accessory path does.
Perform those setup operations at an action transition rather than every frame.
Scale, rate, frame request and evaluation use the existing HSD operations; the
scripting controls add no separate per-call storage.
