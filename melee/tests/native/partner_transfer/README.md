# Actual alternate-member transfer

This console fixture compares the complete unchanged `ftCommon_8007EFC8`
body from immutable SDK66 with live `ftCommon_ActivatePartner`. The old body
is renamed only in the extracted test translation unit. It is not a runtime
delegate. The retail destination finish and authored destination
`on_form_activated` have the same controlled effect in healthy comparisons.

Run from the repository root, using a fresh output directory on A:

```powershell
python tests/native/partner_transfer/run.py --reference .local/baseline66 --output A:/FreakMelee-test-archives/partner-transfer-review
```

The runner verifies the reference snapshot's SDK/compiler hashes before and
after execution. It compiles complete extracted live transfer and Player
role/mapping bodies, the unchanged actual common fighter reset, and actual
`native_callback_lifetime.c`, and complete actual pickup notification body
`ftpickupitem_80094818`. It records all live headers and source hashes;
this is an explicitly live-header candidate comparison against an immutable
old reference, rather than an SDK66 module acceptance claim. The bundled
reference Clang compiles the console executable without a game or GUI.

The current matrix contains 1,024 healthy pairs: 512 combinations of grounded
state, star/invulnerability, metal, cloak, two worn items and held-item presence,
for retail and independently authored pairs. Whole Fighter structures,
ordered calls, transformed roles, the intentional destination generation
advance, both persistent per-form state arrays, and original `src->x198C = 2`
assignment are checked. The common reset bodies must remain byte-identical
to the reference; it advances spawn identity but does not clear module state.

Fifteen preconditions reject missing/invalid relationships, wrong active role,
same entity, other player, missing finish, mixed retail/authored definitions,
private stock finish supplied to authored forms, inherited authored kind,
unregistered source, and dispatch saturation without Fighter mutation or
external calls. The 325 lifetime cases cover removal with PAGE_NOACCESS
userdata, generation recycling, kind/Definition/userdata/ft_data replacement
for either fighter at every returning call site, including the alternate
normal-color and visible-cloak branches. A destination finish recursively
attempting the same player's transfer is rejected. Failure and success both
balance the transient player mask and shared dispatch depth.

Thirty-three retained-item cases cover both worn slots and the held slot:
registry removal with protected userdata, recycling, same-generation userdata
replacement, kind/logic changes, source or destination binding removal, and
removal already in progress. Positive ownership updates are allowed, because
attachment legitimately moves each item's owner. Invalid input items stop
before role swap; invalidated items after callbacks stop continuation rather
than skipping attachment and falsely reporting a completed transfer.

The actual pickup body executes an authored callback which can retire the
item while the destination fighter survives. Its shared counter sink checks
that a live item still resolves before reproducing the first `itGetKind`
userdata read. That proves retirement cannot reach the unsafe counter input
or the subsequent attachment. The test deliberately leaves a dangling
destination binding after simulated registry removal, rather than relying on
removal to clean both fighters. Actual item removal only follows current
`Item.owner`; before held-item attachment that can still be the source.

## Deliberate boundaries

Collision, material/color, camera, item attachment/animation/removal and finish
operations are controlled ordered sinks. Worn-item binding sinks preserve
the actual fields written by `ftCommon_8007F948`/`8007FE84`; their inverse
geometry functions `8007FA00`/`8007FDA0` do not clear the source item pointers.
This does not validate the interiors of those engine helpers, renderer,
paired package staging, transform motion timing, gameplay or rollback.

The retained-item continuation is now guarded. Intrinsic attachment animation
internals remain a separate scoped boundary:
`it_8026B9A8` calls `HSD_JObjAnimAll` and then uses the retained item in
`it_80279BE0`/`it_802799E4` without an internal generation gate. These intrinsic
joint/material/legacy item-command paths are not represented as authored
pickup callbacks here, and this fixture does not establish destructive
retirement reachability inside them. The surrounding transfer validates the
retained item immediately after the helper returns. A failed transfer after
callbacks can leave ordered mutations
already performed; only invalid **entry preconditions** promise no mutation.

## Root integration

Root owns `compiler.c`/`module.c` in this namespace and CMake. This engine
fixture is independently run with Python; no generated module is needed.
If integrated as an extraction target, dependencies are `run.py`, `test.c`,
`ftcommon.c`, `fighter.c`, `ft_0C88.c`, `ft_0DF0.c`, `player.c`,
`kinds/ftCommon/ftpickupitem.c`, `item.c`, `it_2725.c`, `it_26B1.c`,
`plbonuslib.c`, and
`native_callback_lifetime.c`, plus their normal included headers. The old
reference is separately hash-verified and must remain immutable. The runner
captures exact compiler commands and the acceptance result in `receipt.json`.
