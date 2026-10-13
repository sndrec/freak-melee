# Interpreted connected floor following checkpoint

Date: 2026-10-10. Ordinary fighter/article execution uses the typed interpreter.
The generated C path remains a test reference only.

## Change

`stage.follow_floor(line, x, y, z, distance, max_gap, found, out_x, out_y, out_z)`
now invokes the original `mpLib_80056C54` geometry traversal from the interpreter.
The argument is a full native-range integral line index, five finite Floats,
and distinct writable Bool/Float output destinations. Locals and owned
fighter/article state are valid destinations. The input expressions execute
before outputs are published. The operation does not move the actor or change
its contact state. Native failure publishes the supplied origin and `false`,
matching the original exported script recipe; malformed arguments and retired
sources do not publish outputs.

The helper retains its original rounded Float arithmetic and primary/secondary
connected-line traversal. Its PPC stack-matching `sqrt_tmp - 4/-5` destinations
were outside a local array. They now point at a real volatile local Float.
The intervening-wall distance accumulator is initialized, eliminating a read
of indeterminate storage if a caller begins on a wall; ordinary floor starts
already reset it before traversal. No move-specific behavior was added.

Pickup blocking was already the generic Boolean `items.block_ground_pickup`.
Bowser, Fox/Falco, Pikachu/Pichu and Mewtwo templates now assign that property.
The Sheik/Zelda C-composition fixtures use the same spelling. The reference
exporter accepts the typed assignment/read and no longer accepts the obsolete
method. The combat documentation now describes the property spelling.

## Validation

- Native client, content tool, source family/package runtime and capture enclosure
  targets built successfully with the Windows toolchain.
- `ctest --test-dir build/native -R "^native_fighter_script$" --output-on-failure`
  passed; this is exporter/reference coverage, not gameplay acceptance.
- The stage-query fixture passed **21,459 checks / 1,930 cases / 1,746 native
  pairs / 738 VM calls / 95 negative cases**. This includes 330 new direct-native
  comparisons for connected floor following plus absence and malformed argument
  cases. The real frontend, prepared package, Fighter helper and Article signal
  execute output publication through the typed VM.
- The unchanged six-source successful cohort remains Captain Falcon, Ganondorf,
  Marth, Roy, Luigi and Yoshi. Donkey Kong now prepares through floor following
  and stops at `cue.consume(3)` on line 198. Bowser passes pickup blocking and
  stops at `collision.check_facing_ground_ledge()` on line 233. Fox/Falco retain
  their article metadata/owned asset gates. No installed assets changed.

## Receipts

- `A:/FreakMelee-test-archives/script-floor-follow-oct10-final/receipt.json`
  SHA256 `36af952a618b2d6df7276ee83d2583eea940e07be3bacb30f3ab6e1c9eb97cbf`;
  all 1,754 pinned inputs independently rehashed unchanged.
- `A:/FreakMelee-test-archives/source-floor-follow-oct10-final/receipt.json`
  SHA256 `cd161e38b7f38eb203eebf436af74db175717538d689fd89a05b9f64245720f9`;
  all 27 pinned inputs independently rehashed unchanged.
- `A:/FreakMelee-test-archives/floor-follow-independent-pins-oct10.json` records
  that second pass over the receipts.

## Limits

The direct-native comparison exercises the binding's argument/publication recipe
against the same extracted engine kernels; it is not an independent mathematical
oracle or a console comparison. Native stage topology is controlled and valid.
The fixture varies flat/sloped floor chains, positive/negative traversal, gaps
through walls, ceilings, secondary links, disabled/hidden links, missing stage
geometry, and an exhausted chain. It does not admit arbitrary malformed or cyclic
map graphs. Initial endpoint admission and source generation/scene are varied;
there is no retirement observer inside this pure geometry path. A broad complete
stage solver, assets/rendering, whole DK Hand Slap behavior, and whole-match
rollback performance remain outside this acceptance. No game window was launched.
