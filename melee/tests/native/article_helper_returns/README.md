# Article helper typed-return compiler and callback fixture

This namespace compiles the actual compiler and actual generated callbacks in
both action and named-state module forms. No fighter activation or game is run.
Existing Void helpers retain their direct function ABI. Typed helpers use the
same `-> bool/int/float` declarations and assignability as fighter helpers.

`compiler.c` takes three arguments: `probe.fms`, action output header, and state
output header. It checks forward typed calls, separate article namespaces,
typed-return validation, final unconditional returns, recursion, Void/scalar
misuse, wrong arguments and postfixes, and held-status restrictions. Each
generated header should be named `article_returns.inc` in a separate directory.
`module.c` includes that header; no compile-time fixture macros are needed.

The generated runtime checks cover ordered nested calls, high-bit IDs, -1 and
integers above 2^24, truth conversion, Float widening, helper-in-condition and
return arithmetic, Bool/Int/Float neutral exits and Void calls. The controlled
Host can remove/recycle an Item or recipient, change recipient kind/Definition,
or transfer the current owner. PAGE_NOACCESS Item storage exposes any unsafe
continuation before revalidation. A missing recipient must skip its arguments;
a surviving Item may continue after recipient loss without storing its result.
Eight-argument creator and owner calls cover pure arguments, ordered nested
mutation and source invalidation before later field reads. The compiler also
rejects an eight-argument expression that exceeds its 1024-byte emitted-line
bound; the argument-count limit does not promise unlimited expression size.

The same-Definition creator resolution and live-registry checks are explicit
controlled boundaries here. This is generated code acceptance, not a new proof
of native resolver/backend dispatch, allocator or package lifecycle behavior.
The existing actual backend tests cover those distinct contracts.

Standalone current-source candidate command:

```powershell
python tests/native/article_helper_returns/run.py --compiler .local/baseline61/compiler/clang.exe --output A:/FreakMelee-test-archives/article-helper-returns-candidate
```

The runner hashes compiler inputs, all relevant current headers and fixture
sources, and rejects changes during the run. Using a frozen compiler executable
does not make live candidate sources/headers frozen SDK61 evidence. Root's
coordinated SDK62 build is the final integration boundary. The runner creates
only console processes and stores all binaries/logs/receipts in the supplied
ignored output directory.

Target recipe for root: compiler executable with `compiler.c`, the actual
`NATIVE_FIGHTER_SCRIPT_SOURCES` and `native_fighter_parameters.c`; generate both
headers through its three-argument interface. Two module executables each
compile `module.c` with their generated directory, `src` and Dolphin includes,
`MELEE_NATIVE=1`, `_CRT_SECURE_NO_WARNINGS`, `_USE_MATH_DEFINES`. They link only
the normal Windows CRT/kernel imports used by the controlled PAGE_NOACCESS
boundary. All three executables are console CTests. No CMake edits are included
in this namespace.
