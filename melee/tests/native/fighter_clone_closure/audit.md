# Supported fighter clone source-closure audit

2026-10-11 update: current Workshop selection now uses `samus_full.fms`, making
all 26 supported forms independently declared. A fresh normal source audit of
the selected templates against copied owned drafts passes 26/26; see
`A:/FreakMelee-test-archives/clone-roster-current26-oct11-a/receipt.json`.
This proves complete body/metadata/package preparation, not exhaustive gameplay
acceptance. The earlier read-only census and pinned hashes below are historical;
its missing Samus rig/grapple/lifecycle statements are superseded by
[the gameplay checkpoint](../../../docs/native_source_samus_gameplay_checkpoint.md).

Read-only census, 2026-10-04. Checkout HEAD: `b3a7b0b39e6a78c6c9139e2bd651675a8e6acd9f`. This audit changed only this file; it ran no build, comparison suite or game. Source hashes below pin the inspected implementation, including uncommitted work if any. Kirby, bosses and adventure enemies are excluded. General common engine mechanics, immutable retarget rig tags and shared particle/model recipes are permitted; borrowing a stock fighter callback or private fighter state is not.

**Conclusion: 26 copied bases are supported; 25 selected member templates declare independent behavior, while Samus remains deliberately inherited/partial. Owned demo clip construction for all 26 does not establish 26-fighter behavior independence.** The 25 independent declarations remove live donor callbacks, but this is not a renewed exhaustive behavioral acceptance claim. There are concrete remaining demo-presentation omissions and a deferred video-demo selector path.

## Actual selected sources

Historical census: the shared Fox/Falco source paths and hashes below describe
the earlier audit. Current Workshop selection uses `examples/fox_wip.fms` and
`examples/falco_wip.fms`; see
[the interpreter article checkpoint](../../../docs/native_script_fox_falco_articles_checkpoint.md).
The historical hash table is not a current acceptance receipt.

`src/melee/ft/native_fighter_runtime.c:131–158` registers the 26 raw copied bases. Workshop's real selector is `src/melee/ui/native_shell_workshop_editor_script.cpp:94–120`, rather than an arbitrary similarly named example. Fox/Falco and Pikachu/Pichu share templates, so there are 24 selected files.

| Member(s) | Workshop template | Source policy |
|---|---|---|
| Mario | `examples/mario_full.fms` | Independent declaration |
| DrMario | `examples/drmario_full.fms` | Independent declaration |
| Captain | `examples/captain_wip.fms` | Independent declaration |
| Ganon | `examples/ganon_wip.fms` | Independent declaration |
| Luigi | `examples/luigi_wip.fms` | Independent declaration |
| Purin | `examples/purin_wip.fms` | Independent declaration |
| Fox, Falco | `examples/fox_falco_wip.fms` | Independent declaration |
| Mars | `examples/marth_wip.fms` | Independent declaration |
| Emblem | `examples/roy_wip.fms` | Independent declaration |
| Donkey | `examples/donkey_wip.fms` | Independent declaration |
| Koopa | `examples/koopa_wip.fms` | Independent declaration |
| Pikachu, Pichu | `examples/pikachu_pichu_wip.fms` | Independent declaration |
| Samus | `examples/samus_wip.fms` | **Inherited / partial** |
| Link | `examples/link_full.fms` | Independent declaration |
| CLink | `examples/young_link_full.fms` | Independent declaration |
| Mewtwo | `examples/mewtwo_wip.fms` | Independent declaration |
| Yoshi | `examples/yoshi_wip.fms` | Independent declaration |
| Ness | `examples/ness_wip.fms` | Independent declaration |
| Peach | `examples/peach_full.fms` | Independent declaration |
| GameWatch | `examples/gamewatch_full.fms` | Independent declaration |
| Zelda | `examples/zelda_full.fms` | Independent declaration |
| Seak | `examples/sheik_full.fms` | Independent declaration |
| Popo | `examples/popo_full.fms` | Independent declaration |
| Nana | `examples/nana_full.fms` | Independent declaration |

Raw clone C Definitions intentionally preserve their original `source_kind` and motion table as unscripted construction bases. That alone is not a defect: generated independent modules replace those decisions. A raw clone installed without its authored module remains a stock-behavior clone, not an independent fighter.

## Covered implementation boundaries

- **No inherited fighter callback table for independent modules.** `tools/native_fighter_script.c:302–343` requires authored load/load-special-attributes, sets `source_kind=None`, clears every registered fighter handler plus inherited item/article logic, and requires replacement of each originally nonnull custom animation/input/physics/collision phase. `src/melee/ft/ftdata.c:1518–1566` clears stock tables when the source is None. `src/melee/ft/native_fighter_runtime.c:1209–1284` then publishes owned callbacks, motion data, costume, action/demo banks, effect metadata and CPU tables. `BehaviorKind` at `native_fighter_runtime.c:514–528` returns a custom identity for None, rather than secretly restoring a donor kind. Direct common motion-policy bindings are explicitly common engine callbacks (`tools/native_fighter_script_motion_policy.c:72`), not stock fighter special delegates. The phase guard does not prove every stock-kind branch elsewhere is covered.
- **Owned rig and article lookup.** All 25 independent selected members have explicit `.bones` in their clone Definition; Samus alone lacks it. Stock part/omission copying is gated by non-None source at `native_fighter_runtime.c:804–852`. `ResolveArticleForItem`/`ArticleLogic`/`ArticleAssets` at `530–583` use active Definition bindings, current owned data and owned logic. Literal asset-kind versus selected article-declaration census found only `Samus_GBeam` missing; no other selected base's literal article binding lacked a matching authored declaration. This is a declaration census, not proof every emitted item invocation has the correct behavior.
- **Current reset and CPU entry.** `src/melee/ft/fighter.c:587–658` snapshots generation after the new-life reset, calls the rebound OnDeath and then actual CPU initialization. `src/melee/ft/kinds/ftCommon/ftCo_0A01.c:694–716,890–904` supports authored early history preparation and initialization; later CPU policy hooks precede stock branches. Nana's full selected source declares prepare/initialize/spacing/stick-step/stick-limit/drop/mode/output at `examples/nana_full.fms:1497–1565`; Popo's capture policy is scripted. There is no remaining need to impersonate Nana for the accepted follower preparation path.
- **Shared kind branches with explicit authored decisions.** Examples include angled motion (`src/melee/ft/ft_0892.c:277–280`), taunt (`kinds/ftCommon/ftCo_AppealS.c:62–71`), air tether (`ftCo_AirCatch.c:60–72`), dynamics wind/interaction/ground policy (`src/melee/ft/ftdynamics.c:394–421,593–600`) and friction (`src/melee/ft/ftmovement.c:465–477`). The selected Fox, Link, Yoshi, Peach, Mewtwo, Bowser, Marth/Roy and Popo/Nana scripts author their exceptional decisions rather than invoking those stock handlers. Purin's additional attachment dynamics are skipped centrally by `ftdynamics.c:42` using the owned attachment mask, so the remaining raw Purin reset-loop shortcuts are not by themselves evidence of a live hat donor dependency.
- **Paired actors.** `native_fighter_runtime.c:1106–1123` rejects a two-member package unless both members have modules and None source, and alternate forms have owned activation. Zelda/Sheik authored activation is at `examples/zelda_full.fms:690` / `sheik_full.fms:962`. Popo/Nana use family IDs and typed exports, with the healing/Sleep policies at `nana_full.fms:1473` / `popo_full.fms:1331`. The primary/companion and alternate-form metadata are distinct; stock character IDs are not a fallback companion. `src/melee/pl/player.c:2192–2214` selects the actual owned ending form and constructs both companion demo actors when appropriate.
- **Owned demo bytes now exist.** `src/melee/ft/native_fighter_project.c:193–250,281–336` materializes all populated demo descriptors into the existing owned animation file at stock-disc construction, including explicit physical Nana source resolution. `native_fighter_runtime.c:925–930` decodes both action and demo tables; absent optional demos remain absent without a runtime donor archive. `src/melee/gm/gm_1798.c:559–590` verifies decoded owned result clips and the actual selected member before making a result model. `src/melee/ft/ftdemo.c:51–65` checks the chosen win/lose entry and wait clips. These are asset/runtime dispatch facts; this audit does not rerun the earlier 343-clip constructor evidence or prove every presentation item policy.

## Concrete remaining implementation

### 1. Samus: genuine inherited-versus closure gap

`examples/samus_wip.fms:1–3,1012–1015` explicitly leaves Grapple and load/death native; it has no independent declaration. The selected base `src/melee/ft/kinds/ftSamus/ftsamus_clone.c:10–18,32–54` retains the stock motion table and special callbacks, four article assets, no explicit bone/omission mapping. Three combat articles are authored at `samus_wip.fms:583,702,903`; **there is no Grapple Beam article declaration**. Retaining `source_kind=Samus` does not make a custom live `fp->kind` equal Samus: the raw-kind common grapple branches below can also omit their stock path for this partial custom clone. Inheritance is not proof that the unported grapple currently works.

Actual outstanding decisions include:

- Ground grab, pull/wait, release, throws and air tether; Samus branches remain in `src/melee/ft/kinds/ftCommon/ftCo_AirCatch.c:107–211`, `ftCo_0D8E.c:192`, `ftCo_0D95.c:28,117,176`, `ftCo_Throw.c:159–161`, and the catch common callers. Custom states357/358 still use common callbacks containing those private Samus decisions (`src/melee/ft/kinds/ftSamus/ftsamus.c:208–237`).
- `src/melee/it/kinds/itsamusgrapple.c:40–49` owns nine native physics states, constructors, linked-node solver and fighter callbacks; it cannot be treated as an ordinary engine item simply because other resource articles are now generic.
- Stock OnDeath resets six private fields at `ftsamus.c:260–270`; the script has only a secondary-death reset at `samus_wip.fms:1020–1024`, and explicitly warns the ordinary missile serial is not reset by native union writes. Stock OnLoad/register/attribute scaling and throw accessory setup are `ftsamus.c:272–284,328–350,352–376`. Their decisions need ordinary script variables/owned tuning and explicit rig/attachment metadata, not retained native union state.

Bounded next Samus batch should first freeze a source inventory of this grapple/catch/throw closure and author load/death resets plus the exact rig, then translate its article/linked solver and common protocol entries. Existing resource, capsule, pose, catch, current-reference and attachment APIs should be reused; audit any truly missing typed operation before extending core. Do not add None until all motion, lifecycle and article decisions are covered.

### 2. Fox/Doc: small authored demo-presentation gap

The actual shared demo entry table is `src/melee/ft/ftdemo.c:43–48`: mode0 calls `ftCo_800BECB0`, mode1 `ftCo_800BED88`, mode2 `ftCo_800BEF04`, mode5 `ftCo_800BF034`, mode6 `ftCo_800BF108`. These shared bodies consult `on_presentation_item` before raw stock-kind switches:

| Policy omitted by current independent source | Actual source sites |
|---|---|
| Fox gun, modes0/1 | `src/melee/ft/ft_0BEC.c:49–63,114–130` |
| Fox gun, modes5/6 | `src/melee/ft/ft_0BF0.c:37–50,78–90` |
| Doc held pill, mode6 | `src/melee/ft/ft_0BF0.c:96–103` |

`examples/fox_falco_wip.fms` has neither `on presentation_item` nor `on demo_enter`; a None/custom identity consequently receives no gun from these switches. **This is an omitted owned presentation decision, not stock delegation.** Falco's absence must remain intentional: the original switches are Fox-only. The blaster's presentation state10 is already scripted at `fox_falco_wip.fms:1252–1258`, but no presentation producer initializes and selects it.

Doc's actual `on demo_enter` at `examples/drmario_full.fms:318–333` owns the two mode1 result pills. It does not cover the separate mode6 single held pill. Ness already authors the corresponding bat policy (`examples/ness_wip.fms:1590–1601`), so no new Ness gap is inferred.

`ftDemo_CreateFighter` invokes the Definition `on_demo_enter` **after** shared mode entry and a live-generation check (`ftdemo.c:143–154`); it is not a preconstructor replacement. Exact producer source is `src/melee/it/kinds/itfoxblaster.c:514–552` (zero-velocity, owner-centered spawn, commands set to1 before AB54 pickup), and the already-audited pill constructor for Doc. Existing generic `articles.create_with_velocity`, signals/commands, `items.attach(handle,raw_part,false)`, held-slot binding and explicit scale suffice in principle; validate real ordered construction and generated callbacks rather than route back to stock producers. `tools/native_fighter_script_primitives.c:428–476` contains those generic attachment/binding surfaces.

**Recommended next autonomous slice:** complete Fox/Doc authored demo presentation using existing APIs and focused original/generated constructor + mode0/1/5/6 comparisons, including absent creator/item allocation and callback invalidation. This is materially smaller than Samus full grapple closure and does not require a new fighter-specific Host or behavior-kind workaround.

### 3. Wider video-demo modes remain deferred, despite copied clips

`ftdemo.c:107–109` directly calls `ftData_UnkDemoCallbacks0[kind]` for mode>=9. `ftData_AssignStockCallbacks(None)` clears that selector, and only stock Mario/Luigi (plus excluded Kirby/boss) provide it (`ftdata.c:1468–1501`). `ftCo_800C70D0.c:28–45` / `ftCo_800C7178.c:28–45` still use stock behavior identity to choose Wait/Walk instead of the general video entry. Copying clips14/15 does not author these mode selectors or the alternate video motion table. A custom mode9/10 attempt can reach a null callback before `on_demo_enter`; this is outside current versus/results0–6 ownership, not a reason to restore live donor handlers. A later generic demo-range/motion-table contract and guarded unsupported-mode entry is needed if those scenes become supported.

## Limits and non-gaps

- This audit distinguishes raw copied bases, selected source templates and registered generated modules. It did not build each current source into a DLL or refresh prior gameplay receipts; independent declarations and source-phase guards alone do not establish full behavioral parity.
- Immutable construction source identifiers, rig retarget tags, effect/sound IDs, engine ordinary items and common callbacks are intentional shared engine data/mechanics. They are not a license to resolve live stock fighter attributes, callbacks or animation archives. The checked owned article/result lookup paths do not do that.
- Kirby interactions/copy machinery and adventure `itzako` display queries are excluded from this closure verdict; their raw stock tables are not counted as implemented custom-copy support.
- Old ledger prose or source comments saying a component is pending were not used as implementation truth where the live selected source already composes accepted components. The reverse also applies: a constructor asset receipt is not a whole-fighter closure receipt.

## Source pin (SHA-256)

Selected template files and decisive implementation inputs, in inspected order:

```text
src/melee/ui/native_shell_workshop_editor_script.cpp 2947fd99732d55480f585323694e9213a5ab9637ce5c16dd329ac837528cd8a9
tools/native_fighter_script.c 3caa1e191af3bc344c8788944537ddf857a94ff10608d7cf7a5c1b1b80397da8
src/melee/ft/native_fighter_runtime.c 3296974d182ef02a9bbd9a10a6a1e7e106631286e4b732c37df2d8a6cc9f7b89
src/melee/ft/ftdata.c 1109bdde60f875f63548ed4dc24ca46bf5d8f4dbb28abf464222ac82fe665004
src/melee/ft/fighter.c 60e5774c7fb1e34f2d2ba619d768777d8ea6930352f5791a6eb5506f143d7f41
src/melee/ft/ftdemo.c 3217dbe90b0e957cb9a67c946f6c8bb10d629beaba8baafecc66044883ac8402
src/melee/ft/ft_0BEC.c 3ef060bd2ee8c7c642c52833f24ce222b1db06f2eb3ac10c0609400ead515e84
src/melee/ft/ft_0BF0.c 56decc0a089c08098f4ce1606318af96791d4c814301de2ae9f0a5cfaf0464c5
src/melee/ft/kinds/ftSamus/ftsamus_clone.c 17159d9085acb17176fdefc367f76a227491077bbc509b2811952831a76b67d4
src/melee/ft/kinds/ftSamus/ftsamus.c 4354a04f30d030426e3faf6790b6e5af10cd3db5aad320d63e161b2d179828db
src/melee/it/kinds/itsamusgrapple.c ee68aeedf8ff1bb23145b73b510bf468265e854125d4dc420e2e79faef5d478a
src/melee/ft/native_fighter_project.c a7ee199292fba86bf53a233667b677d415e7edbfe5c2189aedda388b2bd0db76
src/melee/ft/native_fighter_demo_copy.c a38c84dff9d5e6022bd30a468f3e7e9607308f4841b1452c7f6bd26e03c04788
src/melee/gm/gm_1798.c 172913c65c88fcef700510a77114de201ab68cdcdcb9dcca37c524bd7f870744
examples/mario_full.fms ec3242459f3fd661924fd55ba320a71fe987f09d7746fb63833571a16b018e59
examples/drmario_full.fms 58506c117e515664dc01eb3fdc429247f3dcf18ba6295a39d73c2ef286c9a428
examples/captain_wip.fms f1a20ef4a0ce91547be81fc062c6c428d9b3826b321c9adfbcc2d98408cffde7
examples/ganon_wip.fms e90c3d447cbda712d8e6ad29e188b04af668e5ca5fe42eba13e516be1b07a9c7
examples/luigi_wip.fms 10894f6adb7ddbb4f0deb691bb18b7e0cc0a0774ee0c4c317920ff4d4ad62b56
examples/purin_wip.fms 305aa624b401116d74fdd0af06be39253ef04a5bb3923fe2a6bce7521f1dbde8
examples/fox_falco_wip.fms efb31457e20b147323874df4ef7fdb7bf640bfed27d24e93ba57773416788501
examples/marth_wip.fms a0820475af37c04e906c64391fc2986c2752a6024edea026c1ce1f65b1ce55fd
examples/roy_wip.fms c976f3ad914a75dec4750776eda3a3de4766a8e2c5ae7e4725e80313abba73ed
examples/donkey_wip.fms f801e232aad2b7807082c464fe5fa37572e2b68279a73bd8e5a9df51aa725f4f
examples/koopa_wip.fms fdd9f6f210333ce69187c15523f1ff4837ed7e25a8cc62c6afd971c2dcc51ae3
examples/pikachu_pichu_wip.fms 5a45a79615093f6db2c681a85fbb4c53f214935f9db6f6fa100ef2c4fd03bf0d
examples/samus_wip.fms 2e020bddaa3ec12e8b2bbaad94bb466fe66a8ef3072bc1c87ff3092ac09f69fb
examples/link_full.fms c269f02365164491342cc2f635ed79078ba37fccef07f74afaab0b6e543ad076
examples/young_link_full.fms 37de10b2191327fb23cf9d2d54f98de0926e95dabcd6aaa3e8f57b0ee5d3b2d2
examples/mewtwo_wip.fms e4384f663a31ba8ebe7e38cfcb11bdfc57133b7385c553c8f4fd9c87bd4aaaa0
examples/yoshi_wip.fms fa722b73ce41fc012c46c64bf50a1b06f65a73086943fd54769a1106d2b71c27
examples/ness_wip.fms e093fc133ae903372b2b08c5295cba080f277a3ccf66ffb6bb287c761b92ee0f
examples/peach_full.fms 945402b1fe8949dd113256ca4ce139c20d8ad3bec9b94f10451a658eaaf0bcb7
examples/gamewatch_full.fms 433ab2aabf26912cdc6b4e60aeb26c4d6aec39aedcd500d9a0ce4232b530d597
examples/zelda_full.fms c7bd4e913df9f8638dfd32b0dc45fa2490cffe78c1dcd22b5862ed19b59cb5fa
examples/sheik_full.fms 8bd438c78ddaadb4b1f0e835031029c67bdc3a95519d048a717f6f78e27b36aa
examples/popo_full.fms 80d8b09103b741b2429a3a610514b5fed1301852f7173d66043fe9d0f6c2feb2
examples/nana_full.fms ee531c36f25b587d0aca9a53e07005513089f2f2776212e5370ba0751041f497
```
