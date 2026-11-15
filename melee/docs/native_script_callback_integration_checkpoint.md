# Callback constants and owned accessory integration

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The constant catalog uses binary search. Its `Callback.secondary_damage` entry
had been inserted ahead of `Callback.damage`; restoring alphabetical order fixes
the actual Link/Young Link and Sheik/Game & Watch preparation failures. The
constants fixture now handles all current native literal spellings, checks every
catalog value against its native declaration, and pins inputs before execution.

`reflector.keep_owner` now reads/writes the existing `x2218_b4` ownership flag
through the shared guard backend, retaining the owned Fighter/program admission.
It is independent of reflector enablement and descriptor behavior. The earlier
Mewtwo audit incorrectly described this as missing reflect-hit tracking; that
tracking operation was already implemented.

Mewtwo's ten accessory restoration calls now bind explicit owned Confusion,
Disable and Teleport helpers. Their bodies were moved unchanged from the motion
callbacks. These are ordinary per-tick accessory callbacks, not article-cue
gates: native Confusion polls command variables and Teleport clears itself after
its effect. Jigglypuff's Sing restoration uses the same generic callback API.
Rollout uses existing root-scale APIs; explicit integer conversions preserve
the native integer charge accumulator's truncation after Float assignments.
Mewtwo's charge-sound fraction and projectile angle are explicitly Float locals.

## Evidence

- Constants: **6,635 checks**, **1,052 native declaration comparisons** and
  17-file lexical census with no unresolved constant spellings.
  `A:/FreakMelee-test-archives/script-callback-constants-oct10-b/receipt.json`
  SHA-256 `b91399d3fd90d66d7ac434e5b06e1485c2b5e16072c717d6a9bbb48fa5e1b18c`,
  1,608 pins.
- Connected guard regression: **17,635 checks / 2,983 cases**. New reflector
  policy checks execute through the real Fighter actor/VM and shared backend;
  whole-Fighter comparisons verify neighboring enablement/behavior flags stay
  unchanged. Invalid Bool/type, generation, source and scene inputs reject without
  mutation or result publication. Outer property routing and unrelated native
  leaves remain controlled; no actual reflected-item contact is inferred.
  `A:/FreakMelee-test-archives/script-reflector-owner-oct10-final/receipt.json`
  SHA-256 `ac14c44d40351ab94699e3a1639c43e2552f4ac4f0f9e2d0a40f101ba805bc96`,
  1,749 pins. Newly referenced unrelated damage/jump leaves abort if reached.
- Full 26-form preparation audit: **16 pass**, including Link, Young Link,
  Sheik and Game & Watch. Kirby excluded; Sheik/Nana counted separately.
  `A:/FreakMelee-test-archives/source-full-roster-oct10-callback-c/receipt.json`
  SHA-256 `cdafcc6f0e20987b94a4190ece94b50040465606c7d75e277066bb40ca0a83a8`,
  51 pins.
- Pins independently rehashed in
  `A:/FreakMelee-test-archives/callback-constants-independent-pins-oct10.json`.
  Seven Windows targets build; `native_fighter_script` CTest and diff check pass.

## Remaining preparation boundaries

The ten other forms still reject at their first diagnostic: Jigglypuff's hitbox
group query, Mewtwo's release signal payload, DK's heavy-item damage scheduler,
Bowser's vector normalization, Fox/Falco and Pikachu/Pichu's owned entity metadata,
Ness's model-animation bundle metadata, and Samus's independent draft import.
These are not exhaustive inventories. Preparation is not full clone gameplay
acceptance. Complete actor activation, move/interrupt/article behavior and
paired-fighter comparisons remain open, as does compact whole-match rollback.
No installed assets changed and no game window opened.
