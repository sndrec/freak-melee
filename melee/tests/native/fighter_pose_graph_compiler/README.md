# Named fighter pose compiler fixture

The actual compiler emits both action and named-state module forms. `compiler.c`
takes `probe.fms ACTION_HEADER STATE_HEADER`; name each output `pose_probe.inc`
in a separate directory. `module.c` includes that header without fixture macros.
It verifies all required Hosts, sorted named graph indices/recipes, the exact
14-operation sequence, live/alternate destination selection, independent
weights (including negative/non-normalized pairs), finite input rejection,
animation ID bounds, rejected-bank no-op, invalid validation without mutation, nested argument
invalidation and generation guards after all callback-bearing pose operations.
Source retirement uses PAGE_NOACCESS storage; recycling changes the creation ID.

Compiler negatives cover graph bounds/duplicates/placement/path grammar,
unknown graph/destination, arity/type errors, scalar use of actions and forbidden
article/model-effect contexts. The emitted statement bound remains 1024 bytes;
the tiny-output test checks rejection and neighboring canaries.

The runtime validator, graph/clip resolver and traversal calls are explicitly
controlled Host boundaries. This fixture tests actual generated C, not real
asset decoding, traversal mathematics or fighter/gameplay behavior. The separate
engine-owned tests establish those contracts, including actual missing/replaced/
poisoned animation-bank rejection before any table access. Nested start, frame
and both-weight calls test written order and removal before later field reads.

```powershell
python tests/native/fighter_pose_graph_compiler/run.py --compiler .local/baseline62/compiler/clang.exe --output A:/FreakMelee-test-archives/fighter-pose-compiler-candidate
```

The runner uses current candidate compiler inputs and headers, pins every input
and generated header by SHA-256, and rejects changes during a run. A frozen
Clang executable alone does not make this frozen SDK62 evidence. It launches
console compilers/fixtures only; no CMake, game, installation or rendering test.

Root CMake recipe: add `tools/native_fighter_script_pose.c` to the compiler source
list; compiler fixture executable uses `compiler.c`, all actual compiler units,
and `native_fighter_parameters.c`. Generate two headers through its three-arg
CLI. Compile `module.c` twice against those generated directories with the normal
SDK/Dolphin includes and `MELEE_NATIVE`, `_CRT_SECURE_NO_WARNINGS`,
`_USE_MATH_DEFINES`. All three executables are console CTests; Windows CRT and
kernel imports serve the protected-memory checks. No CMake edits are owned by
this namespace.
