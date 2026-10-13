# Grounded Link / Young Link Catch and article cleanup composition

This unregistered source fixture composes the grounded tether policy with all
accepted immutable-author article removal callbacks in one generated module.
Accepted component namespaces and default templates remain unchanged.

## Reproduction

```powershell
python -B tests/native/link_grounded_composition/source_compare.py --snapshot .local/baseline44 --output .local/link-grounded-FRESH
```

Use a fresh output outside the immutable snapshot. The console runner normally
clones Link and Young Link from the game ISO, imports DAT-backed attributes and
assets with the frozen content tool, generates C, and compiles ordinary author
DLLs. It also imports and loads a separate recipient DLL with independent state
layout and owned parameter storage. No manually supplied FMP values, alternate
importer, game launch or global build is used. `prepare.py` runs the import/C/DLL
portion alone; its receipt does not claim callback acceptance.

`acceptance.json` records commands, source/component/generated hashes, the
snapshot manifest hash, exact case counts and controlled boundaries. SDK,
compiler and content-tool hashes are verified before and after the batch.
Generated artifacts, packages and logs belong only in the fresh output.

## Authored decisions and actual source paths

`ground.component.fms` supplies the eight common hooks:

| Hook | Decision and source |
| --- | --- |
| catch_allowed | No live immutable-author Hookshot; `ftCo_Catch.c:fn_800D8E94` |
| catch_start | Reset the ordinary authored frame counter; `ftCo_800D8C54` |
| catch_frame | Standing/dash create, launch, retract and removal timing; `ftCo_0D8E.c:fn_800D8EC8/fn_800D9228` |
| catch_exit | Remove a live Hookshot before common Fall; `fn_800D8E30/fn_800D949C` |
| catch_wait_release | Remove Hookshot after accepted pummel or directional throw; `ftCo_CatchWait_IASA/it_802A7AAC` |
| catch_pull_anchor | Change to capture retraction and bind the owned endpoint, or return false for common-bone fallback; `fn_800D9CE8/it_802A7840` |
| catch_pull_ready | Missing article or completed retraction; `ftCo_CatchPull_Anim` |
| catch_pull_follow | False for Link/CLink; actual `fn_800DA190` does nothing for these kinds |

The same action supplies `capture_wait_enter`, preserving the second actual
`it_802A7840` notification after the victim's high/low CaptureWait transition.
Common input priority, counter gates, acceleration reset, motion selection,
frame selection, registration, pummel/throw short circuit, common capture anchor
rebinding and victim notification execute inside extracted whole common bodies.
The Link/CLink side uses native policy; the custom-kind side uses actual generated
hook pointers. There are no authored calls to stock fighter policy handlers.

The native no-follow accessory callback is executed and proven a no-op. The
script deliberately leaves `accessory1` NULL through the generic false hook;
comparison treats those two inactive representations as behaviorally equivalent.

`tether.component.fms` retains the full accepted Hookshot callbacks and helpers,
adding only an article-local `capture_bound` result and a `bind_capture` signal.
That signal directly changes mode and attempts the production resource anchor
binding. Failed binding uses the existing false-hook common bone fallback.

Pickup/mode model scaling now reads `creator.instance_scale_y`: the original
`itLinkHookshot_Logic20_PickedUp_inline` reads Fighter.x34_scale.y. The `Derive`
solver's separate root-scale input remains unchanged. Tests use instance scale
1.25 and HSD root scale2, then execute actual `it_8026BAE8` to prove the intended
input and item-attribute multiplication rather than equating the two scales.

Assembly reads accepted Bow/Arrow, Boomerang, Milk and full cleanup components
from `../link_article_cleanup` without modifying them. Declaration/helper/article
ordering is direct source composition. Link's unreachable Milk declaration is
omitted; Young Link retains its real asset and complete five-article matrix.
Remove signals preserve Arrow trails-before-removal, Bow recipient-owned export
cleanup and complete Hookshot resource release. The recipient's export executes
from its actual independently loaded DLL and retains its own state/parameters.

## Matrix

The console fixture checks:

- Standing and dash grab entry and whole frame timelines across both facings,
  constructor success/failure and standing obstruction success/failure.
- All four common input entry functions across live/absent Hookshot, held-item
  throw priority, LR/A and common/guard counter conditions. Priority stays ahead
  of the authored eligibility callback.
- Common animation completion after the authored frame policy, catch exit,
  incomplete/completed CatchPull and high/low victim CaptureWait transitions.
- CatchWait release across absent/present article, pummel and directional throw,
  including OR short-circuit ordering. This is not a grab-cut or pickup test.
- Generated-only safe anchor fallback for missing article/resource, plus the
  strictly defined original missing-article readiness path on both sides.
- Full accepted article cleanup presence masks, current damage-owner transfer,
  reflection, both trail slots, Arrow launch, Boomerang mode/used flag, direct
  cleanup, bound damage/death callback ordering and repeated cleanup.
- Retained high-bit IDs, sentinel/stale/recycled/foreign/wrong-logic/wrong-kind
  rejection; absent Bow recipient export still removes the article; independent
  recipient retuning leaves author parameter bytes unchanged.
- Exact resource borrower and callback ownership, replaced bindings, missing or
  switched creators, graph/model free ordering and poisoned storage after free.

The final receipt reports grounded ordered comparisons, full cleanup source
cases and generated safety cases separately. These counts are source tests,
not gameplay frames or full-fighter acceptance.

## Explicit controlled boundaries and limits

- Common motion engine, animation availability, throw input acceptance,
  registration, capture flags/effects, global item counters, audio/effects and
  segment collision responses are controlled boundaries. The whole decision and
  continuation bodies execute; movement/rendering engine internals do not.
- Hookshot creation builds matched fixed graphs (ground construction15 nodes;
  retained cleanup graphs Link22/Young Link15) and declared article state. The
  normal constructor/held/flight solvers are not executed here. Their accepted
  separate source evidence is reused as input, not claimed as new execution.
- Actual production resource/ref/signal/event/creator/joint/lifetime operations
  and actual Item removal helpers execute. HSD allocation/model construction and
  final entity freeing use fixed controlled storage; no renderer runs.
- Borrowed capture anchors are inspected only in actual common capture motions;
  Wait uses that union for other state. Production cleanup does not scan arbitrary
  union pointer bytes. Original stale raw-pointer cases are not executed.
- Original Hookshot release leaves endpoint borrowers stale. Authored resources
  detach exact matching part/capture/hitbox borrows before freeing; replaced
  foreign bindings and callbacks survive. These are deliberate safety extensions.
- An absent collection safely falls back to a common capture bone. A live article
  still follows its original completed flag for readiness. The original absent
  endpoint entry has undefined pointer behavior and is not compared strictly.
- Frozen generated C has discarded guarded result/equality warnings; an original
  `gobj == NULL` no-op also warns. Diagnostics are preserved without compiler or
  source-policy changes. No warning-free-build claim is made.

No generic SDK gap was needed for this composition. Source acceptance does not
activate the default fighter or establish full Link/Young Link independence.

## Root reproduction on SDK46

The independent root run through fresh normal clone/import projects passes
5,764,024 assertions for Link and 10,278,334 for Young Link. Together it covers
1,546 grounded cases, 18,432 cleanup source cases, 32 ordered timelines and
70 separately counted authored safety cases. Receipt:
`.local/link-grounded46-root/acceptance.json`. The controlled boundaries above
still apply; this does not activate either default template.
