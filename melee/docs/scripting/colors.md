# Colors and model presentation

These controls work for any fighter or authored article. A color is an `int`
containing four bytes in RGBA order. It is never converted through a float.
See [fighter presentation](presentation.md), [article models](article_models.md)
and [target queries](targets.md) for related operations.

## `color.clear_attack()`

Clear the Fighter's attack-color overlay through the shared native color
primitive. It releases that overlay's command pointer and active-color flags
and resets its priority. It does not clear the regular or special overlay,
reset timers, change costume materials, or remove effects.

This takes no arguments and returns Void. It is restricted to the executing
source-owned Fighter. The selected actor, program, storage, rig and imported
model remain retained across the native operation; invalidation withholds the
result without undoing already completed changes.

## `color.rgba(red, green, blue, alpha) -> int`

All four arguments must be integers. They are evaluated once, from left to
right. Each component uses its low eight bits: 256 becomes 0 and -1 becomes
255. Alpha 255 is opaque. A packed color whose high bit is set appears as a
negative signed integer; its bytes remain intact.

```text
visuals.outline_color = color.rgba(255, 80, 20, 255)
visuals.set_diffuse_color(visuals.outline_color)
```

The packer is available in fighter and article expressions, including pure
model-effect calculations. Presentation mutations require the owning fighter
or article callback; an effect callback cannot modify fighter presentation.

## Fighter controls

| Member | Type | Behavior |
| --- | --- | --- |
| `costume.index` | `int`, read-only | Current native costume index. |
| `model.base_scale` | number, read-only | Fighter's common model-scaling attribute. |
| `model.instance_scale_y` | number, read-only | Per-instance Y scale. |
| `model.scale` | number, read-only | Instance Y scale multiplied by the common model-scaling attribute. |
| `model.effect_scale` | number, read-only | Common effect-size attribute, independent of model and rendered-root scale. |
| `visuals.outline_color` | `int` | Read/write the fighter's secondary-pass outline color. |
| `visuals.set_diffuse_color(color)` | statement | Set diffuse RGBA on the fighter's current costume material tree through the shared engine operation. |
| `visuals.preserve_secondary_material` | `bool` | Preserve the original material contribution in the secondary render pass. |
| `model.instance_scale_z` | number | Read/write the per-instance scale component used by Melee's root-width calculation. |

The instance scale is distinct from a joint's local scale and from the
fighter's gameplay attributes. The native engine retains Melee's field name:
this Z component contributes to root X width. Nonfinite and zero writes are
ignored; negative finite values are permitted. Writing it does not immediately
recalculate the model transform. Diffuse color changes traverse existing
materials; they do not replace textures or create materials, and the shared
operation replaces the diffuse RGBA value, including its alpha.

## Owned outline visibility

Declare an immutable lookup slot before any code blocks:

```text
visibility_asset 10
action CostumePresentation:
    on load:
        let bool installed = visuals.install_outline_visibility(10)
```

Slots are 0 through 15 and cannot be declared twice. A visibility lookup indexes
costume draw objects, whereas article model layers index joints; they are
different asset types. A slot cannot also be declared as an article or a joint
graph. Registration validates each declared lookup against all imported
costumes. Installation rechecks the live costume and returns `false` for
invalid or absent data.

`visuals.install_outline_visibility(slot) -> bool` installs the owned lookup as
the fighter's secondary visibility table. The current costume parts must already
be initialized. Use `on load` for initial appearance; another callback may also
install a valid lookup on the same live owned costume. Retired/recycled fighters,
stale owned asset layouts and invalid draw-object ordinals cannot install borrowed
data. This operation allocates nothing.

## Controller colors

`targets.controller_color_present(target) -> bool` reports whether the
live target has a valid controller color. `targets.controller_color(target)
-> int` returns the packed color, or transparent black (`0`) when unavailable.
Both direct interpreter operations accept an integral creation ID or a captured
Fighter Object such as `current_owner`. `item.current_owner` supplies a U32
creation ID. All 32 handle bits are preserved; floating-point handles are rejected.
The Object route validates the retained physical generation and pinned scene,
without looking up the item's owner again. Missing targets return false/zero;
stale Object tokens or a retired caller fail without publishing a result.
Controller indices 1-4 select the native common player-color table; other indices
or absent common data have no color. These pure queries work in fighter and
article callbacks and do not change either model. They do not require the target
to own an interpreted program or share the caller's fighter kind.

## Article controls

| Member | Type | Behavior |
| --- | --- | --- |
| `item.outline_color` | `int` | Read/write the article's outline color. |
| `item.controller_tint` | `int` | Read/write its secondary controller tint. |
| `item.controller_tint_enabled` | `bool` | Enable the controller-tint contribution. |
| `item.material_opacity` | `int` | Read/write the byte used by the article material pass; writes outside 0..255 are ignored. |
| `item.layered` | `bool` | Enable/disable validated model-layer rendering from this article's own immutable asset. |
| `item.set_diffuse_color(color)` | statement | Apply diffuse RGBA to this article's current material tree. |

Enabling layered rendering validates the current primary model, parent joints
and dynamic-bone ordinals before binding the article's own imported lists.
Missing or malformed metadata leaves layering disabled. Disabling clears the
binding. Replacing the primary model clears both the binding and enabled flag;
additional model slots do not replace the primary model. Layer indices retain
source order and repetitions, because render pass ordering matters.
