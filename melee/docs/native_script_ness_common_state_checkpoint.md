# Ness canonical bindings and common fighter state

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The full Ness source audit drops from 56 to 14 first-errors-per-body across all
418 indexed functions. Metadata and body coverage remain complete; the package
is still rejected. The count is not an exhaustive count of remaining fixes:
only the first error in each failed body is reported, and later preparation
stages have not been accepted.

## Implementation

Article reads of root-owned immutable parameters now use `program.attrs`, so
continued item behavior does not require a surviving creator. Receiver scale
reads use `.model.instance_scale_y`, incapacitation uses `.fighter.incapacitated`,
and helper calls use `.helpers`. Callback identity uses the existing generic
`callbacks.matches(Callback.slot, helper)` operation. CPU randomness uses
`random.float()`. Floating collision angles are explicitly Float locals, and the
reflector ownership byte is converted to the required Bool.

The PK Fire spawn body is an ordinary root helper bound with
`callbacks.accessory`. Stock `ftNs_SpecialS_ItemPKFireSpawn` checks and clears
`throw_flags_b0` itself. The script keeps its single `cue.article.consume()`;
using the cue-consuming callback binding too would prevent the spawn. No stock
special handler is called and no gameplay callback body is replaced with a stub.
This wiring has source/compile coverage, not a live Ness gameplay result yet.

New reusable engine bindings:

- [`smash`](scripting/smash.md) exposes seven per-instance writable charge/shake
  fields and the shared shake-table count. Native enum modes and manual mode 4
  are documented constants. Stock Ness uses that manual value directly.
- `floor.is_platform` reads the native stored floor flag, independent of its
  material index.
- `physics.mirror_velocity` calls the existing native X/Y reflection routine,
  retaining its zero/non-unit inputs and unchanged Z.
- `state.down_bound` enters the existing common native knockdown continuation,
  retaining owned Fighter/motion/storage checks across it.

No fighter-specific binding, compatibility alias, new gameplay heap allocation
or custom charge/reflection formula is added. Smash data stays inside the
existing Fighter state. Constant catalog entries remain sorted for binary
lookup; the new intrinsics use explicit dispatch before older semantic ranges.

## Verification

Seven Windows targets rebuilt and script/parameter CTests passed 2/2. The real
library Item/Fighter fixture checks both controlled raw and interpreted Fighter
actors. Each role performs 112 writes with whole-Fighter comparisons, 64 platform
flag queries and 32 exact `lbVector_Mirror` comparisons. An owned actor executes
the new charge/reflection helper through the actual Fighter VM. Negative signed
values, mode/byte rejection, absent shake tables and retired sources are covered.
No production engine routine is substituted.

The same batch retains native Item animation comparisons: both full Link owners
pass 3,125 checks/50 cases each, with 110 graph comparisons, eight frames and two
page restores. Both primary-only owners pass 1,789 checks/six cases each with
12 comparisons. Totals: 9,828 checks, 112 cases, 244 animation graph comparisons,
896 field writes, 512 platform queries, 256 native mirrors, four executed Fighter
helpers, 16 animation frames and four page restores.

This uses controlled userdata, shared-table input and isolated publication.
It does not establish a complete native Fighter constructor/scheduler scene,
live knockdown entry, Ness move/gameplay parity, visuals/audio or whole-match
rollback. The existing Item cohort deliberately exercises a rejected model
animation call; its expected fault is not a clean full-game result.

Receipts:

- `A:/FreakMelee-test-archives/source-item-common-fields-oct10-e/receipt.json`,
  SHA-256 `dfcecc56ff63fee6ae4d950f656ad8b2b42f9d425cac27d8d0809bf2069f8dd8`.
- `A:/FreakMelee-test-archives/source-roster-common-fields-oct10-a/receipt.json`,
  SHA-256 `52e6a720b8eb47444f6cecd917a1a958e77f4f1f709ae735588dacb6b9143d6c`.
  Current read-only census retains 24/26 accepted non-Kirby forms. Ness rejects
  with 14 body diagnostics; Samus still lacks a valid independent draft.
- `A:/FreakMelee-test-archives/ness-common-state-oct10-final/receipt.json`,
  independent frozen/current code, fixture, assets, binary and audit rehash.
  The fixture README was changed only to record completed results afterward.

Remaining Ness errors include vector rotation/ceiling-Z observations, immutable
article parameters read by fighter code, Item terrain/audio and stage identity,
dynamic authored child selection, resource-joint animation removal and root-joint
scale. Address these as generic engine/source-data operations; do not bypass
rejection or substitute stock fighter callbacks to accept the package.
