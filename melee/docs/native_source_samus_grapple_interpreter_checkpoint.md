# Samus grapple interpreter authoring checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-10. Samus remains an unregistered WIP fighter. This reconciles historical
grapple authoring with the production interpreter, without declaring gameplay
or complete clone acceptance.

`tests/native/samus_grapple_constructor/compose.py` combines the main template,
fighter tether policy, ground Catch policy, real solver Article, constructor
and control signals. Authoring stays split across small files. The initial audit
collected 14 diagnostics across 252 bodies; the final candidate prepares all
**271 bodies**. Its single diagnostic is the deliberately absent independent
Fighter declaration, not a hidden body rejection.

## Source reconciliation

- Owned raw joint **51** supplies both Item attachment and borrowed resource
  joints; the old literal 4 was wrong. It is not a semantic bone alias.
- Callback checks use `callbacks.matches`. Receiver helpers and instance scale
  use current generic APIs. Active solver decisions use the retained
  `attachment_owner`, separately from current damage ownership.
- Creator cleanup checks the expected handle and frees resources even after
  control-owner expiry. Forced destruction has an explicit cleanup callback.
- Ground/air launch, retraction and capture signals live on the real GBeam
  Article. Capture changes mode and clears hitboxes before publishing its
  endpoint anchor, following shared native Catch ordering.
- Ground sparks read the endpoint world matrix, draw three RNG values in native
  order and spawn effect 1011 from that resource's GObj.
- Air obstruction reach is **1.8**, versus ground reach 2. Air contact uses
  `collision.landed`, whose binding runs the stock `ft_80081D0C` test. The broader
  air-landing callback performs additional wall/ledge behavior and is not used.
- Zero-length geometry is rejected before dividing. This is an authored
  malformed-data policy: interpreter division errors cannot reproduce the old
  C draft's nonfinite quotient followed by a guard.

## Evidence and limits

Receipt:
`A:/FreakMelee-test-archives/samus-interpreter-geometry-oct10-sealed/receipt.json`.
The hidden host cohort passes **33,378 checks**, **480 geometry comparisons**
and **6 safety cases**, plus fixture compilation with `-Wall -Wextra -Werror`.

The production compiler, storage and VM execute the unchanged
`InitializeGeometry` helper. Count, truncation/doubling order and nine scaled
values are compared against the exact arithmetic slice before allocation in
`it_802B75FC`. Cases cross four motions, five scales, three modified-mode values
and eight tuning variants. Receiver scale/motion, Article-only metadata and
imported tuning are controlled. The native reference excludes link allocation;
this is not a complete constructor comparison. Source, headers, tool/compiler
binaries and copied asset project are hashed before and after execution.

Other solver bodies and control callbacks have preparation proof only here.
Node/model/animation order, capture runtime, effects, solver timelines and
gameplay require fresh interpreter comparisons. Older generated-C receipts apply
to their historical sources, not these edited components. Modified grapple
code-sequence/homing/hitboxes, air-wall effects, lifecycle/scaled fighter
parameters and throw attachments still need closure. No game or collection was
touched. No new scripting API was needed.

The obsolete generated-C constructor runner is removed. Native comparison
bodies remain for the next interpreter runtime cohort.
