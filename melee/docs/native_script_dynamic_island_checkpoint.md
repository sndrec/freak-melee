# Interpreted dynamic island bounds checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10. Ordinary fighter/article scripts execute through the typed
interpreter. Generated C remains test-reference infrastructure only.

## Delivered

`stage.island_bounds(x, y, z, probe_y, found, left_x, left_y, right_x, right_y)`
now performs the original vertical floor/island lookup followed by
`mpIsland_8005ACE8`. It refreshes dynamic endpoints rather than substituting
cached island coordinates. On a missing floor, island or stage map, all four
coordinates receive the supplied origin X/Y and `found` is false. Success and
failure publish only after the source/program/storage, scene and geometry are
still current. Inputs must be finite Floats, the projected endpoint must remain
finite, and the five destinations are distinct writable Bool/Float fields.
Both Fighter and Article state, along with local destinations, are supported.
The operation does not move the actor, change contact state, allocate or draw RNG.

Dynamic endpoint admission is shared with `stage.compare_floor_islands`: validate
the selected joint's floor range and line endpoint geometry before refreshing.
The cached `stage.read_line_island_bounds` contract is unchanged.

Bowser's two old `physics.air_animation_motion()` calls and Fox/Falco's one call
now use the existing `physics.animation_velocity_plain()`. Both old and current
paths resolve to `ft_80085134`: horizontal self-velocity is TransN Z times facing;
vertical velocity is TransN Y. This is a source-backed API migration, not a new
physics algorithm. The obsolete exporter statement and unused duplicate host
slot were removed. The C reference test executable and DLL were rebuilt with
the current host layout; no compatibility entry was retained.

## Evidence

- Hidden stage-query cohort: **21,997 checks / 1,970 cases / 1,758 native pairs /
  750 VM calls / 114 negatives**. New point-query comparisons use the native
  lookup/refresh sequence separately extracted at `762ce905a`, with only function
  and internal-call names renamed. The receipt pins those reference bodies.
- Static/dynamic endpoints, absent floor/island, signed-zero origin fallback,
  exact floor-probe/endpoint counts, whole Fighter preservation, actual compiled
  helper evaluation once, and Article signals are exercised. Invalid argument
  types, overflow, stale source identity and selected island joints reject without
  output publication. Explicit fixture observers change source/geometry during
  probing or bounds refresh; none publish partial results.
- Windows client, content tool, source family/package runtime, capture enclosure,
  reference script test and reference fixture targets built successfully.
  `native_fighter_script` CTest passed after rebuilding the host-layout consumers.
- The six-source successful cohort remains Captain Falcon, Ganondorf, Marth, Roy,
  Luigi and Yoshi. DK advances from dynamic island bounds to damage-reaction
  observation at line 438. Bowser advances from animation motion to item hit-group
  allocation at line 297. Fox/Falco retain metadata/owned-asset gates. No installed
  assets changed and no game window was opened.

## Receipts and limitations

- `A:/FreakMelee-test-archives/script-dynamic-island-clean-oct10-final/receipt.json`
  SHA256 `80e18f487ae3942df42f2106dfe9d0959caaefdf618daa4cc24be3a5404f13a7`; all 1,756 input pins independently rehashed unchanged.
- `A:/FreakMelee-test-archives/source-dynamic-island-clean-oct10-final/receipt.json`
  SHA256 `1d9321203654f4f76114e157f0f93bd55138e5d989f5c947d8e1e20887cbdc06`; all 27 input pins independently rehashed unchanged.
- The second-pass verification is
  `A:/FreakMelee-test-archives/dynamic-island-independent-pins-oct10.json`.

Native intersection and endpoint-coordinate leaves are controlled. This verifies
query sequencing/publication, not actual moving-stage assets, full map traversal,
whole DK CPU behavior or gameplay parity. Native map topology is valid; arbitrary
corrupted/cyclic map tables are outside admission. The explicit retirement
observers are test injections, not a claim that native bounds reads currently
execute authored callbacks.

An additional attempt at the older standalone kinetic fixture did not compile:
its strict aggregate-initializer policy rejects the newer descriptor's implicit
zero fields (`object_or_integral`, etc.). It has not been repaired or counted as
passing evidence. Its log is retained under
`A:/FreakMelee-test-archives/script-dynamic-island-kinetics-oct10-final/compile.log`.
The current migration is supported by the unchanged common operation's source,
full Windows build, source preparation and reference export test; a fresh direct
kinetic-runtime comparison is not claimed by this checkpoint.
