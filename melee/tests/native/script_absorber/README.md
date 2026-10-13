This hidden console fixture links the actual source frontend, preparation,
publication, Fighter actor, VM, receiver, fixed GObj pool and absorber backend.
It compares native `ftColl_CreateAbsorbHit` field changes against interpreted
creation, including raw joints 43/104, invalid-index no-op, negative radius,
signed zero and nonfinite Float values. Creation leaves accumulated damage,
hit count and position-update policy unchanged.

The exact Fighter Absorb dispatch branch and subsequent counter resets are
extracted from `Fighter_ProcessHit_8006D1EC`. Both action and move declarations
run real source callbacks. Missing hooks remain neutral without stock fallback;
fault, removal, native generation changes and source-identity changes stop the
native tail. Separate foreign native and separately published actor cases use
the real Object receiver/VM to read/write scalar absorber fields; those access
paths require no shared source rig. Packed actor state restoration explicitly restores the native
Fighter peer separately.

Imported rig/package selection, property entry routing, position writes and
player HP/statistics leaves are controlled. Artificial schema mutation at the
pure position-write boundary tests the native continuation guard; it is not a
claim that an unmarked pure write terminates subsequent source statements.
The actual absorber guard and
native Heal implementation execute; Heal's existing dispatch case is extracted
without its unrelated combat branches. This does not prove collision geometry,
item absorption, the complete hit-processing tail, rendering or game parity.

Run from the repository root with a fresh private archive:

```powershell
python tests/native/script_absorber/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-absorber-final
```
