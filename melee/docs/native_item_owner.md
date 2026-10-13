# Ordinary items and authored fighter ownership

Ordinary engine items retain their built-in assets and callbacks. An authored
fighter can create one, attach it to a rig part, inherit its current owner's
scale, and publish it into either fighter-held slot. These are separate
operations: creation does not pretend to pick up the item, attachment does not
publish a fighter slot, and clearing a slot does not destroy the item.

`native_item_owner.c` implements the shared backend. The scripting reference is
[Fighter engine-item operations](scripting/engine_items.md).

## Identity and lifetime

`Item.native_creator` records the original fighter, its creation ID, kind, and
active authored-definition identity at allocation. Ordinary item private unions
remain available to their built-in behavior. Reflection and pickup can change
`Item.owner` without changing the retained creator.

Ordinary-item creator resolution requires the actual live fighter registry and the recorded
creation ID, kind, and definition. It never follows an address that has been
recycled for another fighter. The common Engine-selected constructor validates
that creator again after its instance callback and before the cached-owner
tail. If the callback removed or replaced the creator, it destroys the surviving
new item and reports failure.

Binding and attachment accept a full 32-bit item creation ID, with `-1` reserved
for absence. An ordinary item must belong to this creating fighter. An authored
article uses its existing package-and-creator check. That authored check compares
current definition lookups; this migration does not add rejection of definition
replacement to existing article behavior. Reading a held slot accepts
any currently live item in that slot, including one picked up from another
fighter. Clearing a held slot only clears that pointer; the script remains
responsible for item release or destruction when appropriate.

Attachment validates the raw rig part against the actual per-kind part table
and joint, invokes the engine's real pickup/attachment path, and rechecks both
fighter and item lifetime afterward. Generated callbacks also stop if creation
or attachment synchronously invalidates their source.

## Construction and scale

Creation uses `Item_CreateEngine`, whose explicit Engine policy bypasses both
parent-package asset substitution and authored callback substitution. It accepts
loaded ordinary common-item kinds; it does not reinterpret a fighter-specific
article as an ordinary item. Spawn position, velocity, facing, collision group,
and initial item damage remain explicit. Nonfinite inputs and damage outside
the signed 16-bit spawn field are rejected before construction. In-range
fractional damage truncates toward zero, following that field's representation.

Scale inheritance uses the item's current owner and the actual common scale
routine, including its geometry refresh order. It optionally remembers the
result in `Item.throw_scale`. It does not silently use the original creator's
scale after ownership changes. The saved-scale migration and throw/drop timing
are documented in [Remembered throw scale](native_item_throw_scale.md).

## Acceptance boundary

These are generic engine contracts, not a claim that a complete fighter has
been translated or that ordinary-item collision and presentation have received
new gameplay acceptance. Source fixtures explicitly identify controlled
allocation, model, audio, and geometry boundaries. Full composed fighter
initialization, cleanup, and phase comparisons remain separate promotion gates.

### Integrated checkpoint, 2026-10-02

The full native build and 52 selected CTests pass. The real ownership backend
and extracted constructor/attachment/scale bodies pass 97 cases / 1,052
assertions. Both generated module forms pass 48 cases / 1,204 assertions against
controlled Host boundaries, alongside 106 compiler checks. A fresh Dr. Mario
results-pill import and compiled module pass 2,199 source comparisons after the
direct binding callsite migration. No game was launched.

Evidence and immutable runtime/SDK inputs are retained in
`.local/item-contracts-integration29/receipt.json` and `.local/baseline29/`.
Rook was replacement-installed against this layout with unchanged script and
285 own-rig animation clips; verification reports zero borrowed clips.

The owner fixture is `tests/native/item_owner_test.c`, with actual source
extraction in `tests/native/item_owner_fixture.py`. It runs through the regular
`native_item_owner` CTest. The generic compiler and generated-module targets,
including partial animation imports and the parameter bound, are registered in
`cmake/NativeItemScriptTests.cmake`.
