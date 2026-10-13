# Authored spatial queries

`compiler.c` generates the action and motion-callback modules when passed two
output filenames, in that order. Both modules also include the actual generated
article created/physics callbacks. `module_test.c` compiles with
`QUERY_MODULE="script_queries_action.inc"` / `QUERY_CALLBACKS=0` or
`QUERY_MODULE="script_queries_callback.inc"` / `QUERY_CALLBACKS=1`.

Compiler checks cover typed numeric origin inputs, an Int creation handle,
declared article kind, distinct Float outputs, scalar state and typed locals,
arity, undeclared kinds, arrays/expressions as output targets and invalid
article/model-effect contexts. Fighter state uses `state`; article state uses
`item.state`. The primary-model translation is local to that model root, not the
Item's world position. Same-package immutable author checks belong to the
actual Host backend, not to an inferred current damage owner.

The generated-module fixture exercises sequential effectful argument helpers,
high-bit IDs without float conversion, failed-query output preservation, and
three-coordinate publication only on success. Every query and each helper can
retire the source with PAGE_NOACCESS or recycle it at the same address. No later
coordinate, result or callback write may execute. Both new Host pointers and
the required generation query must be present at module opening.

Host spatial queries and basic landing are controlled engine boundaries here;
they intentionally can retire the source to verify generated continuation
guards. Their native geometry, nearest-player selection and immutable article
ownership are separate backend evidence. This fixture launches no game and
makes no rendering or whole-match claim.

The coordinated live SDK69 query targets pass all three CTests: 44 compiler
programs and 740 generated-module checks per route (1,480 total). Receipt is
`build/native/Testing/Temporary/LastTest.log`; root's coherent target build and
CTest invocation supplied the matching new Host ABI. The first compiler build
log `.local/sdk69-query-compiler-build.log` is retained: its rejected positive
was the test's eight-space created-body indentation, corrected to the existing
twelve-space article callback grammar. Query lowering needed no correction.
Production/compiler fixture inputs were frozen for that SDK69 acceptance;
its committed inputs and counts remain separate from the extension below.

## Position-query extension (SDK70 batch)

The next extension adds `targets.read_position(Int id, outX, outY, outZ)` in
fighter and article contexts using the same distinct Float scalar/local output
rules. It reads current fighter position, not a camera sample or model root.
The new generated export and article dropped/thrown callbacks exercise zero,
ordinary, above-float-precision, high-bit and all-bit-set IDs, both failure/success
outputs, effectful handle evaluation and caller protection/recycling. Actual
native position/registry source evidence lives separately in
`tests/native/fighter_position_query`. The SDK69 counts above remain historical;
the expanded coherent live SDK70 compiler/module targets pass all three scoped
CTest targets: 72 compiler programs and 2,110 generated checks per route
(4,220 total). Original69 cases remain in the expanded suite. The retained
CTest console log is
`A:/FreakMelee-test-archives/fighter-position70-second/generated-ctest.log`.
These sources are stable for root's coordinated full SDK70 build/freeze; this
scoped acceptance does not substitute for that full build or native backend
gameplay evidence.
