# Air physics contract fixture

The fixture uses the production frontend, prepared Fighter program, typed storage,
actor, helper execution and VM with the actual fixed GObj pool. The shared landing
resolver, all three directly adjusted native callers, `ft_800851D0` and the
FallSpecial platform predicate are mechanically extracted. Healthy comparison
bodies come from pre-batch commit `7b077ac85`, never moving HEAD.

The previous adjusted-velocity result was **4,133 checks / 71 cases / 354 native pairs / 169 source
invocations**. The final receipt is
`A:/FreakMelee-test-archives/script-air-physics-final/receipt.json`.

The current fixture also covers plain animation-Y velocity against the original
`ft_800851C0` body at the same pinned revision. Fourteen native/source pairs
preserve all velocity components and Float bits with and without an Item pointer,
while Bunny Hood data is absent. The plain operation has no multiplier dependency.
An extra argument rejects without publishing a call result.

- 64 native/source landing pairs cover contact, environmental interruption,
  wall/cliff choices, facing -1/0/1, ledge cooldown and alternate map query flags.
  Another 192 pairs compare all three guarded native callers to their unchanged
  healthy bodies, including the native third caller's separate aerial tail.
- 98 velocity pairs preserve exact F32 output bits across signed zero, normal
  values, infinity and NaN, with and without bunny Item-pointer presence. Missing
  bunny data is required only on the multiplying branch. The Item pointer is
  deliberately never dereferenced.
- Platform/no-line threshold cases execute the actual `ftCo_80096CC8` predicate.
  Source landing executes an owned helper through the VM only after true contact;
  native false environmental response continues the original wall/cliff policy.
- Reached map, environmental, wall, cliff and helper observations inject a real
  actor VM fault, physical free, same-address pool reuse, userdata replacement,
  native generation change, classifier replacement or owned data substitution.
  Failed calls preserve their output; map invalidation stops the cached position
  copy, and caller fences stop subsequent native tails. A reached nested helper
  fault also prevents the remaining authored statement.
- The frontend rejects None and numeric callback arguments. Runtime admission
  rejects wrong-return, parameterized and event callbacks, malformed code IDs,
  argument counts, and selected layout mismatch before collision mutation.

Selected package facts and native motion/data tables are controlled but checked
by the actual extracted ownership/layout validator. Geometry, environmental
response, wall/cliff routines and the position-write leaf are controlled; the
position leaf records actual helper execution and deliberately triggers the
mutation cases. These observations do not assert spontaneous reentry in a pure
field read. Native comparison callbacks exist only in the oracle; the source
operation executes a code ID directly without an authored native callback.

This proves the bounded ABI, arithmetic, ordering and continuation contracts. It
does not prove a game scene, imported DAT, full collision, animation or float
behavior. All subprocesses are hidden; every source/header/compiler and generated
body input is hash-pinned and rechecked before receipt publication.

Run from the repository with a fresh private A: directory:

```powershell
python tests/native/script_air_physics/run.py `
  --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-air-physics-final
```
