# YoYo initialization source comparison

This is an **unregistered initialization fixture**, not a shipped Ness template
or a YoYo physics port. It supplements the accepted four-resource cleanup/Bat
fixture without changing that checkpoint.

```powershell
python tests/native/ness_yoyo_initialization/run.py `
    --snapshot .local/baseline32 `
    --output .local/ness-yoyo-initialization-source32
```

The runner verifies the supplied immutable SDK/compiler/tool hashes, checks
original source hashes against `original_inputs.json`, extracts the actual
unchanged reference bodies and generates callbacks using the snapshot tool.
It compiles the generated callback, actual snapshot resource backend and actual
shared joint unit directly with frozen headers and vcvars14.44. It invokes two
console tools only; no game, global build, package, `script-build`, live-header
overlay or stock handler delegation is involved. Generated C, originals, binary,
commands, logs and the new receipt stay in `--output` outside tracked sources and
the snapshot. No ISO is required for this bounded source comparison.

Frozen32 acceptance: **566,018 checks, zero failures**, comprising504 original/
generated source pairs and60 defined failure cases. The successful matrix covers
counts2/3/20/90, all seven source motions341..347, both facings, three XYZ positions
(including nonzero Z) and three instance-Y scales. Edited counts exercise the
same unchanged original source with edited attribute values and the generated
code with matching owned parameter values. `fighter_parameters.fmp` provides
eight named fixture values; callbacks read owned `item.params.link_count`, never
the live DAT. Import metadata remains in the unregistered draft for future normal
project import. This fixture does not separately exercise the DAT importer.

## Actual bodies and observable checks

The reference extraction includes `it_802BE9D8`, `it_802BE65C` and its string loader,
`it_80275328`, `it_802A24D0`, actual draw/free callbacks, the actual SpawnItem
initializer and YoYo pickup. Both paths execute actual `mpColl_80041EE4` and
`mpColl_SetECBSource_Fixed`; the generated path executes actual resource node,
model, draw, animation and retirement implementations from frozen32. Neither
constructor nor collision initialization is replaced with a duplicate expected
algorithm.

Checks cover:

- Saved creator identity, source action, reversal only for345, head/tail
  publication, exact adjacent node links and allocation count.
- Node position/zero velocity and collision current/last/previous/x28 history,
  defined flags, all defined initial ECB shapes, category/frame/floor and radius.
- Raw joint42, string radius1, tip radius2*instanceY, original remembered
  surface0, ordinary authored pin/touch/contact state and model source selection.
- String/tip load/registration order, attachment after all nodes, material-only
  animation remove/add/rate1/request0/advance, then picked-up state0/flags2.
- Initial draw outcome: strings use a flag-gated callback with false pin flag;
  the tip initially renders. Generic resource draw now receives `node==tip`,
  matching that observable result.
- Successful retirement and deterministic atomic cleanup when object, link or
  model allocation fails at each of20 node positions.

The new descriptor `NativeArticleResourceInit` supplies initial position before
collision initialization. Six-argument `.create(part,radius,category,x,y,z)` closes
the seed gap found in checkpoint30. Post-construction position writes alone
could not establish all original history fields. The source fixture uses this
substantive constructor directly through the generated Host call.

## Boundaries and remaining limits

HSD allocation uses fixed test pools with poisoned link memory; model loading
checks the String/Tip descriptor and supplies a bounded test joint graph. HSD
animation/render calls record source order and draw outcomes, not pixels or real
DAT animation execution. Active Definition/creator lookup and item allocation,
attachment and common state entry are explicit identity-checked boundaries.
Unexercised geometry/matrix/flag operations abort if unexpectedly called. The
fixture compares defined initialized observations, not uninitialized padding.

The60 failure cases exercise authored/production cleanup only. Original failed
allocation can inspect uninitialized `prev_link`, traverse freed neighbor data,
or continue with missing models; those undefined paths are not executed or
presented as parity. Defined atomic release is an explicit behavior judgment.

Four physics solvers, seven fighter smash phases, common presentation hooks,
real model import/rendering gameplay and whole Ness independence remain open.
No default template or prior accepted candidate was promoted or modified.
