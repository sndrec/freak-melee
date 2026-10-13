# Interpreted common action finishing and damage release

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10

## Change

The ordinary interpreter now admits `state.finish()`,
`capture.release_damage(retain_collision_credit, force_upward)` and
`capture.release_damage_fall()`. These directly invoke shared engine routines;
no stock fighter special handler or generated-C execution is used.

Finishing selects `ftCommon_8007D92C`, preserving its airborne Fall versus
grounded `ft_8008A2BC` choice. Damage release applies `ftCo_800DE2A8` to the
retained victim, then either `ftCo_800DE7C0` with the two authored options or
`ftCo_80090780` for DamageFall. The shared damage routine intentionally unlinks
the pair. The bindings retain both actor/program/storage identities rather
than selecting another victim after that continuation. A missing victim is
a no-op; a present victim requires an established mutual capture relation.

## Evidence

- Seven configured Windows targets build, including `melee_native`, the content
  tool, source-family/source-package runtime tests and reference script targets.
- Reference `native_fighter_script` CTest passes.
- Capture cohort passes **36,102 checks / 5,139 cases**, an addition of
  **1,372 checks / 63 cases**. The new healthy matrix has **42 comparisons**:
  two ground/air finish cases and forty release cases across actor ownership,
  victim absence, ground/air state, collision-credit/upward options and the
  separate DamageFall path. Both direct binding calls and compiled interpreter
  helpers execute. Remaining cases cover invalid arguments/relations and actor
  generation or published-program retirement.
- Finish and DamageFall native bodies are pinned at `f3acd8313`, with entry
  names renamed only. Ordinary release uses the original exporter's common
  apply-damage / launch sequence, whose source is included in the input pins.
  Full Fighter payloads and engine order are compared.
- A ten-source read-only census still has six passing sources: Captain Falcon,
  Ganondorf, Marth, Roy, Luigi and Yoshi. DK advances to
  `facing.animation_direction` at line 592; Bowser advances to
  `capture.drop_victim()` at line 468. Fox/Falco retain their owned entity/schema
  gates. Source preparation is not full installed-fighter activation.
- All 1,772 runtime receipt inputs and 27 census inputs independently rehash
  unchanged. No installed fighter assets were modified or game window opened.

## Receipts

- `A:/FreakMelee-test-archives/script-action-finish-oct10-d/receipt.json`
  SHA-256 `bfd61addd5a3e789a74cef627b77c159c1ba4ead9b694a8743d723425b4c5045`
- `A:/FreakMelee-test-archives/source-action-finish-oct10-a/receipt.json`
  SHA-256 `d39c40ea988e989d0c6ffff2d5a474c2b1dd035b82f9882754d3439eb49066bc`
- Independent verification:
  `A:/FreakMelee-test-archives/action-finish-independent-pins-oct10.json`

The earlier `script-action-finish-oct10-{a,b,c}` directories are failed attempts,
not acceptance evidence. They exposed a descriptor initializer error, fixture
integer-width / duplicate-leaf errors, and an invalid fixture attempt to request
a live view after identity retirement. The fixture now reads retained physical
storage for those identity-only injections; production lifetime checks were
not weakened.

## Limits

DamageFall executes its actual native entry body, with controlled parasol
choice, drift clamping and damage initialization leaves. Ground/air finish
entries, throw-hit application and normal launched damage remain controlled
leaves. This batch therefore proves dispatch, options, ordering and retained
lifetime behavior, not complete throw damage, delayed collision-credit target
lifetime, rendered gameplay, whole-fighter activation or compact rollback.
Retiring the holder does not cancel the victim's independent native DamageFall
continuation; it does prevent the caller VM from executing its next statement.
