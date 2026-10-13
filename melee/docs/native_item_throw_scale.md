# Shared remembered item scale

`Item.throw_scale` now owns the remembered scale used by Bob-omb, Mr. Saturn
and Turnip. It sits beside the live instance scale `scl`. The allocator sets
it to1 independently of the attribute-derived instance scale. Bob-omb and
Mr. Saturn keep their original spawned-callback reset to1.

The Turnip constructor remembers `scl` after attachment and owner-scale
inheritance for exactly Bob-omb, Mr. Saturn and Turnip. Beam Sword still skips
owner inheritance and saving. Other item kinds do not acquire this behavior.
The obsolete private saved-scale members are removed from the three item
types; there is no union alias or compatibility copy. Remaining private field
names are historical decomp names, not claims about their new byte offsets.

The restoration policy stays in the item's actual callbacks:

| Item/path | Restoration timing |
| --- | --- |
| Bob-omb thrown | After state selection and presentation, only if remembered scale !=1. |
| Bob-omb dropped | No immediate scale restoration. |
| Bob-omb state10 animation | Only after the existing timer query returns false, following state selection/presentation; scale !=1 remains required. The spinning update retains its original final position. |
| Mr. Saturn thrown | Always, after the existing state/visibility/animation work. |
| Mr. Saturn dropped | No immediate scale restoration. |
| Turnip thrown | After state selection, appearance and hitbox damage update. |
| Turnip dropped | No immediate scale restoration. |

State changes that reset the live attribute scale do not clear `throw_scale`.
Actual `it_80274484` still writes uniform joint scale, marks the matrix dirty
when required, then refreshes hitbox geometry and the two remaining geometry
helpers in their existing order.

## Source evidence

Standalone source verification passes **204 constructor/throw pairs +200
spawn/drop/state10 pairs**, with **16,022 assertions**, including three tests
of the exact allocator initialization slice. Evidence is retained under
`.local/peach-throw-scale-migration/`: generated includes, source receipt,
compile/run logs and result manifest. No gameplay or global build was run for
this fixture; coordinated integration acceptance remains separate.

`tests/native/peach_rare_scale_stock_compare.py` extracts the actual candidate
callbacks and the original bodies from commit `4df2ecc99`. Original private
type declarations remain test-only, retaining the original widened-pointer
union behavior. The semantic reference repairs only the two constructor
aliases to the receiving Bob-omb/Saturn fields, as established by retail PPC
stores. It then projects named private fields and their remembered scale into
the new candidate representation. No callback branch, timer, return value,
math or ordering is replaced by a hand-copied equivalent. The comparison also
checks the common item state, joint rotation/scale and complete instrumented
callback trace; raw historical-layout byte equality is not claimed.

The separate legacy variant retains the old constructor aliases. It still
produces exactly **22 of25 Bob-omb saved-scale mismatches** in the historical
negative matrix. The previous untouched receipts in
`.local/peach-rare-scale-repair25/` retain the earlier153-pair typed-union repair
and its negative evidence. This migration does not redefine those failures as
passing stock behavior.

The ISO check pins the constructor at0x802BD4AC, size0x17C,
SHA256 `3f500c05cdf4c6735c7abca7c62598aea32265f294dd8d2ad65bb13c0d88be51`.
Retail stores at0x802BD5F4/0x802BD600/0x802BD60C target item+0xDE8/0xDE0/0xDE0.
Those are the original console dataflow reference; the new shared field is a
deliberate native representation change, not a claim that retail used it.

Constructor allocation/attachment/owner inheritance, state-transition
internals, presentation, timer query and weighted Turnip selection are
instrumented boundaries. The real absolute-scale body runs, while geometry
helper internals are observed rather than reimplemented. Turnip selection
uses a controlled descriptor0; separate direct throw/drop cases exercise
descriptor0/1. This is not whole-item collision, audio, rendering, ordinary
constructor-selection or authored Peach acceptance.

## Reproduce and integrate

From the repository directory:

```powershell
python tests/native/peach_rare_scale_stock_compare.py --root . --output .local/peach-throw-scale-migration --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso"
cmd /c .local\peach-throw-scale-migration\compile.cmd
& .local/peach-throw-scale-migration/test.exe
```

The console command uses frozen27 clang and the current headers for this new
layout. It does not build or load a gameplay DLL. New gameplay packages must
use the matching future runtime/SDK snapshot.

The existing `native_peach_rare_scale_test` target and three generated include
outputs remain sufficient. Its generation dependencies should additionally
include `src/melee/it/itCommonItems.h`, `itCharItems.h`, `types.h`, `item.c` and
`it_2725.c`, because reference projection, allocator slice and absolute-scale
body now depend on them. No extra linked library/source unit is required.
