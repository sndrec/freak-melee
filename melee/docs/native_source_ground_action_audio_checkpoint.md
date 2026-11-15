# Interpreter ground specials, action observations and global audio

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Seven generic operations are now connected: four ground-special input checks,
the current action category and incapacitated flag, and global audio playback.
Fighter decisions remain authored scripts. No stock-specific special handler
fallback, compatibility alias or original native kernel change was added.

## Public contracts

- [Ground special checks](scripting/ground_special_controls.md) use the native
  cached input ages and enter the corresponding authored hook synchronously.
  Side entry preserves facing and surface-friction momentum behavior. Motion
  changes are permitted; caller retirement or a nested fault prevents result
  publication and the outer script continuation. The existing ground-kernel
  missing-hook handled/no-op behavior remains explicit, including side effects.
- [Fighter observations](scripting/fighter_observations.md) expose read-only
  `action.category` and `fighter.incapacitated`. They read semantic bitfields,
  not host union byte positions. Actual receiver routing accepts stock or
  separately owned source Fighters without requiring the caller's bank.
- [Global audio](scripting/global_audio.md) accepts exact Int expressions and
  returns the native signed instance handle. Native doubling/clamping and
  high-ID fallback remain intact; only undefined signed-overflow inputs reject.
  Fighter and Article ownership is retained through result publication.

Sheik's chain Article now uses `program.attrs.chain_attachment_part` for its
two references to immutable root tuning. This is a direct ownership spelling
correction; attachment and collision behavior were not replaced.

## Verification

The client, content tool, source-assets and selected integration targets build;
16 selected CTests pass. Console comparison fixtures execute the actual
compiler, prepared programs, actors, VM, storage and fixed GObj pools. No game,
GPU or audio device was opened.

| Grouped fixture | Checks | Native comparisons | Execution coverage |
| --- | ---: | ---: | --- |
| Ground controls and specials | 36,668 | 1,453, including 676 special pairs | 1,489 source invocations |
| Fighter observations | 4,916 | 772 | 357 VM calls, including 192 receivers |
| Global audio | 499 | 25 | 9 Fighter/Article VM calls |

Ground tests run unchanged special kernels and facing/friction selection.
Selected motion, animation, input-angle and material leaves are controlled.
The existing scheduler extractor's stale PositionX anchor was repaired without
changing its remaining tail. A controlled callback-free position writer can
inject reentry; the proof is withholding the enclosing native check's output
and outer caller tail, not interruption of arbitrary inner hook statements.
Whole fighter behavior, retail stage assets and rendering remain unverified.

Observation tests replace the prior fixture's controlled read dispatcher with
exact production property routing. Typed receiver queries cover the source
bank, a separate independently prepared bank and stock targets. Package/rig
metadata and payload construction remain controlled. This is not stock-target
VM or complete gameplay acceptance.

Audio tests mechanically compare unchanged `lbAudioAx_800237A8` and
`fn_80023750` bodies with a silent AX argument/result sink. They cover the live
Destroyed hook window, signed handles, argument sampling and result publication.
They do not execute sound-bank loading, the mixer, the sound pool or audio
rollback journal. Native audio side effects are not undone by guard failure.

Current receipts, independently rehashed by root:

- `A:/FreakMelee-test-archives/script-ground-special-controls-oct10-final/receipt.json`
  (1,720 inputs), SHA256
  `4a6856f7dbfbe84facb051d989f94b1a08654a5d79deeb6433c2bdced3294e32`.
- `A:/FreakMelee-test-archives/script-fighter-observations-category-oct10-final/receipt.json`
  (1,690 inputs), SHA256
  `15dd649818f129f60f8d9a0389a564709a9c669b8f784fc78388a1ed48a20dc2`.
- `A:/FreakMelee-test-archives/script-global-audio-oct10-04/receipt.json`
  (1,818 inputs), SHA256
  `b3cd90ff22c54d7b4e3c9fa17dc0ca99a13c091d9eaed2a049103a7b3dfe9081`.

## Next integration boundary

Six of ten retained full sources now prepare: Mario, Dr. Mario, Peach, Game &
Watch, Zelda and Sheik. Exact remaining first diagnostics are Link 707 / Young
Link 711 at `controls.try_guard`, and Popo 1019 / Nana 1023 at
`collision.copy_from`. The census is preparation evidence, not full fighter
runtime acceptance. No bodies were removed to obtain a passing result.

Receipt `A:/FreakMelee-test-archives/source-ground-action-audio-census-oct10-final/receipt.json`,
SHA256 `4cc19880f49a268135a7f61ed04e828ca4cbd4e171eeba9c05a82c918a78950c`;
root verified the rebuilt tool and all ten source hashes.

Continue with those shared operations, actual paired roles/member mapping,
public resource-animation application and full fighter acceptance. Compact
whole-match save/load remains unfinished; Regions/Pages still owns broad
snapshots. The full goal remains active.
