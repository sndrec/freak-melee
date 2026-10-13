# Actual interpreted Item owner fixture

```powershell
python tests/native/script_item_actor/run.py --compiler .local/baseline72/compiler --output A:/FreakMelee-test-archives/script-item-actor-proof
```

The runner requires a fresh A: directory, uses CREATE_NO_WINDOW, bounds each
subprocess to120 seconds, and retains complete logs/commands plus production/
fixture/header/compiler hashes. Shared runtime scratch compiles separately with
MELEE_ROLLBACK_HOST_STATE; owner/GObj units use the actual forced .mstate section.
No game, asset loader or renderer launches.

Current receipt:
`A:/FreakMelee-test-archives/script-item-actor-current-review/receipt.json`.
**32 cases/1193 checks pass**, including:

- Actual FMS compilation and scoped entity state, Bool/Void/payload dispatch,
  two source owners sharing one physical kind and two recipes sharing NativeHoldable.
- Explicit transient callback activation, owned helper/event ABI rejection,
  shared native pointer takeover and exact five-slot state reset behavior.
- Normal authored continuation after a state transition; native phase Bool removal
  result remains its actual contract.
- Packed replay, source peer resurrection/reuse, immutable/typed/duplicate/length/
  Bool state rejection and unchanged full actor/index state on failed load.
- Real GObj proc/deferred-free ordering: Destroyed completes state writes, owned
  view retires, parent VM stops, native userdata cleanup finally detaches.
- Fault retention with attached ownership, missing Destroyed, generation0,
 512-slot exhaustion, empty/full occupied snapshots and host scratch exclusion.

Actual production parser/compiler/storage/event/payload/execution/VM and actor/
snapshot code runs with full GObj constructor/queue/deferred-delete/userdata units.
`boundary.c` controls unrelated native primitives, other owner scene callbacks,
immutable spec publication/assets and the native Item payload provider/removal leaf.
It does not simulate the full Item asset constructor, stage/collision/rendering,
root program publication or factory. The peer restore test restores real GObj
pool records plus controlled Item payload and scheduler roots before restoring
the interpreted owner; actor load does not resurrect those external owners itself.

The512 active fixture records pack137232 bytes using a12-byte typed Article state.
The actor reservation is4386816 bytes plus8192 bytes of fixed object-to-actor
index. These are component quantities; no whole match size or timing claim follows.
See `docs/native_script_item_actor_ownership.md` for contracts and remaining scope.

Creation value is guarded actor-owned F32 state, default0, with a finite setter
and packed finite validation. Getter failure preserves output; nonfinite setter
and first/last-record snapshot corruption leave all actors unchanged. Current
acceptance: `A:/FreakMelee-test-archives/script-item-creation-value-actor2/receipt.json`,
PASS1219 checks/38 cases. Factory callback timing is covered by the connected
child fixture, not this controlled native payload provider.
