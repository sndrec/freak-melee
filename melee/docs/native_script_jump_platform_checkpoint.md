# Interpreted shared jump and platform-drop operations

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10

## Change

The ordinary interpreter now executes `jump.prepare(input, short_hop)`,
`jump.check_short_hop()`, `jump.launch(play_sound, multiplier)`,
`physics.jump_motion()` and `state.platform_drop(motion, flags, frame)`.

Preparation assigns the original common startup fields. The remaining jump
operations use the existing native short-hop, launch and jump-physics routines.
They preserve released/profile jump inputs, full/short-hop velocity, horizontal
momentum and clamping, stick timer reset, optional sound requests and the first
physics callback's gravity/drift delay. Scripts still choose startup timing,
motions, ground/air transitions and carrying/victim policy.

Platform-drop entry uses the original common routine. The existing movement
binding validates its actual owned motion tables before entry. Native entry
retains its Fighter/program/storage lifetime across motion change and stops
before floor-skip/timer updates if that change retires the actor. This adds no
healthy gameplay math or transition-policy changes. The jump binding likewise
retains source identity before publishing a result or allowing VM continuation.
No additional forwarding module or runtime allocation was introduced.

## Evidence

- Seven configured Windows targets build; `native_fighter_script` CTest passes.
- Combined capture suite: **79,237 checks / 8,270 cases**, adding **23,054 checks /
  1,516 cases** to the preceding input-query checkpoint.
- **1,480 healthy native/interpreter comparisons** use bodies pinned at
  `95d8528b3`: 360 short-hop detection cases, 1,000 launch cases, 12 first-frame
  physics cases and 108 platform-drop cases. Function/internal-call names are
  the only reference body alterations. Direct bindings and compiled helpers
  match whole Fighter payloads and applicable native call order.
- Eight preparation cases match the original authored two-field assignment
  recipe. Twenty-six rejects cover typed/count/range arguments, non-finite
  launch/frame values, absent sound/common data, owned motion-table mismatch
  and invalid source identity without output publication or Fighter writes.
- Two platform-entry retirement cases change actor generation or published
  program identity during motion change. Both stop native floor/timer updates
  and the following interpreted statement.
- The ten-source census still prepares Captain Falcon, Ganondorf, Marth, Roy,
  Luigi and Yoshi. DK advances to `damage.update_timer()` at line 825. Bowser
  retains the callback-owner correction and attached-capture initialization
  boundary at line 597. Fox/Falco retain their entity metadata/owned asset gaps.
- No installed assets changed and no game window opened.

## Receipts

- `A:/FreakMelee-test-archives/script-jump-platform-oct10-f/receipt.json`
  SHA-256 `eec731a0dfcd546e5c468c08bcc51e6510d2a5c728d47172f47bac060b05bbf5`
  independently rehashed **1,801** unchanged input pins.
- `A:/FreakMelee-test-archives/source-jump-platform-oct10-a/receipt.json`
  SHA-256 `388cb3c5bddcc396fa8ac7145cd573e65bf4cb6384a2ebcd06ddf3e7e41b00f6`
  independently rehashed **27** unchanged input pins.
- `A:/FreakMelee-test-archives/jump-platform-independent-pins-oct10.json`

Fixture attempts `script-jump-platform-oct10-{a,b}` failed during audio-leaf
setup; `c` lacked the native sound-constant header; `d` lacked unrelated retained
jump-observation symbols; `e` exceeded the fixture's fixed source buffer.
These are failed harness attempts, not acceptance evidence. The fixture setup
and test-only source capacity were corrected; `f` is the passing pinned run.

## Limits

Audio, ground-to-air clearing, drift clamping and subsequent air physics are
controlled fixture leaves. Native launch arithmetic, short-hop decisions,
first-frame dispatch and floor-skip assignment execute through the production
binding/compiler/VM paths. Real platform collision, rendered jumping, the whole
airborne physics path, complete DK/Bowser activation and compact whole-match
rollback performance remain open.
