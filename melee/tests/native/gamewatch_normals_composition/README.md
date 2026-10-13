# Unregistered Game & Watch normal props and lifecycle

This namespace composes the immutable Chef52, specials53 and contracts56 sources
as dependencies. It does not register a fighter, modify accepted components or
run a game. Generated C is compiled with the supplied snapshot's Clang and SDK;
the content tool's embedded live-header `script-build` route is not used.

```powershell
python tests/native/gamewatch_normals_composition/run.py `
  --snapshot .local/baseline57 --output .local/gamewatch-normals57-root
```

The receipt verifies every snapshot hash, original source hashes and accepted
dependency hashes, then records actual generated C and fixture hashes. Console
compilation selects `vcvars_ver=14.44`. Binary outputs and logs stay in `.local`.

`fighter.fms` owns six prop handles, the saved Manhole-held handle, all clears,
the complete damage chain, Greenhouse reuse and air-prop landing callbacks, death
reset, charge initialization, item presentation and full-bucket color reset.
`props.fms` owns Greenhouse/Manhole/Torch/Parachute/Turtle/Sparky article policies.
`pending_entries.fms` is a complete **uncompiled** normal-entry/48-phase draft,
with five pure common IASA bindings; it is not included by the runner.
`compose.py` combines readable sources in declaration order and replaces the
accepted three-prop damage chain with the complete nine-prop chain only in output.

The console matrix passes 4,164,287 checks over 53,612 grouped cases against
unchanged extracted stock bodies and actual generated callbacks. It covers 39,264 article animation/pickup/destroy/pause/
resume cases; 4,096 nine-prop cleanup cases across all 512 live handle masks,
saved held-item restoration and ground/air cleanup; 7,168 reuse/landing accessory
cases; 3,072 death resets plus 12 item/color/load-slice cases. Two of those 12
check authored load initialization only; they do not compare whole OnLoad.

Presentation, animation, destruction and held-item operations are controlled
engine boundaries. The destruction boundary executes each actual stock or
generated `destroyed` callback before retiring the item. The actual
`native_callback_lifetime.c` list registry validates source generations. Unknown
or unexercised Host operations trap rather than silently returning success.
Healthy owners and absent owners are compared; foreign/reflected owners, actual
allocation/attachment, stale borrowed Item pointers and paper rendering are not.

All six constructor callback drafts compile but are unexecuted. They express
actual attach/debug order and explicitly leave paper palette/outline/visibility
and frame mapping open. The primary world origins are **raw joint ordinals**
32/1/17/21, matching original direct `parts[FtPart_*]` indexing. They must not be
replaced by semantic alias remapping without source evidence.

The only lowering excluded from executable SDK57 source is Manhole's nonzero
command wind branch: the runner changes its two provisional operation lines to
`pass` and tests only command0==0. Its original child-world and wind functions
trap if invoked. Item position is not substituted for child world position.
Torch charge uses actual available `targets.smash_charge_active`; it is not a
mirrored creator variable. No complete-fighter, paper-render or gameplay claim
is made. See `contracts.md` for exact remaining entry and rendering contracts.

Important preserved source decisions:

- Damage order: Greenhouse, Manhole, Torch, Parachute, Turtle, Sparky, Judge,
  Panic, Rescue. Air-prop clears resume all surviving air props in P/T/S order.
- Explicit owner clear occurs before removal, the destroyed callback clears
  again, and the fighter helper clears after removal. These repeated boundaries
  are preserved, including held-item restoration and callback-slot clearing.
- Greenhouse/Manhole/Torch bind damage/death only when those slots are NULL;
  air-prop landing binds unconditionally. Greenhouse and Torch clear their
  handles without clearing damage/death slots; air-prop clears clear both slots.
- Turtle/Sparky landing signals test motion350, not their own351/352. Their
  retained ranges are348..351 and349..352, not two isolated motions.
- Parachute expires on exact frame30 equality; Torch appears on exact frame3.
- Death resets Judge history/impulse, Panic damage and Chef history, but keeps
  Panic charge. Load initializes charge; NULL handles become explicit -1 IDs.

The fixture is a supported source checkpoint, not the pending full normal entry
checkpoint. Paper metadata/base/import and complete versus closure remain open.
