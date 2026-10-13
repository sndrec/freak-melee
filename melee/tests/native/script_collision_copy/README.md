# Collision history copy fixture

```powershell
python tests/native/script_collision_copy/run.py --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe --output A:/FreakMelee-test-archives/script-collision-copy-final
```

Use a fresh private A: directory. The runner mechanically extracts the unchanged
`mpCopyCollData`, `nativeFighterRelations_CopyCollision` and `ftLib_8008741C`
bodies and compares them with `4b32ba62a`. The actual compiler/catalog/Host
dispatch, Prepare/Publish, Fighter actors, VM, lifetime guards and fixed GObj
pool execute. Source data/motion assets are controlled; unrelated native leaves
abort or reject.

The historical `fighter_relations` 256-seed collision-field oracle is exercised
for stock and owned targets. Complete caller/target records and explicit copied
and retained fields are compared, including arbitrary float bits and stage
indices. Actor helpers cover returned, local and discarded Bool results and
one evaluation of the authored ID expression. Additional cases cover high-bit
IDs, self/zero/absent targets, retired objects, poisoned non-pool source memory,
exact Int admission and physical/program/storage faults before mutation.
Controlled userdata is valid Fighter storage; the engine's live-registry userdata
allocation invariant remains a prerequisite. Poisoned memory cases concern
unregistered GObj addresses, not arbitrary non-null Fighter userdata pointers.

No callbacks are invented in the native copy kernel. No game, audio device,
stage solver, DAT importer or simultaneous-companion runtime is exercised.
The receipt pins production, fixture, kernel, compiler, header, documentation
and retained example inputs.
