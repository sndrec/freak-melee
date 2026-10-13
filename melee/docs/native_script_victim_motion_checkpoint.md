# Interpreted victim motion entry — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Implemented two reusable captured-fighter operations in the direct interpreter:

- `capture.set_victim_capture_motion(motion)` calls the original common attached
  capture initializer. The victim's freeze flag, holder animation donor, attachment
  update, grab exclusion mask and initial animation update retain their native order.
- `capture.initialize_hold(motion)` calls the original common shouldered initializer.
  The damage-dependent timer retains native integer truncation, mash fields reset,
  and an already active held-wait animation is not restarted.

Neither calls a stock fighter's special handler. The script chooses the holder's
move decisions; the engine supplies common victim bookkeeping. A missing victim
is a no-op. Present victims require their own admitted motion table and reciprocal
live holder relation. Timer data must be finite and fit the native integer cast.
The shared motion-table admission previously embedded in capture entry is now a
named predicate reused by entry and these bindings, without a forwarding adapter.

The original common attached/shouldered motion bodies retain both physical actor
identities and their exact authored programs/storage across callbacks. Retirement,
program identity changes or lost victim links stop cached-pointer continuations.
Healthy motion, timer, flag and callback policy is unchanged.

## Evidence and boundaries

- Capture scheduler: **31,291 checks / 4,874 cases**, including **16 newly pinned
  native/interpreter comparisons** and **63 new cases** in total. References use
  the common entry bodies at `5cf0c21d6`, changing only function names and the
  internal call to the hold-motion entry. Tests compare whole Fighter payloads,
  native call order, freeze flags and animation donors for stock/scripted victims,
  with same/different initial motions, and invoke actual compiled script helpers.
- Invalid signatures, motion tables, source identities, victim links and timer
  data do not publish results or begin changing actors. Missing victims are no-ops.
  Actor-generation, reciprocal-link and published-program changes at motion entry
  and hold movement clearing stop the native continuation.
- All seven Windows targets build; `native_fighter_script` reference CTest passes.
- The read-only ten-source roster cohort retains six successful preparations.
  DK advances to `throws.update()` at line 587; Bowser advances to
  `capture.set_victim_attached_throw` at line 430. Fox/Falco retain their
  metadata/owned-asset gates. No complete fighter activation is claimed.

Motion loading, pose evaluation and attachment geometry are controlled leaves in
the capture fixture. These checks do not establish animated cargo/command-grab
gameplay, rendering or compact whole-match rollback. No installed assets changed,
and no game window or audible process was launched.

The earlier DK diagnostic at line 451 was incorrectly described as
`state.enter_at_speed`; it was `capture.initialize_hold` on the next source line.
Explicit-speed state entry was already implemented. The earlier checkpoints and
outer plan now name the correct gate.

## Preserved receipts

All receipt paths are under `A:/FreakMelee-test-archives`. A separate verification
rehashes every pinned input against the final checkout; both final receipts are
unchanged. Earlier failed outputs remain separate diagnostic runs, not acceptance.

| Receipt | SHA-256 | Input pins |
| --- | --- | ---: |
| `script-victim-motion-oct10-final/receipt.json` | `8080cd9a42af9ab17094b5d870c9f2bf900eac60c9e8770c9e52e7f97b6fb130` | 1,760 |
| `source-victim-motion-oct10-final/receipt.json` | `0389a475ffe8f40d52a909782f1d1ae734c9e212656fce44584236047252e98a` | 27 |

Independent verification: `victim-motion-independent-pins-oct10.json`.

The first attempt lacked the direct `fighter.h` declaration include. The next
fixture attempt exposed a controlled leaf that accepted its previous capture
flags but not the actual native `FreezeState` flag; the leaf now accepts that
native flag, and each new pair explicitly compares reference and binding flags.
No production behavior was changed to satisfy either fixture failure.
