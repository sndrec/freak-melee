# Owned Peach primary Articles

This hidden console cohort links the already-built native game library and its exact `native_source_assets_test` link dependencies. It compiles only this test executable; it never invokes Ninja, starts the game, installs content, or modifies the input cache/draft. `run.py` copies the supplied private cache into a fresh A: directory and fingerprints the library, DLLs, compiler, current production sources/headers, fixture files, input cache and package receipt before/after.

```powershell
python tests/native/source_peach_articles/run.py --compiler .local/baseline72/compiler/clang.exe --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --package A:/FreakMelee-test-archives/source-peach-live-oct09-corrected/package.json --output A:/FreakMelee-test-archives/source-peach-articles-final
```

The packet contains the actual independently copied fighter-4 FMA, fighter/animation/costume DATs and complete canonical Peach source. `nativeFighterRuntime_Load` performs real SourcePackage preparation, activation lease and table/program publication. This uses neither a dummy Definition nor a controlled successful package/spec getter. The stock fighter Article bank is left NULL; common data and common Items come from the actual ISO `ItCo.dat`, and common Fighter data from `PlCo.dat`.

| Article | Native kind | Authored states | Primary graph |
| --- | ---: | ---: | --- |
| Explode | 98 | 2 | Actual null-root anchor |
| Parasol | 103 | 3 | Actual imported JObj graph |
| Turnip | 99 | 5 | Actual imported JObj graph |
| Toad | 104 | 2 | Actual imported JObj graph |
| ToadSpore | 111 | 1 | Actual null-root anchor |

The cohort runs actual HSD heap/Obj/ID initialization, native fixed GObj/Fighter/actor scene pools, common Item initialization, `Item_CreateAuthored`, model and procedure registration, all 13 `Item_80268E5C` state setups, native animation/DATA advancement, and each canonical Animation phase through the real VM. It verifies no authored C state callbacks are installed. Parasol is bound through the current interpreted Fighter Item backend before holder-dependent phases. It also runs Turnip Created and Parasol HeldStatus, verifies immutable creator/attachment identities, rejects runtime clearing while owners live, round-trips the packed Item component, then calls actual `Item_8026A8EC` for all five (including native Destroyed dispatch, model/animation cleanup and Detach). Item refs retire, animation/proc/Fighter/GObj pools empty, and the runtime lease releases.

The retained parent is a real GObj/fixed Fighter owner with actual published assets and real Actor/VM/storage. Its payload fields are initialized explicitly; the actual Load hook executes. Handle state defaults are written through real storage because the fixture does not invoke the full Fighter Death/visual initialization sequence. This is not full `Fighter_Init` acceptance. No production function under test is substituted.

This does not run rendering, audio, stage contacts, a full tick scheduler, or the effect-producing Explode/Spore Created callbacks. Turnip Created uses the actual imported attributes and native RNG. The snapshot test mutates only actor-owned creation_value while all native Item/model peers remain live; it proves a component round-trip, not whole-match rollback or resurrection. Explicit VM-call counts exclude absent Spawned slots and indirect Destroyed/nested recipient callbacks. The known SDK duplicate `MTXPerspective` array-bound declaration warning is suppressed; other fixture warnings are errors.

The separate root live match is authoritative for gameplay/render coverage and fault-isolation limits. This fixture adds owned-graph constructor/state/removal evidence, without claiming that a previous invalid-source match crash is resolved globally.
