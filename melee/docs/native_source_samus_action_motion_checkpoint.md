# Samus action and ground-motion checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-10, following `2b9109f20`. The all-body audit covers 172 functions and
now reports **three** failing bodies, down from five. Samus remains a partial
authoring base: the package is rejected, `behavior independent` is not enabled,
and this checkpoint does not claim an accepted clone or gameplay parity.

## Shared capabilities

- [`action.current_metadata`](scripting/fighter_observations.md#actioncurrent_metadata)
  reconstructs the active canonical word from native semantic fields. It keeps
  live statistics changes, signed high-bit words and move/category/flag values
  without depending on the host union's byte order or bitfield allocation.
- [`item.slide_velocity`](scripting/item.md#itemslide_velocity) reads/writes the
  three native ground-slide components separately from the main velocity. The
  production property route checks the live owned Item and scene, changes no
  other fields and allocates no script storage.
- [`collision.ground_standard_box`](scripting/stage_movement.md#custom-standard-ground-collision)
  calls the unchanged shared `ft_800847D0` response with a stack-local authored
  box. It preserves facing conversion, collision history/position writes and
  Fall entry when contact is lost. Source retirement after a nested transition
  prevents successful output publication; completed native writes persist.

The Samus template now uses canonical metadata extraction and the namespaced
slide vector. Its bomb-jump starting animation frame is explicitly Float, as
the imported parameter can contain a fractional frame. No special behavior is
delegated to a stock Samus handler, and no compatibility aliases were added.

## Evidence

Private receipts and logs are under `A:/FreakMelee-test-archives/`:

- `samus-action-slide-oct10-b/receipt.json`: **30,913 checks**, 3,701 cases,
  4,971 native pairs and 2,232 interpreter calls. New cases compare 1,024 live
  stock/owned metadata words against the unchanged native decoder, execute
  512 interpreted metadata reads, and check 64 Item slide-vector cases through
  actual actor/VM and production routing. Tests preserve unrelated payload
  fields and reject read-only/wrong-class/stale-scene/stale-generation/dead-storage
  accesses. Asset construction is controlled; this is not gameplay acceptance.
- `samus-standard-box-oct10-d/receipt.json`: **20,185 checks**, 840 cases,
  780 native pairs and 140 actor/interpreter calls. The extended collision-copy
  cohort runs actual `ft_800847D0` and `ft_80082838` bodies, with 128 direct and
  128 interpreted box comparisons over both facings and contact outcomes.
  Stage solver and Fall-entry leaves are controlled; a Fall leaf detaches the
  real script actor to verify retirement handling. Invalid values reject
  before collision. No retail-stage or match execution is claimed.
- `samus-action-motion-checkpoint-oct10-a/receipt.json` seals the current build,
  CTest and source-audit logs, both fixture receipts, tool/source hashes and
  dependency-pin verification. All seven native build targets passed; both
  `native_fighter_script` and `native_fighter_parameters` CTests passed.

Earlier fixture archives are retained as failed/provisional evidence: the
first collision run exposed stale unselected link dependencies; the second
exposed a dispatch route through an unavailable action family. Production now
dispatches the new operation directly to its collision backend. Neither failed
run is counted as acceptance.

## Remaining audit failures

The batch checks every body independently, rather than stopping the whole file
at the first error. It still reports the first error *within* each failing body:

| Body | First remaining unsupported operation |
| --- | --- |
| Fighter `bomb_jump` | `damage.reset_callbacks()` |
| Missile targeting | `targets.fighter_ahead(...)` |
| Bomb accessory | `item.overlaps_owner(0)` |

Stock `ftCommon_8007DB58` does more than clear callbacks: it resets shared damage
bookkeeping and **invokes** damage/death cleanup hooks. Substituting
`callbacks.clear_damage_death()` would silently change behavior. That shared
operation, target-search policy and attack/hurt-capsule overlap are the next
translation gaps. Fixing them can reveal later expressions in the same bodies.
Grapple/lifecycle composition and real Samus match comparisons remain separate
work even when preparation succeeds. The user collection was not changed.
