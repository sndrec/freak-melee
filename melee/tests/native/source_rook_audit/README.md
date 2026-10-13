# Exact Rook source audit

Run `run.py --compiler <MSVC-target clang.exe> --output <fresh A: folder>`.
The runner composes `fighter_state.fms` then sorted `moves/*.fms` exactly as
Workshop does, preserving source bytes and recording file/line mappings. It
uses production parsing, event signatures, indexing and the mechanically
extracted current production compiler context/resolver. It invokes the actual
body compiler, move lowerer and bytecode builder. No original file changes.

The 2026-10-07 baseline receipt is
`A:/FreakMelee-test-archives/source-rook-audit-oct07-final/receipt.json`:
the exact source parses and indexes **50 callbacks and 12 Float state fields**;
**35 bodies lower**, **15 fail**. Inline `if ...: return/pass/action`, untyped
locals and all Workshop Item event signatures are accepted. All 15 Item bodies
compile. This is not whole-package acceptance: the legacy `base ftDataRook 341
12` node is unsupported metadata, and the nine Item declarations have no owned
asset bindings in this source-only probe. Forty metadata nodes reject because
their owning Item needs actual assets. Eight PNG icons exist; Rook fighter,
animation and costume DATs and a valid `assets.fma` are not present here.

The motion extent 341/12 is copied from the authored base line solely for this
body census. No named special IDs, imported table or live model are guessed.
The exact 52 call-span probes in failed bodies use the shared lexer plus the
actual expression parser/resolver; inline parent/child nodes can duplicate a
span. They reveal failures hidden behind each body's first diagnostic.

| Authored surface | Canonical route / required work | Source evidence |
| --- | --- | --- |
| `random()` | Rewrite to existing `random.float()`; preserve RNG order. | Catalog `RandomFloat`; reference generator uses `engine->random_float`. |
| `state.land()` | Rewrite to existing `state.basic_landing()`. | Reference code maps to `ftCo_Landing_Enter_Basic`; current catalog/runtime already exposes it. |
| `state.land_special(flag, lag)` | Rewrite to existing `state.landing_special(flag, lag)`. | Reference code maps to `ftCo_LandingFallSpecial_Enter`; current typed descriptor exists. |
| `items.pull(state.snack_choice)` | Explicit `uint(state.snack_choice)` conversion. No implicit Float-to-integer argument conversion. | Current `items.pull` formal is U32; old emitter explicitly casts recipe index to uint16. |
| `callbacks.on_hit_state(344)` | Authored helper bound with existing `callbacks.deal_damage(Helper)`; helper enters numeric state344. | Old emitter assigns `deal_dmg_cb`; handwritten `rook_side_hit` changes motion to344, frame1/rate1. `callbacks.damage` binds `take_dmg_cb` and is incorrect here. |
| `collision.check_ground_ledge(mode)` | Genuine generic Bool primitive calling `ft_CheckGroundAndLedge` with exact I32 mode. | `ft_081B.c:202`; updates collision/position and checks interrupt. It cannot become a cached contact property. |
| `collision.cliff_catch()` | Checked Bool cliff attempt using `ftCliffCommon_80081298`. | Handwritten Rook uses the checker. The old generator's standalone statement erroneously maps to unconditional `ftCliffCommon_80081370`; parent explicitly chose handwritten intent. |
| `controls.aerial_attack_actions()` | Canonical `controls.try_aerial_jump()` Bool, calling `ftCo_800CB870`. | `ftCo_JumpAerial.c:65` checks capture, then multijump/air-jump input. Legacy Host naming says attack incorrectly. Existing `controls.air_actions` is wider Fall IASA and is not equivalent. |

Two additional parity points matter during the later authored rewrite:

- Old implicit `let` declarations and handwritten Up-B locals are Float.
  Current inference makes `max_x`, `drift`, `base_y`, `add_y` Double, while
  `ratio`, `stick_x/y`, `turn` remain Float. Explicit `let float` keeps the
  original rounding points; compilation alone does not establish numeric parity.
- Generated `authored_on_hit` adds `start_animation` after its frame1 change;
  handwritten `rook_side_hit` only changes motion. The canonical helper should
  follow the selected native intent rather than blindly copy the old emitter.

The resource/metadata closure still needs a real owned Rook asset record: roots
`ftDataRook`, `PlyRook5K_Share_joint` / matanim, 327 actions, 14 demos, 50 joints,
owned bone roles and 12 motion rows, including the state341 aerial-jump contract.
Those values are source-audited at `kinds/ftRook/ftrook.c`; actual DAT bounds and
shape must validate before publication. The nine generic holdable recipes need
owned common Item assets and eight pinned/decoded PNGs; CrimeShard has no icon
directive. No authored native callback pointers or old code-key fallback belong
in the ordinary candidate. Parent/Samus own that resource transaction separately.

This audit does not execute Rook, validate whole metadata, prove every native
phase default or infer missing resources. The proposed generic primitive work
is separate from the later authored spelling/helper rewrite.

## Implemented generic primitive slice and canonical source rerun

The dedicated native_script_binding_ledge unit now provides the three genuine
operations above, with exact Bool contracts, Source generation/owned-bank
pre/post guards and staged output. The narrow Bool name is try_aerial_jump;
no misleading attack alias was added. The actual top-level catalog dispatcher
routes their IDs before the wider enum families.

`A:/FreakMelee-test-archives/source-rook-ledge-dispatch-oct07-final/receipt.json`
records 13,941 checks / 2,304 comparisons through the actual top-level Invoke
and unmodified native decision bodies. Geometry, cliff entry and jump entry
leaves are controlled; retirement and absent owned-source validation suppress
outputs. This establishes neither real collision geometry nor full jump entry.
The latter derives motions from x2D0/ext attributes in ftCo_JumpAerialF1.c and
still requires the actual Rook imported data/state341 contract.

After the parent updated the authored surfaces, the exact canonical source
parses, indexes and lowers **51/51 functions** (50 callbacks plus SideHit), with
12 owned fields. The receipt is
`A:/FreakMelee-test-archives/source-rook-canonical-audit-try-jump-oct07/receipt.json`.
All eight Up-B locals are now explicitly Float. Root metadata is now fighter:
behavior independent; every recipe references items/base.fib. The source-only
probe supplies no blueprint, DATs, owned icons/model binding or actual facts.
Therefore 51 successful bodies are explicitly not complete resource/metadata,
package or runtime acceptance. Parent owns those gates and the authored rewrite.

Naming audit: the reference compiler already has controls.aerial_jump_actions
for the full ftCo_JumpAerial_IASA (Void), distinct from CB870. The narrow new
Bool primitive is controls.try_aerial_jump, with no alias. Parent confirmed
that distinction and changed Rook accordingly; full IASA remains a separate
source/kernel contract.

## Generic jump catalog closure

The jump runtime worker owns the native kernel and source actor entry hooks.
The catalog composes these exact public contracts:

| Surface | Type / access |
| --- | --- |
| `jump.begin(motion, x, y, z, reset_timer, animation_driven, worn_sound)` | `(Int, Float, Float, Float, Bool, Bool, Bool) -> Void` |
| `jumps.used` | Int, readable/writable |
| `jumps.max` | Int, read-only |
| `bones.root.add_rotation_y(delta)` | Float -> Void |
| `controls.aerial_jump_actions()` | Void, full shared `ftCo_JumpAerial_IASA` |
| `collision.basic_landing_velocity_threshold` | Float, read-only |
| `collision.try_wall_jump()` | Bool |

`controls.try_aerial_jump()` remains the distinct narrow Bool attempt. New calls
are effectful and carry the native invalidation flag. The jump.begin
animation-driven variant is a runtime gate: this initial source-safe slice
rejects true before mutation; a Bool argument accepted by the frontend does not
claim that variant is implemented.

The semantic/bytecode fixture at
`A:/FreakMelee-test-archives/source-jump-catalog-oct07-second/receipt.json`
passes 39 checks against the actual catalog, statement compiler and bytecode
builder. It verifies formal types, emitted IDs, writable versus read-only paths,
strict Bool arguments, fighter context and Void-condition rejection. Runtime
families are explicitly unavailable here. Actual jump engine/actor acceptance
belongs to the separate runtime worker's evidence.

Old isolated catalog fixtures link the rejecting
`tests/native/script_bindings_unavailable_jump.c` leaf. Full jump fixtures must
link the actual jump unit, not that leaf. Parent owns CMake source-list changes.
