# Fox/Doc authored demo presentation

This source-only slice adds Fox's presentation gun and Doc's Sleep pill to the actual production templates. Falco deliberately creates no presentation item. It does not claim gameplay, renderer, complete demo asset, or whole-fighter acceptance.

Run from the repository with a fresh output directory:

```powershell
python tests/native/fighter_demo_presentation/run.py --snapshot .local/baseline72 --output A:/FreakMelee-test-archives/fighter-demo-presentation-root --iso '../Super Smash Bros. Melee (USA) (v1.02).iso'
```

The runner performs real frozen-tool `clone-stock`, `import-parameters` and `script-c` for DrMario, Fox and Falco, then compiles each full generated DLL and the console fixture using the frozen compiler and SDK explicitly. It never invokes `script-build` with live SDK headers. Child processes are hidden; no game, user content store or global build is used. `--reuse-projects` is available for diagnosed fixture-only reruns; the accepted receipt uses fresh projects without that option.

The final worker receipt is `A:/FreakMelee-test-archives/fighter-demo-presentation72-accepted/receipt.json`:

| Member | Exact pairs | Safety cases | Checks |
| --- | ---: | ---: | ---: |
| Doc | 8,738 | 20 | 755,624 |
| Fox | 994 | 19 | 48,051 |
| Falco | 968 | 4 | 7,767 |
| Total | 10,700 | 43 | 811,442 |

## Compared production policy

The extractor takes complete actual `ftCo_800BECB0`, `ftCo_800BED88`, `ftCo_800BF034` and `ftCo_800BF108` bodies, their setup/Fox/Doc helpers, `it_802AE994`, the Blaster reset/pickup/presentation callbacks, and `itDrMarioPill_802C09C4` with its initialization, pickup, accessory and presentation callbacks. Selecting the original non-native hook branch is the only policy preprocessing; complete current native entry bodies dispatch the generated callback. The fixture invokes the generated `on_demo_enter` after the shared entry, matching the documented outer `ftDemo_CreateFighter` ordering. That outer constructor is not extracted here.

- Modes 0/1: Fox initializes commands to one before pickup chooses article state 10, then applies the caller instance/base-scale multiplier. Doc mode 1 retains two ordered vitamin exclusion draws, held mode 1 then auxiliary mode 0, with the original caller scale placement. Doc mode 0 and Falco produce no item.
- Mode 5: Fox creates the attached state-10 gun without an extra caller scale. Doc and Falco produce no item.
- Mode 6: Fox follows the same unscaled constructor. Doc creates mode 2, initializes appearance/lifetime/accessory/spin, performs the actual physics refresh, then attaches at the mapped right thumb. Pickup repeats lifetime/state/manual calls; the final constructor manual call follows attachment. Falco produces no item.
- Position versus previous-position, planar Z, zero velocity/damage/group, both parents, initial-collision flags, command flags, item states, appearance, spin, physics/collision snapshots, exact ordered primitive calls, held/auxiliary publication and vitamin RNG state are compared. Full-width fighter/item generations are used.
- Follow-up animation/accessory comparisons cover facing and NULL/creator/foreign current owners. Fox stock retains its original creator for presentation scaling; the previous script's `owner` query incorrectly required current-owner equality. The corrected script uses the existing creator queries. The real failed comparison is preserved in `fighter-demo-presentation72-final-third/Fox/result.log`; no fields were masked to pass it.

Actual generic `nativeArticle_Create`, creator/logic lookup, `nativeItem_Attach`, bind/clear, module `find_owned_article`/`signal_owned_article`, `Item_8026AB54`, `it_802750F8`, ID lookup, bone mapper and the real callback lifetime registry are extracted or linked. Per-member `source_correspondence.json` verifies that every extracted API/producer/demo body equals the live checkout after newline normalization; raw frozen-file SHA values are retained separately. The exact semantic bone arrays are extracted from the corresponding frozen clone definitions.

## Controlled boundaries and safety

Allocation, item initialization/model construction, shared motion publication, full item animation/state installation, root SRT setter/getter plumbing, attachment geometry, common lifetime/appearance/spin primitives, sound dispatch and the actual animation evaluator are explicit controlled engine boundaries. The stub item-state boundary preserves the real accessory-clearing and callback-installation effects; the allocator selects actual generated spawned/pickup callbacks. The common frame refresh body and attachment callback dispatch are real. The fixture uses a synthetic base motion table for unrelated phases; it is not a replacement for a full module callback-coverage audit or a retail model/animation test. Item attributes for shared model scaling and fighter common scales are controlled; named fighter defaults and pill lifetime use the actual raw-DAT import pipeline.

Safety cases separately invalidate/recycle the source or item at created callbacks, refresh, item-state installation, pickup animation or the shared motion boundary. The controlled created hook first executes the actual generated callback; the real runtime dispatch then observes the injected invalidation. Removing a registry entry also replaces its user-data pointer with inaccessible address 1 so stale reads fail. These cases establish the tested source continuations, not blanket protection for arbitrary native callbacks.

Defined camera/Sleep allocation failures are also paired against original bodies. Original mode0/1 callers scale a NULL allocation result without a guard; these are native source UB, so only the authored failure policy is tested there. Doc's second result allocation failure preserves the first pill. On malformed attachment rigs, the authored policy discards the new unattached item and preserves a surviving previous held slot; this is a deliberate safety behavior outside retail valid-rig parity. No stock handler delegation, new Host API, persistent transport state, digest masking or compatibility path is introduced.
