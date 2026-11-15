# Interpreted attached capture and callback ownership checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-10. Ordinary runtime remains the interpreter. Generated C is a reference
backend, not the installed fighter's execution path.

`capture.begin_attached(motion)` now initializes the selected victim through the
shared native capture body. The holder retains its program, variables and
parameters. Cleanup, reciprocal links, timer, facing, pose donor, airborne
bookkeeping, wiggle settings and movement reset retain their native ordering.
Both actor/program lifetimes are retained across callback-capable steps, so a
removed actor cannot receive later pose or timer writes. Invalid argument types,
motion tables and stale links fail before entry.

Corrected the generated-C reference's `grabbed` callback argument naming to
match interpreter ownership: native callbacks supply `(victim, holder)`, while
authored `gobj` and `fighter` always mean the holder. Bowser and Mewtwo now query
ordinary `airborne` for the holder. Removed the old `event.holder_airborne` and
`capture.set_attached_throw` reference-only spellings.

Mewtwo's following victim throw transition uses the generic
`capture.victim_freeze_state` observation and
`capture.set_victim_attached_throw(...)`. The original common Mewtwo throw body
tests the victim's `x2222_b6`, not the holder's. The new readonly Bool returns
false without a victim and never substitutes the owner flag. These changes do
not introduce a Mewtwo-specific scripting operation.

Validation:

- Seven configured Windows targets build, including the native client/content
  tool and source-family/package runtime test executables.
- `native_fighter_script` CTest passes. The new source-generation probe checks
  the reference callback ABI and explicit victim destinations. Its initial
  attempt also used `move` handlers, which that reference parser does not admit
  for grabs; the probe now exercises its actual `action` callback path. This is
  generated-source verification, not execution of a rendered capture.
- Capture scheduler: **90,000 checks / 8,512 cases**, including **242 new cases**:
  192 healthy whole-Fighter and engine-order comparisons with the initializer
  pinned at `fc4b737ec`, 24 actor/program retirement cases across eight steps,
  16 rejected argument/link/table cases and 10 victim-freeze observations.
  Healthy initialization executes through both the binding and owned VM helper.
- Final source census still prepares Captain Falcon, Ganondorf, Marth, Roy,
  Luigi and Yoshi. Bowser advances from the old callback field at line 597 to
  flame article owner-parameter access at line 782. DK remains at damage timer
  updates at line 825. Fox/Falco asset/entity gates remain unchanged.

Final receipts, independently rehashed against current inputs:

- `A:/FreakMelee-test-archives/script-attached-capture-oct10-b/receipt.json`
  SHA-256 `d91c1a1b3dabfffa5232d649b423c568f4e118dca3493003621a4cb665acda9f`;
  **1,805 input pins** unchanged.
- `A:/FreakMelee-test-archives/source-attached-capture-oct10-b/receipt.json`
  SHA-256 `d7d58a32e9a1ff30b20902638b53fc820a919cf0a00cdfdce871fe00061c2af7`;
  **27 input pins** unchanged.
- `A:/FreakMelee-test-archives/attached-capture-independent-pins-oct10.json`
  also pins the reference exporter sources/test executable and final build log.

Cleanup, bone attachment, motion entry, animation evaluation and ground-to-air
clearing are controlled fixture leaves. Neither full Bowser/Mewtwo activation,
rendered rig capture, collision gameplay nor compact match rollback is claimed.
No installed assets changed and no game window opened.
