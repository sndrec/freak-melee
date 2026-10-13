# Samus grapple interpreter authoring

This source composition maintains the real constructor, healthy solver,
fighter tether policies, common ground Catch hooks and Article control signals.
Fresh Workshop drafts use `examples/samus_full.fms`, the complete independent
export of these components. The focused fixture composition omits activation.
Exhaustive lifecycle acceptance and coordinated visual review remain open.

Current evidence: [grapple checkpoint](../../../docs/native_source_samus_grapple_interpreter_checkpoint.md) and [owned lifecycle checkpoint](../../../docs/native_source_samus_lifecycle_checkpoint.md).
The [normal-loader gameplay checkpoint](../../../docs/native_source_samus_gameplay_checkpoint.md)
adds actual stock/interpreter replay comparisons: both facings of grab,
air grapple, all throws, wall hang expiry and terminal pull, modified ground
grapple with an active endpoint capsule, and modified wall hanging/pull with L.
Separate chain-node positions, velocities, collision history and model hashes
are compared as well as the fighters and main item. Damage interruption and
two live-chain deaths followed by ordinary grapple reuse are covered too.
The [grapple policy checkpoint](../../../docs/native_source_samus_grapple_policy_checkpoint.md)
adds input-code/hang-timer VM comparisons and the authored air wall-contact policy.

```powershell
python tests/native/samus_grapple_constructor/interpreter.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --tool build/native/native_content_tool.exe --base-project A:/FreakMelee-test-archives/samus-source-authoring-oct10-b --output A:/FreakMelee-test-archives/samus-interpreter-next
```

Use a fresh A: output directory and a private copied Samus project. The runner
audits all composed bodies, strictly compiles its fixture and compares the exact
InitializeGeometry helper through the production VM with the native constructor's
pre-allocation arithmetic slice. It also executes composed load, attribute-reload
and primary-death callbacks, plus the grapple code-sequence helper, against the
whole stock bodies. The modified hang helper is compared with the unchanged
native conditional/timer slice before ground/attack exits. Receiver values,
setup/visual leaves, metadata and imported tuning
are controlled; this does not prove node construction or full solver/gameplay.
Inputs are pinned before/after. All subprocesses are hidden, without game/audio.

Source authoring remains split between examples/samus_wip.fms, the fighter and
ground/lifecycle components in samus_tether_composition, the healthy Article component in
samus_grapple_article_composition, and this folder's fighter.constructor.fms,
constructor.component.fms and controls.component.fms. The Article has 35 fields
(353 aligned bytes), 32 helpers and capacity for 90 nodes. Both attachment paths
use owned raw joint51. Retail animation arrays select index0; absent channels
use None. Malformed geometry and failed resources use explicit cleanup policy;
collision group0 replaces native uninitialized spawn data.

Historical generated-C tests stopped on October7 at the incorrect attachment
index. The obsolete run.py is removed. fixture.py, test.c and shared_math.h.in
retain native comparison bodies and controlled leaves for the next interpreter
runtime cohort. Older receipts do not accept the current edited components.

## Actual-client acceptance batch

```powershell
python tests/native/samus_grapple_constructor/gameplay.py --iso 'C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso' --base-project A:/FreakMelee-test-archives/samus-source-authoring-oct10-b --output A:/FreakMelee-test-archives/samus-gameplay-next
```

Use a fresh A: output. The candidate is packaged and accepted only in that
directory's isolated collection. Each replay launches the native client hidden
and muted, checks both actual fighters against stock, and requires covered
resource states and cleanup. `--case` selects named cases for development;
the default runs all 28 and writes a complete `receipt.json`. Input mutation
during a run invalidates the receipt even if individual comparisons pass.

## Workshop template export

```powershell
python tests/native/samus_grapple_constructor/compose.py --independent examples/samus_full.fms
python tests/native/samus_grapple_constructor/compose.py --check-template examples/samus_full.fms
```

The `native_samus_template_source` CTest checks that the shipped source matches
its separately maintained components. The exported template owns all behavior;
the copied C Definition remains an asset-authoring reference, like other stock
clone bases. Existing user drafts are not overwritten.
