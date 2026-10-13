# Immutable fighter visibility assets

Fighter visibility assets are DObj variant lookup tables, independently selected
by `FtCharacterAssetLayout.visibility_mask`. Its16 bits address the existing
character asset slots. Visibility, article, and joint-graph masks must be
disjoint. Format-owned non-article slots remain reserved; the PaperArticles
format's slot10 is now imported by the same generic visibility decoder as any
other declared slot.

The importer publishes each lookup in
`FtCharacterAssets.visibility_assets[slot]`. Each table has exactly the active
model group's imported row count, at most11 groups. Each group has0..256
variants; each variant has0..256 byte DObj ordinals. Empty selections and repeated
ordinals are valid. The legacy `fighter_items` slot aliases the same allocation
for actual stock callers; it is not independently decoded or allocated.
Owned runtime installation never reads or casts `void**` assets.

Package preparation validates every declared lookup against its actual imported
costume descriptor graph. Particle/spline joints do not contain DObj lists, and
instance joints do not contribute a second traversal of their child graph.
Every referenced ordinal must exist. No bone-name mapping or article layer
metadata is involved. Immutable storage follows the imported character arena
and package lifetime.

`nativeFighterRuntime_InstallOutlineVisibility(fighter, asset_slot)` returns Bool
and publishes the lookup into fighter visibility table4. It succeeds only
inside that fighter's actual load callback. The actual `Fighter_Create` callsite
uses balanced `EnterLoad`/`LeaveLoad` markers; these operations never invoke or
wrap a callback. They retain the source's full generation, kind, Definition,
runtime slot and `ft_data` identity. A removed/recycled source, changed kind or
Definition, retired/rebound slot, foreign source, or undeclared asset fails.
Leaving a callback never dereferences the potentially retired source.

Installation additionally checks the live fighter's group count and every
referenced actual DObjList entry, including non-NULL pointers. This catches
native costume substitutions and differing live model graphs before publishing
the table. A failure leaves the pointer and all visibility state untouched.
A success changes only `x5AC.xC[4]`; it preserves `cleared[4]` and performs no
visibility updates or draw calls. Existing parts setup must run first, because
it clears table4. Installation is a load-time contract, not a live table-swap
mechanism after drawing.

Load contexts share the existing64-entry synchronous dispatch budget. They use
fixed storage and support nested creation. At saturation, or when the new
source is not registered/live, the actual load callsite skips that callback;
it does not publish visibility or consume an unbalanced context. Ordinary
callbacks below the cap retain their original invocation and order. Package
retirement invalidates borrowed ownership before arena/module cleanup, even
when the same slot and Definition address are rebound later.

The public authored declaration and Host/compiler wiring are integrated by the
coordinating compiler batch (`visibility_asset SLOT` and
`visuals.install_outline_visibility(slot)`); backend acceptance alone does not
claim those snippets compiled or a paper renderer drew correctly. Installation
uses the existing common fighter outline/body/outline passes, driven by the
non-NULL table4. Article layered drawing instead uses article **joint** lists
and has a separate immutable schema and constructor lifecycle.
