# Actual Item scheduler and direct VM lifecycle fixture

Run from the repository root with a fresh output directory on A:

```powershell
python tests/native/script_item_scheduler/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-item-scheduler-review
```

The runner hides compiler/test consoles, writes all generated bodies, objects,
executables and logs to the requested archive, records input hashes and commands,
and rejects changed inputs. It performs no global build or game launch.

`extract_scheduler.py` selects native preprocessing branches and mechanically
extracts actual constructor/initializer, state setup, phase, static/transient
event, interaction, out-of-bounds, entered-air and destruction bodies. It does
not replace them with copies of expected behavior. `test.c` supplies real FMS to
the production compiler/typed storage/VM, and uses the real Item actor, receiver,
actor-access unit and native GObj registry/proc/deferred deletion implementation.
Tests fail unconditionally with a nonzero exit independent of NDEBUG.

The 27 cases cover source phase/common-tail behavior and hitlag gating, reentrant
state revision protection, transient native override, pickup/drop/throw/air,
hitlag, reflect/clank/shield/absorb/damage returns, touched/jumped/grab events,
external recipient import, target retirement, entry membership deletion/reuse,
newborn exclusion, pre-event owner cleanup semantics, actual immediate/deferred
destruction, missing Destroyed retirement, and constructor selection/script
failure cleanup. The runner also compiles
the complete production interaction unit and verifies its transient scratch/depth
are outside `.mstate` in the COFF object.

`boundary.c` controls asset/model/geometry/audio/common leaves and immutable spec
selection, and exposes unavailable unrelated native operation families explicitly.
Its position-write boundary triggers selected reentry/removal scenarios; authored
decisions and helper calls still execute in the real VM. Actual Item receiver views
are linked; the unowned Fighter target has explicitly unavailable owned
view/helper operations. Source Workshop protocol operations are unavailable in
this article-only executable and must be covered by the separate protocol fixture.
Rendering, real scene asset acceptance, full aggregate snapshots and external
article factories are outside this evidence.

Captured-item coverage executes the actual animation/physics/collision
schedulers for thirty ticks, preserving the owned program and storage while
suppressing all normal phases. It checks suppressed phase records and state
through the actual actor save/load path, then runs the real destruction hook
and native teardown. The native Item capture flag is restored explicitly in
this actor-only probe; this does not prove an aggregate whole-match restore.
The unowned native phase route is also checked. Shared Workshop holdable
protocol leaves remain unavailable, rather than pretending to validate them.
