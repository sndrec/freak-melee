# Interpreted movement input and victim animation-family observations

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10

## Change

Nine generic operations now execute through the ordinary interpreter:

- Read-only `controls.throw_horizontal_threshold`, `throw_up_threshold` and
  `throw_down_threshold` preserve the native common control values.
- `controls.walk_requested()`, `walk_stop_requested()`, `turn_requested()`,
  `platform_drop_requested()` and `jump_input()` call the actual common input
  predicates without choosing or entering a state.
- `capture.victim_animation_family` reads the retained victim's animation tag
  using the existing native owned-asset lookup. Stock victims report their kind;
  scripted victims report their own asset metadata, independently of runtime
  kind. Missing victims report `AnimationFamily.none`.

The existing movement, Fighter observation and capture modules own these calls.
No forwarding module, fighter-specific behavior primitive, allocation or native
gameplay body change was added. Directional decisions and resulting motion
transitions remain in the authored script.

## Evidence

- Seven configured Windows targets build; `native_fighter_script` CTest passes.
- Combined capture suite: **56,183 checks / 6,754 cases**, adding **6,032 checks /
  544 cases** to the walk checkpoint.
- **496 native/interpreter comparisons** use bodies pinned at `6a69c522d`:
  126 walking/stopping/turning cases, 84 platform-drop cases, 280 jump-input
  cases and six victim animation-family cases. Reference function/internal-call
  names are the only body alterations. Direct bindings and compiled helpers
  preserve whole Fighter payloads and matching typed results.
- Jump cases cover threshold equality, tilt timer expiry, tap-jump enabled /
  disabled, X/Y and explicit short-hop inputs, and tap-jump priority. The actual
  profile input predicates execute. The actual platform classifier executes
  against controlled line flags, including unrelated flag bits.
- Twenty-one threshold cases preserve Float bits, including negative zero,
  infinity and quiet NaN as observations; no guessed defaults or value clamping
  was added. Twenty-seven negative cases cover absent common data, invalid
  source/count/scene and stale captured relations without output publication.
- The ten-source census still prepares Captain Falcon, Ganondorf, Marth, Roy,
  Luigi and Yoshi. DK advances to `jump.prepare` at line 706. Bowser advances to
  an older `event.holder_airborne` callback assumption at line 597: the ordinary
  interpreter runs `grabbed` in the capturing fighter's program/storage with an
  explicit victim payload, whereas the older C exporter derived that field in
  its swapped actor context. This requires an explicit translation correction
  and attached-capture initialization, not silent actor substitution.
- Fox/Falco retain entity metadata/owned-asset preparation gaps. No installed
  assets changed and no game window opened.

## Receipts

- `A:/FreakMelee-test-archives/script-cargo-input-oct10-a/receipt.json`
  SHA-256 `6df6c6810eceb7a105fbcd867c87315b39afe1348b242e82d0f38ae6e0e795fc`
  independently rehashed **1,796** unchanged input pins.
- `A:/FreakMelee-test-archives/source-cargo-input-oct10-b/receipt.json`
  SHA-256 `a9d1604f3da30b00fdb63c6cc66726c4aebc2cbaa3e64ca8ffcff1ca11a1294b`
  independently rehashed **27** unchanged input pins.
- `A:/FreakMelee-test-archives/cargo-input-independent-pins-oct10.json`

The earlier `source-cargo-input-oct10-a` census used the build before the final
diagnostic wording adjustment; `b` is the final current-build census. All source
diagnostics remain partial migration evidence, not full gameplay acceptance.

## Limits

Floor line flags and package metadata are controlled fixture inputs. The actual
input predicates, native platform classifier, owned animation-family getter and
binding/compiler/VM paths execute. Full stage collision, jump launch,
platform-drop transition, complete DK/Bowser activation and compact whole-match
rollback performance remain open.
