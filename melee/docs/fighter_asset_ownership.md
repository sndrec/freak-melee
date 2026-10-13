# Fighter asset ownership

A cloned fighter copies its DAT files, animation bank and costume. Its native
definition also declares how those copied assets are decoded. Asset layout is
data; it does not select the stock fighter's special-move handlers.

## Declaring copied assets

`NativeFighterDefinition.asset_layout` contains article-slot and joint-graph
masks, the serialized item-data schema, and any raw byte ranges embedded among
the DAT's big-endian scalar words. For example, Yoshi's twelve enclosure-frame
bytes remain bytes rather than being reordered as three integer words.

`article_assets` maps each article kind to a slot in this fighter's own imported
DAT. A binding may read the kind from a decoded attribute word when the original
archive stores that choice there. Article creation and additional article model
imports resolve these bindings directly. They do not look up the source fighter
kind. Several families can share a script module while each package declares
only its own article assets.

The decoder rejects overlapping article/graph slots and conflicts with slots
reserved by typed item schemas. Runtime loading rejects duplicate asset slots,
duplicate resolved article kinds, invalid attribute offsets and missing bound
assets. Existing stock archives declare the same layouts explicitly at their
import callsites.

## Skeleton and effects

`bones` maps semantic parts such as `HipN` and `TransN` to this rig's actual
joint indices. Explicit mappings take precedence over inherited stock maps.
Each semantic role is declared once; several roles may share one custom joint.
The final declared role supplies that joint's inverse lookup. Rook deliberately
shares her ThrowN joint with the auxiliary TransN2 role.
`omissions` describes runtime parts inserted separately from the costume and
figatree; independent definitions can provide those records directly.

Mewtwo declares all 44 valid semantic pairs for its 68-part rig. Yoshi declares
39 valid pairs for its 70-part rig. Both maps were read from the supplied v1.02
`PlCo.dat`, including the two auxiliary translation parts. Neither has omitted
parts. Rook retains her existing authored rig map.

`effect_bank` selects immutable effect assets; `0xFF` means no character-specific
effect archive. The clone owns this choice separately from callback inheritance.

`demo_motion_names` owns the four result, intro, ending and wait animation
archive roots. All twenty stock clone bases copy these literals from their
original asset declarations. Registration binds a per-package copy after
clearing stock callbacks, so match-end animation loading does not depend on
the original fighter's callback family.

## Independence boundary

`source_kind` still identifies inherited gameplay callbacks for partial ports.
Copying the asset metadata does **not** make those partial fighters fully
scripted. A fighter switches to `Ft_Kind_None` only after all required
fighter-specific behavior is authored and compared against stock behavior.

The generated module may declare independent behavior while its raw catalog
base retains stock fallback for untouched drafts. Package validation still
requires the matching asset base and code key. A compiled independent module
must provide its bone mapping; the callback ownership guard rejects any
untranslated nonempty animation, input, physics or collision slot in that
base's extended motion table.

The authoring catalog separately records which stock template began the draft.
That provenance can remain useful for importing a new draft after a finished
template no longer inherits stock gameplay. Animation-family metadata likewise
remains separate from gameplay callback selection.

Native modules are rebuilt against the current definition and host layouts;
the derived layout fingerprint rejects mismatched binaries. There is no older
asset-format migration layer.
