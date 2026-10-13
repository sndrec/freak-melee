# Owned source ledge entry

`state.catch_ledge()` takes no arguments and returns Void in a Fighter source context. It invokes `ftCliffCommon_80081370` directly after validating the selected source bank, owned motion layout/data, actor storage and complete physical lifetime. `collision.cliff_catch()` retains its existing Bool query/entry behavior; the new operation does not replace that policy or invent a ledge predicate.

The native catch order remains facing adjustment, common airborne setup, motion 252, animation processing, velocity reset/ledge bookkeeping, catch physics, sound/invulnerability/rumble, floor effect and final sound. Missing floor still enters the actual shared Fall path; a healthy Fall continuation retains the original catch tail.

The shared catch helper now stops after a motion, animation, Fall or effect callback invalidates its captured caller. Animation processing also uses the same complete physical/source guard between its four existing native stages: a source VM fault with an unchanged native generation cannot continue to later animation stages. Ordinary motion changes remain healthy and do not count as retirement.

The hidden `tests/native/script_ledge_entry` fixture runs the actual source compiler, catalog, VM, Fighter actor, storage and registered GObj pool. It extracts the actual catch/physics/animation bodies and pure common field operations. Eight healthy combinations compare native arguments, order and resulting fields against the prechange catch implementation pinned to `24b898f1d`; 42 boundary cases cover deletion, same-slot replacement, userdata/generation/classifier mutation and real nested VM faults. Receipt: `A:/FreakMelee-test-archives/script-ledge-entry-final/receipt.json` (806 checks).

Package data/rig selection, geometry, full motion-change implementation, animation stage bodies, audio, rumble and effects are controlled leaves. Explicit lifecycle injection at their exits is not evidence that current audio/map reads synchronously invoke scripts. This fixture does not claim game, DAT, rendering or complete motion/command-stream acceptance.
