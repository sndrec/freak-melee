# Generic item launch and sweep checkpoint — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Fox and Falco now prepare their existing launch-data, reflected-facing,
previous-position stage sweep and pending-hitlag-clearing script calls through
generic owned-Item APIs. This is an interpreter preparation checkpoint, not full
fighter or rendered gameplay acceptance.

## Engine and authored behavior

- Read-only `item.launch_angle`, `item.launch_speed`, `item.launch_origin.xyz`
  expose the initial factory launch data with their stored Float bits intact.
- `item.reflect_facing` reads the engine's reflection-facing value independently
  of original creator, attachment owner and damage owner.
- `item.sweep_stage(previous_x, previous_y, previous_z)` calls the original
  `it_8026E9A4`. Contact replaces current XYZ position, while a miss leaves it
  unchanged. This segment query is separate from ECB flags/cached position.
- `item.cancel_hitlag_request()` clears pending `xCA8`, matching the stock trail
  damage callback; it does not stop an active hitlag timer.
- Both blaster scripts address existing receiver properties through
  `owner.fighter.action_detaches_items`, `creator.fighter.incapacitated` and
  `creator.model.root_scale_y`. No shorthand compatibility aliases were added.
- Gun state 10 copies the original creator's rendered root Y into the gun's
  XYZ joint scale. Stock `copy_jobj_scale` uses this same uniform value. This
  deliberately leaves Item instance scale and collision scaling unchanged.

The operations use the existing source/program/schema and physical lifetime
checks, with no per-call heap allocation. Stage-query inputs must be finite;
stored observations preserve signed zero, subnormals, infinities and quiet NaNs.
Native mutations already performed remain in place if the source retires before
result publication.

## Grouped verification

The focused console matrix passes **9,384 checks / 1,428 cases / 1,137 native
payload comparisons / 121 VM calls**. Seventeen authored helpers compile through
the real frontend and execute through the interpreter. New sweeps compare
complete Item payloads against mechanically extracted `it_8026E9A4`; hitlag
clearing compares against `itFoxIllusion_Logic14_DmgDealt`. Actor publication and
stage geometry are controlled fixture boundaries. Unselected matrix/material
dependencies abort if called; they cannot manufacture a successful result.

Seven Windows targets build, and the script/parameter CTests pass. Fresh isolated
clone/import audits visit all **256 Fox / 253 Falco functions**, complete metadata
and body traversal, and report **9 failing bodies each**, down from 16. Neither
fighter is admitted. Each failing body still reports its first error; fixing one
can expose another.

Remaining reported groups include rotated effects, reflector wind, stage/radio
operations, throw-shot bone segment angles, gun directional effects, and trail
secondary-model duplication. The next batch remains driven by the inventory.

Evidence:

- `A:/FreakMelee-test-archives/fox-falco-item-observations-oct10-d/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-item-observations-audit-oct10-a/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-item-observations-independent-oct10-a/receipt.json`

Observation attempt `-a` exposed stale fixture dependencies for unrelated matrix,
rig-table and friction operations. Attempt `-b`/`-c` exposed an incorrect fixture
assumption that the catalog rejected integer-to-Float numeric conversion. The
invalid-type case now uses a nonnumeric value; production numeric conversion was
not changed. Failed attempts are retained. No game process, normal installed
package or user draft was changed. Full fighter and compact rollback acceptance
remain open. The project goal stays active.
