# Dr. Mario results-pill translation

Status: the actual generated full Dr. Mario module passes 2,199 standalone source comparisons against frozen baseline11. No results-scene gameplay acceptance is claimed.

The generic demo-entry hook runs after common demo setup. Entry1 authors two presentation pills through the full Dr. Mario module: mode1 selects state5/animation4 and binds the normal held slot; mode0 selects state4/animation3 and binds the auxiliary slot. Neither mode performs pickup or bone attachment. The stock Sleep entry's mode2 remains outside this result-constructor comparison.

The constructor performs lifetime setup and two identical state transitions around appearance baking, then clears commands, installs original-creator accessory behavior, resets spin, and runs a synchronous physics/collision pass. The caller scales only after that pass. The two random color selections remain interleaved with complete creation/binding/scaling, preserving candidate order, draw count, and current/previous color state.

`tests/native/drmario_results_stock_compare.py` extracts actual production stock constructor, caller, color selection, accessory/state callbacks, and refresh body from the matching frozen SDK. It also executes production generic article creation, creator lookup, held/auxiliary binding and spawned-callback dispatch. Its standalone C fixture recompiles the actual full generated module with the frozen compiler/SDK, checks its layout fingerprint, and loads the imported parameters. Common engine stubs observe primitive arguments, order, spawn positions, state, flags, commands, transforms and pre-update snapshots. The real lifetime predicate is linked. The generated pill has no spawned callback, so the engine's spawned dispatch preceding authored `on_created` has no pill-policy effect.

Coverage comprises 2,178 ordered two-pill construction/color comparisons, six accessory/animation comparisons with both original and transferred damage ownership, one unrelated demo-mode check, one missing-creator check, four physics/collision removal/recycling checks, and nine production binding checks. It includes both facings, all first draws, valid and out-of-range previous/current colors, nonunit model/item scales, nonzero original z, distinct fighter item-center and origin, and manual-lifetime resets. Binding rejects invalid/stale handles, removed/recycled or wrong creators, and foreign package identity; transferred damage ownership remains valid.

Stock's stale-pointer behavior is not reproduced; defined creator lifetime checks are intentional. Failed-stock constructor null dereferences, mode2 pickup, model descriptor traversal/appearance rendering, and the complete demo-scene entry are not established by this fixture. Common appearance/spin/scale primitives are observed engine stubs; their internal asset traversal is not compared here. Direct field writes are compared as state effects rather than requiring the same helper call representation. Ordinary projectile/taunt runtime evidence belongs to the existing Mario/Dr. Mario ledger.

Run only with a matching already-built full Dr. Mario project and frozen toolchain:

```text
python tests/native/drmario_results_stock_compare.py --snapshot .local/baseline11 --project .local/drmario-results-ctor/baseline11-project-work/project --batch .local/drmario-results-ctor/baseline11-fixture
```

Evidence: `.local/drmario-results-ctor/baseline11-fixture/{build.log,result.log,result.json}`. The project was cloned from the actual ISO through baseline11's content tool, imported 39 owned parameters from `examples/drmario_full.fms`, and generated through the normal script-build route. The evidence manifest hashes the recompiled module, generated C, FMP, FMS, fixture, compiler, frozen manifest and extracted production sources. This runner does not build the game or start a game process.
