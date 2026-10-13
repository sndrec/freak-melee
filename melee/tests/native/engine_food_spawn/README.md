# Actual engine food construction and airborne target evidence

Source-only console evidence for generic engine operations. No default fighter edits, gameplay, whole build, or food behavior replacement.

`fixture.py` extracts the complete actual `it_8028FAF4` from `itfoods.c` and complete actual `it_80274ED8` from `it_2725.c`. Neither body is replaced by an equivalent test algorithm. The food fixture controls only three dependency boundaries: `it_8026D324` availability, `it_8026BB68` caller center, and `Item_80268B18` allocation. The real global counter increment executes directly.

577 cases / 12,197 checks cover NULL caller and two distinct caller identities, four supplied positions, three caller centers, four defined signed counter values, availability false/true and allocation failure/success. Availability precedes source/position reads, including an unavailable poisoned caller plus NULL supplied position. If available, NULL caller uses supplied position, while a caller uses its center as current position and preserves supplied previous position. Facing is -1; velocity/damage/group are zero; both parents are NULL; collision flag b0 is true; only successful allocation increments the counter. Inputs stay unchanged. Floats compare exact bits, including signed zero.

Only fields initialized by the real body are projected. Its unspecified SpawnItem members and padding are never read or synthesized as zeros. This does not prove allocator/model/spawned-callback behavior, food-variant RNG or healing, or signed-counter overflow. Allocation is a controlled boundary, including the assertion that it observes the old counter before the helper's success-only increment. Generated DSL/source continuation tests belong to the separate compiler worker.

`fighter_target_test.c` is narrowly extended with actual airborne-query cases. The scoped fixture also extracts the real `nativeFighterTarget_Airborne` and exactly those new test cases into a standalone console executable. The query uses the actual fighter registry and full32-bit creation IDs. Cases cover ground/air, a later registry node, zero/missing IDs, removed poisoned userdata, a live node with NULL userdata, address reuse, IDs16777217 /0x80000001 /UINT32_MAX, and NULL registry. Other target helpers remain covered by the existing root-linked target test; this small extraction does not claim to retest them.

## Reproduce

```powershell
python tests/native/engine_food_spawn/run.py --root . --compiler .local/baseline55/compiler/clang.exe --output .local/engine-food56-root-repro
```

Use a fresh output directory. The frozen55 clang executable is the toolchain; fixture headers and source are the current56 inputs and are hashed in the receipt. The same runner accepts a frozen SDK source root, while the exact new target-test cases remain repository-owned input.

Final worker receipt: `.local/engine-food56-worker-receipt/result.json`. Worker counts are source evidence; root grouped-build/frozen56 acceptance is a separate gate.

## Root-owned CMake recipe

- Generate `food_spawn.inc`, `airborne_body.inc`, `airborne_cases.inc`, `extraction.json` with Python `fixture.py --root <repo> --output <binary fixture directory>`.
- Dependencies: generator, actual `src/melee/it/kinds/itfoods.c`, `src/melee/it/it_2725.c`, `src/melee/ft/native_fighter_target.c`, and `tests/native/fighter_target_test.c`.
- Food target: `test.c` plus generated `food_spawn.inc`; native defines and repository `src`, Dolphin includes, generated fixture includes. No additional engine libraries or alternate constructor implementation.
- Optional scoped airborne target: `airborne_test.c` with the generated two airborne includes; same include/define setup and no additional engine libraries. This executes the actual query and exact new assertions.
- Existing `native_fighter_target` already links the real runtime and will run the new assertions in the normal root suite.

No CMake, production or shared compiler changes are owned by this namespace.
