# Actual authored behavior lowering for the isolated evaluation

`prepare.py` reads the normal pipeline's Popo/Nana generated C and uses Clang's
typed AST and complete record layouts. It emits two instrumented native modules
and two interpreted modules under a fresh `A:` output directory. It does not
change the FMS language, production compiler, content store or runtime.
This prototype interprets the current FMS compiler's typed lowering. Its C AST
input is an evaluation shortcut, not a proposed final direct-source frontend or
an interpreter adoption decision. Its results do not establish Luau performance.

```powershell
python tests/native/interpreter_evaluation/prepare.py `
  --pipeline A:/FreakMelee-test-archives/ice-climbers-scripted-pipeline72-root-final `
  --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/interpreter-prepared-example
```

All generated authored fighter helpers, action callbacks, article helpers,
article signals/state/logic callbacks and model-effect callbacks are lowered.
An unclassified generated function, unsupported syntax/type, missing bitfield
layout, or register/local capacity overflow is an error. There is no fallback
that executes an unsupported authored function's original native body.

The existing module/immutable-definition setup and parameter import remain
native. Generic packed-state accessors and owner/holder accessors also remain
native; the receipt lists the exact definitions retained. This choice adds
native gateway crossings where the native compiler can inline an accessor.
Consequently the result measures this evaluator's complete integration cost,
including those crossings; it does not isolate pure instruction dispatch or
establish Lua/Luau performance.

The VM executes Clang's scalar operation types and conversions, preserving
Float intermediates and Double intermediates separately. Logical expressions
short circuit. Nested returns inside generated statement expressions return
from the enclosing interpreted function. Native struct fields use Clang's
actual field offsets; bitfield writes preserve surrounding bits. Local vectors
and descriptors occupy the existing fixed per-frame local arena, and their
addresses can be passed to typed native Host operations.

Typed native call gateways are generated from the actual call expressions and
function-pointer types. They invoke the existing Host operation with its normal
C ABI. Discarded aggregate return values retain that native ABI. Pure CRT
floating classification builtins use direct typed builtin gateways. No authored
physics/rope/follower decisions are collapsed into a native gateway.

Each authored C callback entry has a fixed generated ingress function whose
body enters the evaluator. This is confined to the experiment and preserves
the existing engine's callback pointer identity, slots and recipient-export ABI.
Direct authored helper calls execute VM functions without another C ingress.
Synchronous Host calls may reenter through another ingress, using the same
context's next fixed frame. Existing emitted generation guards execute in the
VM before the authored caller touches its state again.

Both generated variants export:

- `FreakMelee_EvalFunctionCount()`, `FreakMelee_EvalFunctionName(index)` and
  `FreakMelee_EvalFunctionEntries(index)` for actual authored coverage.
- `FreakMelee_EvalStats(uint64_t out[4])`: entries, native calls, interpreted
  calls, errors. Native modules report entries and zero for the other counters.
- `FreakMelee_EvalResetStats()` to clear transient measurement counters.

Counters and interpreter call frames are transient evaluator bookkeeping.
Canonical fighter/article state still lives in the existing native bytes and
therefore follows the normal rollback restoration path. The complete workload
must establish that no VM frame or authored mutable state survives a tick.

`scalar_check.py` compares actual Squall input and acceleration decisions,
position exports and a fighter bitfield across 4,096 cases per module. It hashes
the resulting bytes and requires native/interpreted equality and zero VM errors.
It is a correctness fixture, not a timing workload. It does not validate Host
physics, live resources, recipients or full game rollback.

`rope_check.py` executes actual Gumstrings anchor, constraint, retraction and
orientation/render helpers across 128 cases per module, including the complete
40-node declared capacity. Resource storage/attachment outputs are controlled
flat arrays and square root is native. It compares all resulting resource
values, matrices, drawing flags, joints, article state bytes and resource
observation count. Authored nested helpers and loops must execute in the VM.
This establishes lowering correctness for those cases, independently of
imported HSD graphs, stock parity, lifetime or game timing.

The complete evaluation must also exercise the real eight-fighter simulation,
four follower CPU policies, article updates, active rope loops and synchronous
recipient calls. Inspect coverage counters and exact/normalized rollback hashes
before interpreting timing. Compilation and scalar parity alone do not prove
that workload or a production interpreter architecture.
