# GObj pool scene lifetime: native home/VS/results audit

2026-10-07. Source and focused console evidence only; root owns live builds and
hidden/muted game runs. Native pool backing passed the reported root-run
eight-actor census and64 verified15-tick rollback windows at1b371. This document
does not convert that one VS workload into all-scene acceptance.

## Real native routes

`gmvsmode.c: onExitCss` routes a locked native selection to
`gmVsMode_State_Practice`. The normal home path to a match is therefore:

```
CSS -> Practice -> SSS -> VS -> Results -> CSS
CSS -> Practice -> CSS
VS -> SuddenDeath -> Results -> CSS
```

CSS->VS directly is a controlled shortcut, not the current home flow. A practice
match uses `GS_VS` but has a different mode-state ID from normal VS. These IDs
must remain distinct in logs/fixtures. Source `gmVsMelee_ExitVs` chooses Results
or SuddenDeath through the actual multiple-winner result; the next scene creates
new objects with a new pool epoch, rather than migrating the prior fighter slots.

## Source entry ordering

The complete current `gm_1A3F.c: gm_801A4014` orders operations as follows:

1. Resolve current mode state; prepare netplay when requested (object census
   refuses online). Retire the prior GObj/proc logical scene before preload.
2. `preloadState`, then mode-state `on_enter`, then `gm_801A4BD4`.
3. `gm_801A4BD4` runs actual HSD registry initialization and creates the GM no-op
   GObj/proc. Then set current GameSceneInfo and invoke scene `on_enter`.
4. Run the complete scene frame loop. Invoke scene `on_exit`, prepare tied-match
   fighter assets if necessary, shut down netplay, and invoke mode-state `on_exit`.
5. Resolve next state and finish movie cleanup. Retire this scene only on entry
   to the next state, before its preload can reclaim old userdata/model heaps.

Thus the disabled pool interval covers preload and mode-state data preparation,
but ends **before** scene actor/model construction. Scene and state exit callbacks
still see live current-epoch GObjs. Retirement runs no new userdata/model
destructor sweep, clears actual roots and expires private references. Original
ordinary GObj removal/deferred deletion still runs its real HSD callbacks.

## Audited concrete call chains

| Mode state | Before HSD registry init: mode `on_enter` chain | After HSD init: scene construction |
| --- | --- | --- |
| CSS/home | `onEnterCss` -> `gmVsMelee_EnterCss`: settings restore, CSS/VS data copy, `lbDvd_SetupVsPreloadCache`; then `nativeHome_EnterSelection`: home flags, rumble mapping, local-player data and netplay-clear. | `mnCharSel_Scene_OnEnter` imports CSS assets/text, then `mnCharSel_802640A0` creates camera/menu/display/token GObjs. Its source constructors run in the scene handler, after bind. |
| Normal VS | `onEnterVs` -> `gmVsMelee_EnterVs`: `gm_80167BC8` fills rules/preferences, copies players, subcolors, rumble settings and `gm_LoadAnnouncer`. Optional practice stage calls `nativePractice_Configure` on StartMeleeData. | `gm_Scene_Vs_OnEnter` -> `fn_8016E730`: camera, effects, player state, collision, stage, item/fighter setup and match GObjs; then HUD/timers and workshop presentation hook. |
| Home Practice | `onEnterPractice` -> same `gmVsMelee_EnterVs`, then `nativeHome_EnterPractice` -> `nativePractice_Configure`: rules, dummy PlayerInitData, editor-preview flag and frame-end callback selection. | Same VS scene handler with practice stage/config. The frame-end sandbag callback performs fighter lifecycle work later during gameplay, not while mode data is configured. |
| Results | `onEnterResults` -> `nativeHome_EnterResults` captures local input mapping; clear netplay; `gmVsMelee_EnterResults` -> `gm_80177724` zeroes ResultsMatchInfo, then copies MatchEnd. | `gm_Scene_Results_OnEnter` loads result assets, creates light/model GObjs, result display proc, player model/fighter/camera presentation. Its CreateGObj calls occur in the later scene handler. |
| SuddenDeath | `onEnterSuddenDeath` -> `gmVsMelee_EnterSuddenDeath`: copy rules/players, subcolors/rumble, `gm_SetupSuddenDeath` changes stock/damage/eligibility data. | `gm_Scene_SuddenDeath_OnEnter` configures sudden-death flags then delegates to the VS scene constructor after the new epoch is bound. |

The examined mode-data helpers do not call CreateGObj/SetupProc. Relevant leaves
include `gm_80167BC8`, `gm_SetupSubColors`, `gm_LoadRumbleEnabled`,
`gm_RumbleEnabledForPlayer`, `nativePractice_Configure`,
`grPractice_SetEditorPreview`, `gm_80177724` and `gm_SetupSuddenDeath`.

Audio has an actual GObj constructor in `lbAudioAx_800263E8`, so a keyword search
of the entire audio file cannot prove this boundary. `gm_LoadAnnouncer` calls
`lbAudioAx_80026F2C`, `lbAudioAx_8002702C`, `lbAudioAx_80027168` and
`lbAudioAx_80027648`: these operate on bank loading/masks/completion. The envelope
GObj constructor is separate and is not called by that examined bank path.
Asynchronous file/bank completions still need live transition checks; do not
infer every future callback is harmless from this list alone.

Preload's Toy_803127D4 and tyDisplay_8031C8B8 bodies reset cached pointers. Heap
recreation runs HSD class/object/ID/RNG/RObj forget operations, not an introduced
GObj destructor pass. Persistent preload assets are not persistent live GObj
registries. Native workshop exit releases its retained presentation while the
current scene is still active (`nativeHome_ExitPractice`); its source helper
must not be moved after retirement.

No concrete CreateGObj-before-bind defect was found for these listed native
home/VS/results/sudden-death mode-data paths. That is a scoped source conclusion,
not proof for every game mode, custom callback or asynchronous completion.

## Focused source-based lifetime fixture

`tests/native/native_gobj_scene_lifetime/` mechanically extracts complete current
`preloadState`, `firstState`, `nextState`, `findState`, `gm_801A4014` and
`gm_SetNextGameModeStateId` bodies and private routing types without rewriting.
It links actual HSD GObj/proc pool, constructor, priority, callback, drawing and
retirement code.

The controlled boundary hooks model mode-data preparation, scene asset/actor
construction, preload heaps and scene frame loops. They use fixture-owned GObjs
to verify actual pool ownership and phase order. **The real CSS/results/practice
assets, menu logic, roster selection and result rules are not executed here.**

Current receipt:
`A:/FreakMelee-test-archives/native-gobj-scene-lifetime-second/receipt.json`.
Thirteen transitions,417 assertions,13 real proc ticks and13 real source draw
callbacks; one tied-match preparation and zero retirement destructor calls.
It verifies retired objects cannot allocate/resolve during preload or mode entry,
each scene bind advances the epoch, exit callbacks retain live current ownership,
old references expire, and normal next-state routing reaches the supplied routes.

```powershell
python tests/native/native_gobj_scene_lifetime/run.py --compiler .local/baseline72/compiler --output A:/FreakMelee-test-archives/native-gobj-scene-lifetime-check
```

The runner uses CREATE_NO_WINDOW, hashes the actual source bodies/current inputs,
and writes fresh A: compile/fixture/receipt logs. It launches no game and changes
no production GM code. Broader allocation/model ownership is still page-tracked.

## Root live transition handoff

Current replay/record CLI requires `--match`, which starts directly in VS and
skips home CSS. Existing replay-only fixtures therefore cannot prove the full
home route without new GM replay support. No new replay flag or permissive
allocator fallback has been introduced for this audit.

Use normal offline startup with `--hidden --mute --object-census` and an isolated
content root. Set `FREAK_MELEE_CONTROL_ID` to a unique local channel and
`FREAK_MELEE_OBJECT_CENSUS` to a fresh A: output. Root can use the existing
`native_control` mailslot virtual pad/UI commands and screenshot capture to
operate the observed hidden scene. No OS keys, sound or visible game window are
needed. Exact screen coordinates and selection defaults require an actual
root-observed screenshot/state; do not fabricate a coordinate script from source.

Required root-run evidence:

* Home CSS -> Practice -> home CSS, including workshop restart/release if used.
* Accepted home match route through SSS -> VS -> Results -> CSS. Let the actual
  match-end/result logic choose the route, rather than forcing a scene index.
* Optional real tie -> SuddenDeath -> Results -> CSS when readily reproducible.
* Every entered scene prints a census with errors0, sensible actual GObj/proc
  counts and no initialization/storage-retirement assertions. Confirm no old
  actor/model reference is used after heap reclamation and the next epoch starts
  cleanly. Forced shutdown flushes the last census; it does not prove the next
  scene's constructor.

Use bounded launch/interaction timeouts and fresh logs. Census mode is untimed;
no result here establishes performance. Do not label all scenes accepted until
those actual asset/UI transitions complete without errors.
