# Owned transient accessory checkpoint

2026-10-11. Samus remains an **unregistered authoring candidate**. This
checkpoint closes the throw-accessory data and scripting prerequisites; it
does not establish complete grapple construction or rendered gameplay parity.

## Implementation

`FtAccessoryAsset` replaces the anonymous Samus pointer-layout structure with
an immutable root, a base animation and up to 16 animation variants. Each
animation independently contains joint, material and shape channels. The DAT
importer normalizes serialized pointers once and validates graph provenance
against the owning import arena. The existing grapple archive decoder still
identifies the structured payload in fighter item slot 4; arbitrary item slots
are not automatically accessory recipes. Both native Samus and interpreted
fighters consume the normalized type directly. No compatibility payload remains.

The generic `accessory` namespace exposes construction, animation installation,
scale, frame request/evaluation, geometry clock rate, bone following and disposal.
It retains the source, published program, rig and model across engine operations.
The loop-policy argument preserves looping geometry clocks when false and never
changes material or texture clocks. Disposal unpublishes the model before freeing
it. Construction, animation installation and follow constraints use ordinary
tracked HSD allocation at setup transitions; repeated follow calls add constraints.

Samus's authored `Tether_CreateThrowAccessory` helper chooses the owned throw
variant, installs base and variant animation, applies instance scale and the
non-looping rate policy, evaluates frame zero and follows raw rig joint 51.
`on throw_presentation` invokes it at the common post-victim-setup slot. It calls
engine primitives and does not delegate to a stock Samus handler. The four throw
motion IDs, asset slot and bone index are source choices, not scripting API rules.

See [accessory scripting reference](scripting/fighter_accessory.md) and the
[previous common throw checkpoint](native_source_throw_presentation_checkpoint.md).

## Evidence

- The real `native_fighter_character_assets` test imports every stock fighter
  archive. For Samus, it independently resolves the serialized root, base and
  four throw variants and compares their identities to the normalized recipe.
  Generic cases cover zero/one/four/16 variants, material channels, malformed
  pointer slots and foreign or invalid provenance. Positive shape-animation
  channel import is not separately exercised by this new cohort.
- `A:/FreakMelee-test-archives/accessory-controls-oct11-reviewed/receipt.json`
  records 113,212 checks and 9,934 cases in the connected capture/VM batch,
  including 170 accessory cases. The exact authored helper is prepared into
  bytecode and compared with the native throw constructor in 144 combinations
  of four throws, three scales, three rates and geometry loop flags. Six
  retirement boundaries stop continuation, 12 malformed calls leave output
  unchanged without an engine operation, and eight cases cover both loop
  policies. Actual HSD animation traversal and clock helpers execute.
- `A:/FreakMelee-test-archives/samus-accessory-source-oct11-reviewed-b/receipt.json`
  prepares all 282 composed Samus bodies. Its only diagnostic is the deliberately
  absent independent Fighter declaration. Existing geometry comparisons pass
  33,378 checks/480 comparisons plus six safety cases; lifecycle/input/hang
  comparisons pass 77,145 checks, 128 lifecycle cases, 384 reloads, 8,200 sequence
  cases and 240 hang cases.
- Eight native targets build, including the client, content tool, source-family
  and package runtime tests. The character-asset, script and parameter CTests
  pass. Both changed substantive C modules pass strict syntax warnings with the
  existing duplicate matrix-declaration warning excluded (`-Wno-array-parameter`).

The connected accessory cohort supplies controlled graph records and observes
model construction, animation installation/evaluation and follow constraints.
Its native comparison therefore proves ordering, selectors, scale and clock
policy at those boundaries, not live graph evaluation or rendering. The retail
import cohort independently exercises the real DAT normalizer. No game was
launched, no installed collection changed and no upstream push occurred.

## Remaining closure

Full retail grapple node construction, solver/catch integration, resource and
borrower lifecycle acceptance, and rendered throw/gameplay checks remain open.
The typed recipe does not yet have a Workshop authoring interface. These tests
do not promote Samus to an independently cloneable fighter or complete the full
project goal.
