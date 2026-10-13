# Normal owned pair pipeline: live SDK71 checkpoint

This is a new gate separate from the frozen70 unsupported-catalog probe. It
uses the rebuilt live71 content tool and matching live headers, hashes them
before/after, and leaves its member projects/packages uninstalled. The root
held those inputs during execution. SDK72 changes are outside this receipt.

```powershell
python tests/native/zelda_sheik_owned_pair/pipeline.py --tool build/native/native_content_tool.exe --clang .local/baseline70/compiler/clang.exe --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/zelda-sheik-normal71-root
```

The compiler executable is the identical bundled Clang toolchain; its old
directory name does not designate SDK headers. The normal `script-build` route
uses the content tool's actual live include paths. This command therefore
requires an explicitly coordinated live source/tool handoff. All subprocesses
use `CREATE_NO_WINDOW`; text reads/writes are UTF-8. Do not change inputs during
the proof. Do not substitute a frozen tool's embedded live includes and call
that a frozen SDK acceptance.

Each member follows actual `clone-stock`, `inspect-project`, normal
`import-parameters`, `script-build` with its project's actual code key,
`bundle-project`, and `verify-package`. The family follows `compose-family
alternate` and `verify-package`. Production base sources and actual retail
motion tables feed a separate DLL ownership probe; original motion callbacks
are fail-fast fixture sentinels, not delegates. All nonnull generated gameplay
phase, Definition, article state/event/signal and export pointers belong to the
member DLL. Metadata and source tunings are checked against their actual copied
DAT and PlCo records. Earlier original gameplay/lifecycle comparisons remain
separate accepted evidence.

Accepted receipt:
`A:/FreakMelee-test-archives/zelda-sheik-normal71-census/receipt.json`
SHA256 `99e066f6fb837a5008e993d4836358449bfee977ba9c9db942436cfbb29fd13c`.

- Zelda: 371 ownership checks, 72 phases, 24 Definition callbacks, 25 article callbacks;
  package `305da792d0bd42dad23a0511ce4ac4f74df1a9ec48f38e24e7144c080ca04eef`.
- Sheik: 505 checks, 96 phases, 22 Definition callbacks, 50 article callbacks;
  package `5b393591e2014fc7501f51fd3d5088f36fca81abf87db616d99914ee40bade7e`.
- Alternate family: `2fe14ca97dfd6742ec79a0faaa646afc39ad2f05e4a977d876b10108391aca82`.

There are 262 physically copied action clips for Zelda and 265 for Sheik.
Every figatree member, its root and its bytes match its retail animation
archive. Action entries262–265 retain genuine tag33 (`Ft_Kind_None`):
`PlyTaro_Share_ACTION_TZeldaThrow{F,B,Hi,Lw}_figatree` and
`PlyTaro_Share_ACTION_TSeakThrow{F,B,Hi,Lw}_figatree` respectively. They are
embedded payloads, not references to another fighter package. Actual
`ftAnim_8006F628` / `ftanim_tracks` retargeting reads the immutable PlCo source
rig/omissions at index33 and the member's owned destination map/parts; it does
not read a foreign fighter's ftData or Definition. The common source rig has
53 parts and the identity semantic map0–52. Retagging these clips to the custom
member would incorrectly bypass that common retarget path.

The tool reports270 own-rig tags +4 common tags for Zelda and273+4 for Sheik.
Those counts include twelve nonempty **demo records** per member without
decoded payloads. `animation_inventory.py` records this distinction rather
than using zero borrowed tags or the printed total as a completeness claim.

## Actual down-throw selector

The historical frozen70 README calls msid222 the forward throw. That is an
erratum: msid222 is down throw; forward is219. The new actual source uses the
victim's immutable AnimationFamily for the Bowser/GigaBowser special pose.

```powershell
python tests/native/zelda_sheik_owned_pair/selector.py --source-root .local/baseline71/sdk --selector-source src/melee/ft/kinds/ftCommon/ftCo_Throw.c --clang .local/baseline71/compiler/clang.exe --output A:/FreakMelee-test-archives/zelda-sheik-selector71-root
```

This is an explicit source overlay: frozen71 predates the small selector
repair, so it supplies immutable headers, common AnimationFamily body and
actual registry lifetime implementation; the complete actual modified
`ftCo_800DD4B0` body comes from the named, hashed source. The downstream
`ftCo_800DD398` transfer is controlled and records its inputs. No mirrored
selector is used. Raw and custom Peach/Zelda/Sheik/Mario victims, both Koopa
forms, another attacker, all four throws, weight-independent masks, authored
selector priority and invalid outputs are covered. Source retirement with
PAGE_NOACCESS and spawn-generation reuse both stop transfer.

Accepted overlay receipt:
`A:/FreakMelee-test-archives/zelda-sheik-selector71-overlay/receipt.json`,
SHA256 `63eb6ba5263ef5d1b1a450407bbaf01171316dda734e9605b8dc780a9359b480`:
1,092 checks /214 cases /2 destructive cases. The earlier matching live71
pass is preserved at `zelda-sheik-selector71-second`; the frozen71-old-body
failure is preserved at `zelda-sheik-selector71-frozen` and is not accepted.
Same-generation kind/Definition mutation is not newly guarded by this selector;
the fixture does not claim whole capture/throw execution or recipient lifetime
coverage beyond the production source's existing contract.

## Demo payload and current custom results boundary

```powershell
python tests/native/zelda_sheik_owned_pair/demo_audit.py --source-root .local/baseline71/sdk --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --pipeline-receipt A:/FreakMelee-test-archives/zelda-sheik-normal71-census/receipt.json --output A:/FreakMelee-test-archives/zelda-sheik-demo71-root
```

The read-only audit locates all12 absent demo entries per member and hashes
their exact embedded disc DAT members. Results (demo indices0–9, with index4
empty) are in `GmRstMZd.dat` / `GmRstMSk.dat`, under
`ftDemoResultMotionFileZelda` / `ftDemoResultMotionFileSeak`. IntroL/R indices10/11
are under the matching `ftDemoIntroMotionFile*` roots in `IrAls.dat` (shared
singleplayer intro archive; original callers also select IrEzTarg/IrEzTuki/
IrEzFigG). Ending index12 is in `GmRegEnd.dat` under `ftDemoEndingMotionFile*`.
The matching ViWait archives/roots also exist, but demo record13 is empty in
these copied retail tables and is not counted as a missing nonempty clip.

Actual source path:

1. `ftCharacterAssets_Import` owns x14 demo descriptors and their names/command
   data in the character arena. The zero-initialized decoded x14 clip pointers
   are not filled by importing descriptors.
2. Runtime Prepare imports only action xC into `animation_arena`. It retags
   both descriptor banks, copies demo root names, and sets the demo count;
   this does not decode demo clips. Package verification counts x8/tags and
   therefore currently accepts those undecoded descriptors.
3. Stock results use `gm_80160438`'s stock character filename table, then
   `fn_8017A67C` calls `Player_LoadResultMotion`. That native importer finds
   the copied result root name, imports the first ten demo records from the
   disc archive, and stores a separately allocated arena in
   `ftData_UnkIntPairs[kind].data`. It skips the alternate member for a
   transformation mapping. This is a later disc load, not package ownership.
4. Custom character IDs are absent from `gm_80160438`'s stock-only table, so
   that results model-creation branch is skipped. `gmresult.c` supplies custom
   title/winner UI through `nativeShell_ShowCustomResults`; that UI is not a
   custom model animation import.
5. Original intro/ending/ViWait paths pass HSD archives through
   `Player_80036E20` → `ftDemo_SetArchiveData`, selecting copied root names.
   `ftDemo_CreateFighter` switches x24 to the demo table. The native
   `ftData_80085B98` excludes retail pointer-patching and does not provide an
   equivalent intro/ending/ViWait decoder; `ftData_80085CD8` consumes the
   decoded pointer directly. Those contexts cannot be accepted from metadata
   names alone. Singleplayer/cinematic contexts remain deferred.

Audit receipt `A:/FreakMelee-test-archives/zelda-sheik-demo71-audit/receipt.json`,
SHA256 `119fb7c9d2fce7e1117812bdb3b11736476f47be0603d3ccb6c5849834e5adc6`.

The reusable closure would package/import explicit owned demo clip payloads
and validate each nonempty declared demo record, with actual results routing
using the registered member. It should not masquerade as tag retargeting or
depend on stock archive lookup. This report proposes no shared edit. Normal
versus entry action clips are part of the copied action bank; standalone
singleplayer intros and rendered custom results are separate capabilities.

No game, rendered results, alternate transfer, installed package or Workshop
family editing acceptance is claimed. Only the initial cloned costume is
packaged; the earlier five-costume retail census remains separate evidence.

## Later rendered SDK72 acceptance

The original71 module layout differs from72 and is correctly rejected before
entry. The normal pipeline was reproduced using current headers/tool into
`A:/FreakMelee-test-archives/zelda-sheik-normal72-runtime-worker` without a layout
bypass. Its fresh family is
`1ac7244b65e6e0664c985dfe90cccc48d50b4c9ad0314e05f5329746f323b5aa`.

`runtime.py` reproduces two stock/custom3200-input cycles. Pass the fresh normal
pipeline `receipt.json`, a fresh A: output directory, and `--run` to authorize
the isolated accept and games. Without `--run`, it only prepares input files.
All games use `--hidden --mute --fast`, retain drawing, and send no OS input.
It verifies both actual transform directions and all specials in addition to
calling the existing exact trace comparison. No user content is installed.

Both cycles passed:6400inputs,12800P1/P2states,112actions,1031itemstates,6400RNG.
Receipt:`A:/FreakMelee-test-archives/zelda-sheik-runtime72-worker/receipt.json`.
This closes actual transform activation for the tested sequences. It does not
exercise damage, KO, CPU, item-owner transfer, other costumes or workshop editing.
