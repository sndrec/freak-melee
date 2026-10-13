# Owned item resource integration

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Source packages now carry an optional canonical `resources.fmr` bundle. Its
entries contain exact logical names and owned PNG/blob bytes. Project opening,
package parsing, source checking, building and runtime preparation all use the
same bundle; Workshop source publication swaps its corresponding resource bytes
alongside the accepted facts and FMS. No runtime path opens an icon by filename.

Workshop Item declarations specify `blueprint items/base.fib` and optionally
`icon items/snack0.png`. A blueprint contains the original DAT plus an explicit
Article descriptor offset. Its importer constructs an owned model/attribute graph
through the existing substantive Article and command importers. Shared blueprint
entries import once per package. They provide data, not native behavior callbacks;
the interpreted Item and shared held-item protocol remain the behavior authority.
This preserves the existing capsule placeholder for art-free recipes while
billboard recipes select their own decoded PNGs.

Visual preparation happens after typed program preparation, before activation.
Each visual retains its declaration's own identity and the common code bank.
Publication retains external pointers to the stable candidate owner. Drawing
checks the actual Item Spec, blueprint/model or decoded visual identity. Missing
declared art does not silently draw another recipe. Decoder allocations are
loading-transition work and are released only after matching actors, programs,
callbacks, draw operations and package leases have drained.

Resource authoring commands atomically replace exact entries while preserving
the others. `project-resource-item` accepts an original DAT and explicit Article
offset. `project-resource-stock-item` copies the selected common item data from
the supplied ISO at authoring time; it introduces no runtime stock lookup.
`export-fighter-facts` copies an authoring template's data-only facts into FMAS.
This is not an old-project or package compatibility path.

Rook's example FMS now uses the current canonical calls and an owned blueprint
for each recipe. Side-B binds the dealt-damage helper, not the victim damage event.
The helper preserves the hand-written native motion change, without the old C
emitter's extra animation restart. Cliff catch uses the checked native attempt;
the old emitted standalone call incorrectly entered a cliff state unconditionally.
The narrow jump-input routine is `controls.try_aerial_jump()`. It is distinct
from the full shared aerial-jump IASA. Locals retain explicit Float storage and
item selection explicitly converts its Float recipe index to UInt.

The landing notification now dispatches a source code ID at the actual shared
air-to-ground transition. It does not require a native Definition. The three new
ledge/jump primitives call the exact shared native routines, with owned-source
and lifecycle checks before publishing their Boolean result.

Evidence under `A:/FreakMelee-test-archives/`:

- `source-resource-codec-four-mib`: 12,244 checks / ten cases, including all eight
  unchanged Rook PNGs, canonical limits, aliases and failure preservation.
- `source-resource-tool-rook-facts-review`: 98 checks / eleven actual CLI cases,
  including atomic replacement/lock failure and independently checked ISO data.
- `source-item-visual-per-declaration-final02`: 323 checks with actual WIC pixels
  and controlled graphics/registry leaves. Per-declaration identities are distinct.
- `source-item-package-final-root`: 28 checks through the real SourcePackage and
  actual Capsule DAT/PNG import; two recipes share a blueprint, missing resources
  preserve the accepted owner, and leases block premature release.
- `source-rook-resource-preflight-final-root`: all 51 Rook functions and nine recipes
  prepare with actual copied Rook DATs/facts, eight PNGs and the owned blueprint.
  Original user project files remain unchanged.
- `source-rook-ledge-dispatch-oct07-final`: 13,941 checks / 2,304 native primitive
  comparisons through the actual top-level invocation dispatcher.

The client/tool build and focused interpreter/store/packet regression suite pass.
These results do not establish actual Rook gameplay or renderer appearance.
State 341 still needs its four shared jump phases translated, and full authored
jump entry requires the generic APIs identified in `native_rook_jump_closure.md`.
The raw unusual turn/count values are preserved for that work. No user project
has been replaced or installed by this checkpoint. Paired families, richer
resource graph declarations and complete explicit rollback state remain open.

PNG framing/CRC and decoded pixels are separate checks. WIC tolerates some invalid
compressed streams, so successful platform decoding is not a strict compressed
stream validation claim. The actual provided PNGs decode and retain their bytes.
