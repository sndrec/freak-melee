# Normal attack primitives

This is one connected source/frontend/Prepare/VM/Fighter-actor/native-kernel
fixture for the five generic operations. It compiles the real catalog and normal
attack binding unit, exact current jab/rapid/pickup scheduler bodies, the current
collision predicate, and a frozen native rapid-jab callback reference.

```powershell
python tests/native/script_normal_attack_primitives/run.py `
  --compiler .local/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-normal-attack-primitives-fresh
```

The current source program calls:

- `attack.enter_jab(motion) -> Bool`
- `attack.enter_rapid_jab(motion) -> Void`
- `attack.update_rapid_jab(Finish) -> Void`
- `items.try_ground_pickup() -> Bool`
- `collision.air_aerial_landing_callback(Finish) -> Void`

Actual common bodies preserve pickup consumption, motion/animation order,
rapid-jab frame/cue/ready/again decisions and the collision predicate's contact
and environmental response. The direct rapid completion query replaces the
callback parameter in the shared body; the source binding runs its same-program
helper only when requested. Native Game & Watch and the explicit advanced Host
also consume that query directly. No trampoline, callback stack, native authored
source handler or forwarding shim is introduced.

The current rapid routine is compared against unchanged baseline72 native
`ftCo_800D6C60` across 64 frame/ready/again/cue/pickup combinations. Native flags,
cue consumption, operation order and completion decisions match. The frozen
function is confined to this private comparison; production has no old API.
The VM/source route additionally covers nested completion helpers, exact helper
ABI/owner checks, invalid/missing/gap motions, NULL helper rejection, stale
generation/epoch, source facts mismatch, retirement and real VM faults. Bool and
Void outputs remain unchanged after a native boundary faults. Geometry fault
tests show that the current collision predicate stops before copying cached
collision position back to the Fighter.

The original scheduler-to-source-to-common path is exercised too. Consumed jab
pickup suppresses source accessory registration; consumed rapid pickup still
allows the source's following accessory registration, preserving the distinct
authored decisions. Real Fighter packed-state save/load with its explicitly
restored native Fighter peer reproduces rapid completion. A reached helper
cannot save while its VM is running; execution unwinds to idle and later saves
normally. No completion state or stack needs rollback storage.

The immutable imported-asset publication boundary is controlled at a stable
NativeSourcePackage address. Source compiler/preparation, selected program,
typed storage, actor view, pool identity and the exact production
`nativeScriptBindings_SourceFighterLayout` checks are substantive. Native motion
table/data/facts pointers are explicitly installed by the fixture; this does
**not** prove DAT import or candidate admission. It never uses a successful fake
Prepare/Run/helper or a donor Definition for the source actor.

Ordinary engine pickup, motion, animation, hit bookkeeping, geometry and response
leaves are controlled. Current native common decision bodies remain unchanged
by extraction apart from private static-name collisions. Source values and
helpers execute the real VM and actor. The source body, actual native files,
compiler and consumed headers are fingerprinted and checked at completion.
Every child process is hidden with `CREATE_NO_WINDOW`, has a 120-second bound,
and writes only to a fresh private A: output. No game or global build runs.

This establishes the grouped primitive, completion and lifetime contracts. It
does not establish full Game & Watch assets/behavior, real stage geometry,
rendering, sound or game acceptance. Historical frozen SDK runners retain their
old callback ABI as reference evidence; the active current emitter and Host
fixture use `rapid_jab_should_finish`.
