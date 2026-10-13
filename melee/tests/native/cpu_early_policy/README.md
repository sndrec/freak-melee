# Early authored CPU policy console gate

SDK71 implements four optional Definition callbacks: `on_cpu_prepare(gobj,
history_hint) -> int32_t`, `on_cpu_drop_command(gobj) -> bool`, and
`on_cpu_stick_step/limit(gobj) -> int32_t`. They replace the respective early
kind choices; absent callbacks preserve the original Nana/ordinary paths.
`event.history_source` is RO Int only in prepare. No new Host callback or
fighter-specific solver is introduced.

Prepare sees supplied mode, distance40, false combat-candidate gate, level and
x14 before the existing mode switch. The script can set strict Int `cpu.mode`,
finite nonnegative Float/Int `cpu.follow_distance`, and strict Bool
`cpu.combat_candidates_enabled`. The latter controls both candidate generation
and the existing nearby-target A/R-retap branch. Common initialization still
resets the gate false later, including when prepare wrote true. Returned IDs
preserve all32 bits and resolve again at the actual history-copy site; stale or
zero IDs seed origin/facing1. Exactly buttons, lstick and x4/x5 are cleared in
all30 records. Cstick and xA/xB remain untouched. Cursors remain records5/0.

Drop policy runs after neutral commands and the squat fast route. True consumes
the remaining queue; false retains the original policy. The two stick hooks
run step then limit, accept0..127, and publish only after both continuations
validate source generation, userdata, kind, ft_data and Definition. Invalid
results preserve that component's original default; unknown native CPU levels
retain untouched outputs unless a valid authored result supplies a component.
There is no clamping or Float handle conversion. The five actual continuing
travel callers independently validate the same source after AA320.

## Reproduction

Root integrates the compiler and both module fixtures into CMake. Console only:

```powershell
cmake --build build/native --config Release --target native_script_cpu_early_compiler_test native_script_cpu_early_action_test native_script_cpu_early_callback_test native_callback_engine_lifetime_test --parallel 2
ctest --test-dir build/native -C Release -R "^(native_script_cpu_early_(compiler|action|callback)|native_callback_engine_lifetime)$" --output-on-failure
python tests/native/cpu_early_policy/run.py --snapshot .local/baseline70 --current-snapshot .local/baseline71 --output A:/FreakMelee-test-archives/cpu-early71-final
```

The runner verifies the immutable compiler and original CPU source against the
snapshot manifest, extracts actual definitions (not callsites), emits both
actual generated modules using the built compiler fixture, then compiles one
controlled-policy source matrix and two generated-policy/source compositions.
With `--current-snapshot`, current bodies/headers/compiler and the pinned
`bin/native_script_cpu_early_compiler_test.exe` come from immutable71; consumed
SDK headers are individually checked against that manifest. `--compiler-test`
can explicitly override the generator. Without `--current-snapshot`, the runner
uses live checkout bodies/headers and the configured build generator with the
original snapshot compiler. Receipts distinguish these scopes. Use a fresh
output directory and retain original70 as the comparison reference.

## Evidence and limits

SDK71 full release and124 grouped checks passed under root coordination.
Scoped CMake build and all four selected CTests passed. Compiler:33 programs;
actual generated action and callback modules:103 checks each. Compiler negatives
cover arity/context, Bool/Float Int results, non-Bool handled result, read-only
history event, strict mode/gate writes and invalid article/model-effect use.
Generated checks cover zero/high-bit/full32-bit event values, nonfinite/negative
distance preserving default, healthy results, and PAGE_NOACCESS removal plus
same-address/new-generation reuse stopping later state writes.

Source receipt `A:/FreakMelee-test-archives/cpu-early71-frozen-final/receipt.json`:

| Matrix | Cases | Checks |
| --- | ---: | ---: |
| Actual bodies, controlled callbacks | 9,259 | 29,994 |
| Same actual bodies + generated action callbacks | 9,338 | 30,421 |
| Same actual bodies + generated callback-route callbacks | 9,338 | 30,421 |

Full extracted bodies: A101C initialization, A08F0 drop, AA320 step/limit, and
AA42C/AA844/AABC8/AACD0/AD54C continuing travel callers. Six actual neutral,
teeter/turn and stick interpolation helpers are also extracted. Healthy old/new
comparison spans supplied modes0..29, levels-1..10 at initializer/helper sites,
five native kinds, floor hit/miss, both air states and Wait/Dash/teeter caller
motions. Unknown-level travel with uninitialized native outputs is excluded;
the helper itself is tested with defined sentinel output values. Safety cases
retire or replace generation, kind, Definition, userdata or ft_data. Source
and generated compositions test thirty-record retention and a history source
retired between prepare and copy. Both generated policies reproduce original
healthy Nana initialization through the already-supported late spacing hook.

Controlled boundaries: VirtualAlloc/VirtualProtect fixture storage, live
Snapshot/Alive boundary, Definition and creation-ID lookup, validated runtime
history-hint boundary, planner predicates, command sink, floor probe, RNG,
B9704 and the late mock callback. Position/facing, CPU field writes, command
ordering, original full function control flow and new guards execute actual
extracted production C. The new runtime counterpart resolver and actual
ProcessDeath callers are separately tested by root; this fixture does not
pretend its controlled lookup is their implementation. A normal life reset
changes generation before A101C; root's real reset fixture validates this and
its success Bool. No pre-reset-generation check is inferred here.

No game, renderer, controller sampling or independent mode6 claim. B0918,
B101C, B0AF4, B7AAC and the partner-specific special-motion decisions remain a
separate follower port. No family template was promoted. Historical design and
unimplemented follow-up choices remain in PLAN.md and the source proposal.
