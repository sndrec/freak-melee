# Interpreted shared walking and animation-facing ground motion

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10

## Change

The ordinary interpreter now calls the original shared walking routines through
`walk.enter(...)`, `walk.type`, `walk.update_animation()` and `physics.walk()`.
The script supplies its own three motion slots, animation lengths, reference
speeds and acceleration multiplier. These operations do not delegate cargo or
other move decisions to a stock fighter handler.

`physics.ground_animation_facing_motion()` runs the original ground-animation
physics routine using saved animation facing rather than logical facing. Native
animation displacement, deceleration and floor projection remain unchanged.

The existing common-movement module retains source/program/storage identity
across native continuations. Entry admits all three consecutive owned motion
slots before changing state. Non-finite entry parameters, zero reference speeds
and invalid current walk variants reject without publishing a result. Native
gameplay bodies were not modified in this batch.

## Evidence

- Seven configured Windows targets build; `native_fighter_script` CTest passes.
- The combined capture fixture passes **50,151 checks / 6,210 cases**, adding
  **11,659 checks / 999 cases** to the previous checkpoint.
- The new cases contain **983 healthy native/interpreter comparisons**: 15 walk
  entries, 96 animation-rate updates, 810 walking-physics combinations, 48
  animation-facing ground-physics combinations and 14 velocity classifications.
  Both direct bindings and compiled helpers run. Whole Fighter payloads and
  applicable native call order match bodies pinned at `8010badfe`; reference
  alterations rename functions/internal calls only.
- Four actor/program retirement cases stop continuation after motion entry or
  initial animation evaluation. Twelve invalid-argument/state cases leave
  output unpublished and Fighter payloads unchanged.
- The read-only ten-source census still prepares Captain Falcon, Ganondorf,
  Marth, Roy, Luigi and Yoshi. DK advances from walking entry to
  `controls.throw_horizontal_threshold` at line 686. Bowser advances from
  animation-facing physics to `capture.victim_animation_family` at line 552.
  Fox/Falco retain their entity metadata/owned-asset preparation gaps.
- No installed assets changed and no game window opened.

## Receipts

- `A:/FreakMelee-test-archives/script-walk-oct10-d/receipt.json`
  SHA-256 `e9d5be5cc33ef92d721856b35b1ccbb78ef1fc3f29a1694d1c142981a2e4ab2f`
  independently rehashed **1,785** unchanged input pins.
- `A:/FreakMelee-test-archives/source-walk-oct10-a/receipt.json`
  SHA-256 `9c00b72a77195277b20d920d4507edd0b8ca335617d2cab068fd4a3acf523325`
  independently rehashed **27** unchanged input pins.
- `A:/FreakMelee-test-archives/walk-independent-pins-oct10.json`

`script-walk-oct10-a` failed because the extracted reference functions lacked
dependency declarations; `b` exceeded the fixture's fixed function capacity.
Both are failed attempts, not acceptance evidence. Dependency ordering/includes
and the test-only function pool were corrected. `c` passed before final receipt
scope metadata was added; `d` is the final pinned acceptance run.

## Limits

The floor material multiplier, motion entry and animation evaluation are
controlled fixture leaves. Actual native acceleration/deceleration, rate
selection, slope projection and complete binding/compiler/VM paths execute.
This does not establish rendered walking, complete DK/Bowser activation or
compact whole-match rollback performance. Those remain open.
