# Unregistered Chef and food composition

This is a console source comparison, not a selectable Game & Watch template.
The synthetic base exposes only Chef motions353/354. No raw catalog/base,
Workshop registration, default template or installed package is modified.
Paper rendering, palette selection, actual assets and all other moves remain
unimplemented here. `candidate.fms` must not be promoted as a full fighter.

Reproduce on Windows with the immutable52 tool/SDK/compiler:

```powershell
python tests/native/gamewatch_chef_composition/run.py --snapshot .local/baseline52 --output .local/gamewatch-chef52-review
```

The runner verifies every snapshot-manifest input, generates real C with the
frozen content tool, and compiles that C against the supplied frozen headers
with direct Clang and vcvars14.44. It never uses script-build's embedded live
include paths, opens a game or writes inside the snapshot. Outputs, binaries,
generated C and logs stay in the requested scratch directory.

The reference includes the complete unchanged `ftgamewatchspecialn.c` and
`itgamewatchchef.c`, plus mechanically extracted actual `it_80272860` and
`random_int_excluding` bodies. No original callback body is rewritten.
`original_inputs.json` pins these source identities to the audited52 input.
The broad matrix compares actual generated callbacks to those original bodies.

Accepted52 source result: **742,990 checks, zero failures,38,070 pairs**:

- 7,166 fighter pairs cover both entries/eight phases, command combinations,
  held/pressed B, disabled-loop latching, animation completion, fractional and
  exhausted caps, entry resets, preserved collision/loop frames and exact flags.
  Accessory cases cover two-result histories (including duplicate/out-of-range
  history values), both facings, all five selected food variants, allocation
  failure and exact RNG call count/bound/state advancement.
- 30,254 food pairs cover both states/all six phase slots, all five variants,
  lifetime thresholds, rotation, zero/nonzero horizontal velocity, every stage
  mask0..15 and all seven interactions. They verify wall reflection precedes
  expiry, zero horizontal speed avoids a stage probe, absorbed writes life0
  after expiry, expiry preserves Z and blinking uses recursive visibility.
- 650 edited-tuning pairs cover signed/zero gravity, terminal thresholds,
  signed-zero velocity and edited loop frames. Reference attributes receive the
  same owned binary32 parameter bits as generated code, avoiding a false test
  difference from separately recomputing decimal constants in C.

All compared finite scalar results and recorded calls are exact. HSD_Randi
uses a controlled provider shared by both families; the test checks call bound,
count and provider-state consumption, not the global engine RNG implementation. These counts
do not imply bitwise replay, pixels, real allocator capacity or every possible
float input. No undefined source state is imitated: food variant is0..4;
history values outside that range are defined exclusion inputs and are tested.
The fixture FMP contains editable controlled values, **not claimed ISO defaults**.
Real PlCo/article attribute import remains part of the later copied-base batch.

Controlled engine boundaries are allocation, HSD world position/matrix dirty
propagation, animation evaluator and state transition, stage-mask probe and
shield/reflection common interaction. Constructor checks verify the actual
defined spawn tuple (bone previous position with Z0, current owner center,
parents, initial collision, group, velocity and damage). They do not compare
uninitialized unused bytes of the original SpawnItem stack object.
The actual source reflection inline and actual callback-generation registry
execute; its low-level reflection body is controlled. These are healthy source
pairs, not destructive-callback acceptance. Core lifetime tests own that proof.

Paper setup is explicitly narrowed to its existing common inherited-scale
step. Body/outline materials, controller tint, three pass-local mesh swaps and
rendering are excluded. Original `it_8027CE64` is a controlled fixture boundary;
it is never called by authored code. The actual initial material/RNG/allocation
ordering must be separately checked when reusable paper metadata/rendering is
implemented. The Chef history death handler is an unregistered **two-field
slice**; the complete nine-prop damage/death chain is not translated or accepted.

No new generic operation was needed for RNG, collision or lifetime. One syntax
detail: current item gravity DSL supplies the default ItemAttr values; the
candidate directly authors the original small sign/terminal scalar formula for
variant tuning. The existing Host operation already supports explicit values.
Item joint channels use `.rotation.x` and recursive `.visible`.

Shared proposal and whole-fighter scope are recorded in
`.local/gamewatch-chef-next-batch/report.md`. Required next engine work is owned
solo base/assets plus generic immutable layered-render lists/palette/material
contracts; later complete normal entries and prop/lifecycle batches remain open.
