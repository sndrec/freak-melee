# Costume model attachments

Attachments are extra costume joint trees bound to a fighter bone: hats,
accessories, or other models that should follow the fighter without becoming
an item. Scripts choose their assets, binding, and optional cloth chains. The
engine handles drawing and standard fighter material/visibility passes.
There is no call to a stock fighter's costume handler.

Loaded attachments draw automatically in both common fighter render paths, after
the body and using its view/material pass. Common part-visibility changes update
their visibility groups too; fighter invisibility and accessory control flags
still apply. Scripts do not need a rendering callback. See the
[render checkpoint](../native_script_attachment_render_checkpoint.md) for the
five-costume ordinary-interpreter comparisons and their limits.

See [fighter callbacks](fighter.md), [owned articles](article.md), and
[combat/presentation operations](combat_visuals.md). Articles have item state,
collision, and callbacks; a costume attachment has none of those automatically.
For a transient model disposed of by ordinary motion cleanup, see
[fighter accessories](fighter_accessory.md).

## Properties

| Property | Type | Meaning |
| --- | --- | --- |
| `costume.index` | int | Current costume slot. Independent custom fighters currently have one base slot; an installed skin can replace that slot's asset graph. |
| `costume.variant` | int | Model-table variant owned by the selected skin. Stock costumes use their normal costume number; a custom skin declares this value independently of its asset slot. |
| `items.heavy` | bool | Whether the fighter's held item is heavy. False if there is no held item. |
| `items.hold_kind` | int | Held item's engine hold category; 0 if there is no held item. The stock item data uses categories 1–4. |
| [`items.held`](fighter.md) | bool | Whether the fighter currently holds an item. |

These are read-only properties. Use ordinary [`state`](language.md#fighter-state)
for changing fighter variables and [owned parameters](language.md#fighter-attributes)
for tuning values.

Use `costume.variant` when a skin's hats, accessories or saved visibility rows
depend on its model variant. The engine also selects the corresponding
visibility and texture-animation rows from the fighter's own DAT. A variant
outside a particular table uses row zero; it does not change the asset slot.
Use `attachments.has_root` when skins differ by named accessory graphs rather
than table variants.

The content tool accepts a variant when importing a skin:

```powershell
native_content_tool import-custom-costume costume.dat FIGHTER_ID "Blue costume" 2
```

Omitting the final argument uses zero. Valid values are 0–255. This is immutable
skin metadata and participates in the package content ID; it is not mutable
fighter state or a reference to another fighter's implementation.

## Methods

The `attachments` methods, costume/item properties and part-animation methods
are available in ordinary interpreted packages. Attachment allocation and cloth
setup are accepted only while the fighter's initial `on load` callback runs,
including local helpers called from it. Reinvoking a load hook later does not
reopen that allocation scope. Root queries and cleanup do not allocate new
models. See the [attachment checkpoint](../native_script_attachment_checkpoint.md)
for the connected test boundaries.
See the [hold-category checkpoint](../native_script_hold_category_checkpoint.md)
and [part-animation checkpoint](../native_script_part_animation_checkpoint.md).

### `attachments.has_root(root)`

Returns **bool**. Looks up a public joint symbol in the selected costume's DAT,
including a selected custom skin. It does not import or allocate a model.
`root` is a quoted literal asset symbol, for example `"Accessory_TopN_joint"`.

Use root presence when different skins have different accessory graphs. A
custom fighter's costume slot number alone does not identify a skin's contents.

### `attachments.load(slot, root, bone, visibility_entry, visibility_costume)`

Creates a joint tree in one of four fixed attachment slots, **0–3**. Call from
`on load`, directly or through a helper. Creation uses the normal HSD game heap
at the scene transition; it does not allocate on each rendered frame.

- `root`: public joint symbol in the selected costume DAT, as a quoted literal.
- `bone`: actual index in the fighter's part/joint table. It is checked against
  that table's size; it is not an offset into an old fighter-specific union.
- `visibility_entry`: an imported accessory parts descriptor in the fighter
  asset table. **-1** uses the model's existing visibility without such a table.
- `visibility_costume`: row of that descriptor's costume visibility table.
  This asset row can differ from `costume.index` on an independent fighter.

The script statement has no return value. An occupied slot, missing root,
invalid bone/visibility row, or failed graph import does not create a model.
Check root presence first and verify the intended accessory in preview. The
main fighter's material classes, render-pass ordering, and subaction accessory
visibility flag apply to these trees as they do to the stock costume accessory.

### `attachments.add_dynamics(slot, first_descriptor, count)`

Adds cloth/dynamic chains from the fighter DAT to the loaded attachment. Call
from `on load` after loading the graph. The descriptors' bone indices refer to
the attachment tree. They append to the fighter's existing body dynamic sets;
the engine keeps their animation ownership separate from body-only channels.

Each attachment can receive one contiguous descriptor range. The combined
body and attachment sets must stay below the existing **10-set** engine limit.
Missing joints, empty chains, invalid ranges, or insufficient capacity leave
the attachment unchanged. The script statement has no return value.

### `attachments.clear_all()`

Releases every authored attachment and its dynamic chains. Use in
`on user_data_remove` when authoring lifecycle cleanup. The engine also performs
this release during fighter destruction, so omitted custom cleanup cannot
leak attachment trees. Repeated cleanup is harmless. Do not clear accessories
on every death unless the fighter also recreates them before respawning.

### `visuals.set_part_animation(group, animation)`

This operation, `visuals.show_part_animation`, `visuals.hide_part_animation`, and
the immediate `visuals.apply_part_animation` operation are available in
interpreted packages.

Selects the saved animation for one of the fighter's **five** part-animation
groups. This only updates the selection; it does not immediately apply the pose.
These groups come from the fighter's asset data, distinct from attachment slots.

### `visuals.show_part_animation(group)`

Applies the group's saved animation at the standard fixed-pose rate of zero,
if a selection exists. This is the shared operation used to show the held-item
hand pose. It is different from [`visuals.apply_part_animation`](fighter.md#visualsapply_part_animationgroup-variant-duration),
which chooses and immediately applies an animation with a supplied blend duration.

### `visuals.hide_part_animation(group)`

Removes that part-animation layer and restores the current main animation's
pose for its bones (or their costume rest pose if no main animation is active).
It preserves the saved selection so `show_part_animation` can reapply it later.

## Example

```text
func load_accessory():
    if attachments.has_root("Accessory_TopN_joint"):
        attachments.load(0, "Accessory_TopN_joint", 5, -1, 0)

action FighterEvents:
    on load:
        load_accessory()
    on user_data_remove:
        attachments.clear_all()
```

Asset symbols in the example are placeholders; choose actual roots from the costume.
Jigglypuff's [template](../../examples/purin_wip.fms) shows four accessory roots,
visibility rows, and two cloth configurations using this same API.

## State and verification

Attachment slots, visibility controls, HSD model instances, and dynamic nodes
belong to the fighter/game heap restored by rollback. Desync diagnostics include
their controls, pose, and materials. Stock-versus-script traces compare the real
attachment joint trees despite their different stock/authored storage locations.

The [costume comparison](../../tests/native/purin_costume_parity.py) requires a
rendered attachment for every non-default costume and the recorded Rollout hit.
See the [translation ledger](../jigglypuff_script_translation.md) for evidence
and remaining branch coverage.
