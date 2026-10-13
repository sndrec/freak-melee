# Interpreted multi-jump and collision-box checkpoint

The ordinary interpreter now supports these generic engine operations:

- `jumps.configure_multi(...)`: thirteen typed tuning values publish the
  fighter's own common multi-jump table through `nativeFighterJump_ConfigureMulti`.
  Valid configuration preserves jumps already used and sets the total maximum
  to the configured aerial count plus one. Invalid ranges return false without
  mutation; wrong types/nonfinite values reject the script call.
- `jumps.multi_vertical_speed(index)`: reads an impulse from that table through
  `nativeFighterJump_MultiVerticalSpeed`, including native zero results for an
  unenabled/unbound table, invalid count or out-of-range index. A non-null bound
  table must use instance-owned attribute scratch.
- `air.drift_threshold`: reads the engine's common `x258` horizontal-stick
  threshold. It does not substitute a fighter parameter or alter the value.
- `physics.air_drift_with_limits(threshold, acceleration, speed)`: calls actual
  `ft_80084E1C`, including fastfall/gravity, friction and clamped horizontal
  acceleration. This is not just a horizontal-velocity assignment.
- `collision.air_box_ledge(direction, top, bottom, left_x, left_y, right_x,
  right_y)`: calls actual `ft_8008239C`, preserving facing-based box mirroring,
  ledge direction, cooldown/disabled-ledge branches and environmental response.

Native calls use retained Fighter/program/storage identity and validate output
publication. Multi-jump configuration and queries allocate nothing. No
fighter-specific handler, generated module or script API alias is introduced.
Jigglypuff's Rollout template now explicitly converts its Float facing value
to the native Int direction argument; this is the conversion the C reference
previously performed implicitly.

## Evidence and limits

- Connected compiler/actor/VM cohort: **101,698 checks / 9,563 cases**, including
  **181 new cases** for tuning counts, optional motion ranges, all impulse rows,
  invalid/absent tables, drift/fastfall branches, mirrored collision boxes,
  ledge blocking and invalid arguments/source contexts.
  `A:/FreakMelee-test-archives/script-multi-jump-policies-oct10-final/receipt.json`
  SHA-256 `ca195e020b1eada08c160c2194003d5a23cff5bb28d0788f2b1b045b907394ae`;
  **1,821 independently verified pins**.
- Existing guard regression with the actual shared jump implementation linked:
  **17,635 checks / 2,983 cases / 2,760 native pairs**.
  `A:/FreakMelee-test-archives/script-multi-jump-guard-regression-oct10/receipt.json`
  SHA-256 `41b2c6b1bef880f622c1400c9d4c22c1f90164e8a79c83aa1458bfa182825a07`;
  **1,750 independently verified pins**.
- Read-only roster audit: still **19/26 forms prepare**. Jigglypuff's 192 visited
  bodies now report **3** failing bodies, down from 6: the Rollout collision
  body reaches its next missing operation, `effects.spawn_oriented`, and the
  accessory load/remove bodies remain blocked on attachment methods.
  `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-multi-jump-final/receipt.json`
  SHA-256 `e88acc2a6d0b02a895276889f5a989a952d32acef08be3bfd391c55e981c1d66`;
  **206 independently verified pins**.
- Independent report:
  `A:/FreakMelee-test-archives/multi-jump-independent-pins-oct10.json`.
- Seven Windows targets build; the reference `native_fighter_script` CTest passes.

The fixture runs actual multi-jump, drift, acceleration/friction, fastfall,
box-mirroring and air-box bodies. Publication data/motion definitions are
controlled. Stage contact/environment response and sound playback are recorded
leaves; there is no full stage collision or audible/rendered jump proof. The
source host routes the threshold property directly to its actual common
property backend. This does not execute the whole production property
dispatcher, which is built separately. No visible/audible game launch or
installed-content change is claimed, and preparation remains distinct from
complete fighter gameplay acceptance.
