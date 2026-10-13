# Generic authored layered article rendering proposal

Read-only source/design checkpoint. No renderer, DSL, default template or accepted
fixture changed. `retail_census.json` records fresh USA v1.02 DAT reads, not a
material/GX test. This proposal closes a genuine rendering dependency of the
accepted Game & Watch behavior components; those components remain source-phase
acceptance rather than complete visible independence.

## Source contract and concrete retail census

`ftGw_Init_OnLoad` (ftgamewatch.c:522) copies special attributes, sets instance
Z scale, stores outline RGBA, colors every costume material diffuse value, and
installs item-slot10 into fighter visibility pass4. Slots0..9 are articles.
The four palette variants share the costume graph. Fresh PlGw.dat hash is
`1c057a2882b7b42f3184b96ddf5221cae50e69652b00f6c78819c819804e0a1f`.
Body RGBA values are 000000ff, 6e0000ff, 00006eff and 006e00ff; outline is
ffffff80. Width is binary32 0.009999999776482582. These are derivation evidence,
not proposed hardcoded engine defaults. Owned parameters must import the actual
attribute bytes at +4..+19 and +20..+23; width is the float at +0.

| Slot | Article | Physical/declared bones | Normal indices | Outline indices |
| --- | --- | --- | --- | --- |
| 0 | Greenhouse | 11 | 4,7,10 | 3,6,9 |
| 1 | Manhole | 6 | 5 | 4 |
| 2 | Fire | 16 | 5,8,11,15 | 4,7,10,14 |
| 3 | Parachute | 10 | 6,9 | 5,8 |
| 4 | Turtle | 18 | 4,7,11,14,17 | 5,8,10,13,16 |
| 5 | Breath | 5 | 4 | 3 |
| 6 | Judge | 38 | 7,10,13,16,19,22,25,28,31,34,37 | 6,9,12,15,18,21,24,27,30,33,36 |
| 7 | Panic | 11 | 4,7,10 | 3,6,9 |
| 8 | Chef | 8 | 4,7 | 3,6 |
| 9 | Rescue | 8 | 4,7 | 3,6 |

Every listed joint has a valid parent. Lists do not intersect in this DAT.
Largest list is11, largest graph38. `DynamicBoneTable` actually owns100 pointers;
its capacity, not the observed38, is the relevant existing engine limit.
The first special-attribute word of every article is a relocated visibility
reference. Chef has a 0x74-byte special-attribute record; the other records are4
bytes. The referenced retail visibility record is16 bytes: u16 count at0,
relocated pointer at4, u16 count at8, relocated pointer at12. Do not memcpy it
into a native pointer structure. On 64-bit the existing typed representation
has counts at0/16, pointers at8/24, sizeof32. Both importer paths already decode
these values, but differ on empty-list validation and count limits.

`it_8027CE64` in itzako.c:718 executes this order:

1. Enable layered drawing (xDCF.b3).
2. Read body color from the explicit fighter argument via ftLib8008770C,
   which chooses Game & Watch or Kirby by raw kind; write all model diffuse
   materials through it80278574.
3. Read outline color from that explicit argument via ftLib80087744 into xBC8.
4. Obtain current damage owner's controller color through ftLib800870BC;
   store its presence in x5C8 and its RGBA in xBC4 only when present.
5. Inherit current damage owner's model scale through it80274594, which also
   updates joint scale, hitbox scale and the two common scale update paths.
6. Bind visibility reference in the private item union.

Colors selected from explicit creator and scale/tint selected from current owner
are different contracts. No new API should quietly collapse them. Reflection
must not reselect body/outline automatically; constructor scripts own snapshots.
Render-time active color-overlay selection is separate: it80278108 uses the
current owner's ColorOverlay only while layered AND attached (xDC8.x13),
otherwise the item's own overlay. That dynamic common effect remains shared.

## Exact shared draw/material behavior to retain

`it_8026EECC` retains normal visible/hitbox-debug gates, owner invisibility gate,
knockback display displacement and camera selection. Camera1 plus layered means
three passes; camera0 draws ordinary nonlayered items only. This is not three
unconditionally rendered copies of a model.

- First outline: b4=true,b5=false; hide normal subtrees, show outline subtrees,
  draw, show normal subtrees, hide outline subtrees.
- Body: b4=false,b5=false; draw without changing those lists.
- Final outline: b4=true,b5=true; same hide/show/draw/restore sequence.

Each hide/show checks the immediate parent's local JOBJ_HIDDEN bit before
changing the selected subtree. Preserve list order and HSD Set/ClearFlagsAll
semantics, including instance handling. These operations do not snapshot and
restore arbitrary authored visibility flags. Stock leaves normal shown and
outline hidden when the parent passes its gate; do not change that behavior by
introducing a visibility stack. Absent parents/indices in custom data are import
or activation errors, not unchecked draw accesses.

`fn_80277D8C` sets no-Z-update only for the first outline pass; final outline at
opacity255 uses its specific custom PE descriptor. `it_80277F90` adds light
color TEV only outside outline passes. `it_80278108` skips final-outline tint,
uses xBC8 on the first outline, and preserves the existing integer blend of
controller tint and ColorOverlay plus opacity x5C9. Keep the blend/reg-allocation
implementation and its branch order. A color query or a HSD visibility test
cannot prove these PE/TEV/GX operations render correctly.

The additional native_models[3] loop currently follows the common root draw and
is outside these layered passes. Initial authored layer declaration targets
actual GObj root/slot0 only, which covers all ten stock articles. Do not silently
promise it will layer extra model slots; any broader slot support needs explicit
metadata and separate draw evidence.

## Smallest immutable data change and direct callsite migration

Rename the existing visibility type to a generic `ArticleModelLayers`, using
u16 normal_count/outline_count plus const u8* normal_indices/outline_indices.
Change both real importers and every direct consumer/type declaration; no typedef
compatibility alias. The original record's bytes retain the same meaning.

- Add immutable `Article::model_layers` outside special-attribute union layout.
  PaperArticles imports populate it from their already decoded reference.
  Generic authored declarations populate it using the declared reference recipe.
- Add `Item::model_layers` outside xDD4, initialized NULL on creation; binding
  comes only from this item's own Article, never a script pointer. Existing
  stock it8027CE64 assigns that typed field at its original final statement.
- Delete itdraw's it8026EECC_VARS pointer-punning macro. Actual three-pass
  consumers read item->model_layers directly. Standard item/material callers
  still execute the same real functions. No fake Item or stock-kind delegation.
- Delete the old gamewatch/model_visibility union member once its real callers
  migrate. Chef variant remains an ordinary separately typed field for stock,
  rather than relying on an unused renderer prefix. Rescue's retained parent
  stays its own stock gameplay field. Native script state layout stays intact.
- Migrate the Kirby pan and Chef alias callsites too, because they share these
  actual stock paths. This is typed stock infrastructure maintenance, not an
  authored Kirby port (Kirby is out of scope).

Importer/schema/ModuleOpen validation checks relocation and exact16-byte source,
byte-span lengths, actual model graph ordinal count, non-NULL listed joints and
parents, and the real100-entry dynamic-table bound. Preserve duplicate indices
and list order; no clamp, sorted rewrite or invented non-overlap requirement.
Empty lists can be valid generic data, with NULL indices iff count0. Harmonize
both importer paths under that explicit contract. Existing general article
importer maximum256 list entries is a bounded proposed starting cap, not a
Game & Watch stock parity count; reject unsupported larger descriptors explicitly.
Record the cap as an engine constant and explain that repeated indices can make
list length larger than graph size. No per-frame allocation or DAT reads.

Immutable pool lifetimes follow Article/import arena. Clear retained bindings
before releasing that arena/module; detached validation slots must not retire
an active Definition. Mutable Item fields and HSD material changes use existing
tracked game-state/active-heap rollback paths; no second save/load scheme.

## Proposed generic DSL/API (unimplemented)

Article header `render layered attribute 0` means ordinary shared renderer plus
an immutable local reference at special-attribute offset0. This replaces the
separate `render standard` declaration for this article; reject both together.
The declaration provides metadata, not a fighter setup callback. Activation
imports/validates the recipe while immutable assets are available; compiler C
contains the constant offset only. No pointer cast or raw item-kind query.

Article operations, all strict typed:

| Surface | Type | Actual storage/operation |
| --- | --- | --- |
| item.layered | RW Bool | Existing b3, enable only with valid bound immutable layers |
| item.set_diffuse_color(rgba) | Int -> Void | Actual it80278574 model walk, packed Int converted bytewise to GXColor |
| item.outline_color | RW Int | Existing typed xBC8 color, packed RGBA |
| item.controller_tint_enabled | RW Bool | Existing x5C8 presence flag |
| item.controller_tint | RW Int | Existing typed xBC4 RGBA |
| item.material_opacity | RW Int | Existing x5C9 byte, reject outside0..255 |
| targets.controller_color_present(handle) | Int -> Bool | Valid live fighter and nonzero supported controller selector |
| targets.controller_color(handle) | Int -> Int | Actual common controller palette value or0 absent, preserving all32bits |

Colors use packed numeric 0xRRGGBBAA with explicit shifts; never host-endian int*
aliases into GXColor. `attribute byte` already imports actual owned component
bytes; scripts can construct Int colors from them. Alternatively a generic packed
RGBA import declaration can be added later, but it is not necessary for this
batch. No Float signals/parameters for packed colors. Controller query must use the actual typed x6DC_colorsByPlayer[4] with selector
1..4 -> index0..3; fighter.c explicitly asserts selector<=4. Selector0 has no
tint, and out-of-range selectors are rejected. ftLib800870BC currently indexes
x6D8[selector] through a one-int declaration; directly migrate its actual
implementation to a typed color lookup rather than reproducing that C array
overrun. Preserve existing stock destination bytes using typed GXColor or memcpy
at its adjusted callers. Handle0 or dead/recycled fighter yields absent/0.
A second confirmed input issue must close in this batch: ftCommonAssets_Import
word-decodes the common struct and only restores RGBA bytes at x7D8. It does not
restore x6DC..x6EF. Fresh PlCo colors at x6DC are ffffff50,00000050,000000ff,
808080ff; little-endian word import currently leaves50ffffff,50000000,ff000000,
ff808080. Preserve the actual ordered RGBA byte span during import. x6EC is
ff8080ff, used by the shared material selector5; keep it distinct from the four
controller colors. ftmaterial_tev.c uses selector4/5 for metal/special material
effects and must read typed colors, not cast the x6D8 integer array. Scalar
x6D8[0] remains gameplay data, used by ftcoll; do not convert it into a color.
This conclusion comes from actual importer source and retail bytes, with no
material/GX test claimed.

Direct common-palette repair sites are ftcommon_assets.c::ftCommonAssets_Import
(byte-preserving memcpy at offsetof(ftCommonData,x6DC_colorsByPlayer), including
the typed extra4bytes); ftlib.h/c::ftLib800870BC signature becomes
bool(HSD_GObj*,GXColor*) and its sole direct caller itzako.c::it8027CE64 passes
&item->xBC4 without int* cast; ftmaterial_tev.c::ftMaterial800BF6BC selects
colorsByPlayer[selector-1] for1..4 and the explicit x6EC color for5. Update
ftCommonData.x6EC to a GXColor field if giving that final color a typed name;
its4byte extent is unchanged. fighter.c already reads the typed four-entry
array and benefits from corrected imported bytes without a forwarding helper.
Retain x6D8 as its actual scalar gameplay integer and adjust all color callers
directly. Packed-Int script queries assemble RGBA from bytes; they must not
return host memory integer aliases.

The draw-time active owner ColorOverlay remains a shared shader concern, with
live current-owner validation, rather than a constructor snapshot.

Scripts choose colors from their immutable own parameters and the validated
creator's selected costume.variant (or an Int palette index stored in creator
state). Current owner's handle comes from item.current_owner. Scale uses existing
item.inherit_owner_scale at the original point. No new bundled setupPaper helper.

Constructor timing needs an explicit authored post-create configuration signal
at each original constructor site: after attachment/pickup and notification for
ordinary props; after Judge frame and orientation; after Chef variant setup and
notification; before Rescue's final state initializer. Generic import can bind
validated layers before on_created, but scripts should set layered/colors/tint/
scale in the original ordering, not move the whole CE64 block blindly before
pickup. Reuse same-author signals/helper DAG; signal payload0 carries no palette.
Audit whether an existing notify-holder constructor can reach each order before
claiming the whole constructor translated; report any missing generic ordinary
notification primitive separately. No stock renderer-initialization delegate.

Fighter OnLoad still needs small general presentation operations:
`visuals.set_diffuse_color(Int)`, `visuals.outline_color` RWInt,
`visuals.install_outline_visibility(Int asset_slot)->Bool` with immutable
registered lookup validation, and a strict Bool preserving original materials
in the translucent pass (x2224_b0; name pending final API review). The install
operation publishes this package's typed existing slot10 lookup into pass4;
it must reject article/joint graph slots, never cast arbitrary item pointers.
Do not duplicate parts rows or confuse DObj visibility lookup with article joint
lists. Instance Z scale and costume.variant already exist. x2223_b1 has no read
consumer in this checkout beyond initialization; record its write, do not invent
a semantic public toggle. x2222_b6 is general motion/position-freeze control and
must be audited against movement consumers, not named as a paper renderer flag.

## Coordinated implementation and evidence batch

1. Typed metadata/callsite repair first: itCharItems/forward/types, bothimporters,
   itdraw, CE64 and all shared stock alias constructors, typed controller palette
  getter/shared shader reads and common RGBA byte-span import. New immutable logic
   recipe, activation validation, Host/compiler packed-color methods and docs.
   Fingerprint all affected public/Item/Article offsets; no version shim.
2. Normal clone/import all ten actual visibility records,5colorvalues/width and
   fighter slot10. Compare raw receipt above to owned immutable tables. Assert
   host64 layout, truncated16-byte/relocation/index/parent/disabled rejection,
   custom larger graphs within100, empty/duplicate lists and NULL cases.
3. Actual generated constructor/body/outline/tint/scale source comparisons:
   independent author, same/foreign/current-owner transfer, all4palettevariants,
   controller absent/present and deterministic failure ordering. Separate
   controlled attachment/allocation/notification boundaries from real callbacks.
4. Actual HSD graph + original/shared draw/material bodies: camera0/1/other,
   parent-hidden gates, animated model variants, opacity0/1/254/255, controller
   color overlay combinations, held versus free, invisibility/displacement,
   three-pass visibility/order/materialdiffuse effects, extra-slot scope.
   Trapped GX/TEV sinks prove command/PE setup agreement only; rendered result
   still needs an explicitly coordinated hidden+muted drawn game acceptance.
5. Source audit rejects ftLib8008770C/87744, ftGw/Kirby handlers/private union
   accesses in generated owned callbacks; renderer cannot inspect donor kind.
   Preserve existing accepted behavior matrices, label their presentation mocks.

No source comparison, HSD test, material execution or game is claimed by this
proposal. No implementation should claim full Game & Watch independence until
constructor/immutable ownership and real drawn acceptance close these gates.
