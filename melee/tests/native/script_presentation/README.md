# Source-backed sound and joint presentation fixture

Run from the repository root with a fresh output directory on A::

```powershell
python tests/native/script_presentation/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-presentation-review
```

Strict C11 warnings-as-errors, O2, disabled FP contraction and hidden process
creation. The receipt records input hashes, compile commands, output and exits.
No game launches.

The actual sound ID/channel bodies, native sound policy, RNG, semantic part
table, rotations, matrix dirty propagation and world-position computation
are tested. Only audio device calls, matrix construction and immutable
Definition observation are controlled. Unrelated engine families reject any
attempted execution in the standalone dependency view.

The matrix-vector source declares a 4-row array where the actual header uses
3 rows. Extraction changes only that parameter's row extent to the real
declaration; both types decay to the same pointer and the native arithmetic
is unchanged. This adjustment is recorded by source hashes and extractor.

Bulk comparisons and actual source-to-VM tests cover output publication,
duplicate/incorrect output targets, read-only policies and
retirement/replacement. Live audio/render behavior and full model/article
effect receivers remain outside this fixture.
# Source package boundary

The stock audio/rotation oracle normally returns NULL from its package getter.
Six additional cases install explicitly controlled source-rig metadata and check
readiness, exact data/table ownership and joint-count rejection. These execute
the real binding checks but claim neither DAT preparation nor a real actor
scheduler. Complete source package/runtime acceptance is verified separately.
