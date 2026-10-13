# Ground tether timing and fighter removal decisions

Unregistered source fixture. It does not install common Catch/CatchPull/CatchWait
hooks or activate a complete Link/Young Link fighter.

```
python -B tests/native/link_tether_ground/source_compare.py --snapshot .local/baseline36 --output .local/link-tether-ground-FRESH
```

The runner verifies frozen SDK/compiler/tool hashes, normal-clones both fighters,
imports attributes into owned FMP, generates full candidates, compiles each DLL
and invokes actual generated helpers in an isolated console executable. Unused
required Host operations trap. Generated output stays in the output directory.

Final receipt: `.local/link-tether-ground36-final-frozen/acceptance.json`.
Each fighter: 304,345 assertions / 6,490 cases / zero float difference, including
48 ordered standing/dash timelines and 448 cleanup configurations.
Combined: 608,690 assertions / 12,980 cases / 96 ordered timelines.
Both sound families, facings, scales0.5/1/1.75, creation failure, standing
obstruction, extend/retract/removal edges and consumed-continuation Bool run.
Dash performs no obstruction query. Handles include high bits and absent -1.

## Actual stock bodies

| Source | Actual extracted bodies |
| --- | --- |
| ftCommon/ftCo_0D8E.c | fn_800D8EC8, fn_800D9228, ftCo_800D94D8, fn_800D949C |
| ftLink/ftlink.c | ftLk_Init_BoomerangExists, ftLk_800EAF38, ftLk_800EAF58, ftLk_Init_OnDeath |
| ftLink/ftlinkspecials.c | ftLk_SpecialS_RemoveBoomerang1 |
| ftLink/ftlinkspecialn.c | ftLk_SpecialN_ProcessFv10, ftLk_SpecialN_ProcessFv14 |
| ftCLink/inlines.h and ftclink.c | checkFighter2244, ftCl_Init_80149268, ftCl_Init_OnDeath |
| itlinkhookshot.c | it_802A7AAC |

The cleanup matrix tests32 presence masks x2 Boomerang-used values x7 contexts:
full damage/death2 helper, Boomerang-only helper, initialization reset,
death2/death3/death1 order, Hookshot-only helper, grounded-exit notification
and CatchWait release notification. Removal order, five references,
used flag, accessory2/3/death1 bindings and visual group order are compared.
Ground exit with no chain is a defined no-op; CatchWait release with no chain
clears its known bindings. These are separately tested. Death dispatch order comes from ft/ft_0D31.c:101-110; the surrounding full death
engine function is not executed here.

## Controlled boundaries

Actual original helper bodies execute unchanged. Article construction, reference
lookup, signal effects and destruction are controlled. Thumb semantic resolution,
world point/matrix setup, stage obstruction, audio and visual writes are controlled.
Launch speed is imported into the boundary; constructor-derived scale/coefficient
is separate accepted held-constructor evidence. Actual generation/class lifetime
code is linked. No stock fighter decision callback is invoked by generated script.

The candidate includes full Hookshot article code, but this matrix does not run
its solver/resource destruction. Other article policies are not copied here.
Thus this proves fighter removal ORDER and final references across controlled
article boundaries, not full composed resource/renderer/gameplay acceptance.
Existing held/deployed/remaining/AirCatch/article/resource fixtures stay unchanged.

All cleanup bindings here retain their original/current damage owner.
articles.owned/remove_owned checks current Item.owner. Transfer/reflection
composition needs immutable typed refs and per-article authored removal signals.
Arrow explicit remove must destroy its two trails before generic item destruction,
as it_802A8A7C does; Milk must detach owner before destruction. Those actual
cross-article bodies/resource interactions remain pending.

Void timing helpers publish ordinary Bool tether_ground_consumed. A future Bool
common frame hook can return it after calling the helper. No return wrapper or
native solver is added.

## Common integration gate

See API.md and `.local/link-tether-ground-inventory36/{inventory.json,report.md}`.
CatchPull start-frame hook is not a capture anchor/readiness hook. Existing
capture_wait_setup/enter belongs to victim CaptureWait, not captor CatchWait.
throw_finish occurs too late to replace cleanup after a successful CatchWait
throw/cut decision. Defaults and full fighter activation remain gated.
