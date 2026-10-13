# Owned arrow trail descriptor census

The four actual Link/Young Link arrow trail roots import successfully, but **all
four remain rejected for construction**. The original default-only census found
custom texture combiners. The subsequent bounded operand budget and native
shader comparisons now cover their allocation demand; the actual material
environment and graph ownership/construction are still unclosed.

The historical descriptor-only probe is archived at
`A:/FreakMelee-test-archives/arrow-descriptor-census-final/`, including `probe.c`,
`run.py`, commands, logs and `receipt.json`. It passed 55 checks with stable input
hashes. Run `python A:/FreakMelee-test-archives/arrow-descriptor-census-final/run.py`
to repeat against the recorded private inputs and existing native library.
No production, template, metadata, allocator or build configuration was changed.

## Actual imports

Inputs are the private copied drafts in
`A:/FreakMelee-test-archives/fighter-callback-source-census/fighter-2/` and
`fighter-3/`. The probe compiles the current FMA decoder, character/article import
and DAT graph import sources directly. Remaining engine dependencies link the
existing `build/native/melee_runtime.lib`, also fingerprinted. It reads each
FMA's owned schema and root name, uses `ftCharacterAssets_Import`, selects the
declared Arrow asset at slot 3, and calls `itArticle_ImportModelRoots` at attribute
offsets 36/40. Source headers and compiled source inputs are recorded in the
receipt; native ID/Vec/Mtx/RObj allocator descriptors are initialized by their
actual functions. No native graph is constructed.

| Member | DAT root | Arrow attribute block | First root | Second root |
| --- | --- | ---: | ---: | ---: |
| Link | `ftDataLink` | 16200 | 138888 | 140968 |
| Young Link | `ftDataClink` | 16676 | 140904 | 142984 |

Offsets are in DAT data, excluding its header. All four graphs have one unnamed
JObj, DObj, skin PObj, MObj, Material and TObj, plus one custom TObjTev. There are
no RObj references, explicit matrix, optional PE, TLUT, LOD, custom class names,
instances, envelopes, shapes or particle/spline nodes reported by the census.
JObj flags are `0x20080000`, material mode `0x60000012`, texture flags
`0x00340010`. The texture participates in diffuse lighting and modulates color
and alpha. Secondary animations are not part of these declarations.

Each graph's current rounded core payload demand is 856 bytes:

| Allocator | Rounded size | Count |
| --- | ---: | ---: |
| Class | 192 | 1 |
| ID entry | 24 | 1 |
| Class | 64 | 3 |
| Class | 32 | 2 |
| Class | 384 | 1 |

Potential lazy scale-vector demand adds one 16-byte Vec slot. These are payload
sizes only: reservation metadata/alignment, generated shader allocations,
concurrent occupancy and replacement headroom are additional. The production
census reports `uncovered=129`, exactly `TevCompiler | MaterialEnvironment`.
Its zero TEV counts mean **unbudgeted**, not absent.

## Two actual custom combiner shapes

Link and Young Link have identical descriptor fields for each corresponding
trail. Arrays below are `[op,bias,scale,clamp,a,b,c,d]`.

| Field | First trail | Second trail |
| --- | --- | --- |
| Active | `0xc0000077` | `0xc000007f` |
| Color | `[0,0,0,1,133,128,8,15]` | same |
| Alpha | `[0,0,0,1,7,7,7,4]` | `[0,0,0,1,7,4,67,7]` |
| Konst RGBA | `[255,255,255,0]` | `[255,255,255,170]` |
| Tev0 RGBA | `[255,255,255,0]` | same |
| Tev1 RGBA | `[0,0,0,0]` | same |

Actual `TObjMakeTExp` calls `MakeColorGenTExp` for both active channels. The color
uses stored register-0 RGB, konst RGB, texture RGB and zero. First-trail alpha
copies texture alpha; second-trail alpha multiplies it by konst alpha. Both use
ADD, zero bias, scale 1 and clamp. `MakeColorGenTExp` creates a register-0 staging
expression; these are not equivalent to removing the custom descriptor.

## Smallest next support batch

The demand checker now validates bounded **combiner operand shapes**, not
fighter names or DAT hashes: ADD/SUB, zero bias, scale1 and clamp with native
constant/texture/stored-register inputs. It counts each staging occurrence,
unique constant sources across both arrays and every texture participation.
Other arithmetic/invalid operands and excess demand remain rejected.

`tests/native/hsd_reservation` now imports the four actual arrow roots and runs
the real MObj/TObj expression builders, compiler, complete DAG optimizer and
cleanup against ordinary and reserved allocators. The final receipt at
`A:/FreakMelee-test-archives/hsd-custom-tev-final3/receipt.json` passed11,265
checks/341 material cases/4 actual roots, including live recompilation. Claims
contain only shader allocation bins, without spare graph capacity. Each arrow
budgets five expression/stage slots and two/three constants. The earlier prose
mistook alpha-map value3 for blend; it is MODULATE and allocates no blend
constant. The original descriptor arrays/receipt remain accurate.

Current arrow census therefore retains `MaterialEnvironment` alone (128),
instead of the original129. This is shader allocation and native-output parity
evidence, not graph/model admission. The remaining graph core requires the
856-byte demand above plus lazy scale and shader backing, all under actual
scene ownership. Arbitrary future compiler register legality is not guaranteed
by the finite allocation rule.

In particular, clearing `TevCompiler` means the validated operands' allocation
paths are budgeted; it does not prove every combination packs into native
constant/output registers. Future construction admission needs actual compile
legality validation and an explicit failure policy before an assert-capable
constructor executes. No constant-count heuristic substitutes for that proof.
The current source tree has no production census caller: only the demand
implementation/header and isolated tests reference these flags. Materials still
retain128 and no model metadata/construction path consumes a cleared flag as
legality permission.

Use the actual effective-empty toon environment and reject shadow materials
initially, validating actual default classes/methods at construction. Do not
clear `MaterialEnvironment` merely because this descriptor has no toon texture:
`MObjLoad` adds `RENDER_TOON` and compilation can splice the global environment.

Then reserve scene-owned backing and execute the real graph loader for these
four imported roots, proving cleanup, slot replacement and shader parity. Owned
root selectors, source guards, lazy matrix/draw allocation and retained-history
backing must be connected before metadata is accepted. Boomerang animation
bundles and Hookshot resource collections remain separate later slices.

## Historical descriptor-only input identities and limits

These hashes belong to the original55-check descriptor-only receipt, including
the earlier default-only demand source. The final connected shader receipt
records its own current source/header hashes and341-case results separately.
The actual DAT/FMA input bytes remain identical. Historical SHA-256:

| Input | Hash |
| --- | --- |
| Link fighter DAT | `84c806ea3760c7290ba9d9eaec3588ce2d2ebdef19b8505418a1cd0c91c60dde` |
| Young Link fighter DAT | `e614e91710e8a8a648c2da749d2c4bb964caa35d4dcf61ccf7d4810738f02a60` |
| Link FMA | `26b721be78a6304f4d66fb6a15453fe74a5c95ebfb42336e3dcfc5bc7ab87ef0` |
| Young Link FMA | `2c5ae8a78a96ffdae3f968964b98f8c27082fe435674eb7e9413ff9b991be12e` |
| Demand source | `88a73f14128c57032e3e793fd2866f39531e91a511b32c6537165c171092b4d1` |
| TObj source | `b8e0a6c87466b9666fc541b30627187cd5c5a9f419c6241f7ed178227f8b5150` |
| MObj source | `c7dd30cef04812880bc49f42531cc8ff3eb94c2754f00b1d1b072541b9f7f6ed` |

The original probe's two nonzero recipe hashes are distinct table keys for the requested
roots, not claims about compiled source name resolution. Private transition
malloc/calloc owns immutable input/descriptor memory only. This is real DAT
import and descriptor census evidence, not HSD construction, shader execution,
source actor/VM binding, drawing, game fidelity or rollback acceptance. That
historical run did not change any uncovered flag. The current bounded budget
clears TevCompiler for these operands while preserving MaterialEnvironment;
neither run enables production metadata or graph construction admission.
