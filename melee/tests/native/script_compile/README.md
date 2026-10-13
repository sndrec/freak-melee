# Transactional direct program compiler

```powershell
python tests/native/script_compile/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-compile-oct07
```

`native_script_compile.c/h` owns complete assembly into caller-reserved inactive
banks: pinned source, hierarchy/IR/index, typed callback/helper bodies, native
owner contexts, scoped event reads, bytecode, execution records, state layout
and resolved parameter tables. It validates helper acyclicity and source
ownership, detects duplicate event identities, and calculates maximum synchronous
helper depth/register requirements. It performs no runtime allocation or
generated-C compilation.

The caller supplies two independent stable banks. Every candidate buffer is
checked for overlap with itself and the accepted bank before any mutation.
Failure leaves the accepted pointer and all its bytes unchanged; a successful
candidate replaces only that pointer. Keep old banks pinned until all executing
actors release them. State instances and VM/payload scratch remain separately
owned runtime objects.

The built-in minimal schema requires one independent fighter directive, root
helpers/state/parameters and action callbacks. It rejects imported attributes,
moves/articles/items/effects, assets and unknown metadata. A wider metadata
validator must explicitly accept and validate every non-executable source node;
this callback is a load contract, not an alternate parser or engine fallback.
All owner parameters must be resolved before publication. Cross-owner helper
calls remain rejected until context-independent helper semantics are proved.

The fixture executes the compiled program for 16 ticks with actual immutable
owned state/parameter accessors and controlled native property reads/writes.
It checks byte-for-byte preservation after source/type/metadata/return/recursion,
capacity, import-readiness and cross-owner failures, including same-bank and
shared-buffer rejection. An explicit expanded article schema verifies context
selection and qualified own-state/parameter bindings. Native execution symbols
in `native_boundaries.c` are deliberately absent link boundaries; this fixture
does not schedule actual engine actors. Actual native registry/execution tests
are separate.

This establishes code/owned-state construction and transactional publication
for the accepted schema. It does not validate full fighter package assets,
independent motion coverage, Workshop installation or gameplay parity. Those
require the broader typed metadata contract and live entry integration. The
runner starts all subprocesses with CREATE_NO_WINDOW on Windows.

Exported recipient helpers also accept typed arguments; this fixture checks
prepared call arity and rejects non-numeric arguments. Actual recipient execution,
conversion and lifetime tests live in `script_target_exports`. The runner requires
a fresh output directory and verifies that compiler and source hashes remain
unchanged throughout the run.
