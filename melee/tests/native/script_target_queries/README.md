# Source target query fixture

Actual source compiler/catalog, bytecode/VM, Fighter and Item actors, storage,
GObj registry/pool, and selected-source guards execute. A prepared owned package
and rig are controlled publication data; foreign targets are real stock-class
Fighter GObjs without source actors. Exact native `Position`, `PositionDelta`,
`Airborne`, `IsFighter`, `ftLib_8008741C` and `ftLib_80086BEC` bodies provide
reference comparisons, checked against checkpoint `37c81e872`.

The pure queries do not invoke engine callbacks. Mutation cases deliberately
inject a change at the extracted caller guard observation before publication.
This proves retained identity and output barriers; it does not claim reading
cur_pos or pos_delta causes reentry. The production query unit itself is compiled unchanged.

Position output is SuccessOnly and preserves native F32 bits without sanitizing
NaN, infinity or signed zero. Zero/missing/wrong-class IDs return false and leave
three outputs unchanged. Target class, scene, physical pool generation, native
spawn ID, userdata and kind are captured once; a later mismatch fails without
reacquiring a replacement. Airborne uses exact GA_Air equality. No target
Definition, source program, native authored callback or creator relation is used.

```powershell
python tests/native/script_target_queries/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-target-queries-review01
```

The output directory must be fresh on A:. All subprocesses are hidden. No game,
global build, DAT item construction, or user project is touched.

The bounded matrix compares 64 raw native coordinate/airborne tuples, including
NaN payloads, infinity and negative zero. It exercises both caller classes through
real VM SuccessOnly writes; full-width signed/U32 handles including UINT32_MAX;
zero/missing/wrong-class targets; malformed widths/types; and guard-time native
generation, kind, userdata, class, physical pool reuse and source retirement/fault
changes. Unrelated native families explicitly reject; they never stand in for
successful target-query behavior.

The delta extension compares 256 raw-bit seed tuples and explicit signed-zero,
NaN/infinity patterns against the unchanged direct ftLib getter and legacy
native target reference. Fighter and Article VM calls publish SuccessOnly delta
outputs and preserve them on a miss. Guard-time mutations cover both queries.
An actual Fighter helper also executes 257 queries with local Float output
destinations, including preserved defaults for a missing target. Compile-time
negatives reject duplicate/non-writable/wrong-typed outputs, floating handles
and wrong arity.
PAGE_NOACCESS registry heads reject before dereference; actual-pool list cycles
and malformed matched gobj links fault without publication. Native Fighter
userdata is valid engine-owned memory in this fixture: neither shared lifetime
membership nor the binding proves arbitrary non-null userdata allocation extent.

## Native field and terrain expansion

The grouped cohort compares all five target scalar queries against unchanged native reference bodies, including native Float bit patterns, missing targets and signed creation IDs. Real Fighter and Article VM calls publish these reads; source and target retirement injection applies to every new query. These are physical native observations, with no authored target-state access or game acceptance claim.
