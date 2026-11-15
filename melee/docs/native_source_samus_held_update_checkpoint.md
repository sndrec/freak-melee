# Samus held grapple and pre-launch hitlag checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-11. Samus remains an unregistered authoring candidate. This extends the
[factory checkpoint](native_source_samus_factory_checkpoint.md) with connected
held updates and comparisons against the unchanged original accessory routines.

## Animation correction

`nativeArticleResource_SetWorldMatrix` copies a finite world matrix onto its
retained model root and sets `JOBJ_USER_DEF_MTX`, `JOBJ_MTX_INDEP_PARENT` and
`JOBJ_MTX_INDEP_SRT`. The original held grapple applies the same combination
before advancing its endpoint animation.

The live animation validator treated those flags as an immutable descriptor
contract, so a subsequent resource animation advance returned false. The script
ignored that optional Bool result and the endpoint stopped advancing. Validation
now permits the complete world-matrix override on the retained root. Partial
root substitutions and descendant substitutions remain rejected. Graph identity,
topology, classes, controller provenance, tracks and material admission are
unchanged. This is a generic model operation correction, with no Samus-specific
binding or native special-handler delegation.

## Evidence

`A:/FreakMelee-test-archives/samus-held-oct11-j/receipt.json` passes with frozen
inputs unchanged and matching live inputs at completion:

- 72 constructor pairs across four motions, three scales, both ground/air
  samples and normal/modified input sequences.
- 576 normal held updates against `fn_802B7E34`, with changing attachment
  translations and Up/Down/Up/A sequence progression.
- 432 pre-launch hitlag updates against `it_802BACC4`, covering motions212,
  214 and357. Motion358 has no original pre-launch branch and is excluded here.
- Exact joint transforms, flags, controller and track state, plus material and
  palette state; controller binding addresses are omitted from raw comparisons.
  Endpoint matrices use the existing bounded float comparison.
- Two deliberate matrix-flag refusal cases, plus the existing174 palette
  mutation refusals and17 retail animation census cases.
- 72 real Item factory cases, including144 bound AccessoryBefore updates and
  108 bound AccessoryHitlag updates before the existing removal checks.
- 2,678,532 total assertions;52,628 material/palette comparisons.

An isolated before-fix control, using the original validator and corrected
fixture setup, fails the held controller comparison. This development control
is recorded in `.local/samus-held-before-control.log`; it is not a separate
frozen acceptance receipt.

The shared Link resource regression passes in
`A:/FreakMelee-test-archives/script-item-resources-held-regression-oct11/receipt.json`:
7,033 assertions,25 cases and39 interpreted/native animated graph pairs,
including resource retirement and captured restore/replay. The client and
source-asset target builds pass, along with the four focused character assets,
script compile, fighter parameters and resource metadata CTests.

Fixture maintenance was necessary: the palette-refusal corpus rewound only the
source graph, and repeated constructor cases retained the previous primary
pose. Both comparison graphs now start at the same requested frame, and each
constructor starts with a freshly loaded primary graph. Earlier failed runs
a through i are not acceptance evidence.

## Owned rig correction and remaining work

The existing October10 owned rig implementation is complete:41 semantic
bindings,60 joints and no omissions. The refreshed retail audit passes against
the current ISO common tables and all five costumes in
`A:/FreakMelee-test-archives/samus-rig-refresh-oct11/receipt.json`. The fixture
also checks those facts in the imported draft. Older claims that this map still
needed implementation were stale and have been corrected.

The parent Fighter, inputs and counters remain staged. This does not establish
full Fighter construction, flight/constraint solver behavior, catch integration,
full lifecycle scheduling, rendering or whole-match rollback. Geometry/effect
leaves remain controlled and are not needed by the held cases. Independent
registration still waits for those remaining contracts. No game was launched,
installed package changed or upstream push performed.
