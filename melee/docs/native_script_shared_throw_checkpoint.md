# Interpreted common throw operations — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Connected a complete group of shared holder/victim operations to the direct
interpreter, without stock special handlers or character-specific scripting APIs:

- `capture.set_victim_hold_motion(motion)` changes the held pose without resetting
  its mash timer; the native held-wait exception remains intact.
- `capture.set_victim_throw_motion(motion, rate)` enters common thrown state with
  explicit playback rate and the actual throw-victim setup hook/statistics path.
- `capture.set_victim_attached_throw(motion, flags, start, initialize)` enters the
  holder-linked attached pose with explicit flags/frame/initial-update policy.
- `capture.sync_victim_facing()` copies facing without a state or position change.
- `capture.victim_frame` observes the retained victim's frame, or zero if absent.
- `throws.update()` processes native reversal, release and animation-travel pause
  cues. Successful release intentionally removes the captured relation.
- `physics.throw_motion()` applies the native ground/air throw movement policy.

Motion operations admit the recipient's actual installed table and both actor
lifetimes. Rates/frames are finite, flags fit their native unsigned width, and
absent victims are no-ops. Common callbacks stop after retired/replaced actor,
program or capture identities; failed operations do not publish a result.

The attached throw helper's sole-use `ftCo_Thrown_Enter` inline body was moved into
its actual public entry and removed from the shared header. There is no forwarding
adapter. The ordinary thrown entry now uses the existing capture-pair predicate
instead of repeating separate holder/victim lifetime logic. Throw cue processing
retains both actors through release and pause. The post-release damage entry also
checks the victim's exact lifetime before its following DI operation, because
damage entry can invoke an authored knockback hook.

## Validation

- Full capture/throw scheduler: **34,730 checks / 5,076 cases**. This batch adds
  **202 cases** and **160 native/interpreter comparisons** for entry/tick/movement,
  plus one separately compared native post-release entry and two retirement cases.
- Comparison bodies are pinned at `ac62f15ba`; entry names/internal helper names
  are renamed only. The cue matrix uses the same controlled release leaves for
  both original and production bodies. Tests compare exact Fighter payloads,
  call order, flags, rates, frames, donor and release-credit/upward arguments.
- The matrix covers simultaneous reverse/release/pause cues, already-paused state,
  absent/present stock/scripted victims, forward/downward release, matching and
  differing victim frames, ordinary ground/air movement, and signed/zero rates.
  All newly exposed calls also execute through compiled script helpers.
- Invalid signatures/motions/recipient tables/source identities/links/float values
  preserve output and actor data. Actor generation, program identity and capture
  link retirement stop entry/release/pause continuation. The actual post-release
  body is separately extracted through the real damage initializer and controlled
  DI leaf; a retired actor/program prevents that DI call.
- Seven Windows targets build. `native_fighter_script` reference CTest passes.
- Read-only source census retains six successful preparations. DK now reaches
  `state.finish()` at line 588; Bowser reaches `capture.release_damage` at line 462.
  Fox/Falco retain their metadata/owned-asset gates. These are source preparation
  results, not whole-fighter activation or gameplay acceptance.

## Preserved evidence and limits

Paths below are under `A:/FreakMelee-test-archives`. Independent verification
rehashes every pinned input against the final checkout; both remain unchanged.

| Receipt | SHA-256 | Input pins |
| --- | --- | ---: |
| `script-shared-throw-oct10-final-b/receipt.json` | `792591f154c119e96b7b6336495f1f608a64a2b136d0cc0e0340dbf9c386ac71` | 1,765 |
| `source-shared-throw-oct10-a/receipt.json` | `94c5f525674fbc115bd8db8f79b16fb53a320af512f7cd8ecee4cb947cf25cb2` | 27 |

Summary: `shared-throw-independent-pins-oct10.json`. Earlier failed runs are
separate diagnostic evidence and are not counted as acceptance.

The initial descriptor used a fixed-float convenience macro with a different
signature; it now declares its actual Int/Float signature explicitly. Fixture
repairs kept reference cue release on the same controlled leaf, reset the new
damage mode before existing cases, and supplied the victim's reciprocal hitlag
link/generation in the older hook fixture. Actual `CapturePulled` entry establishes
those links before common throw entry; the old fixture supplied only `victim_gobj`.
Production guards were not weakened to accommodate fixture setup.

Main cue-matrix release damage/damage entry, animation propagation, attachment
geometry and ordinary movement are controlled leaves. DI is controlled in the
separate actual release-finish test. These tests do not prove release collision
geometry, full DI, rendered animation, complete cargo/command-grab gameplay,
whole-fighter activation or compact whole-match rollback. No installed assets
changed and no game window or audible process was launched.
