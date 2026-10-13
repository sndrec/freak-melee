# Owned stock demo construction and persistence

These checks exercise the real content tool in isolated A: directories. They do
not change installed user content. Use a fresh output directory for every run.

```powershell
python tests/native/fighter_owned_demos/run.py --output A:/FreakMelee-test-archives/owned-demos-constructor-review
python tests/native/fighter_owned_demos/paired_pipeline.py --constructed A:/FreakMelee-test-archives/owned-demos-constructor-review --output A:/FreakMelee-test-archives/owned-demos-pairs-review
python tests/native/fighter_owned_demos/save_undo.py --source A:/FreakMelee-test-archives/owned-demos-constructor-review/Mr/project --output A:/FreakMelee-test-archives/owned-demos-save-review
```

The constructor check calls `clone-stock` for all 26 supported stock fighters
(Kirby is excluded). An independent Python ISO/FST/DAT reader checks every
populated demo descriptor against its exact retail clip: 343 clips, unchanged
names/flags/command/blend fields, and owned aligned offsets. Intentionally empty
descriptors remain empty. Fox's descriptor 4 is populated; the oracle reads the
actual source rather than treating that entry as universally empty. Nana's wait
clip is physically copied into Nana's own bank during construction.

The paired check requires the accepted Zelda/Sheik and Ice Climbers SDK72 source
packages at the paths recorded in `paired_pipeline.py`. It pins their compiled
modules, scripts and other assets, replaces only fighter/animation DATs with the
fresh constructor outputs, and uses normal cache/accept/clone/bundle/verify
commands. Each member registers with 13/13 populated demos decoded. This verifies
asset ownership and registration; it does not rebuild or compare special behavior.

The save check grows an action clip in a private Mario draft, changes a fighter
attribute afterwards, and verifies animation undo/redo preserves that attribute.
Fighter undo preserves the current animation offsets. A Windows read handle that
denies rename makes the second file replacement fail after the first succeeds;
the current fighter/animation pair must be restored exactly and temporary files
removed. This is an operation-level rollback check, not power-loss atomicity.

Accepted receipts under `A:/FreakMelee-test-archives`:

- `owned-demos72-cli-root/receipt.json`: actual 26 constructors / 343 clips.
- `owned-demos72-pairs-root/receipt.json`: both paired package pipelines.
- `owned-demos72-save-root/receipt.json`: actual save/undo/failed replacement.

The separate `fighter_animation_editor` fixture covers editing either descriptor
bank and structural aliases. `fighter_owned_results` covers import and scene-reset
ownership. Rendered match-to-results checks are a separate acceptance boundary;
passing these console checks alone does not prove results-screen appearance.
