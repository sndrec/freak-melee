# Owned animation and results creation

Run from the repository root:

```powershell
python tests/native/fighter_owned_results/run.py --snapshot .local/baseline72 --output A:/FreakMelee-test-archives/fighter-owned-results-repro
```

The output must be new. The receipt hashes the immutable support SDK/compiler, live owned importer/header and extracted source bodies. It does not alter the snapshot. This fixture intentionally uses the live migration of the bank header ahead of SDK72; all other types and the DAT/FigaTree decoder are frozen72 inputs.

Initial acceptance: **7,927 checks**, `A:/FreakMelee-test-archives/fighter-owned-results-final/receipt.json`.

Results-transition regression acceptance: **9,224 checks**, `A:/FreakMelee-test-archives/fighter-owned-results-reset-fix/receipt.json`. The fixture additionally extracts both frozen72 pre-fix and live corrected `ft_800852B0` bodies. Earlier `first` and `second` outputs preserve include/type fixture compile errors; no engine behavior failed. This is console source evidence, not rendering/gameplay acceptance.

The actual `ftAnimationBank_Import`, `HSD_DatOpen` and `lbAnim_DecodeFigaTree` bodies decode a controlled three-clip DAT bank into one arena. Checks distinguish action and demo tree storage, mutate a demo tree without changing the action tree, preserve deliberately empty descriptors, allow only absent optional demo symbols, reject present malformed/nonfinite trees, reject missing mandatory actions, and report the failed table/index. The tiny controlled clips test storage and validation; the separate `fighter_demo_copy` matrix owns real retail-byte provenance/decoder evidence.

Actual extracted `ftDemo_HasResultClips` is checked across all 1,024 masks for variants -1 through 5. Result requirements are Win1 0/1, Win2 2/3, Win3 5/6, and looping Lose 9. Empty descriptor4 is not a required clip. Actual extracted `Player_CreateDemoFighters` uses real player/allocator structures and the original mapping type: alternate result selection chooses the member matching the final fighter kind; simultaneous families create both in immutable order. Invalid foreign selected kinds and unregistered alternate members do not create a model, and the global mapping remains unchanged. The `ftDemo_CreateFighter` allocator/model construction and runtime Definition lookup are explicit controlled sinks, not real asset-loading or renderer proof.

Production result admission skips stock archive loading for owned fighters, verifies required decoded entry/loop clips for every created member, and warns only on results entry when missing. Gameplay import accepts an edited older project whose optional named demos are absent; no donor fallback is added. Present corrupt optional clips still reject registration. Results camera/scale use common configuration with unit scale and zero character offsets for custom kinds, never a stock donor. Missing models still retain actual custom camera metadata. The camera creator now returns its created GObj, closing its previous missing-return undefined behavior.

Remaining product evidence: real demo fighter creation, GPU appearance and paired match-end results are parent-owned coordinated acceptance. Retail result portrait/taunt textures still receive arithmetic `gm_80168B34` frames for custom CSS kinds in `gmresultplayer.c` (464/1078/1096/1113); this is an identified presentation gap, not a source array bounds claim. Existing custom text overlay and neutral emblem handling are separate accepted paths. Victory themes/announcer lookup safely have no custom match.

## Real integration failure and reset correction

The hidden rendered parent run preserved at `A:/FreakMelee-test-archives/owned-demos72-results-root/zelda-game.log` successfully played the newly demo-owned Zelda family but then reported missing decoded variant0 clips. The confirmed source path was `fn_8017AA78 -> ftDemo_ObjAllocInit -> Fighter_800679B0 -> ft_800852B0`. The latter cleared `gFtDataList` for every runtime kind and also nulled the active package's actual costume entry. Package Definitions and malloc-owned arenas remained active. This was a publication reset, not an import failure.

The actual reset now preserves only kinds with an active owned Definition. The fixture runs the previous reset and demonstrates that the result clip gate becomes false, then the corrected reset and demonstrates that owned data, decoded clips, costume joint/material/native-assets pointers and animation-bank publication survive. Ordinary retail fields and common bookkeeping still reset. A later controlled unregistered-Definition pass clears the formerly owned entries, checking the exemption is tied to current ownership; actual package retirement/free remains covered by the existing runtime lifecycle matrix.

`ftData_80085B98` intentionally does not need a `pair.data` value for owned clips. Its native build does not apply raw archive offset rebinding; it allocates per-instance scratch and reads the retained demo count. `ftData_80085E50` returns the decoded table's `x14` directly. Demo instance unload frees those per-instance scratch objects, not the package animation arena; the arena remains owned by `release_slot`. This inspection adds no donor/archive fallback.

Root independently reproduced the updated fixture: **9,224 checks**, receipt `A:/FreakMelee-test-archives/fighter-owned-results-reset-root/receipt.json`.

The rebuilt hidden/muted client now reaches inspected winning results with Zelda, transformed Sheik, and both Ice Climbers, using the newly owned paired packages. Receipt: `A:/FreakMelee-test-archives/owned-demos72-results-fixed-root/receipt.json`. This is one selected victory variant per run, not all loser/team/costume branches or exact stock image parity. Every task-owned test client was force-stopped after executable-path verification. The original failed rendered run remains evidence and is not reclassified as a pass.
