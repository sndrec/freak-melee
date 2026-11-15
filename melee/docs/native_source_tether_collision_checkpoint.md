# Interpreter tether collision checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch finishes the remaining interpreter admission gaps in the retained
Link and Young Link sources after `3faa5864c`. The complete authored source
bodies remain intact; there is no stock-special delegation.

## Connected operations

The two remaining `collision.air()` calls now use the existing
`collision.landed` property. Both execute `ft_80081D0C` and use the same native
contact result. The property updates collision history, position and
environmental responses; it is not a cached ground/air observation. Its output
is published only while the exact physical Fighter, scene, program, storage and
owned motion layout remain current. The old callable spelling is rejected.

`collision.air_catch_hit()` directly executes `ftCo_AirCatchHit_Coll`. Contact
above the fighter's scaled landing threshold selects neutral; equality and
below select basic landing, including the existing held-hammer choice. Native
Landing, HammerLanding and HammerWait stop cached continuations if an owned
hook retires, replaces or faults the caller. No additional geometry policy or
fighter-specific decision is introduced.

See [stage movement](scripting/stage_movement.md) and the
[connected fixture](../tests/native/script_common_motion_collision/README.md).

## Evidence and limits

The client, content tool, source-assets target and selected integration targets
build. Thirteen production inputs remained unchanged across the grouped build.
All sixteen selected CTests pass. The older State fixture required its actual
scene epoch and parameter-table pointer; production admission was not weakened
to accommodate its incomplete fake actor.

Root independently rehashed both receipts and their pinned inputs:

| Receipt | Scope | SHA256 |
|---|---|---|
| `A:/FreakMelee-test-archives/script-common-motion-tether-oct10-final/receipt.json` | 2,338 checks, 28 retained common-motion comparisons, 28 tether comparisons and 70 guard cases; 1,713 pinned inputs | `8f5e2cdca973d18c8ad374ba324fed08d8eda35a225e8a3954b6e7267d42bc53` |
| `A:/FreakMelee-test-archives/source-tether-companion-census-20261009-205434/receipt.json` | Actual package preparation of ten retained full sources | `55566921681166b2c07f64331d50fe0735e06ba7a956b85be14427027eb96867` |

The tether fixture runs the real frontend, catalog, actor/storage, VM, fixed
GObj pools and extracted native collision, threshold and landing continuations.
Owned Landing, LandingAction, GroundedMotionSetup and StateChange hooks execute
through the VM. The motion engine dispatch, stage geometry/environment, ordinary
neutral response and renderer remain controlled leaves. Fault injection tests
continuation and output suppression; it does not imply ordinary geometry calls
normally invoke authored hooks. These comparisons establish the shared primitive
contract, not complete Link gameplay or constructor acceptance.

All ten retained full sources now prepare: Mario, Dr. Mario, Link, Young Link,
Peach, Game & Watch, Zelda, Sheik, Popo and Nana. Link prepares 357 functions,
102 imports and 12,237 instructions; Young Link prepares 378 functions,
102 imports and 12,694 instructions. This uses the actual content tool and copied
fighter assets, rather than a parser-only check. Preparation remains separate
from activation and gameplay acceptance.

Companion member/history, constructor and teardown integration is the next
bounded cohort; simultaneous-companion activation remains closed. Compact
whole-match save/load is still unfinished, with Regions/Pages retained as the
broad oracle. No game, GPU or audio device was opened in this batch.
