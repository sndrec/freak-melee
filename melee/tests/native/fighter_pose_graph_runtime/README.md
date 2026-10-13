# Owned fighter pose runtime source proof

Engine source is stable for SDK63 integration. This namespace does not activate Sheik Chain or any stock clone policy.

## Scope and exact source boundaries

`fixture.py --root REPO --output DIRECTORY` mechanically extracts the complete three public owned runtime resolvers and eight shared pose bodies and the actual descriptor iterator into `runtime.inc` and `shared.inc`. It writes SHA-256 source provenance in `sources.json`. `test.c` compiles those bodies with the real `native_fighter_pose.c`, `native_callback_lifetime.c`, and `ftparts_table.c`.

The synthetic fixture controls only the four package slots, selected costume lookup and lower SRT/track-installation dependencies. It observes real shared copy/blend/load calls and operands, flag/omission branches, compensation selection, owned descriptor records, topology rejection, bank identity and clip bounds. Shared interpolation mathematics, HSD allocation/animation callbacks and complete package loading are not simulated acceptance claims.

The clip arena is actually allocated at the production 2 MiB capacity. Owned clip lookup checks bank provenance before reading the table. Tests include absent/replaced/out-of-span banks, PAGE_NOACCESS bank/clip/source pointers, full-width generation reuse, malformed nodes/streams, alternate costume identity, cycles, duplicate physical parts, omissions, actual iterator depth29/30, nested copy-costume forest boundaries and finite nonzero compensation scale. Fresh lookup of a recycled live fighter may resolve its new data; a continuation carrying the old generation fails the actual registry predicate.

## Console result

Run from the repository:

```powershell
python tests/native/fighter_pose_graph_runtime/run.py --root . --baseline .local/baseline62 --output A:/FreakMelee-test-archives/pose-runtime-final2
```

The final standalone run passed **1,061 checks**. Evidence is `A:\FreakMelee-test-archives\pose-runtime-final2\{result.json,compile.log,run.log,sources.json}`. The receipt pins all live source headers, Dolphin headers, complete extracted source units and frozen62 compiler files. This is the changed runtime candidate built by that compiler; it is not represented as an unchanged SDK62 runtime. Earlier diagnostic attempts are retained separately.

Root CMake recipe: custom command produces `runtime.inc`, `shared.inc`, `sources.json` with the exact source dependencies listed by `fixture.py`; target sources are `test.c`, `native_fighter_pose.c`, `native_callback_lifetime.c`, `ftparts_table.c`; include generated directory, `src`, Dolphin include; define `MELEE_NATIVE`, ordinary console math/CRT defines. No game process is needed.

## Retail asset acceptance

```powershell
python tests/native/fighter_pose_graph_runtime/run_retail.py --root . --baseline .local/baseline62 --iso "../Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/pose-retail-final2
```

The retail console test passed **822 checks / 40 graph-clip-destination combinations**. It imports the complete actual `PlSk.dat` and `PlSkAJ.dat` through production import units, plus all five actual Nr/Re/Bu/Gr/Wh costumes. The real owned resolvers and validator accept both slot4/5 child graphs, both Chain clips305/308, all eight operation categories and both destinations (where valid). Retail `PlCo.dat` supplies the exact semantic map and confirms 58 physical/runtime parts and zero omissions. This is not a hardcoded identity rig.

Actual `ftParts_80074E58`, `ftParts_SetupParts`, `ftParts_8007462C` execute to produce the part map and initial flags. The allocator uses fixed arrays. JObjs are materialized with exact descriptor preorder/parent/sibling edges, flags and Euler/SRT channels; instance children are not descended. DObj/material construction is deliberately omitted (zero DObjs), so material class updates and live HSD allocation are controlled. Dynamic-bone initialization and animation-driven b5 mutations are not executed here; the synthetic fixture separately exercises b0/b4/b5 combinations. Quaternion evaluation, AObj evaluation, interpolation math and Chain's authored control policy remain separate scopes.

Evidence: `A:\FreakMelee-test-archives\pose-retail-final2\{result.json,compile.log,run.log,retail_inputs.json}`. The receipt pins the linked runtime/engine libraries, source/header/compiler inputs and exact extracted retail archives. The libraries are SHA-checked again after running. They come from root's coherent SDK63 candidate build; the local compiler is frozen62 and this test is not described as an unchanged SDK62 runtime.

CMake retail recipe (root ownership): run `fixture.py` plus `retail_fixture.py --root REPO --iso MELEE_ISO --output DIRECTORY`, generating `runtime.inc`, `shared.inc`, `rig.inc`, `retail_setup.inc`, `sources.json`, `retail_inputs.json` and extracted DAT files. Setup dependencies are `ftparts.c`, `ftparts_bind.c`; iterator dependency is `ftanim_tree.c`. Target `retail_test.c` + real `native_fighter_pose.c`, `native_callback_lifetime.c`, `ftparts_table.c`, with the same full library links as `native_fighter_character_assets_test` and `/opt:ref`. Include generated directory/src/Dolphin; ordinary MELEE_NATIVE/CRT/math defines. Run the console target with the generated directory argument; copy/search the same Aurora DLLs as the existing asset test. No game or scene initialization is required.
