# Generic fighter capsule coordinate acceptance

This namespace tests `hitbox.current(IntIndex).x/y/z` and `hitbox.previous(IntIndex).x/y/z`. It does not modify or activate Sheik scripts.

`compiler_test.c` compiles the actual fixture twice: an ordinary action and `base ftDataTest 341 1` with an owned `state_341_physics` handler. Both generated modules run the same `module_test.c`. Each performs 126 cases and 1,143 checks (252 cases / 2,286 checks combined). Negative generator cases reject Float/Bool indices, Bool/unknown values, bad arity/axis/suffix, scalar calls, compound writes, Float-to-Int reads, and article/effect contexts. No Host/layout changes are required; random calls are controlled solely to observe exactly-once ordered evaluation.

`fixture.py SOURCE OUTPUT` mechanically extracts complete `ftColl_8007AD18` and `ftColl_8007B8A8` from the specified immutable source and writes `actual.inc` / `source.json`. The former demonstrates current-to-previous transfer and joint-derived update across all four states; the latter changes only joint and offset. `lb_8000B1CC` is controlled to add an explicit joint translation and offset. Whole collision traversal, resource construction, Chain decisions, native package loading and gameplay are excluded.

Reproduce scoped console tests (live changed compiler sources, frozen61 SDK/compiler):

```powershell
python tests/native/fighter_capsule_positions/run.py --root . --baseline .local/baseline61 --output A:/FreakMelee-test-archives/fighter-capsule-positions-root-repro
```

The runner performs no CMake build or game launch. Its receipt hashes compiler sources, immutable SDK/toolchain, fixture inputs, and generated outputs. Earlier failed attempts remain separate. Final source evidence is recorded by the root integration checkpoint.

CMake recipe (root-owned integration):

1. Build a compiler test executable from `compiler_test.c`, `${NATIVE_FIGHTER_SCRIPT_SOURCES}` and `src/melee/ft/native_fighter_parameters.c`, with existing native compiler-test definitions/includes.
2. Custom command runs that executable with `fixture.fms OUTPUT/generated.inc OUTPUT/generated_state.inc`; then runs `fixture.py src/melee/ft/ftcoll.c OUTPUT`. Declare those two generated C files plus `actual.inc` / `source.json` as outputs. Dependencies: compiler executable, fixture.fms, fixture.py, ftcoll.c.
3. Compile `module_test.c` twice with OUTPUT include path and native source/Dolphin includes. The owned-state variant defines `CAPSULE_STATE_ROUTE=1`; action variant omits it. No linked engine libraries are needed. Register both console executables plus compiler generation test if desired.

The script only changes selected coordinates. The complete four capsule structs are compared against explicit expected writes, including preserved state/joint/offset/damage/group/victim records and untouched capsule neighbors. Safety assertions do not claim original stock invalid-input behavior.
