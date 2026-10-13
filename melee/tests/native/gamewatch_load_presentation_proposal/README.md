# Game & Watch owned load and presentation closure

Read-only proposal, not an implementation or acceptance fixture. This namespace
adds only text, a source census, and hash receipts. It does not change the frozen
layered-render proposal, accepted behavior components, a default template, or the
engine. No compilation, material execution, rendered test, or game is claimed.

The reference design is `../article_layered_render_proposal/proposal.md` and its
retail census. Its DAT receipt, not a new DAT inspection in this task, supplies
the palette and width below. Source locations and hashes are in `census.json` and
`source_manifest.json`. The live checkout includes a separate in-progress common
palette repair; this proposal does not claim that repair is tested or frozen.

## Exact load order and remaining operations

`ftGw_Init_OnLoad` does, in order: clear deferred position flag; set the unread
flag; set secondary-pass material policy; copy attributes; initialize Panic
charge to empty; assign instance width; store outline color; traverse the costume
materials and assign diffuse color; install slot10 as fighter visibility table4;
register articles0..9 in Greenhouse, Manhole, Fire, Parachute, Turtle, Breath,
Judge, Panic, Chef, Rescue order.

Common `Fighter_Create` already zeroes the allocated Fighter and runs
`Fighter_UnkInitLoad_80068914`, including the three flag resets, before OnLoad.
`Fighter_80068E64` sets stage width, then `ftParts_800749CC` initializes visibility
tables, then OnLoad overrides width and installs table4. Keep this ordering.
Do not install table4 before parts setup: `ftParts_8007487C` clears it to NULL.
Article registration belongs to owned immutable asset bindings, not ten scripted
calls to a stock item registration handler.

The smallest proposed fighter surface is:

| Proposed surface | Type | Actual operation and timing |
| --- | --- | --- |
| `visuals.set_diffuse_color(rgba)` | Int -> Void | Direct Host binding to `ftMaterial_800BFB4C(Fighter_GObj*, GXColor*)`; construct one stack GXColor from numeric packed RGBA. |
| `visuals.outline_color` | RW Int | Read/write `x610_color_rgba[1]` bytewise; no extra Host or shadow color. |
| `visuals.preserve_secondary_material` | RW Bool | Exact `x2224_b0` storage; it changes material/TEV policy only in the shared secondary pass. |
| `model.instance_scale_z` | RW Float | Exact `x34_scale.z` storage, finite nonzero values; assignment itself does not force an extra transform update. |
| `visuals.install_outline_visibility(asset_slot)` | Int -> Bool | Publish the active package's validated immutable fighter lookup into `x5AC.xC[4]`; no draw or variant change. |

The instance-Z claim in the frozen layered-render proposal is stale: current
fighter scripting exposes only `model.instance_scale_y` and JObj `model.scale`
coordinates. `model.set_scale` changes JObj scale, not the Fighter field. The
new field is necessary: `Fighter_UpdateModelScale` uses it as root **X** scale
when unequal to1, and sets root Y/Z to common model scale. Matrix compensation,
common matrix selection, and ordinary death-camera paths also read the field.
Its historic field name is not a promise to set the visible root's Z axis.
Zero is unsafe because the shared transform path inverts scale. Ordinary
negative finite scale need not be rejected solely to reproduce positive stock
width; reject nonfinite/zero before mutation. Stock width remains exact binary32.

Diffuse construction preserves the real traversal: walk root, sibling/parent
chain, and DObj/MObj/material records; do not descend instance joints; skip absent
materials. Do not substitute one root material write, shader tint, or a wrapper
that delegates fighter setup. The Host type can use the existing actual function
signature `void (*set_fighter_diffuse_color)(Fighter_GObj*, GXColor*)`. Generated
code evaluates the Int once and shifts its uint32 bits into R/G/B/A. Outline
does the same byte conversion directly. No host-endian GXColor/int pointer alias,
Float round trip, global last color, or new palette-selection Host is needed.

`preserve_secondary_material` is deliberately not called an always-on metal
override. `ftDrawCommon_80080C28` sets `x2228_b0=1` for its secondary draw;
`ftDrawCommon_800805C8` clears it for the body draw. Only the conjunction with
`x2224_b0` keeps original non-outline materials instead of substituting common
metal/special MObjs and selects the corresponding common color selector4/5 in
`ftMaterial_800BF6BC`. Outline rendering uses `x2223_b2` and the stored outline;
final-outline tint suppression uses `x2223_b3`. Those two are shared draw-pass
state, not load-time public toggles. None of these consumers routes on stock kind.

## Immutable fighter visibility is not article joint layering

The existing PaperArticles importer already decodes slot10 via
`import_visibility_lookups`, using the imported model group's count. Each group
contains variant records, each record contains a byte list of **DObj indices**.
`ftParts_80074B6C/74CA0/74D7C` index the costume DObj list and maintain the existing
`cleared[4]` state. This is not `ArticleModelLayers`, not a raw joint ordinal,
and not the item's DynamicBoneTable. Table2 separately indexes the metal DObj
list; this proposed installation targets table4 only.

Use a bounded immutable Definition visibility-asset declaration (slot10 for this
package) and a typed importer result, distinct from `article_assets`. A minimal
recipe record can contain an asset slot; the existing PaperArticles DAT schema
supplies the lookup record and group count. Publish a typed lookup through the
owned imported character assets rather than casting arbitrary `void** items` at
the Host boundary. A fixed capacity no greater than the existing16 character
asset slots suffices; no per-frame allocation or second visibility format.

Proposed Host signature:
`bool (*install_outline_visibility)(Fighter_GObj*, int32_t asset_slot)`.
Its actual implementation belongs in `native_fighter_visuals.c`: resolve active
Definition/package ownership, check the declared typed asset and its cooked
lookup, then assign the pointer. This is a substantive validation/publication
operation; it need not wrap any stock handler. A failed call leaves the pointer
and cleared state unchanged. A successful first install preserves the original
`cleared[4]` value rather than eagerly hiding/showing models. Limit initial usage
to load-time installation; replacement after a draw needs a separately specified
visibility-state contract, not an implicit new reset.

Importer/activation validation must prove source relocation/descriptor spans,
group count equal to the active model count (engine maximum11), bounded variant
and draw counts (existing importer bound256), valid list storage, and every byte
index within the actual costume DObj list with a non-NULL DObj. The existing
slot10 importer bounds descriptors but does not prove costume DObj index bounds;
ordinary renderer consumers dereference them without checks. Validate all
selectable costume graphs when binding immutable data. Preserve list order,
duplicate indices and valid empty selections. Do not assume disjoint groups or
equate list length with DObj count. Keep typed references within the active
import arena lifetime and retire them before freeing that arena.

The existing common fighter renderer already selects outline/body/outline from
non-NULL table4 under its camera1 path, using existing visibility and invisible
gates; camera0 follows its existing alternate path. No new GameWatch-kind draw
switch is needed. Article three-pass rendering remains the separate frozen
layered-render implementation proposal, including its parent-hidden semantics,
constructor timing, immutable joint lists, and controller-tint handling.

## Exact owned import values

These are21 editable named values: one Float and20 Byte imports, each Byte
becoming an owned Int. All offsets are relative to original special attributes.
The existing `attribute byte NAME OFFSET` importer reads **raw DAT bytes** before
normalizing parameters; no new packed-color import primitive is required.

| Value | Offsets R/G/B/A | Retail bytes |
| --- | --- | --- |
| body palette0 | 4/5/6/7 | 0/0/0/255 |
| body palette1 | 8/9/10/11 | 110/0/0/255 |
| body palette2 | 12/13/14/15 | 0/0/110/255 |
| body palette3 | 16/17/18/19 | 0/110/0/255 |
| outline | 20/21/22/23 | 255/255/255/128 |

`attribute float model_width 0` imports 0.009999999776482582. Width is not an
extra byte range. Packed colors use numeric0xRRGGBBAA and all32 Int bits, including
high-bit/negative signed representations. Validate edited channels0..255;
strict stock-source selection uses `costume.index`, the actual original
`x619_costume_id`. `costume.variant` can instead select an authored native model
variant: native costume overrides may change it while retaining the raw index.
These agree for unmodified stock indices0..3, but choosing the variant for custom
palettes is an explicit author policy, not unnoticed source parity. A declared
fallback for unsupported custom palette indices also belongs in script, not
donor-kind dispatch. Ordinary fighter state
may retain selected body/outline Int colors for creator-side article snapshots.

The current PaperArticles stock layout has no byte-preservation range. Its
legacy `extended_attributes` are word-decoded, so reading typed GXColor there
would reverse byte order on little-endian hosts. Owned raw-byte parameter import
does not suffer this issue. For a typed original-attribute/import comparison,
the owned layout should additionally declare `{ offset=4, size=20 }` in its
existing byte_ranges. Do not restore the whole attribute block as bytes: behavior
Float/Int fields must remain decoded. This is separate from the common controller
palette fix in the frozen renderer proposal and current live core edits.

## Lifecycle flags and existing authored scope

`x2223_b1`: exactly two assignments plus declaration across current src: common
initialization0 and GW load1. There are no reads. Omit the dead GW assignment
from an authored policy and record this deliberate removal; no semantic toggle.

`x2222_b6`: the GW false write is redundant with common initialization in the
actual load path. It is not a rendering flag. Shared movement may accumulate
velocity until animation/throw gates release it; DeadUpFall, Koopa/Mewtwo throws,
shouldering, HammerTurn and Lift also read it. Current scripting exposes it RO
as `capture.freeze_state`. No new setter is needed for GameWatch OnLoad and no
generic movement-freeze refactor is justified by this load duplication alone.

`x2224_b0`: common initialization0 then GW load1; two real common material/TEV
readers. This write must be authored or declaratively owned, not omitted.

The accepted normals lifecycle already authors: load-empty Panic charge and
handle initialization; death group reset0,-1,0,0,-1,-1,-1,-1,-1,-1,-1 and ordinary
state resets; ordered damage cleanup Greenhouse/Manhole/Torch/Parachute/Turtle/
Sparky/Judge/Panic/Rescue; airborne-only cleanup at relevant normal entries;
item pickup/drop(1,1), visible/invisible(1); full-Panic color reset5,false.
Death deliberately retains Panic charge. Constructor-bound damage/death2,
pre/post-hitlag and accessory helper bindings must remain in the composed source.
GW has NULL stock knockback-enter/exit and model-event/getter slots; no new
knockback or per-DObj fighter callback is implied by table4 installation.

Do not retain `ftGw_Init_OnLoad`, palette getters8014A7F4/8014A814, or their
raw-kind `ftLib_8008770C/87744` routing in an independent module. Authored article
constructors use creator-owned selected colors and the separately live current
owner tint/scale protocol described by the frozen article proposal. Immutable
article slot/kind bindings replace registration, rather than gameplay delegates.
Special-attribute reload becomes owned-parameter handling; no runtime union/DAT
palette reads. Keeping the legacy callback's copied attribute struct solely to
feed a renderer would leave a genuine closure gap.

## Suggested coordinated next batch

Root can implement the five proposed surfaces plus typed visibility-asset
publication alongside the frozen article-layer work. Relevant actual ownership
sites: native_fighter_visuals (publication), runtime Definition/import activation
(typed data/validation), module Host/fingerprint (two fields), fighter primitive
compiler/type/Host requirements (operations and three properties), and the owned
GameWatch load policy (width/color selection/order). Existing ftmaterial,
ftparts and ftdrawcommon paths already consume the state generically; do not add
a fighter-kind branch or forwarding renderer wrapper there.

After implementation, a coherent normal clone/import receipt should verify all
21 values and typed slot10, then actual generated load/source material/visibility
comparisons should distinguish real HSD traversal from controlled GPU sinks.
Constructor article-layer integration and drawn acceptance remain separate
required gates. This checkpoint supplies no hypothetical API tests and makes no
whole-fighter independence or visible-rendering claim.
