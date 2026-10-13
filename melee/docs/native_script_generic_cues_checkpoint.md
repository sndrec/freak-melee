# Interpreted generic cue and ECB checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10. Ordinary script execution remains the typed interpreter;
generated C is a test reference only.

## Delivered

- `cue.consume(bit)` accepts an Int expression in 0..7 and consumes only that
  subaction signal. It returns a Boolean indicating whether the cue was raised.
  Dynamic selectors and repeat calls are supported. The implementation uses
  named native bitfields instead of assuming a PPC bit mask or byte order.
- `collision.unlock_ecb()` invokes the shared `ftCommon_UnlockECB` operation:
  clear the lock-frame counter and the collision sample's locked flag. It does
  not tick physics, query terrain, or change grounded/airborne state.
- Both operations require the retained owned Fighter/program/storage/scene;
  invalid arguments or stale source identity do not mutate or publish results.
  No callback execution or allocation is introduced.
- Facing-directed probes in the Bowser, Fox/Falco, Mewtwo, Jigglypuff and Samus
  templates now call the existing generic `collision.check_ground_ledge` with
  `1 - 2 * int(facing.direction < 0)`. This preserves the stock sign selection,
  including a zero facing value, without another special-purpose API.
  Game & Watch, Sheik and Zelda composition fixtures use the same expression.
  The obsolete reference-exporter facing convenience method was removed.
- DK's two Headbutt effects use the existing `effects.spawn_on_part` plus
  `bones.index(TransN)`; their explicit `effects.active = true` is retained.

## Evidence

The hidden capture cohort passes **18,629 checks / 3,947 cases**. Its additions
include all eight selectors over all 256 cue combinations, repeated consumption,
compiled state-defined selectors, and rejected source/type/range cases. Native
ECB unlocking is mechanically extracted and compared over 1,024 flag/counter
combinations, plus a compiled helper and rejected source contexts. Whole Fighter
payload comparisons preserve unrelated state. The cue reference is a direct
named-field recipe; it is not independent console execution.

The stage cohort still passes **21,461 checks / 1,930 cases / 1,746 native pairs /
738 VM calls / 95 negatives**. The broader combat cohort passes **37,048 checks /
12,240 RNG pairs / 30 native-body pairs**. The Windows native client, content tool,
source family/package runtime and capture-enclosure targets build successfully.
`native_fighter_script` CTest passes after the facing-callsite migration.

The six-source successful cohort remains Captain Falcon, Ganondorf, Marth, Roy,
Luigi and Yoshi. DK now reaches `stage.island_bounds` at line 400; Bowser reaches
`physics.air_animation_motion` at line 278. Fox/Falco retain metadata/owned-asset
gates. No installed assets were changed and no game was launched.

## Dynamic bounds distinction

A proposed composition of `stage.read_floor` and `stage.read_line_island_bounds`
was rejected after its native comparison failed. The existing line-bound query
intentionally reads cached island endpoints; DK's original policy uses
`mpIsland_8005ACE8` to refresh moving-terrain endpoints. The source retains its
original `stage.island_bounds` request pending the correct interpreter binding.
Cached bounds were not substituted and the trial helper was removed. The failed
trial is retained at `A:/FreakMelee-test-archives/script-generic-cue-island-oct10b/fixture.log`.

## Receipts

All inputs were independently rehashed unchanged after the final checks:

| Receipt directory under `A:/FreakMelee-test-archives/` | Inputs | Receipt SHA256 |
| --- | ---: | --- |
| `script-generic-cues-ecb-oct10-final` | 1748 | `5d1f0eb473b23dd45682588155295d61a9d747466510e424ae0e08a23ad86319` |
| `script-generic-cues-stage-oct10-final` | 1755 | `70eeb31b9daa64eb78ffbe5fbe843de7655488075909b550562fea88f875f6e5` |
| `script-generic-cues-combat-oct10-final` | 1678 | `5d0bc332570fb1a2a2aef1e9e39941dae1fd96b6a37b866d7c10a9077d455259` |
| `source-generic-cues-oct10-final` | 27 | `5ea9d30225aece69a8ad765051a5c14e2b759423cb186a754fe278fdd0ee81ef` |

Each directory contains `receipt.json`. The independent verification is recorded
in `generic-cues-independent-pins-oct10.json` at the same archive root.

These are native primitive/interpreter and source-preparation checks. Whole
fighter gameplay parity, asset publication and compact whole-match rollback
performance remain outside this checkpoint's acceptance.
