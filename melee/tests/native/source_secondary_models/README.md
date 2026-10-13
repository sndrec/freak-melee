# Owned static secondary model construction

This private console cohort links the actual already-built native game library.
It reads copied Link/Young Link source asset facts and DATs, imports the owned
Arrow descriptors at attribute offsets 36/40 and Ness Yo-yo String/Tip
descriptors at 80/84, and calls actual
`HSD_JObjLoadJoint`, matrix traversal and `HSD_JObjRemoveAll`. It does not replace
production functions with successful fixture leaves.

The runner requires `--ness` in addition to `--link` and `--young-link` copied
drafts. The Yo-yo tip includes a custom TEV record with no active channels:
native `TObjMakeTExp` ignores that record when building expressions. The census
now accounts for the allocated record without inventing an active shader path.
The real tip also plans the complete 16 x 90 resource graph capacity, including
construction headroom. This checks size planning; the console cohort itself
constructs bounded ordinary/reserved comparison graphs, not 1,440 live graphs.

The fixture initializes a real 16 MiB OS/HSD heap, object allocators, ID table
and standard JObj/DObj/PObj/MObj/TObj class metadata at a transition. It requires
explicit native material-environment admission; descriptor bit 128 is not
silently cleared. `PrepareStatic` runs the actual checked material compiler in
the prebound reservation before construction. Modified standard MObj load and
shader methods and a nonempty global toon image each reject preparation without
changing the output, reservation bytes or active-claim state. Restoring the real
environment restores admission. Each graph is compared under ordinary allocation and a fixed
reservation sized for two complete constructions plus two lazy scale traversals.
The cohort checks real material/TEV graph counts and transform/matrix fields,
reserved payload addresses, no OS heap growth during claims, exhausted third
construction preflight without mutation, replacement headroom, freed-slot reuse
and all bins empty after destruction.

Loading a duplicate descriptor changes the real global ID mapping to the newest
graph. Removing the old graph preserves that mapping. Removing the newest graph
does not restore an older mapping, so a failed replacement cannot safely be
rolled back merely by destroying the candidate. Both orders are tested.

```powershell
python tests/native/source_secondary_models/run.py --compiler .local/baseline72/compiler/clang.exe --link A:/FreakMelee-test-archives/fighter-callback-source-census/fighter-2 --young-link A:/FreakMelee-test-archives/fighter-callback-source-census/fighter-3 --output A:/FreakMelee-test-archives/source-secondary-models-review1
```

The runner never builds shared targets or launches a game. Its hidden compiler
and console processes use a fresh private A: directory. It fingerprints the
compiler, actual linked libraries/DLLs, native/SDK source headers, this fixture,
and copied draft inputs before and after the run. The SDK's duplicate
`MTXPerspective` array-bound declaration warning is suppressed; all other
fixture warnings are errors.

This is static graph construction/lifetime evidence in the initialized standard
class and empty toon/shadow environment. It does not prove source metadata
acceptance, arbitrary materials, rendering, Item actor scheduling, game behavior,
production replacement transactionality or complete rollback ownership.
