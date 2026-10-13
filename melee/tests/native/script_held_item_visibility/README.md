# Stored held-item visibility fixture

Run `python tests/native/script_held_item_visibility/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-held-item-visible-review` with a fresh A: directory. Processes are hidden and no game, renderer or audio process launches.

The actual frontend, catalog, compiler, Prepare/Publish, Fighter actors, Execution/VM/storage, property routing, callback lifetime and fixed GObj pools execute. Synthetic source assets and static Fighter userdata are controlled. Unselected visual primitives abort if reached. Reads compare the exact exporter-mapped native `Fighter.x221E_b3` bit and verify complete Fighter records remain unchanged. This does not prove rendered visibility or native visibility callback behavior.

The same connected cohort now tests the three read-only horizontal smash
controls against the actual extracted `common_attribute` Float ABI. Controlled
common data covers Float bit patterns and signed integer window conversion,
including values beyond exact Float integer representation. Each value passes
through direct production properties and Fighter actor VM execution. Writes,
Article context and null common data reject; the existing source lifetime,
program, storage and retirement negatives apply to all four properties.
