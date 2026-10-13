# Direct shared movement bindings

```powershell
python tests/native/script_common_movement/run.py --compiler .local/baseline72/compiler/clang.exe --output 'A:/FreakMelee-test-archives/script-common-movement-new'
```

Fresh private A: output only. Compile and console processes use `CREATE_NO_WINDOW` and 120-second bounds. No game or global build. `extract(root,out)` is importable for a later focused CMake target; it emits `source_contract.inc` and `native_bodies.inc` with exact source hashes.

Production is only `native_script_binding_common_movement.c/.h`. Catalog/dispatcher integration belongs to the parent/inventory worker.

| Binding ID | Actual native operation | Result |
| --- | --- | --- |
| FacingFromInputThreshold | `ftCommon_8007DA24(Fighter*)` | Void |
| PhysicsForceAirAndUseJumps | `ftCommon_8007D60C(Fighter*)` | Void |
| PhysicsSwitchToGround | `ftCommon_8007D7FC(Fighter*)` | Void |
| CollisionSkipSoftFloor | `ftCo_8009A134(Fighter_GObj*)` | Bool |

All four are Fighter-only, zero-argument operations and may invalidate their source. The binding captures/revalidates full scene epoch, physical pool reference/generation, native generation, exact package/program, actor storage view, schema and parameter identity. It publishes no result after retirement/fault/rebinding; completed native mutations remain observable. Facing requires actual common threshold data. It preserves strict `abs(input) > deadzone`, including equality, signed zero and native nonfinite comparisons, rather than adding new input/clamping rules.

Ground switching directly retains native knockback/player bookkeeping, clamp/unlock/floor ordering and the actual source landing callback branch. It does not reproduce those decisions in the binding or invoke stock authored callbacks. Platform skipping retains the actual platform query and floor-skip write, rather than testing a cached source flag.

Actual acceptance: `A:/FreakMelee-test-archives/script-common-movement-review6/receipt.json`, **PASS1916 checks /229 cases /122 native comparisons /100 identity mutations**.

The fixture compiles the real production binding, native match pool and callback lifetime units; extracts the complete current catalog/source guards and exact original helper bodies (including facing, clamp, ECB unlock, ground landing path, platform query and skip). It compares full Fighter bytes, typed return and exact native-leaf event order against directly calling the original function. Cases cover input/deadzone boundaries, jump/bonus decisions, air/ground branches, offset velocity, knockback decisions and both platform outcomes. Retained identity changes during native calls include fault/logical retirement, source/schema/program/storage changes, epoch changes and same-address pool-slot reuse. Pre-invocation rejection covers invalid ownership/physical source and argument count; equivalent view/parameter replacement or a newly reused slot has no earlier local reference to compare and is specifically tested after capture.

Selected package/program/actor views, floor flags/ground predicate, player/knockback leaves and source landing callback execution are controlled boundaries. The source actor view models its captured identity rejection; the landing leaf records the exact native hook and can retire/fault the source. It does not run a complete prepared source candidate or VM landing body, actual stage geometry, sound/statistics subsystem or game. Padding-variable warning suppression is local to the unchanged extracted retail bodies; production compiles with strict warnings.

Parent integration needs the four catalog IDs/routes plus this unit's native dependencies. Isolated consumers that deliberately have no movement implementation must reject through the parent's explicit unavailable-common-movement fixture leaf; do not return fabricated success or restore a legacy behavior host.
