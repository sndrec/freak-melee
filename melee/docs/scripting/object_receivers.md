# Object receivers

An article can inspect or call its associated Fighter through a typed object
receiver. Choose the relation that the behavior actually needs: an accessory's
owner need not be holding that accessory as an ordinary item.

| Receiver | Fighter relation |
| --- | --- |
| `creator` | Retained original constructor source. |
| `owner` | Original creator while it is still the Item's current owner. |
| `current_owner` | Item's current live native Fighter owner. |
| `holder` | Current owner only while that Fighter's held-item slot points to this Item. |
| `attachment_owner` | Retained attachment/control owner, separate from damage ownership. |

These roots are available in article callbacks. Model-effect callbacks expose
`creator`. Fighter callbacks expose `target`, the engine's most recently
selected body-contact target. It can be a Fighter or Item; it is not a nearest
opponent search. Use [target queries](targets.md) for spatial searches instead.
None of these roots depends on a stock fighter name.

## Contact identity

| Property | Type | Meaning |
| --- | --- | --- |
| `receiver.is_fighter` | Bool | The retained actor is a Fighter. |
| `receiver.is_item` | Bool | The retained actor is an Item. |
| `receiver.is_effect` | Bool | The retained actor is a model effect. |
| `receiver.kind` | Int | The actual Fighter or Item kind; effects have no kind and return zero. |

These properties are read-only. Check `exists` before reading an optional
contact. Stock items support identity checks without an interpreted program;
Fighter-only properties and authored Fighter fields still require a Fighter
receiver with the appropriate program.

```text
action Contact:
    on hurtbox_detect:
        if target.exists:
            if target.is_fighter:
                state.hit_a_fighter = true
            elif target.is_item:
                state.last_item_kind = target.kind
```

Each receiver operation retains its selected instance before accessing it.
Later changes to the native contact pointer do not redirect that operation; actor retirement,
physical slot reuse, class/kind changes, and program/schema changes invalidate
the retained token. This does not repair an already stale engine pointer before
its first capture. A later `target` expression observes the current contact
again; Object values cannot currently be stored in script locals.

## Check an optional relation

`receiver.exists -> bool`

Returns false when the relation is absent or the retained actor has retired.
Check existence before reading an optional receiver's properties. Reading a
property through an absent relation faults the callback instead of fabricating
a motion, transform or state value.

```text
if not current_owner.exists:
    return true
let int owner_motion = current_owner.motion
```

For an ordinary held item, use `holder.exists` and `holder.motion`. For an
accessory that tracks its owner's action independently of the held-item slot,
use `current_owner`. Reflection, transfer and attachment release can change
these relations; `creator` and `attachment_owner` have distinct purposes.

## Native properties and authored state

Supported Fighter engine properties use the same typed binding surface as
local Fighter callbacks. Authored fields use `receiver.state.field`; immutable
authored parameters use `receiver.attrs.field` (also spelled `receiver.params.field`). A helper call uses
`receiver.helpers.name(arguments)` and its declared return type.

Within an article, `item.params.field` selects that article's own parameters;
`owner.attrs.field` explicitly selects its Fighter owner's parameters. For
example, a flame can read `owner.attrs.breath_max_length` while its own speed
range comes from `item.params.min_speed` and `item.params.max_speed`. Use an
explicit receiver rather than a bare Fighter parameter name in article code.

Authored state, parameters and helper calls require the corresponding live
interpreted program and declarations. They are not arbitrary memory access to
a stock Fighter or another package's fields. Receiver operations retain and
revalidate physical identity and source/program ownership before publishing
results or writes. See [functions](functions.md) for recipient helper calls and
[article references](article_references.md) for observing another owned article.
