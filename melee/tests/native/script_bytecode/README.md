# Direct source bytecode and VM fixture

```powershell
python tests/native/script_bytecode/run.py --output A:/FreakMelee-test-archives/script-bytecode-oct07
```

The fixture compiles the actual source hierarchy, program/declaration index,
typed expression/statement compiler, bytecode emitter, numeric values and VM.
It loads `probe.fms` directly. Native operations are controlled typed test
callbacks; no generated C, Clang AST, old Host table, native function pointer in
script values, authored-body native fallback or per-function wrapper is used.
The C compiler builds these engine/testing units only.

Coverage includes typed local/parameter/return conversion, authored direct code
IDs, owned state/parameter IDs and indexed defaults, arithmetic at F32/F64 and
signed/unsigned widths, short circuiting, if/elif/else, nested loops and bounded
break/continue. 4,096 numeric inputs compare actual interpreted execution with
the corresponding C arithmetic reference, including Float32 intermediate
rounding before Float64 operations. Native output destinations preserve old
values on failed success-only queries, validate every output type before any
publication, and retain selector/assignment evaluation order. Chained resource
and joint paths retain each selector argument.

The same fixed VM register/frame pools support synchronous native-to-script
reentry; the fixture checks preservation of the suspended caller's pool state.
Retirement/recycled-generation tests stop the entire helper chain, leave the
external result unchanged and prevent later reads/writes/calls. On Windows a
native operation protects the actor page with PAGE_NOACCESS; an incorrect later
write would fault. Source checks use retained generation metadata outside that
page. This tests interpreter continuation, not the whole-game registry.

Invalid divide, conversion, shift, frame/register exhaustion and missing return
produce explicit diagnostics. Return analysis accepts complete returning
if/elif/else and later guaranteed returns; it makes no loop termination
assumption. Failed lowering restores the bytecode instruction count. Valid
functions contain an unreachable explicit error sentinel so malformed
fallthrough cannot manufacture a default return.

All code, function metadata and source remain immutable during execution.
Caller-owned register/frame pools are transient and complete synchronously;
mutable declared state belongs to the actor's snapshot boundary. The VM imposes
no instruction-budget clamp and performs no runtime allocation. This fixture
does not install programs in the game, resolve every existing native binding,
replace Workshop loading or establish gameplay/performance acceptance.

For a native test target, compile `test.c` with these engine units:
`native_script_source.c`, `native_script_expression.c`,
`native_script_statement.c`, `native_script_program.c`,
`native_script_bytecode.c`, `native_script_value.c`, `native_script_vm.c`.
Include `src`; the executable takes `probe.fms` as its only argument. The runner
starts compiler/test subprocesses with CREATE_NO_WINDOW on Windows.
