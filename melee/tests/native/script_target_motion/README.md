# Retained target motion query

This hidden isolated fixture runs the actual compiler, prepared Fighter/Article
programs, VM, actor/storage code, source guards and fixed GObj pools. It compares
the production query with current extracted `ftLib_8008741C` and
`nativeFighterTarget_Motion` bodies for Fighter and Article callers. Foreign
stock targets retain their own motion IDs rather than using the caller's bank.

The final cohort passes 407 checks, 37 native comparisons, 26 VM query calls,
10 compiler cases, 14 continuation mutations and 11 initial caller/scene
boundary cases. Compiler inputs retain signed/high-bit and above-Float-precision
IDs; Float/String references and wrong arity reject. Canonical Bool arguments
retain the shared integral target-query interpretation of 0/1.

The cohort covers signed motion values, common and character-specific IDs,
full-width creation IDs, missing/removed/recycled targets, compiler type/arity
checks, VM publication and captured physical identity changes. Pure queries
contain no callbacks. A test-only source-guard observer injects mutations to
exercise continuation checks; this is not arbitrary hostile callback
containment. Unselected native engine calls abort or reject explicitly.

Selected immutable package/rig publication facts and userdata are controlled.
No imported DAT construction, source package loader, full Popo/Nana behavior,
game, GPU or audio process is exercised. All compiler and fixture subprocesses
use `CREATE_NO_WINDOW`, and the output directory must be fresh on A:.

```text
python tests/native/script_target_motion/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-target-motion-oct10-final
```
