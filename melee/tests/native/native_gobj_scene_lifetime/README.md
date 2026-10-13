# Source GM scene transition lifetime fixture

See `docs/native_gobj_scene_lifetime_audit.md` for the actual native route and
entry helper audit. This namespace tests the complete current source GM scene
runner/preload/routing bodies with actual HSD fixed-pool lifecycle operations.
Mode data, preload heap/assets and scene/frame payload hooks remain controlled.
It does not execute actual CSS/results/practice UI or imported scene assets.

```powershell
python tests/native/native_gobj_scene_lifetime/run.py --compiler .local/baseline72/compiler --output A:/FreakMelee-test-archives/native-gobj-scene-lifetime-check
```

Passing draft receipt:
`A:/FreakMelee-test-archives/native-gobj-scene-lifetime-second/receipt.json`:
13 transitions,417 assertions,13 real HSD proc and draw callbacks, one tied-match
preparation and no retirement destructor sweep. Epoch/reference/phase ordering
is checked for native home CSS->Practice->SSS->VS->Results->CSS,
home CSS->Practice->CSS and VS->SuddenDeath->Results->CSS.

Inputs/body hashes and console commands are saved in a fresh A: archive.
CREATE_NO_WINDOW is used. No production source edits or game launches occur.
