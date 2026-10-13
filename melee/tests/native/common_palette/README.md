# Common palette byte order and typed selection

Source/runtime-data checkpoint. Production code repairs the confirmed native
RGBA word-decoding defect and replaces integer-array/color pointer aliases with
typed direct calls. No Game & Watch article behavior, DSL, layer installation,
render pass or default template changes belong to this batch.

## Changes

- `ftCommonAssets_Import` preserves the four ordered GXColor records at
  `x6DC_colorsByPlayer` and the separate color at0x6EC after scalar word import.
  The existing color-byte preservation at0x7D8 remains unchanged.
- `ftCommonData.x6EC_specialMaterialColor` is GXColor, still exactly four bytes
  at0x6EC. Common struct size remains0x818. Scalar gameplay x6D8[0] stays a
  scalar decoded word; it is not a palette entry.
- `ftLib_800870BC(HSD_GObj*,GXColor*)` directly reads controller1..4 from
  `colorsByPlayer[selector-1]`. Selector0, unsupported values, NULL object,
  NULL userdata, NULL output and absent common data return false without
  touching the destination. The actual sole CE64 caller passes `&item->xBC4`.
- The complete material selector `ftMaterial_800BF6BC` reads typed controller
  colors1..4 or the distinct material color5. Unsupported selectors have no
  palette tint, while valid outline/overlay behavior remains meaningful.
  Existing metal override4 and special-material override5 retain their order.
  Shader blend arithmetic and TEV construction are unchanged.

These repairs intentionally change the erroneous native color bytes. Retail
PlCo.dat stores ffffff50,00000050,000000ff,808080ff and extra ff8080ff. The old
actual importer produces reversed bytes on Windows; the fixture demonstrates
that defect rather than accepting it as intended parity. Invalid selector
behavior is an explicit defined extension; the old out-of-range pointer-array
lookup is not executed or claimed as a source-equivalence contract.

## Reproduce

Use a fresh output directory:

```powershell
python -B tests/native/common_palette/run.py --snapshot .local/baseline58 --output .local/common-palette-root
```

Optional `--source-root` selects the repaired checkout; `--iso` selects the
original USA v1.02 disc. `--snapshot` supplies immutable SDK/compiler support
inputs. The driver copies frozen adjacent fighter headers into an isolated
include tree, overlays only the owned types.h/ftlib.h, and records every input.
It does not read live shared headers, overwrite the snapshot, launch a game or
use the global build. Actual edited whole ftcommon_assets.c and unchanged
frozen DAT/graph/CPU/color importer units compile together. There are no fake
importer or DAT functions. The frozen legacy importer is compiled separately
under a distinct test-only definition name to expose original byte reversal.

The query and material functions are extracted from actual definitions. The
complete material body is compared with the frozen58 original body using
corrected color bytes for healthy selections. Extraction renames only the old
comparison function; all code and global TEV descriptors otherwise stay intact.
Tests link no behavior delegates, adapters, fake Item contexts or scripted move
substitutes.

## Accepted console evidence

`.local/common-palette-repair58-final2/receipt.json`:

- 19,001 checks, zero failures.
- 17 actual full common imports: retail first, then16 byte-pattern/scalar
  variants; all five color records preserve their original bytes, scalarx6D8
  preserves its decoded value, typed offsets/size stay correct, truncated DAT
  is rejected.
- 256 controller-selector cases, plus NULL/absent inputs and untouched output
  checks. Controller selectors1..4 are the only valid controller query values.
- 4,608 healthy full material-body pairs across selectors0..5, six relevant
  material/pass flags, six alpha values and active/inactive overlay. Both full
  bodies emit the same normalized color/stage command trace. Additional checks
  prove palette RGB and alpha-ratio values at direct sinks.
- 1,000 new safe invalid-selector cases across values6..255 and no tint,
  active overlay, first outline and final outline modes.

Controlled boundaries are the current ColorOverlay lookup and HSD TEV register
allocation/SetReg/stage sinks. Actual full material selector and integer blend
logic execute. The trace records only initialized RGB and shader fields, not
uninitialized unused color alpha/padding or temporary pointer identities.
No TEV pixel evaluation, GX pipeline, model drawing or game acceptance is claimed.
Allocation in this console test is test/import-transition storage, not a new
production per-frame allocation. The actual full importer executes descriptor
and graph decoding; it does not create live HSD/GObj/model instances.

Initial preparation failures were fixture-only: OSReport stub const mismatch
and a missing dat_texture.c source unit. Both are corrected; final source receipt
passes without filtering. No production repair was changed to accommodate them.

## Root integration handoff

Production files: ftcommon_assets.c, ftlib.c/h/dox, ftmaterial_tev.c, the single
x6EC field in ft/types.h, and the CE64 typed-color call statement in itzako.c.
Root's grouped native build and80 checks separately passed after these edits.
Root may invoke this Python driver as one scoped source suite with a fresh output;
no new target is required to prove the bounded source batch. The runtime struct
extent/offsets are unchanged and the altered function's direct caller is migrated.

Separate follow-up: PaperArticles fighter extended attributes also need their
body/outline RGBA byte spans preserved (root-owned asset-layout audit). This
batch only fixes the common palette, and cannot claim that paper constructors,
shader material classes or article layered rendering are now fully authored.
