# Rendered root scale, receiver migration and visibility selections

Audit and root-scale/visibility implementation, 2026-10-09. Root applied the Dr. Mario receiver call-site migrations in the earlier root-scale batch; no game was run here. The initial hidden read-only `native_content_tool source-check` used the owned Game & Watch draft at `A:/FreakMelee-test-archives/fighter-callback-source-census/fighter-5` and canonical `examples/gamewatch_full.fms`; exit1, first diagnostic **433:5 `visuals.set_current_group`**, then unsupported. Tool timestamp was 2026-10-08 23:52. This historical result identified the visibility closure; it does not establish complete later body or asset acceptance after implementation.

## Live Fighter root scale

Implemented catalog property: **`model.root_scale_y`**, F32, Fighter context, Readable|Pure, no Writable or SourceOnly, semantic ID `ModelRootScaleY`. Existing generic captured Fighter receiver namespaces expose it, so Dr. Mario can use `creator.model.root_scale_y`; no new receiver alias, handle type or Item-specific getter is necessary.

Exact native contract is the reference getter `fighter_root_scale_y` in `native_fighter_module.c:296`: obtain the Fighter GObj's actual `hsd_obj`; if absent return1; otherwise `HSD_JObjGetScaleY(root)`. This is **not** `Fighter.x34_scale.y` (`model.instance_scale_y`) nor `x34_scale.y * co_attrs.model_scaling` (`model.scale`). It reads the live rendered root, rather than a semantic-part lookup or imported descriptor scale. Preserve F32 bits, including nonfinite values; this pure observation introduces no finite-value clamp.

The existing retained receiver validates before and after a native-property read (`native_script_binding_receiver.c`). Common native Fighter properties permit recipients without the caller's custom state schema or pinned-program equality. The backend retains physical Fighter/class/data/generation and the actual root pointer; nonnull roots require real JObj ancestry and sufficient native object extent. It rechecks identity after class admission and before output publication, without consulting a donor definition. A retired/mismatched recipient is an error with destination unchanged; it is not the absent-root default. A legitimately absent root on a live admitted Fighter returns1, matching the native getter. No tick allocation is needed.

Focused receipt: `A:/FreakMelee-test-archives/script-root-scale-y-oct09-first/receipt.json`, **34406 checks / 9580 property observations / 43 semantic expressions**, including **15 native root-scale comparisons and 13 rejection cases**. Actual frontend, statement lowering, VM and catalog/property dispatch execute the source getter. Reference getter and class ancestry bodies are mechanically extracted from native source. Registered Fighter/JObj class metadata and mutation during class initialization are controlled; this fixture does not construct owned renderer graphs or execute the retained receiver pool, and makes no game/renderer acceptance claim.

## Dr. Mario receiver call-site mapping

| Current authored expression | Canonical direct expression / evidence |
| --- | --- |
| `owner.model_scale` at585,606 | `attachment_owner.model.scale` when retaining original controller intent. `ftLib_800869D4` (`ftlib.c:468`) calls `ftCommon_GetModelScale`; the existing property performs that exact arithmetic. |
| `owner.motion`, `owner.state.taunt_article/cape_article`, `owner.command1`, `owner.clear_damage_death()` | Use `attachment_owner` for the old attachment/controller semantics. Existing motion/owned state/command/callback projections already implement these operations; no new API. |
| `creator.incapacitated` at618,628,638,648 | `(creator.motion == ftCo_MS_DeadUpFallHitCameraIce or creator.motion == ftCo_MS_Sleep)`. Exact `ftCo_800BF228` (`ft_0BF0.c:115`) tests only those two common motion IDs; both typed constants exist. Keep the current `creator.exists` guard and use the retained creator role. |
| `creator.root_scale_y` at those four sites | `creator.model.root_scale_y`, the implemented live-root observation. Keep the existing attachment/creator existence conditions. |

Relationship distinction is substantive: current `creator` is immutable original creator; `owner` is that creator only while `Item.owner==creator`; `current_owner` is damage owner; `holder` additionally requires the Fighter's held pointer to be this Item; `attachment_owner` is revocable controller (`native_script_receiver.c:363`). The old `owner.model_scale` emission explicitly reads `native_article.attachment_owner` (`native_fighter_item_script.c:671`). A name-only `owner.model.scale` rewrite retains a different relation after reflection/reattachment. Root-owned custom fields/helpers still require exact pinned program/schema on the captured recipient.

## Game & Watch visibility selection closure

Implementation status, 2026-10-09: the three generic methods below now have
catalog entries and substantive owned-source runtime dispatch. `set_current_group`
uses actual 74B0C and retains `.prev`; queries preserve integral width, return
stored signed8 selections (including -1), and return -1 for out-of-range or empty
owned tables. They remain available through generic Fighter receivers. The
existing saved setter is unchanged. Preparation/import/full-script acceptance
is separate from this bounded primitive closure.

The canonical file's missing visibility operation is `visuals.set_current_group(group,value)` at433–449. The existing `visuals.set_group` at1324–1334 is supported and must remain distinct.

| Generic API | Exact primitive/state | Direct contract |
| --- | --- | --- |
| `visuals.set_group(group,value)` (existing) | `ftParts_80074A4C`: writes saved `.prev`, sets x221D_b2; later reset copies saved values into `.idx`. | Existing guarded IntegralAny2 Void operation. |
| `visuals.set_current_group(group,value)` | `ftParts_80074B0C`: if value differs, writes `.idx`, sets x221D_b2 and calls `ftParts_80074D7C(&x5AC,0,&dobj_list)`; leaves `.prev` intact. | Fighter IntegralAny2 ->Void, MayInvalidateSource, actual owned visibility-table/group/value admission. |
| `visuals.current_group(group)` | `nativeFighterVisuals_CurrentGroup`, `.idx` read. | Fighter IntegralAny ->I32, Pure. |
| `visuals.previous_group(group)` | `nativeFighterVisuals_PreviousGroup`, `.prev` read; also needed by Peach's debug taunt. | Fighter IntegralAny ->I32, Pure. |

The current setter admits the full channel0 invalidator footprint when the
channel is marked cleared: every reachable group/alternative, imported TempS
and ordinal span, and current DObj-list bounds. It preserves the actual native
SetFlags null-entry no-op. After native selection it validates captured source,
program/storage, root/parts, imported model/table, lookup and DObj-list identities
before publishing Void. No native pointer or visibility-table recipe enters a
script value, and no tick allocation is introduced.

Focused receipt: `A:/FreakMelee-test-archives/script-visibility-groups-oct09-final/receipt.json`,
**17249 checks / 2245 complete-Fighter native comparisons**, including **544
current-setter comparisons and 774 query comparisons**. The source routines
74A4C/74B0C/74D7C/74A8C, DObjSetFlags and native queries are mechanically
extracted. Owned package/view/import inputs are controlled; this fixture does
not claim DAT import, frontend/VM marshalling, retained receiver invocation,
renderer output or game-scene acceptance.

Current/previous reference getters (`native_fighter_visuals.c:57,64`) return-1 for an index outside model_num/11-entry array. If preserving that neutral query contract, range-check the original integral width before narrowing and return-1 for out-of-range; do not low-bit wrap a huge index into a valid group. Reject floating input. For setters reuse the substantive owned table/variant/outline-channel/alternative admission already in `native_script_binding_visuals.c`; no duplicate lookup implementation. Native selection storage is signed8: preserve the existing direct setter's -1..127 admitted policy rather than silently reintroducing the old emitter's 255 narrowing. GW itself uses only groups5–8 and values-1/0/2. Reads should validate current selected group structure and caller lifetime without requiring a currently visible alternative; hidden -1 is a legitimate result.

The hand-written Oil Panic model policy (`ftGameWatch/ftgamewatchspeciallw.c:108–135`) calls exactly 74B0C with the same groups/values. Replacing it with the saved setter changes reset behavior. Useful focused native comparison: different vs identical value, .idx/.prev separation, dirty-bit behavior and channel0 invalidation; actual reset/copy order in `ftparts.c:91`; bounds/wide-integral rejection and owned variant/outline provenance. Graphics leaves may be controlled, but do not claim rendered mesh acceptance from bookkeeping alone.

No other new visibility API is required by the whole GW file: `visibility_asset 10`, outline installation/color, diffuse/material policies, Fighter item visibility and recursive primary `item.joint("").visible` have direct implementations. `creator.call(PaperBodyColor/PaperOutlineColor)` is an obsolete recipient-call surface in ten Article color setups; exact same-bank known helpers can use `creator.helpers.PaperBodyColor()`/`PaperOutlineColor()` with existing owned receiver code-ID dispatch. `item.set_model_animation_rate` (1839,1843) remains a separate model-animation operation, not visibility. The file also contains Article-context `effects.wind_at_position`; its existing descriptor is Fighter-only, so that is a separate context/backend closure. Neither fact should be hidden by marking visibility schema accepted. This pass does not bypass later Article import/material/layer or complete-program validation.
