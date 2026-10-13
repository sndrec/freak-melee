# Shared throw, heavy-item and dynamics scheduling batch

The roster audit exposed missing common-scheduler hooks after Donkey Kong's
257 bodies and Bowser's 164 bodies compiled. The ordinary interpreter now
prepares and dispatches six generic hooks at existing shared decision points:

- `throw_victim_motion`: typed holder/default-victim motion arguments and an
  Int common victim-motion result, with both capture participants retained.
- `throw_animation` and `throw_finish`: per-throw animation policy before the
  common tick, then Bool completion arbitration after the animation ends.
- `heavy_pickup_finish` and `heavy_item_damage`: completion and reaction policy
  at the shared heavy-item branches, without replacing acquisition or damage
  calculation.
- `dynamics_ground_collision`: Bool secondary-bone floor policy before solver
  execution, with the existing retained dynamics graph checks.

No fighter-specific API, donor callback delegation, generated-C execution or
new script syntax was added. Common policies handle absent hooks; declared
callbacks own their decisions. VM faults and retired actor/program/storage
lifetimes stop cached continuation. See [catch/throw hooks](scripting/catch_hooks.md),
[heavy-item policies](scripting/ground_controls.md#heavy-item-policies) and
[dynamic bone policies](scripting/dynamics.md).

## Evidence

- Catch/throw/heavy-item cohort: **9,187 checks, 224 native comparisons, 18
  contract cases, 74 retirement/fault cases and nine throw mappings**. New
  comparisons cover all four outer throw schedulers and heavy pickup/damage
  selection with present/absent hooks and both Bool outcomes.
  `A:/FreakMelee-test-archives/script-throw-heavy-policies-oct10-final/receipt.json`
  SHA-256 `82531279d085df7c7bbd87f100ebcc71099a4265f7f477f41444c468f581c43f`,
  1,728 independently rehashed inputs.
- Dynamics cohort: **2,808 checks, 253 cases and 120 native comparisons**.
  New ground-policy tests exercise declared policy outcomes and 17 actual
  actor/program/root/graph mutation boundaries plus VM failure.
  `A:/FreakMelee-test-archives/script-dynamics-ground-policy-oct10-final/receipt.json`
  SHA-256 `48840b22a742ecd3c110ba48e6c8f33c4fef2949f3da1c5adc455a1917a77e8e`,
  1,713 independently rehashed inputs.
- Fresh full-roster preparation audit: **18/26 forms pass**, counting Sheik and
  Nana separately and excluding Kirby. Donkey Kong and Bowser now pass complete
  package preparation. Jigglypuff's 23 and Mewtwo's 11 first-per-body diagnostics
  remain visible together; structural resource/entity failures precede body
  checking in Fox/Falco, Pikachu/Pichu and Ness. Samus still has no valid
  independent draft in this audit.
  `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-policies/receipt.json`
  SHA-256 `141d558b476dc934dad6094d2f0c32eee7e74e483bb50277909b6cffc489c8a0`,
  183 independently rehashed inputs.
- Seven Windows targets build; `native_fighter_script` CTest and diff check
  pass. Independent verification is recorded in
  `A:/FreakMelee-test-archives/throw-heavy-independent-pins-oct10.json`.

The native comparisons execute selected original shared scheduler bodies,
with equivalent native Definition policy decisions against interpreted ones.
Motion entry, inner throw ticks, item classification and numerical dynamics
solver/transform leaves are controlled. The damage test copies the exact
heavy-item selector branch; it does not execute the entire damage pipeline.
These are scheduling/continuation checks, not whole-fighter gameplay parity.
No game window, audio or installed-content update was needed.

## Remaining grouped work

Use the [batch diagnostic inventory](native_script_multi_error_audit.md) for
presentation/color, multijump, capture/receiver and owned-entity capability
cohorts. Preparation currently stops at dependent structural stages and the
first missing final scheduler; do not count repeated metadata rows as separate
tasks or preparation passes as completed fighters. Whole-fighter/paired
behavior acceptance and compact whole-match rollback remain open. Goal active.
