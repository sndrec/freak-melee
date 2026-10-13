# Game & Watch constructor source composition (SDK62)

This unregistered namespace compares all ten original constructors with actual
callbacks from a separately compiled and loaded generated DLL. It preserves the
accepted load/specials/Chef/normal source inputs without changing them. It proves
the bounded constructor decisions below, not normal cloned-fighter ownership,
rendered pixels, gameplay or complete Game & Watch independence.

The final acceptance output is
`A:/FreakMelee-test-archives/gamewatch-constructor62-final/receipt.json`:

- 123,648 healthy-domain/failure constructor pairs;
- 18,816 additional controlled paper-presentation owner-transfer pairs;
- 11 separately classified authored safety cases;
- 22,731,448 checks, zero failures;
- ten real retail article imports and ten DLL-owned `on_created` callbacks.

Reproduce with a fresh output directory (the source ISO is read-only):

```powershell
python -B tests/native/gamewatch_constructor_composition/run.py --snapshot .local/baseline62 --output A:/FreakMelee-test-archives/gamewatch-constructor62-fresh
```

`--iso` can select the Melee USA v1.02 ISO. The runner verifies all 3,227 snapshot
hashes before and after, extracts PlGw.dat, assembles the complete accepted
callbacks plus new constructors, generates C with the frozen content tool,
builds a DLL and a separate console fixture with frozen Clang, then runs one
batched matrix. No game process, installation, global build or commit occurs.
The snapshot manifest SHA is
`440b27db0595cc12a730a5e390c6cc9a74d00ed17b7f1ec14117ae6338f9dc4b`;
PlGw.dat SHA is
`1c057a2882b7b42f3184b96ddf5221cae50e69652b00f6c78819c819804e0a1f`.

## Source and decisions

The item files below are under `sdk/src/melee/it/kinds/`. Exact definition
extraction requires a function prefix, including parenthesized HSD function
names, and balanced bodies; call-site braces are not selected. Slot order is
proved by actual `ftGw_Init_OnLoad` in `ftGameWatch/ftgamewatch.c:522`.

| Slot | Kind | Original constructor | States | Authored choices after allocation |
| --- | --- | --- | --- | --- |
| 0 | Greenhouse | itgamewatchgreenhouse.c:21, itGamewatchGreenhouse_Spawn | 4 | Attach/pickup, debug, paper |
| 1 | Manhole | itgamewatchmanhole.c:18, it_802C65E4 | 1 | Attach/pickup, debug, paper |
| 2 | Fire | itgamewatchfire.c:19, itGamewatchFire_Spawn | 1 | Attach/pickup, debug, paper |
| 3 | Parachute | itgamewatchparachute.c:26, it_802C6C38 | 2 | Attach/pickup, debug, paper |
| 4 | Turtle | itgamewatchturtle.c:26, it_802C6F40 | 2 | Attach/pickup, debug, paper |
| 5 | Breath | itgamewatchbreath.c:17, it_802C720C | 2 | Attach/pickup, debug, paper |
| 6 | Judge | itgamewatchjudge.c:23, it_802C7774 | 1 | Attach/pickup, numeral/frame, child orientation, debug, paper |
| 7 | Panic | itgamewatchpanic.c:17, it_802C7D60 | 2 | Attach/pickup, debug, paper |
| 8 | Chef | itgamewatchchef.c:22, it_802C837C | 2 | Variant, lifetime, velocity, manual lifetime, state, debug, paper |
| 9 | Rescue | itgamewatchrescue.c:27, it_802C8038 | 2 | Commands3..0, bounds flag, retained creator, paper, state, advance |

`constructors.pending.fms` remains an unregistered fragment file. Its previously
proposed typed `creator.call` expressions are now accepted and executed by
SDK62. `palette_helpers.fms` supplies exact raw DAT attribute recipes and two
real private fighter helpers. They compute colors from owned parameters and
current costume, without duplicated persistent palette state or a surrogate
x610 diffuse field. No original Game & Watch constructor or palette function is
called by the generated policy.

Explicit creation sets zero velocity/damage, both original parent references,
facing/group and requested positions. Attached props call `attach_to_owner`
once in `on_created`, so synchronous `picked_up` precedes debug and paper. No
additional automatic held spawn is combined with these callbacks. Greenhouse
chooses state0 for Attack11, state1 otherwise; Manhole/Fire clear commands and
hide the model; Panic reads current holder ground/air; Judge uses appearance
frame variant+1 and second-child rotation. Chef preserves flattened previous
Z, ECB-center current position, variant0..4 velocities and manual lifetime.
Rescue preserves paper setup before state/animation and clears xDCC.b3 bounds
removal, not the unrelated xDCF.b3 layered-rendering flag.

The first-child ordinal resolver exposed a correction in the prior isolated
prop draft: empty path `""` is the primary root, `"0"` its first child, and
`"0/0"` its grandchild. New assembly applies BB20/BB44 visibility to the primary
root and Manhole world lookup to its first child. It also replaces the old
provisional Manhole world/wind spellings with shipped generic operations.
Accepted source files remain untouched. The Manhole wind callback is complete
and compiled here, but not executed by the constructor matrix.

## Executed production bodies and controlled boundaries

Original constructors and eight complete original pickup bodies execute.
Original Judge orientation, Chef initialization and Rescue state initialization
execute. Generated callbacks are returned from `FreakMelee_GetFighterModule`
in an actual DLL; Windows allocation-base checks prove all ten created and all
present pickup callback pointers belong to that DLL. Export layout fingerprint
matches the frozen SDK. All 131 parameters are bound to that module.

The console links or extracts these actual common implementations:

- Item_8026AB54 and it_802742F4 attachment/pickup continuations, including the
  real post-callback generation check; Item_8026B074 sound-state cleanup;
- Item_802694CC model advance/frame update, it_80273670 descriptor/frame order,
  it_80275158 lifetime and it_8026B3A8 manual lifetime;
- db_80225DD8 debug byte policy; full it_8027CE64 paper presentation;
- actual ftLib controller-color policy and Game & Watch costume/outline getters;
- actual it_80278574 DObj material traversal and it_80274594 scale policy;
- unified typed layer import, ValidateItem/Bind, native article model Read/Write,
  shared joint ordinal/flag/transform channels, HSD flags/dirty entry body;
- actual nativeArticle_Create, nativeArticle_Creator and generation/list queries.

Common allocation is controlled: it realizes Item/GObj storage, defined initial
flags/commands, immutable imported Article data and live registry membership.
HSD_JObj realization copies the real imported descriptor topology/transforms
into bounded nodes; DObj/material storage is synthetic and deliberately covers
all model nodes. Actual HSD model loading/animation interpretation does not run.
Pose reset, attachment geometry, command interpreter, animation rate/frame
requests, pickup sound and post-scale geometry sinks are explicit controlled
boundaries whose order/arguments are compared. Matrix does not prove emitted
command-stream effects, collision geometry, real rendering or HSD allocation.
The original allocator initializes some fields differently in a real game; the
fixture compares both policies under the same declared initial conditions.

The inherited base is synthetic: 53 physical joints, 40 motion rows and known
special indices. Motion callbacks are not invoked. Runtime Definition/article
asset and retained-ID lookups are bounded fixture boundaries; actual creator
and lifetime validation bodies execute on real structures. Unknown/unexecuted
Host operations trap. This does not run the production module/package loader,
normal stock clone import, full fighter entry or independent ownership gates.

Each of ten Article records is decoded from actual PlGw.dat by the full
`itArticle_Import` and shared DAT/layer decoder bodies. The standalone item
importer's Paper decoder selectors are its existing Kirby Chef/Pan metadata
cases; they select the shared schema for those exact stock records, not Kirby
fighter behavior. Normal fighter importing uses its PaperArticles schema and
is a separate integration gate. Palettes and Chef tuning are sourced directly
from retail bytes, without invented defaults; other accepted phase tuning is
retained in the full composition. Shader/GX/pixel evidence is separate.

## Matrix domains and deliberate limits

Four costumes, controller selectors0..5 and255, both facings, air/ground,
owner scales0.5/1/2, eight command/debug/state conditions, legal Judge0..8,
Chef0..4 and Rescue0..1 cover the 123,648 pairs. Allocation failure has the same
NULL/-1 terminal behavior. Missing dynamic-bone table tests typed layer binding
failure and exact continuation: Rescue still initializes state and advances.
Zero-length layer lists and malformed retail graphs are covered by the accepted
layer foundation fixture, not newly claimed by this matrix.

The additional 18,816 cases use the two unattached constructors with a controlled
allocator changing current damage owner to another live fighter, including a
foreign Definition, distinct high-bit generation, costume, controller and
scale. The original paper body still receives its constructor parent; generated
paper uses validated retained creator helpers. Both take tint/scale from the
new current owner. Debug inputs are intentionally equal: generated debug query
uses current owner, whereas original constructor debug receives parent. These
cases prove paper color/scale separation only; they do not prove that a stock
allocator reflects mid-construction or that arbitrary transferred debug states
match. Normal healthy constructor pairs have parent==current owner.

Ten authored-only cases retire/recycle/mismatch the retained creator while a
distinct current damage owner remains live. Palette mutation skips safely;
layer binding and Rescue state setup still complete. The eleventh compares
Rescue's defined NULL-parent return with generic creation's rejected NULL input.
Other invalid native parent/palette, Chef index, Judge unsigned-wrap and missing
Judge child accesses are undefined/outside the legal source domain and are not
executed as strict source pairs. No claim is made about retail console bugs.

## Foundation history and remaining activation

The immutable SDK61 census remains at
`A:/FreakMelee-test-archives/gamewatch-constructor61-audit-final/inventory.json`.
It correctly found that stock Paper had no raw-color byte span; root repaired
`{4,20}` in SDK62 with separate full retail-import color/neighbor-scalar checks.
The executable runner requires that repaired schema. Owned attribute-byte
recipes had already been correct in61; these are distinct schemas.

Root subsequently audited real copied-base/rig/article metadata separately.
Those live changes are not consumed by this frozen62 comparison. Normal owned
clone import, module/package validation and complete caller/lifecycle/CPU/rig
closure remain necessary before template promotion. Existing complete normal
and special callback source is retained, but only the constructor/pickup slice
is exercised here. Nothing is installed or registered by this namespace.
