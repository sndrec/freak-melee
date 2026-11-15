# Interpreted common damage entry checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10. Ordinary scripts execute through the typed interpreter;
generated C remains test-reference infrastructure only.

## Delivered

`damage.enter(motion, facing)` calls the actual common damage initializer.
The chosen Int must name an installed motion, or be `-1` for its normal
selection. `damage.default_reaction(facing)` calls the common default wrapper,
including ledge cooldown and flower/song selection. Facing is a finite Float;
zero preserves the common direction decision. Neither operation calls a stock
fighter-specific handler or creates a second knockback implementation.

Both bindings validate the exact owned fighter, program, storage, motion layout
and common data before native work and before Void result publication.
The native initializer now captures `NativeFighterLifetime`, checking it after
damage-item bookkeeping, motion change, initial animation and knockback entry.
The animation check was missing, and the prior generation-only checks could
not detect replacement of the published program. Healthy arithmetic, velocity,
motion selection, hitstun and callback setup remain unchanged.

Documentation distinguishes these implemented entries from the still-pending
`items.release_held` operation. The earlier heavy-item migration note incorrectly
described that release as available; live source checking disproved it and the
note is corrected. No substitute throw policy was introduced.

## Evidence

- Owned interpreter/capture cohort: **30,254 checks / 4,811 cases**.
- **316 native reaction comparisons** use separately extracted entry bodies
  pinned at `94abb5792`. Only function names and the inter-function call are
  renamed. The actual current angle, velocity, air-motion and shake-threshold
  helpers run in both paths. Comparisons cover forced/automatic motion,
  ground/air, zero and increasing knockback, five angles including 361, three
  facing arguments, whole Fighter payloads and native call order. Default
  wrapper cases cover eight ordinary elements with/without sandbag status.
- Compiled interpreted helpers execute both entry operations. Invalid motion
  IDs, argument types, non-finite facing, source identity and missing common
  data reject without mutation or publication.
- Retirement injections change generation or the published program at initial
  bookkeeping, motion change and animation initialization. Native continuation
  stops at that exact boundary. A real interpreted `on knockback_enter` is
  faulted through the controlled position-write boundary; the initializer
  stops before its final ice initialization and publishes no result.
- Existing combat cohort: **37,048 checks / 12,240 RNG pairs / 30 native body
  pairs**. Existing stage cohort: **22,002 checks / 1,970 cases / 1,758 native
  pairs / 750 VM calls / 114 negatives**. Their unavailable reaction-entry
  leaves abort if reached rather than pretending to implement the calculation.
- Windows client, content tool, source family/package runtime, capture enclosure
  and reference script executable/DLL targets built. Reference script CTest passed.
- Six prior roster sources still prepare. DK advances to `items.release_held`
  at line 441; Bowser remains at `articles.launch_at_position` at line 315.
  No installed content changed and no game window opened.

## Receipts and limits

- `A:/FreakMelee-test-archives/script-damage-entry-clean-oct10-final/receipt.json`
  SHA256 `f6b50bd641a234553972594ff1f0be09ebf34bae10cba41312b70c0a3898f703`; 1,755 pins.
- `A:/FreakMelee-test-archives/script-damage-entry-combat-oct10-final/receipt.json`
  SHA256 `61b51798f7deda9ac41e8d0cfc9d0c6a084149d11864cecbed1d3040dbc3ebd9`; 1,680 pins.
- `A:/FreakMelee-test-archives/script-damage-entry-stage-oct10-final/receipt.json`
  SHA256 `433c915522f2f962b27455fdb6178a2377832d2570bec2ed6f94a66db15d62bc`; 1,758 pins.
- `A:/FreakMelee-test-archives/source-damage-entry-oct10-final/receipt.json`
  SHA256 `f33033f5585d4311c7d8db0d6e71ae164ae12f92244f50940968eca4c067c0fc`; 27 pins.

All inputs were independently rehashed unchanged in
`A:/FreakMelee-test-archives/damage-entry-independent-pins-oct10.json`.
Actor/package assets, player credit, floor-normal angle, color/flash/camera,
bone rotation, item cleanup and animation/song/ice initialization are controlled
leaves. The native body comparisons establish common reaction arithmetic,
dispatch and lifecycle integration, not the full hit scheduler, SDI, rendering,
DK heavy-item gameplay, Bowser construction or compact rollback performance.
