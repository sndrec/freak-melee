# Rook air-jump closure audit

The initial read-only audit below established the actual asset and engine
contract. The generic jump APIs and Rook's authored entry/four phases are now
implemented. The user's original project remains unchanged; integration used a
private source package made from her actual DATs.

## Asset facts and original source gap

`ftrook.c` installs motion 341 with animation **295**, flags zero, default move
metadata, the shared `ftCo_JumpAerialF1_Anim`/`IASA`/`Coll` functions and
`ft_80084DB0` physics. Animation 296 belongs to Rook's up-special start at state
350. The actual user DAT names clips 295–298 `JumpAerialF1`–`JumpAerialF4`.
Changing the jump to clip 296 would change the observed asset contract.

The user Rook DAT was read through the actual `HSD_Dat` API, with native typed
`offsetof` for the common and multi-jump fields. It contains:

| Field | Actual value |
| --- | --- |
| Common maximum jumps | 2, including the ground jump |
| Common ground vertical impulse | 3 |
| Common basic-air vertical multiplier | 3 |
| Common basic-air horizontal multiplier | 3 |
| Multi-jump horizontal impulse | 1 |
| Multi-jump vertical impulses | Five values of 3 |
| Multi-jump first motion | 341 |
| Helmet first motion | -1 |
| Reverse threshold | 1 |
| Raw turn-frame integer | 1073741824 (`0x40000000`) |
| Raw multi-jump state count | 128 |

The turn-frame word also represents Float 2, but the native schema reads it as
an integer. Neither it nor the count should be silently reinterpreted or fixed.
With normalized stick values, the strict reverse test `< -1` never starts a
turn. The common maximum of two permits one air jump; the raw count of 128 is
not evidence for 128 usable jumps.

`fighter.enable_multi_jump()` still opts Rook into the shared entry path:
`ftCo_800CB870` → `ftCo_800D730C` → `ftCo_800D74A4` →
`ftCo_JumpAerial_Begin`. For the normal first air jump (`jumpsUsed == 1`), this
enters motion 341 at frame zero with horizontal velocity `input.x * 1` and
vertical velocity 3, then increments the used-jump count. Turning off this
policy and using `ftCo_JumpAerial_Enter_Basic` would produce `input.x * 3` and
vertical velocity `3 * 3 == 9`; it is not a fidelity fix.

Source asset import deliberately preserves numeric animation/flags/camera data
and clears authored phase pointers. At audit time Rook had no state-341 source
phases, so launch could enter the correct state without its completion, input,
gravity/drift and collision decisions. `moves/000_AirJump.fms` now owns that
entry and all four phases rather than reinstalling native phase pointers.

## Exact four-phase contract

| Phase | Original shared contract | Required source behavior |
| --- | --- | --- |
| Animation | `ftCo_JumpAerialF1_Anim` | Advance the turn counter; on animation completion enter fall-aerial if `jumps.used >= jumps.max`, otherwise fall. |
| Interrupt | F1 IASA calls `ftCo_JumpAerial_IASA` | Preserve its full special/item/aerial-input order and command-0 gates. |
| Physics | **`ft_80084DB0`**, not F1 Phys | Existing `physics.air_drift()` supplies the original fastfall/gravity and common drift. |
| Collision | F1 Coll calls `ft_80082F28` | Facing ground/ledge probe; on contact choose wait vs basic landing using the scaled landing-velocity threshold; otherwise walljump then cliff catch. |

`physics.air_motion()` is not the jump physics. Nor should Rook acquire Purin's
`ftCo_JumpAerialF1_Phys` merely because the animation names use that family.
Rook's actual table chose the common `ft_80084DB0` kernel.

The initial audit found a misleading control binding calling `ftCo_800CB870`,
which only checks another jump. That Bool primitive is now
`controls.try_aerial_jump()`; `controls.aerial_jump_actions()` runs the complete
**Void** `ftCo_JumpAerial_IASA`. Rook's side special uses the narrow check and
her air-jump phase uses the full input operation. The full jump IASA differs from
fall IASA: automatic aerial action is gated by command 0, and its final aerial
attack check occurs after that gate. `controls.air_actions()` cannot substitute
without changing ordering.

## Implemented source route

The source defines explicit `state_341_animation`, `state_341_interrupt`,
`state_341_physics` and `state_341_collision` callbacks. The existing numeric
layout and motion scheduler already support those code-ID phases; no new layout
or native callback registry is required. The existing Purin source helpers in
`examples/purin_wip.fms` demonstrate the generic turn/completion/collision
decisions, but Rook must retain its own physics choice and data contract.

The authored entry and turn counter use these generic contracts, now supplied
by the direct source binding implementation:

- Wire the already described zero-argument Void `aerial_jump` event into the
  fighter program/actor hook table and actual entry sites in both
  `ftCo_JumpAerial_CheckInput` and `ftCo_800D74A4`. Dispatch source code IDs
  directly; failure/retirement is terminal. No Definition lookup or stock
  special entry should select source behavior.
- Implement `jump.begin(Int motion, Float x, Float y, Float z, Bool reset_timer,
  Bool animation_driven, Bool worn_item_sound)` using the actual generic
  `ftCo_JumpAerial_Begin` primitive. It supplies the jump flag, motion flags,
  counter increment and sound contract that `state.begin` plus velocity writes
  would omit. Rook uses false/false/true for the three Bool arguments. Validate
  owned motion/data and source lifetime; the animation-driven variant needs
  separate closure before it can install a native physics policy for other
  source fighters.
- Supply `jumps.used` and `jumps.max` from the actual typed fighter fields,
  preserving integral widths. The entry computes its owned target from the
  first motion and current jump index; the phase completion reads the real
  maximum instead of inventing a cap.
- Implement the established `bones.root.add_rotation_y(Float)` operation so an
  authored `MultiTurnTick` can preserve the actual add-rotation behavior and
  midpoint facing flip. The current rotate-Y setter is not this operation.
- Implement `controls.aerial_jump_actions()` as the full shared Void IASA kernel.
  The distinct `controls.try_aerial_jump()` Bool operation exposes `ftCo_800CB870`.
- Expose `collision.basic_landing_velocity_threshold` from actual
  `ftCo_800D0EC8` and `collision.try_wall_jump()` from `ftWallJump_8008169C`.
  The facing probe can use the existing `collision.check_ground_ledge` with the
  actual facing sign; `collision.cliff_catch`, wait and basic landing exist.

Use typed imports for Rook's real extended horizontal/vertical impulse,
first-motion, reverse threshold and turn-frame words. An authored counter lives
in ordinary rollback-owned script state. Do not reuse
`nativeFighterJump_MultiVerticalSpeed` for these raw Rook fields: it returns zero
when count 128 exceeds its five-slot limit. Likewise `ConfigureMulti` both
requires a Definition and rewrites `co_attrs.max_jumps = count + 1`; invoking it
with the raw count would change the one-air-jump contract.

Preserve arithmetic width when authoring the turn step. The native code divides
`180.0F` by the integer frame count and applies `MTXDegToRad`'s F32 constant.
Use explicit Float conversion for the division before the F32 `radians`
overload; copying an unsuffixed `180.0` expression would select F64 intermediates
and can change rounding for other profiles.

Both actual native entry sites dispatch the prepared `aerial_jump` code ID.
Failure/retirement is terminal. Source fighters without an authored hook retain
the general basic/multi-jump engine entry, but cannot select native
fighter-specific handlers. See `native_script_jump_bindings.md` for the shared
API and the deliberately unsupported animation-driven variant.

Acceptance should compare the actual shared entry and phase bodies against
interpreted callbacks: one eligible air jump, exact launch velocity/animation,
no second jump, command-gated inputs, gravity/drift, animation completion and
landing/wall/ledge order, plus reentrant retirement. Include the raw unusual
turn/count words as explicit data cases. Do not claim this from resource
preparation or static hook presence alone.

The read-only DAT receipt is archived at
`A:/FreakMelee-test-archives/rook-jump-readonly-audit/receipt.json`.

## Accepted evidence and limits (2026-10-07)

- `script-jump-final/receipt.json` passes 121 checks using actual compiled FMS,
  VM/catalog, extracted shared jump entry/IASA bodies and the actual walljump
  unit. Native begin values and input order are compared, including command
  gates and callback retirement. Asset/controller, audio, motion installation
  and geometry leaves are controlled; this alone is not game acceptance.
- `source-jump-catalog-oct07-second/receipt.json` passes 39 typed catalog checks.
- `source-rook-jump-preflight-root/receipt.json` prepares the complete source:
  57 functions, five raw attribute imports and 1366 instructions.
- `source-rook-jump-snack-live-root/jump/receipt.json` runs the actual private
  source Rook package for 600 input samples in the hidden/muted client. One
  eligible air jump enters state 341 with animation 295; a further jump-button
  attempt does not create another entry. First post-physics vertical velocity
  is 2.865, preserving the launch of 3 and the normal gravity step of 0.135.
  The source actor finishes all requested ticks without a VM fault or nonfinite
  traced Fighter fields.

These receipts live under `A:/FreakMelee-test-archives/`. The live trace does
not prove every wall/ledge contact or reverse-turn asset variant. The source
preserves Rook's unusual raw turn/count words, rather than repairing them by
guess. Her installed user copy has not been replaced. Paired source families,
interactive Workshop acceptance and complete mutable rollback snapshots remain
separate project gates.
